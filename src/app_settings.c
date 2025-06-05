/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/settings.h>
#include <zephyr/kernel.h>

#include "main.h"
#include "app_settings.h"

static int32_t _measurement_interval = 900; /* default: 15 minutes */
static float _float_length = 0;
static float _float_offset = 0;

int32_t get_measurement_interval(void)
{
	return _measurement_interval;
}

float get_float_length(void)
{
	return _float_length;
}

float get_float_offset(void)
{
	return _float_offset;
}

static enum golioth_settings_status on_measurement_interval_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_measurement_interval == new_value) {
		LOG_DBG("Received MEASUREMENT_INTERVAL already matches local value.");
	} else {
		_measurement_interval = new_value;
		LOG_INF("Set MEASUREMENT_INTERVAL to %i seconds", _measurement_interval);

		wake_system_thread();
	}
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_length_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (_float_length == new_value) {
		LOG_DBG("Received FLOAT_LENGTH already matches local value.");
	} else {
		_float_length = new_value;
		LOG_INF("Set FLOAT_LENGTH to %.6f inches", (double)_float_length);
	}
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_offset_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (_float_offset == new_value) {
		LOG_DBG("Received FLOAT_OFFSET already matches local value.");
	} else {
		_float_offset = new_value;
		LOG_INF("Set FLOAT_OFFSET to %.6f inches", (double)_float_offset);
	}
	return GOLIOTH_SETTINGS_SUCCESS;
}

void check_register_settings_error_and_log(int err, const char *settings_str)
{
	if (err == 0)
	{
		return;
	}

	LOG_ERR("Failed to register settings callback for %s: %d", settings_str, err);
}

void app_settings_register(struct golioth_client *client)
{
	struct golioth_settings *settings = golioth_settings_init(client);
	int err;

	err = golioth_settings_register_int_with_range(
		settings, "MEASUREMENT_INTERVAL", MEASUREMENT_INTERVAL_MIN,
		MEASUREMENT_INTERVAL_MAX, on_measurement_interval_setting, NULL);
	check_register_settings_error_and_log(err, "MEASUREMENT_INTERVAL");

	err = golioth_settings_register_float(settings, "FLOAT_LENGTH", on_float_length_setting,
					      NULL);
	check_register_settings_error_and_log(err, "FLOAT_LENGTH");

	err = golioth_settings_register_float(settings, "FLOAT_OFFSET", on_float_offset_setting,
					      NULL);
	check_register_settings_error_and_log(err, "FLOAT_OFFSET");
}
