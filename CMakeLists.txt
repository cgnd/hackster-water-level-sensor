# Copyright (c) 2022-2023 Golioth, Inc.
# Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(hackster_water_level_sensor)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/app_settings.c)
target_sources(app PRIVATE src/app_sensors.c)
target_sources(app PRIVATE src/app_battery.c)
