/*
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_battery.h"

#include <stdlib.h>

#include <nrf_fuel_gauge.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/logging/log.h>

#include "lp803448_model.h"

LOG_MODULE_REGISTER(app_battery, CONFIG_APP_LOG_LEVEL);

/* nPM1300 CHARGER.BCHGCHARGESTATUS register bitmasks */
#define NPM1300_CHG_STATUS_COMPLETE_MASK BIT(1)
#define NPM1300_CHG_STATUS_TRICKLE_MASK	 BIT(2)
#define NPM1300_CHG_STATUS_CC_MASK	 BIT(3)
#define NPM1300_CHG_STATUS_CV_MASK	 BIT(4)

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(pmic_main));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));
static volatile bool vbus_connected;
static int64_t ref_time;

static int charger_read_sensors(float *voltage, float *current, float *temp, int32_t *chg_status)
{
	struct sensor_value value;
	int err;

	err = sensor_sample_fetch(charger);
	if (err < 0) {
		LOG_ERR("Could not fetch sensor data from charger");
		return err;
	}

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = sensor_value_to_float(&value);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = sensor_value_to_float(&value);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = sensor_value_to_float(&value);

	sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_STATUS, &value);
	*chg_status = value.val1;

	return 0;
}

static int charge_status_update(int32_t chg_status)
{
	union nrf_fuel_gauge_ext_state_info_data state_info;

	if (chg_status & NPM1300_CHG_STATUS_COMPLETE_MASK) {
		LOG_DBG("Charge complete");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;
	} else if (chg_status & NPM1300_CHG_STATUS_TRICKLE_MASK) {
		LOG_DBG("Trickle charging");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
	} else if (chg_status & NPM1300_CHG_STATUS_CC_MASK) {
		LOG_DBG("Constant current charging");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC;
	} else if (chg_status & NPM1300_CHG_STATUS_CV_MASK) {
		LOG_DBG("Constant voltage charging");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
	} else {
		LOG_DBG("Charger idle");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
	}

	return nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE,
					       &state_info);
}

static int fuel_gauge_init(void)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters = {
		.model = &battery_model,
		.opt_params = NULL,
		.state = NULL,
	};
	float max_charge_current;
	float term_charge_current;
	int32_t chg_status;
	int err;

	LOG_INF("nRF Fuel Gauge version: %s", nrf_fuel_gauge_version);

	/* Read initial values from the charger used to initialize the fuel gauge */
	err = charger_read_sensors(&parameters.v0, &parameters.i0, &parameters.t0, &chg_status);
	if (err < 0) {
		LOG_ERR("Could not read sensors from charger device");
		return err;
	}

	/* Store charge nominal and termination current, needed for ttf calculation */
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = sensor_value_to_float(&value);
	term_charge_current = max_charge_current / 10.f;

	err = nrf_fuel_gauge_init(&parameters, NULL);
	if (err < 0) {
		LOG_ERR("Could not initialize fuel gauge");
		return err;
	}

	ref_time = k_uptime_get();

	err = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
					      &(union nrf_fuel_gauge_ext_state_info_data){
						      .charge_current_limit = max_charge_current});
	if (err < 0) {
		LOG_ERR("Error: Could not update fuel gauge charge current limit");
		return err;
	}

	err = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
					      &(union nrf_fuel_gauge_ext_state_info_data){
						      .charge_term_current = term_charge_current});
	if (err < 0) {
		LOG_ERR("Error: Could not update fuel gauge termination current");
		return err;
	}

	err = charge_status_update(chg_status);
	if (err < 0) {
		LOG_ERR("Could not update fuel gauge charging state");
		return err;
	}

	return 0;
}

int fuel_gauge_sample(struct battery_status *status)
{
	static int32_t chg_status_prev;

	float voltage;
	float current;
	float temp;
	float soc;
	float tte;
	float ttf;
	float delta;
	int32_t chg_status;
	int err;

	err = charger_read_sensors(&voltage, &current, &temp, &chg_status);
	if (err < 0) {
		LOG_ERR("Could not read from charger device");
		return err;
	}

	err = nrf_fuel_gauge_ext_state_update(
		vbus_connected ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
			       : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
		NULL);
	if (err < 0) {
		LOG_ERR("Could not update Vbus connected state");
		return err;
	}

	if (chg_status != chg_status_prev) {
		chg_status_prev = chg_status;

		err = charge_status_update(chg_status);
		if (err < 0) {
			LOG_ERR("Could not update fuel gauge charging state");
			return err;
		}
	}

	delta = (float)k_uptime_delta(&ref_time) / 1000.f;

	soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get();

	LOG_INF("V: %.3f V, I: %.3f A, T: %.2f °C, SoC: %.2f%%, TTE: %.0f s, TTF: %.0f s",
		(double)voltage, (double)current, (double)temp, (double)soc, (double)tte,
		(double)ttf);

	status->voltage_v = voltage;
	status->current_a = current;
	status->temp_c = temp;
	status->soc_pct = soc;
	status->tte_s = tte;
	status->ttf_s = ttf;

	return 0;
}

static void npm1300_event_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
		LOG_DBG("Vbus connected");
		vbus_connected = true;
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
		LOG_DBG("Vbus removed");
		vbus_connected = false;
	}
}

int app_battery_init(void)
{
	int err;
	static struct gpio_callback event_cb;
	struct sensor_value val;

	if (!device_is_ready(pmic)) {
		LOG_ERR("PMIC device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(charger)) {
		LOG_ERR("Charger device not ready");
		return -ENODEV;
	}

	if (fuel_gauge_init() < 0) {
		LOG_ERR("Could not initialize fuel gauge");
		return -ENODEV;
	}

	gpio_init_callback(&event_cb, npm1300_event_callback,
			   BIT(NPM1300_EVENT_VBUS_DETECTED) | BIT(NPM1300_EVENT_VBUS_REMOVED));

	err = mfd_npm1300_add_callback(pmic, &event_cb);
	if (err) {
		LOG_ERR("Failed to add PMIC callback");
		return err;
	}

	/* Initialise vbus detection status. */
	err = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);
	if (err) {
		return err;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	LOG_DBG("PMIC initialized");

	return 0;
}
