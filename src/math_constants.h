/*
 * Copyright (c) 2025 Common Ground Electronics <https://cgnd.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * By default, picolibc is built for Zephyr with the _ZEPHYR_SOURCE feature test
 * macro unless some other API indicator (like _XOPEN_SOURCE) is set. As a
 * result, this does not include some useful math constants like M_PI.
 *
 * See picolibc/newlib/libc/include/sys/features.h and
 * https://github.com/zephyrproject-rtos/zephyr/issues/66909 for more details.
 */

#ifndef MATH_CONSTANTS_H
#define MATH_CONSTANTS_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif /* MATH_CONSTANTS_H */
