<!-- SPDX-FileCopyrightText: 2025 Common Ground Electronics <https://cgnd.dev> -->
<!-- SPDX-License-Identifier: Apache-2.0 -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Add partition manager report to build artifacts
- Add Zephyr app binary `zephyr.bin` to build artifacts
- Add `zephyr.signed.hex` to build artifacts
- Add Zephyr app's `.config` to build artifacts
- Add MCUboot-compatible update package to build artifacts

## [2.4.0] - 2025-09-10

### Fixed

- Fix hang when Golioth client is not connected
- Fix incorrect units for Golioth connect timeout (`APP_GOLIOTH_CONNECT_TIMEOUT_S` → `APP_GOLIOTH_CONNECT_TIMEOUT_MS`)
- Allow device power management to be disabled (`CONFIG_PM_DEVICE=n`)
- Fix typos and wording in README

### Added

- Add `APP_GOLIOTH_SETTINGS_TIMEOUT_MS` timeout for receiving updated settings from Golioth
- Add Segger RTT debug overlay
- Add InfluxDB pipeline

### Changed

- Enable logging and sensor shell commands in debug overlay
- Switch Golioth stream from async to sync
- Trigger release workflow when release tags are pushed

## [2.3.0] - 2025-09-03

### Fixed

- Actually fix release GHA workflow

## [2.3.0-rc2] - 2025-09-02

### Fixed

- Fix release GHA workflow

## [2.3.0-rc1] - 2025-09-02

### Added

- Add GitHub Actions workflows for building and releasing firmware

## [2.2.5] - 2025-08-29

### Added

- Add `APP_GOLIOTH_CONNECT_TIMEOUT_S` timeout for connecting to Golioth

### Fixed

- Fix a hang when waiting for settings to sync from the server

## [2.2.4] - 2025-08-29

### Fixed

- Fix a bug when settings registration status is reset
- Set Golioth client CoAP RX timeout larger than stream delay

## [2.2.3] - 2025-08-29

### Fixed

- Ensure settings are registered and synchronized before reading sensors
- Enable external flash when the device is not sleeping (needs to be enabled for OTA)

## [2.2.2] - 2025-08-28

### Fixed

- Suspend external flash to save power

## [2.2.1] - 2025-08-28

### Fixed

- Ensure Golioth client does not stop in the middle of an OTA firmware update

## [2.2.0] - 2025-08-28

### Added

- Add additional LTE modem log messages

### Fixed

- Fix an issue where CoAP observations for Settings & OTA were never received

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

