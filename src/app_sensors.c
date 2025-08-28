/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sensors.h"

#include <stdlib.h>
#include <math.h>

#include <golioth/client.h>
#include <golioth/stream.h>
#include <zcbor_encode.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "app_settings.h"
#include "math_constants.h"

LOG_MODULE_REGISTER(app_sensors, CONFIG_APP_LOG_LEVEL);

static struct golioth_client *s_client;

/* Sensor device structs */
static const struct device *const s_accel = DEVICE_DT_GET_ONE(adi_adxl367);

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

static int read_accel_sensor(struct accel_xyz *accel_data)
{
	struct sensor_value accel_x;
	struct sensor_value accel_y;
	struct sensor_value accel_z;

	if (!device_is_ready(s_accel)) {
		LOG_ERR("%s is not ready", s_accel->name);
		return -ENODEV;
	}

	int err = sensor_sample_fetch(s_accel);
	if (err) {
		LOG_ERR("Error fetching low-power accelerometer sensor sample: %d", err);
		return err;
	}

	sensor_channel_get(s_accel, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(s_accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(s_accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

	/* Raw accelerometer output values in m/s² */
	double x = sensor_value_to_double(&accel_x);
	double y = sensor_value_to_double(&accel_y);
	double z = sensor_value_to_double(&accel_z);

	accel_data->x = x;
	accel_data->y = y;
	accel_data->z = z;

	return 0;
}

static void calculate_tilt(struct accel_xyz *accel_data, struct tilt_sensor *tilt_data)
{
	double x = accel_data->x;
	double y = accel_data->y;
	double z = accel_data->z;

	/* A positive tilt angle means that the corresponding positive axis of
	 * the accelerometer is pointed above the horizon, whereas a negative
	 * angle means that the axis is pointed below the horizon. However, the
	 * low-power accelerometer (ADXL367) on the Thingy:91 X is located on
	 * the bottom side of the board, so the axis are reversed relative to
	 * the horizon and the acceleration due to gravity is measured as a
	 * negative value. As a result, the sign of the tilt angles needs to be
	 * reversed.
	 *
	 * The accelerometer positive X-axis points in the direction from the
	 * USB port towards the power switch. The accelerometer positive Y-axis
	 * points in the direction looking into the USB port.
	 *
	 * Assumptions:
	 * 1. The Thingy:91 X is installed right-side up, i.e. the yellow half
	 *    of the enclosure is on the bottom facing the earth and the orange
	 *    half of the enclosure is on the top facing the sky.
	 * 2. The Thingy:91 X is installed with the USB connector pointing down
	 *    the pivot arm in the direction of the float.
	 */
	tilt_data->roll = -atan2(x, sqrt(y * y + z * z));
	tilt_data->roll_deg = tilt_data->roll * 180.0 / M_PI;
	tilt_data->pitch = -atan2(y, sqrt(x * x + z * z));
	tilt_data->pitch_deg = tilt_data->pitch * 180.0 / M_PI;
}

static void calculate_water_level(struct tilt_sensor *tilt_data,
				  struct water_level_sensor *water_level_data)
{
	double float_length = (double)get_float_length();
	double float_offset = (double)get_float_offset();
	double pitch = tilt_data->pitch;

	/* Calculate the height of the float relative to the hinge */
	water_level_data->float_length = float_length;
	water_level_data->float_offset = float_offset;
	water_level_data->float_height = float_length * sin(pitch) + float_offset;
}

static int encode_accel_data(zcbor_state_t *zse, struct accel_xyz *accel_data)
{
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "accel") && zcbor_map_start_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR unable to open accel map");
		return -1;
	}

	ok = zcbor_tstr_put_lit(zse, "x") && zcbor_float64_put(zse, accel_data->x) &&
	     zcbor_tstr_put_lit(zse, "y") && zcbor_float64_put(zse, accel_data->y) &&
	     zcbor_tstr_put_lit(zse, "z") && zcbor_float64_put(zse, accel_data->z);
	if (!ok) {
		LOG_ERR("ZCBOR failed to encode accel data");
		return -1;
	}

	ok = zcbor_map_end_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR failed to close accel map");
		return -1;
	}

	return 0;
}

static int encode_tilt_sensor_data(zcbor_state_t *zse, struct tilt_sensor *tilt_data)
{
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "tilt") && zcbor_map_start_encode(zse, 2);
	if (!ok) {
		LOG_ERR("ZCBOR unable to open tilt map");
		return -1;
	}

	ok = zcbor_tstr_put_lit(zse, "pitch") && zcbor_float64_put(zse, tilt_data->pitch_deg) &&
	     zcbor_tstr_put_lit(zse, "roll") && zcbor_float64_put(zse, tilt_data->roll_deg);
	if (!ok) {
		LOG_ERR("ZCBOR failed to encode tilt data");
		return -1;
	}

	ok = zcbor_map_end_encode(zse, 2);
	if (!ok) {
		LOG_ERR("ZCBOR failed to close tilt map");
		return -1;
	}

	return 0;
}

static int encode_water_level_sensor_data(zcbor_state_t *zse,
					  struct water_level_sensor *water_level_data)
{
	bool ok;

	ok = zcbor_tstr_put_lit(zse, "water_level") && zcbor_map_start_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR unable to open water_level map");
		return -1;
	}

	ok = zcbor_tstr_put_lit(zse, "float_length") &&
	     zcbor_float64_put(zse, water_level_data->float_length) &&
	     zcbor_tstr_put_lit(zse, "float_offset") &&
	     zcbor_float64_put(zse, water_level_data->float_offset) &&
	     zcbor_tstr_put_lit(zse, "float_height") &&
	     zcbor_float64_put(zse, water_level_data->float_height);
	if (!ok) {
		LOG_ERR("ZCBOR failed to encode water_level data");
		return -1;
	}

	ok = zcbor_map_end_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR failed to close water_level map");
		return -1;
	}

	return 0;
}

static int encode_sensor_data(zcbor_state_t *zse, struct accel_xyz *accel_data,
			      struct tilt_sensor *tilt_data,
			      struct water_level_sensor *water_level_data)
{
	int err;
	bool ok;

	ok = zcbor_map_start_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR failed to open map");
		return -1;
	}

	err = encode_accel_data(zse, accel_data);
	if (err) {
		return -1;
	}

	err = encode_tilt_sensor_data(zse, tilt_data);
	if (err) {
		return -1;
	}

	err = encode_water_level_sensor_data(zse, water_level_data);
	if (err) {
		return -1;
	}

	ok = zcbor_map_end_encode(zse, 3);
	if (!ok) {
		LOG_ERR("ZCBOR failed to close map");
		return -1;
	}

	return 0;
}

/* This will be called by the main() loop after delays or on button presses */
/* Do all of your work here! */
void app_sensors_read_and_stream(void)
{
	int err;
	int accel_num_samples = get_accel_num_samples();
	int accel_sample_delay_ms = get_accel_sample_delay_ms();
	char cbor_buf[256];
	struct accel_xyz accel_sample, accel_data;
	struct tilt_sensor tilt_data;
	struct water_level_sensor water_level_data;

	/* Average accelerometer samples */
	accel_data.x = 0;
	accel_data.y = 0;
	accel_data.z = 0;

	for (int i = 0; i < accel_num_samples; i++) {
		err = read_accel_sensor(&accel_sample);
		if (err) {
			return;
		}

		if (i == 0) {
			accel_data.x = accel_sample.x;
			accel_data.y = accel_sample.y;
			accel_data.z = accel_sample.z;
		} else {
			accel_data.x += accel_sample.x;
			accel_data.y += accel_sample.y;
			accel_data.z += accel_sample.z;
		}

		LOG_DBG("Sample %d: X: %.6f, Y: %.6f, Z: %.6f", i, accel_sample.x, accel_sample.y,
			accel_sample.z);

		k_sleep(K_MSEC(accel_sample_delay_ms));
	}

	accel_data.x /= accel_num_samples;
	accel_data.y /= accel_num_samples;
	accel_data.z /= accel_num_samples;

	/* Calculate tilt and water level from accelerometer data */
	calculate_tilt(&accel_data, &tilt_data);
	calculate_water_level(&tilt_data, &water_level_data);

	LOG_DBG("X: %.6f; Y: %.6f; Z: %.6f", accel_data.x, accel_data.y, accel_data.z);
	LOG_DBG("roll: %.2f°, pitch: %.2f°", tilt_data.roll_deg, tilt_data.pitch_deg);
	LOG_DBG("float length: %.2f in, float offset: %.2f in, float height: %.2f in",
		water_level_data.float_length, water_level_data.float_offset,
		water_level_data.float_height);

	/* Only stream sensor data if connected */
	if (golioth_client_is_connected(s_client)) {
		/* Encode data as CBOR */
		ZCBOR_STATE_E(zse, 3, cbor_buf, sizeof(cbor_buf), 1);
		err = encode_sensor_data(zse, &accel_data, &tilt_data, &water_level_data);
		if (err) {
			return;
		}
		size_t cbor_size = zse->payload - (const uint8_t *)cbor_buf;

		/* Send to LightDB Stream on "sensor" endpoint */
		err = golioth_stream_set_async(s_client, "sensor", GOLIOTH_CONTENT_TYPE_CBOR,
					       cbor_buf, cbor_size, async_error_handler, NULL);
		if (err) {
			LOG_ERR("Failed to send sensor data to Golioth: %d", err);
		}
	} else {
		LOG_WRN("No connection available, skipping sending data to Golioth");
	}
}

void app_sensors_set_client(struct golioth_client *sensors_client)
{
	client = sensors_client;
}
