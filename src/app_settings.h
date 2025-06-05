/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_SETTINGS_H__
#define __APP_SETTINGS_H__

#include <stdint.h>
#include <limits.h>
#include <golioth/client.h>

#define MEASUREMENT_INTERVAL_MIN 1
#define MEASUREMENT_INTERVAL_MAX INT32_MAX

int32_t get_measurement_interval(void);
float get_float_length(void);
float get_float_offset(void);
void app_settings_register(struct golioth_client *client);

#endif /* __APP_SETTINGS_H__ */
