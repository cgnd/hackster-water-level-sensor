/*
 * Copyright (c) 2022-2023 Golioth, Inc.
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_SENSORS_H__
#define __APP_SENSORS_H__

#include <golioth/client.h>

struct accel_xyz {
	double x;
	double y;
	double z;
};

struct tilt_sensor {
	double pitch;
	double pitch_deg;
	double roll;
	double roll_deg;
};

struct water_level_sensor {
	double float_length;
	double float_offset;
	double float_height;
};

void app_sensors_set_client(struct golioth_client *sensors_client);
void app_sensors_read_and_stream(void);

#endif /* __APP_SENSORS_H__ */
