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
static k_tid_t s_system_thread;

K_SEM_DEFINE(connected_sem, 0, 1);
K_SEM_DEFINE(ota_sem, 0, 1);

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

static void on_fw_update_state_change(enum golioth_ota_state state, enum golioth_ota_reason reason,
				      void *user_arg)
{
	switch (state) {
	case GOLIOTH_OTA_STATE_IDLE:
		k_sem_reset(&ota_sem);
		break;
	case GOLIOTH_OTA_STATE_DOWNLOADING:
		__fallthrough;
	case GOLIOTH_OTA_STATE_DOWNLOADED:
		__fallthrough;
	case GOLIOTH_OTA_STATE_UPDATING:
		k_sem_give(&ota_sem);
		break;
	default:
		break;
	}
}

static void golioth_client_init(void)
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
	golioth_fw_update_register_state_change_callback(on_fw_update_state_change, NULL);

	/* Initialize app settings module */
	app_settings_init(s_client);

	/* Initialize app sensors module */
	app_sensors_init(s_client);
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_INF("LTE network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME
					? "Registered, home network"
					: "Registered, roaming");

			if (!s_client) {
				/* Create and start a Golioth Client */
				golioth_client_init();
			}
		}
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		LOG_INF("LTE mode: %s",
			(evt->lte_mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("LTE RRC connection state: %s",
			(evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED) ? "Connected" : "Idle");
		break;
#if defined(CONFIG_LTE_LC_PSM_MODULE)
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("LTE PSM parameter update: TAU: %d s, Active time: %d s", evt->psm_cfg.tau,
			evt->psm_cfg.active_time);
		break;
#endif
#if defined(CONFIG_LTE_LC_TAU_PRE_WARNING_MODULE)
	case LTE_LC_EVT_TAU_PRE_WARNING:
		LOG_INF("LTE modem will perform a Tracking Area Update in %lld ms", evt->time);
		break;
#endif
#if defined(CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS)
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
		LOG_INF("LTE modem will exit sleep in %lld ms", evt->modem_sleep.time);
		break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		LOG_INF("LTE modem exited sleep after %lld ms", evt->modem_sleep.time);
		break;
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		// lte_modem_enter_sleep(&evt->modem_sleep);
		switch (evt->modem_sleep.type) {
		case LTE_LC_MODEM_SLEEP_PSM:
		case LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM:
			LOG_INF("LTE modem entered PSM sleep for %lld ms", evt->modem_sleep.time);
			break;
		case LTE_LC_MODEM_SLEEP_RF_INACTIVITY:
			LOG_INF("LTE modem entered eDRX sleep, time %lld", evt->modem_sleep.time);
			break;
		case LTE_LC_MODEM_SLEEP_LIMITED_SERVICE:
			LOG_INF("LTE modem entered limited service sleep, time %lld",
				evt->modem_sleep.time);
			break;
		case LTE_LC_MODEM_SLEEP_FLIGHT_MODE:
			LOG_INF("LTE modem entered flight mode sleep, time %lld",
				evt->modem_sleep.time);
			break;
		default:
			break;
		}
		break;
#endif
	default:
		break;
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
		/*
		 * Since the CoAP keepalive is disabled, the connection to
		 * Golioth will be dropped when the LTE link goes down (e.g.
		 * when PSM is entered). While the device is sleeping, any
		 * services that have active CoAP observations will not receive
		 * notifications. To ensure that the observations are received
		 * eventually, the client is started and stopped each time the
		 * system wakes up, which re-registers the observations. This is
		 * not ideal for power consumption because it requires a full
		 * DTLS handshake each time, but it avoids the need for a
		 * frequent keepalive to ensure observations for settings and
		 * OTA are not missed.
		 */
		if (!golioth_client_is_running(s_client)) {
			golioth_client_start(s_client);
		}

		app_sensors_read_and_stream();

		/*
		 * TODO: this is only checking the status of the OTA service,
		 * but it may also be necessary to check the status of other
		 * services that rely on CoAP observations, such as Settings.
		 * Right now, there doesn't appear to be a way to check whether
		 * there are pending changes to observed settings that still
		 * need to be received. The assumption is that any settings
		 * changes should have been received when the client was
		 * started.
		 */
		if (k_sem_count_get(&ota_sem) == 0) {
			/* Only stop the client when OTA is in the idle state */
			golioth_client_stop(s_client);
		}

		k_sleep(K_SECONDS(get_stream_delay_s()));
	}
}
