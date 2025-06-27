# Copyright (c) 2022-2023 Golioth, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(hackster_water_level_sensor)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/app_rpc.c)
target_sources(app PRIVATE src/app_settings.c)
target_sources(app PRIVATE src/app_sensors.c)

# By default, picolibc is built for Zephyr with _ZEPHYR_SOURCE unless some other
# API indicator is set by the application. This does not include some useful
# constants like M_PI. Setting this to _DEFAULT_SOURCE enables BSD extensions
# which include these useful constants.
#
# See picolibc/newlib/libc/include/sys/features.h and
# https://github.com/zephyrproject-rtos/zephyr/issues/66909 for more details.
#
# An alternative to setting _DEFAULT_SOURCE would be to just to define these
# constants in a header file local to this app.
zephyr_compile_definitions(_DEFAULT_SOURCE)
