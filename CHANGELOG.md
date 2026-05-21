# Changelog — ViSPE ADIN1110

All notable changes to this library will be documented in this file.  
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),  
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.1] — 2026-05-21

### Fixed
- Updated GitHub repository URL in `library.properties`.
- Renamed the `Example01a_BasicSendRecieve` example folder and file to `Example01a_BasicSendReceive`.
- Fixed multiple spelling errors (`recieve`, `recieved`, `conterpart`) across example sketches.
- Fixed `indenticalMacs()` function typo to `identicalMacs()` across the library API and examples.

## [1.0.0] — 2026-05-20

### Added
- Initial public release of the ViSPE ADIN1110 library.
- `ViSPE` class — simple send/receive API with callbacks.
- `Vi_SPE_Advanced` class — full ADIN1110 register-level access (FIFO control,
  promiscuous mode, MSE link quality, MAC address filtering, loopback/test modes,
  stat counters, frame generator/checker).
- Integrated Analog Devices ADIN1110 MAC-PHY driver (v2021 release).
- Arduino SPI/HAL board-support layer for Raspberry Pi Pico W (SPI1).
- 16 example sketches covering basic send/receive, link status, BME280 sensor
  streaming, SerLCD display, frame transceiver, hardware loopback, diagnostics,
  and more.
- `library.properties`, `keywords.txt`, `README.md`, `ATTRIBUTION.md`,
  `LICENSE.md` per Arduino Library Specification v2.2.
