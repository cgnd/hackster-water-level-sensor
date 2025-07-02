/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_sensors, LOG_LEVEL_DBG);

#include <stdlib.h>
#include <math.h>
#include <golioth/client.h>
#include <golioth/stream.h>
#include <zcbor_encode.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>

#include "app_sensors.h"
#include "app_settings.h"
#include "math_constants.h"

static struct golioth_client *client;

/* Sensor device structs */
#if defined(CONFIG_BOARD_THINGY91X_NRF9151_NS)
#include <drivers/bme68x_iaq.h>
const struct device *weather = DEVICE_DT_GET_ONE(bosch_bme680);
const struct device *accel = DEVICE_DT_GET_ONE(adi_adxl367);
#endif

/* Callback for LightDB Stream */
static void async_error_handler(struct golioth_client *client, enum golioth_status status,
				const struct golioth_coap_rsp_code *coap_rsp_code, const char *path,
				void *arg)
{
	if (status != GOLIOTH_OK) {
		LOG_ERR("Async task failed: %d", status);
		return;
	}
}

static enum golioth_status read_accel_sensor(zcbor_state_t *zse)
{
	struct sensor_value accel_x;
	struct sensor_value accel_y;
	struct sensor_value accel_z;
	bool ok;

	/* ADXL362 */
	int err = sensor_sample_fetch(accel);
	if (err) {
		LOG_ERR("Error fetching ADXL362 sensor sample: %d", err);
		return GOLIOTH_ERR_FAIL;
	}

	sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

	double x = sensor_value_to_double(&accel_x);
	double y = sensor_value_to_double(&accel_y);
	double z = sensor_value_to_double(&accel_z);
	double pitch = atan2(-x, sqrt(y * y + z * z)) * 180.0 / M_PI;
	double roll = (atan2(y, z) * 180.0 / M_PI) - 180.0;
	LOG_DBG("X: %.6f; Y: %.6f; Z: %.6f, Pitch: %.2f°, Roll: %.2f°", x, y, z, pitch, roll);

	ok = zcbor_tstr_put_lit(zse, "accel") && zcbor_map_start_encode(zse, 3);
	if (!ok)
	{
		LOG_ERR("ZCBOR unable to open accel map");
		return GOLIOTH_ERR_QUEUE_FULL;
	}

	ok = zcbor_tstr_put_lit(zse, "x") && zcbor_float64_put(zse, x) &&
	     zcbor_tstr_put_lit(zse, "y") && zcbor_float64_put(zse, y) &&
	     zcbor_tstr_put_lit(zse, "z") && zcbor_float64_put(zse, z) &&
	     zcbor_tstr_put_lit(zse, "pitch") && zcbor_float64_put(zse, pitch) &&
	     zcbor_tstr_put_lit(zse, "roll") && zcbor_float64_put(zse, roll);
	if (!ok)
	{
		LOG_ERR("ZCBOR failed to encode accel data");
		return GOLIOTH_ERR_QUEUE_FULL;
	}

	ok = zcbor_map_end_encode(zse, 3);
	if (!ok)
	{
		LOG_ERR("ZCBOR failed to close accel map");
		return GOLIOTH_ERR_QUEUE_FULL;
	}

	return GOLIOTH_OK;
}

/* This will be called by the main() loop after delays or on button presses */
/* Do all of your work here! */
void app_sensors_read_and_stream(void)
{
	int err;
	bool ok;
	enum golioth_status status;
	char cbor_buf[256];

	ZCBOR_STATE_E(zse, 3, cbor_buf, sizeof(cbor_buf), 1);

	ok = zcbor_map_start_encode(zse, 3);

	if (!ok)
	{
		LOG_ERR("ZCBOR failed to open map");
		return;
	}

	status = read_accel_sensor(zse);
	if (status == GOLIOTH_ERR_QUEUE_FULL)
	{
		return;
	}

	ok = zcbor_map_end_encode(zse, 3);
	if (!ok)
	{
		LOG_ERR("ZCBOR failed to close map");
	}

	size_t cbor_size = zse->payload - (const uint8_t *) cbor_buf;

	/* Only stream sensor data if connected */
	if (golioth_client_is_connected(client)) {
		/* Send to LightDB Stream on "sensor" endpoint */
		err = golioth_stream_set_async(client, "sensor", GOLIOTH_CONTENT_TYPE_CBOR, cbor_buf,
					cbor_size, async_error_handler, NULL);
		if (err) {
			LOG_ERR("Failed to send sensor data to Golioth: %d", err);
		}
	} else {
		LOG_DBG("No connection available, skipping sending data to Golioth");
	}
}

void app_sensors_set_client(struct golioth_client *sensors_client)
{
	client = sensors_client;
}
