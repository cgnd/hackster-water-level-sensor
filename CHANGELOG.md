<!-- SPDX-FileCopyrightText: 2025 Common Ground Electronics <https://cgnd.dev> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] - 2025-08-26

### Added

- Add an "App" menu to Kconfig with app-specific options:
  - `CONFIG_APP_STREAM_DELAY_S`
  - `CONFIG_APP_ACCEL_NUM_SAMPLES`
  - `CONFIG_ACCEL_SAMPLE_DELAY_MS`

### Changed

- Set LTE PSM requested active time (RAT) to 6 seconds
- Log a warning message when sensor data is not sent to Golioth

## [2.0.1] - 2025-08-20

### Fixed

- Fix network connection settings to reduce power consumption:
  - Change Golioth CoAP RX timeout to 90000 seconds (25 hours)
  - Change LTE PSM periodic TAU to 93600 seconds (26 hours)

## [2.0.0] - 2025-08-19

### Added

- Add accelerometer sample averaging

### Fixed

- Ensure device-specific settings are received from Golioth before starting sensor measurements

### Changed

- Change `MEASUREMENT_INTERVAL` Golioth setting to `STREAM_DELAY_S`

## [1.1.1] - 2025-08-14

### Fixed

- Fix `VERSION`

## [1.1.0] - 2025-08-13

### Added

- Add debugging config overlays

### Fixed

- Minor `README.md` & `CHANGELOG.md` fixes

### Changed

- Store PSK credentials securely in the nRF9151 and enable offloaded sockets
- Configure for low-power operation

## [1.0.0] - 2025-07-16

Initial water level sensor prototype firmware release.

