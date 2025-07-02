/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <sys/_stdint.h>
LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_DBG);

#include <golioth/client.h>
#include <golioth/settings.h>
#include "main.h"

#include <zephyr/kernel.h>

#include "app_settings.h"

static int32_t _loop_delay_s = 900; /* default: 15s */
#define LOOP_DELAY_S_MAX 43200
#define LOOP_DELAY_S_MIN 1

int32_t get_loop_delay_s(void)
{
	return _loop_delay_s;
}

static enum golioth_settings_status on_loop_delay_setting(int32_t new_value, void *arg)
{
	/* Only update if value has changed */
	if (_loop_delay_s == new_value) {
		LOG_DBG("Received LOOP_DELAY_S already matches local value.");
	} else {
		_loop_delay_s = new_value;
		LOG_INF("Set loop delay to %i seconds", _loop_delay_s);

		wake_system_thread();
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

	err = golioth_settings_register_int_with_range(settings,
							   "LOOP_DELAY_S",
							   LOOP_DELAY_S_MIN,
							   LOOP_DELAY_S_MAX,
							   on_loop_delay_setting,
							   NULL);

	check_register_settings_error_and_log(err, "LOOP_DELAY_S");
}
