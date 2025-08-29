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

void app_settings_init(struct golioth_client *client);
bool app_settings_ready(void);
void app_settings_wait_ready(void);
int32_t get_stream_delay_s(void);
float get_float_length(void);
float get_float_offset(void);
int32_t get_accel_num_samples(void);
int32_t get_accel_sample_delay_ms(void);

#endif /* __APP_SETTINGS_H__ */
