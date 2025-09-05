/*
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __APP_BATTERY_H__
#define __APP_BATTERY_H__

struct battery_status {
	double voltage_v;
	double current_a;
	double temp_c;
	double soc_pct;
	double tte_s;
	double ttf_s;
};

int app_battery_init(void);
int fuel_gauge_sample(struct battery_status *status);

#endif /* __APP_BATTERY_H__ */
