# Attribution

The **ViSPE ADIN1110** library is developed by **Abin Antony** at **Vi Micro Systems Pvt Ltd**
and is built upon the following open-source and proprietary components:

---

## 1. Analog Devices ADIN1110 MAC-PHY Driver

**Copyright (c) 2020, 2021 Analog Devices, Inc. All Rights Reserved.**

Files:
- `src/adin1110.c`, `src/adin1110.h`
- `src/adi_mac.c`, `src/adi_mac.h`
- `src/adi_phy.c`, `src/adi_phy.h`
- `src/adi_spi_generic.c`, `src/adi_spi_generic.h`
- `src/adi_spi_oa.c`, `src/adi_spi_oa.h`
- `src/adi_eth_common.h`
- `src/hal.c`, `src/hal.h`, `src/hal_port_specific.h`
- `src/fcs.c`
- `src/ADIN1110_mac_addr_rdef.h`
- `src/ADIN1110_mac_typedefs.h`
- `src/ADIN1110_phy_addr_rdef.h`

These files are proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

These files have been ported from the Analog Devices evaluation software
to work within the Arduino build system. Their contents may have been
minimally modified from their original form for compatibility purposes only.

---

## 2. SparkFun ADIN1110 Arduino Library

**Copyright (c) SparkFun Electronics — MIT License**  
**Author:** Kyle Wenner <kyle.wenner@sparkfun.com>  
**URL:** https://github.com/sparkfun/SparkFun_ADIN1110_Arduino_Libary

The Arduino board-support layer and C++ wrapper architecture used in this
library is adapted and rebranded from the SparkFun ADIN1110 Arduino Library.

Original files adapted:
- `SparkFun_SinglePairEthernet.h/.cpp` → rebranded as `Vi_SinglePairEthernet.h/.cpp`
- `sfe_spe_advanced.h/.cpp` → rebranded as `Vi_SPE_Advanced.h/.cpp`
- `boardsupport.h`, `boardsupport_ard.cpp` → extended with Pico W / RP2040 support

---

## 3. Vi Micro Systems Additions

The following are original contributions by Abin Antony / Vi Micro Systems:

- Raspberry Pi Pico W (RP2040) SPI1 bus support (`DEFAULT_ETH_USE_SPI1`)
- GP10/11/12/13/6/7 pin defaults for VI SPE RP PICO 2 hardware
- `BSP_SetSPIInstance()` / `BSP_SetSPIPins()` runtime override API
- `setSPIInstance()` / `setSPIPins()` C++ methods on `Vi_SPE_Advanced`
- Fix: `BSP_enableInterrupts()` was incorrectly calling `noInterrupts()`
- Fix: `setRX/TX/SCK` called on concrete `SPIClassRP2040` type (not abstract pointer)
- Verbose diagnostic example with device ID, MSE link quality, and MAC stat counters
