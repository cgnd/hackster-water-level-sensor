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

#define STREAM_DELAY_S_MIN	  1
#define STREAM_DELAY_S_MAX	  INT32_MAX
#define ACCEL_NUM_SAMPLES_MIN	  1
#define ACCEL_NUM_SAMPLES_MAX	  INT32_MAX
#define ACCEL_SAMPLE_DELAY_MS_MIN 0
#define ACCEL_SAMPLE_DELAY_MS_MAX INT32_MAX

bool app_settings_ready(void);
void app_settings_wait_ready(void);
int32_t get_stream_delay_s(void);
float get_float_length(void);
float get_float_offset(void);
int32_t get_accel_num_samples(void);
int32_t get_accel_sample_delay_ms(void);
void app_settings_register(struct golioth_client *client);

#endif /* __APP_SETTINGS_H__ */
