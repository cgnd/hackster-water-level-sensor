/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_settings.h"

#include <golioth/client.h>
#include <golioth/settings.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "main.h"

LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

static int32_t _stream_delay_s = STREAM_DELAY_S_DEFAULT;
static float _float_length = FLOAT_LENGTH_DEFAULT;
static float _float_offset = FLOAT_OFFSET_DEFAULT;
static int32_t _accel_num_samples = ACCEL_NUM_SAMPLES_DEFAULT;
static int32_t _accel_sample_delay_ms = ACCEL_SAMPLE_DELAY_MS_DEFAULT;
static bool _stream_delay_s_registered = false;
static bool _float_length_registered = false;
static bool _float_offset_registered = false;
static bool _accel_num_samples_registered = false;
static bool _accel_sample_delay_ms_registered = false;

K_SEM_DEFINE(registration_completed_sem, 0, 1);

bool app_settings_ready(void)
{
	return _stream_delay_s_registered && _float_length_registered && _float_offset_registered &&
	       _accel_num_samples_registered && _accel_sample_delay_ms_registered;
}

static void check_registration_status(void)
{
	if (app_settings_ready()) {
		k_sem_give(&registration_completed_sem);
	}
}

void app_settings_wait_ready(void)
{
	LOG_INF("Waiting for settings to be registered...");

	/* Wait until all settings are registered */
	k_sem_take(&registration_completed_sem, K_FOREVER);

	LOG_INF("All settings registered successfully");
}

int32_t get_stream_delay_s(void)
{
	return _stream_delay_s;
}

float get_float_length(void)
{
	return _float_length;
}

float get_float_offset(void)
{
	return _float_offset;
}

int32_t get_accel_num_samples(void)
{
	return _accel_num_samples;
}

int32_t get_accel_sample_delay_ms(void)
{
	return _accel_sample_delay_ms;
}

static enum golioth_settings_status on_stream_delay_s_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_stream_delay_s == new_value) {
		LOG_DBG("Received STREAM_DELAY_S setting already matches local value.");
	} else {
		_stream_delay_s = new_value;
		LOG_INF("Set STREAM_DELAY_S setting to %i seconds", _stream_delay_s);

		wake_system_thread();
	}
	_stream_delay_s_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_length_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (_float_length == new_value) {
		LOG_DBG("Received FLOAT_LENGTH setting already matches local value.");
	} else {
		_float_length = new_value;
		LOG_INF("Set FLOAT_LENGTH setting to %.6f inches", (double)_float_length);
	}
	_float_length_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_offset_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (_float_offset == new_value) {
		LOG_DBG("Received FLOAT_OFFSET setting already matches local value.");
	} else {
		_float_offset = new_value;
		LOG_INF("Set FLOAT_OFFSET setting to %.6f inches", (double)_float_offset);
	}
	_float_offset_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_accel_num_samples_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_accel_num_samples == new_value) {
		LOG_DBG("Received ACCEL_NUM_SAMPLES setting already matches local value.");
	} else {
		_accel_num_samples = new_value;
		LOG_INF("Set ACCEL_NUM_SAMPLES setting to %i samples", _accel_num_samples);
	}
	_accel_num_samples_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_accel_sample_delay_ms_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_accel_sample_delay_ms == new_value) {
		LOG_DBG("Received ACCEL_SAMPLE_DELAY_MS setting already matches local value.");
	} else {
		_accel_sample_delay_ms = new_value;
		LOG_INF("Set ACCEL_SAMPLE_DELAY_MS setting to %i milliseconds",
			_accel_sample_delay_ms);
	}
	_accel_sample_delay_ms_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static void check_register_settings_error_and_log(int err, const char *settings_str)
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

	err = golioth_settings_register_int_with_range(settings, "STREAM_DELAY_S",
						       STREAM_DELAY_S_MIN, STREAM_DELAY_S_MAX,
						       on_stream_delay_s_setting, NULL);
	check_register_settings_error_and_log(err, "STREAM_DELAY_S");

	err = golioth_settings_register_float(settings, "FLOAT_LENGTH", on_float_length_setting,
					      NULL);
	check_register_settings_error_and_log(err, "FLOAT_LENGTH");

	err = golioth_settings_register_float(settings, "FLOAT_OFFSET", on_float_offset_setting,
					      NULL);
	check_register_settings_error_and_log(err, "FLOAT_OFFSET");

	err = golioth_settings_register_int_with_range(settings, "ACCEL_NUM_SAMPLES",
						       ACCEL_NUM_SAMPLES_MIN, ACCEL_NUM_SAMPLES_MAX,
						       on_accel_num_samples_setting, NULL);
	check_register_settings_error_and_log(err, "ACCEL_NUM_SAMPLES");

	err = golioth_settings_register_int_with_range(
		settings, "ACCEL_SAMPLE_DELAY_MS", ACCEL_SAMPLE_DELAY_MS_MIN,
		ACCEL_SAMPLE_DELAY_MS_MAX, on_accel_sample_delay_ms_setting, NULL);
	check_register_settings_error_and_log(err, "ACCEL_SAMPLE_DELAY_MS");
}
