/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hackster_water_level_sensor, LOG_LEVEL_DBG);

#include <app_version.h>
#include "app_settings.h"
#include "app_sensors.h"
#include <golioth/client.h>
#include <golioth/stream.h>
#include <golioth/fw_update.h>
#include <modem/lte_lc.h>
#include <zephyr/kernel.h>

/* Current firmware version; update in VERSION */
static const char *_current_version =
	STRINGIFY(APP_VERSION_MAJOR) "." STRINGIFY(APP_VERSION_MINOR) "." STRINGIFY(APP_PATCHLEVEL);

static struct golioth_client *client;
K_SEM_DEFINE(connected, 0, 1);

static k_tid_t _system_thread;

void wake_system_thread(void)
{
	k_wakeup(_system_thread);
}

static void on_client_event(struct golioth_client *client, enum golioth_client_event event,
			    void *arg)
{
	bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);

	if (is_connected) {
		k_sem_give(&connected);
	}
	LOG_INF("Golioth client %s", is_connected ? "connected" : "disconnected");
}

static void start_golioth_client(void)
{
	/* Configure the client to use TLS credentials from the nRF9151 modem */
	const struct golioth_client_config client_config = {
		.credentials =
			{
				.auth_type = GOLIOTH_TLS_AUTH_TYPE_TAG,
				.tag = 1234, /* Replace with your tag ID */
			},
	};

	/* Create and start a Golioth Client */
	client = golioth_client_create(&client_config);

	/* Register Golioth on_connect callback */
	golioth_client_register_event_callback(client, on_client_event, NULL);

	/* Initialize DFU components */
	golioth_fw_update_init(client, _current_version);

	/*** Call Golioth APIs for other services in dedicated app files ***/

	/* Set Golioth Client for streaming sensor data */
	app_sensors_set_client(client);

	/* Register Settings service */
	app_settings_register(client);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {

		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {

			if (!client) {
				/* Create and start a Golioth Client */
				start_golioth_client();
			}
		}
	}
}

int main(void)
{
	/* Get system thread id so measurement interval changes can wake main */
	_system_thread = k_current_get();

	/* Start LTE asynchronously if the nRF9160 is used.
	 * Golioth Client will start automatically when LTE connects.
	 */

	LOG_INF("Connecting to LTE, this may take some time...");
	lte_lc_connect_async(lte_handler);

	/* Wait for connection to Golioth */
	k_sem_take(&connected, K_FOREVER);

	while (true) {
		app_sensors_read_and_stream();

		k_sleep(K_SECONDS(get_measurement_interval()));
	}
}
