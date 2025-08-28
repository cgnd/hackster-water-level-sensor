/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_version.h>
#include <golioth/client.h>
#include <golioth/fw_update.h>
#include <golioth/stream.h>
#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "app_settings.h"
#include "app_sensors.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/* TODO: Is there a better way to determine if we are using runtime PSK auth? */
#define RUNTIME_PSK_AUTH (CONFIG_NET_SOCKETS_TLS_PRIORITY < CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY)

#if RUNTIME_PSK_AUTH
#include <samples/common/sample_credentials.h>
#endif

/* Current firmware version; update in VERSION */
static const char *s_current_version =
	STRINGIFY(APP_VERSION_MAJOR) "." STRINGIFY(APP_VERSION_MINOR) "." STRINGIFY(APP_PATCHLEVEL);

static struct golioth_client *s_client;
K_SEM_DEFINE(connected_sem, 0, 1);

static k_tid_t s_system_thread;

void wake_system_thread(void)
{
	k_wakeup(s_system_thread);
}

static void on_client_event(struct golioth_client *client, enum golioth_client_event event,
			    void *arg)
{
	bool is_connected = (event == GOLIOTH_CLIENT_EVENT_CONNECTED);

	if (is_connected) {
		k_sem_give(&connected_sem);
	}
	LOG_INF("Golioth client %s", is_connected ? "connected" : "disconnected");
}

static void start_golioth_client(void)
{
#if RUNTIME_PSK_AUTH
	/* Get the client configuration from auto-loaded settings */
	const struct golioth_client_config *client_config = golioth_sample_credentials_get();

	LOG_INF("Loaded Golioth credentials from settings subsystem");
#else
	/* Get the client configuration from the nRF9151 modem */
	const struct golioth_client_config gclient_config = {
		.credentials =
			{
				.auth_type = GOLIOTH_TLS_AUTH_TYPE_TAG,
				.tag = CONFIG_GOLIOTH_COAP_CLIENT_CREDENTIALS_TAG,
			},
	};

	const struct golioth_client_config *client_config = &gclient_config;

	LOG_INF("Loaded Golioth credentials from modem credential storage");
#endif
	/* Create and start a Golioth Client */
	s_client = golioth_client_create(client_config);

	/* Register Golioth on_connect callback */
	golioth_client_register_event_callback(s_client, on_client_event, NULL);

	/* Initialize DFU components */
	golioth_fw_update_init(s_client, s_current_version);

	/*** Call Golioth APIs for other services in dedicated app files ***/

	/* Set Golioth Client for streaming sensor data */
	app_sensors_set_client(s_client);

	/* Register Settings service */
	app_settings_register(s_client);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {

		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {

			if (!s_client) {
				/* Create and start a Golioth Client */
				start_golioth_client();
			}
		}
	}
}

int main(void)
{
	/* Get system thread id so measurement interval changes can wake main */
	s_system_thread = k_current_get();

	/* Start LTE asynchronously if the nRF9160 is used.
	 * Golioth Client will start automatically when LTE connects.
	 */

	LOG_INF("Connecting to LTE, this may take some time...");
	lte_lc_connect_async(lte_handler);

	/* Wait for connection to Golioth */
	LOG_INF("Connecting to Golioth...");
	k_sem_take(&connected_sem, K_FOREVER);

	/* Wait for all app settings to be registered and synchronized */
	app_settings_wait_ready();

	while (true) {
		app_sensors_read_and_stream();

		k_sleep(K_SECONDS(get_stream_delay_s()));
	}
}
