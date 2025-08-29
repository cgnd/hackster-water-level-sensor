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

LOG_MODULE_REGISTER(app_settings, CONFIG_APP_LOG_LEVEL);

#define STREAM_DELAY_S_MIN	  1
#define STREAM_DELAY_S_MAX	  86400 /* 24 hours */
#define ACCEL_NUM_SAMPLES_MIN	  1
#define ACCEL_NUM_SAMPLES_MAX	  INT32_MAX
#define ACCEL_SAMPLE_DELAY_MS_MIN 0
#define ACCEL_SAMPLE_DELAY_MS_MAX INT32_MAX

static struct golioth_settings *s_settings;
static int32_t s_stream_delay_s = CONFIG_APP_STREAM_DELAY_S;
static float s_float_length_in = 0.0;
static float s_float_offset_in = 0.0;
static int32_t s_accel_num_samples = CONFIG_APP_ACCEL_NUM_SAMPLES;
static int32_t s_accel_sample_delay_ms = CONFIG_ACCEL_SAMPLE_DELAY_MS;
static bool s_stream_delay_s_registered = false;
static bool s_float_length_registered = false;
static bool s_float_offset_registered = false;
static bool s_accel_num_samples_registered = false;
static bool s_accel_sample_delay_ms_registered = false;

K_SEM_DEFINE(registration_completed_sem, 0, 1);

bool app_settings_registration_status(void)
{
	return s_stream_delay_s_registered && s_float_length_registered &&
	       s_float_offset_registered && s_accel_num_samples_registered &&
	       s_accel_sample_delay_ms_registered;
}

void app_settings_registration_status_reset(void)
{
	s_stream_delay_s_registered = false;
	s_float_length_registered = false;
	s_float_offset_registered = false;
	s_accel_num_samples_registered = false;
	s_accel_sample_delay_ms_registered = false;
}

void app_settings_registration_status_wait(void)
{
	LOG_INF("Waiting for settings to be registered...");

	/* Wait until all settings are registered */
	k_sem_take(&registration_completed_sem, K_FOREVER);

	LOG_INF("All settings registered successfully");
}

static void check_registration_status(void)
{
	if (app_settings_registration_status()) {
		k_sem_give(&registration_completed_sem);
	}
}

int32_t get_stream_delay_s(void)
{
	return s_stream_delay_s;
}

float get_float_length_in(void)
{
	return s_float_length_in;
}

float get_float_offset_in(void)
{
	return s_float_offset_in;
}

int32_t get_accel_num_samples(void)
{
	return s_accel_num_samples;
}

int32_t get_accel_sample_delay_ms(void)
{
	return s_accel_sample_delay_ms;
}

static enum golioth_settings_status on_stream_delay_s_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (s_stream_delay_s == new_value) {
		LOG_DBG("Received STREAM_DELAY_S setting already matches local value.");
	} else {
		s_stream_delay_s = new_value;
		LOG_INF("Set STREAM_DELAY_S setting to %i seconds", s_stream_delay_s);

		wake_system_thread();
	}
	s_stream_delay_s_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_length_in_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (s_float_length_in == new_value) {
		LOG_DBG("Received FLOAT_LENGTH setting already matches local value.");
	} else {
		s_float_length_in = new_value;
		LOG_INF("Set FLOAT_LENGTH setting to %.6f inches", (double)s_float_length_in);
	}
	s_float_length_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_float_offset_in_setting(float new_value, void *arg)
{
	/* Only update if value has changed */
	if (s_float_offset_in == new_value) {
		LOG_DBG("Received FLOAT_OFFSET setting already matches local value.");
	} else {
		s_float_offset_in = new_value;
		LOG_INF("Set FLOAT_OFFSET setting to %.6f inches", (double)s_float_offset_in);
	}
	s_float_offset_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_accel_num_samples_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (s_accel_num_samples == new_value) {
		LOG_DBG("Received ACCEL_NUM_SAMPLES setting already matches local value.");
	} else {
		s_accel_num_samples = new_value;
		LOG_INF("Set ACCEL_NUM_SAMPLES setting to %i samples", s_accel_num_samples);
	}
	s_accel_num_samples_registered = true;
	check_registration_status();
	return GOLIOTH_SETTINGS_SUCCESS;
}

static enum golioth_settings_status on_accel_sample_delay_ms_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (s_accel_sample_delay_ms == new_value) {
		LOG_DBG("Received ACCEL_SAMPLE_DELAY_MS setting already matches local value.");
	} else {
		s_accel_sample_delay_ms = new_value;
		LOG_INF("Set ACCEL_SAMPLE_DELAY_MS setting to %i milliseconds",
			s_accel_sample_delay_ms);
	}
	s_accel_sample_delay_ms_registered = true;
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

void app_settings_init(struct golioth_client *client)
{
	int err;

	if (s_settings) {
		golioth_settings_deinit(s_settings);
	}

	app_settings_registration_status_reset();

	s_settings = golioth_settings_init(client);

	err = golioth_settings_register_int_with_range(s_settings, "STREAM_DELAY_S",
						       STREAM_DELAY_S_MIN, STREAM_DELAY_S_MAX,
						       on_stream_delay_s_setting, NULL);
	check_register_settings_error_and_log(err, "STREAM_DELAY_S");

	err = golioth_settings_register_float(s_settings, "FLOAT_LENGTH",
					      on_float_length_in_setting, NULL);
	check_register_settings_error_and_log(err, "FLOAT_LENGTH");

	err = golioth_settings_register_float(s_settings, "FLOAT_OFFSET",
					      on_float_offset_in_setting, NULL);
	check_register_settings_error_and_log(err, "FLOAT_OFFSET");

	err = golioth_settings_register_int_with_range(s_settings, "ACCEL_NUM_SAMPLES",
						       ACCEL_NUM_SAMPLES_MIN, ACCEL_NUM_SAMPLES_MAX,
						       on_accel_num_samples_setting, NULL);
	check_register_settings_error_and_log(err, "ACCEL_NUM_SAMPLES");

	err = golioth_settings_register_int_with_range(
		s_settings, "ACCEL_SAMPLE_DELAY_MS", ACCEL_SAMPLE_DELAY_MS_MIN,
		ACCEL_SAMPLE_DELAY_MS_MAX, on_accel_sample_delay_ms_setting, NULL);
	check_register_settings_error_and_log(err, "ACCEL_SAMPLE_DELAY_MS");
}
