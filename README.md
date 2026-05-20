# ViSPE ADIN1110 — Single Pair Ethernet Library
**Version 1.0.0** | Vi Micro Systems Pvt Ltd

**Author:** Abin Antony  
**Email:** abinantony.dev@gmail.com  
**Role:** R&D Engineer  
**Company:** Vi Micro Systems Pvt Ltd

---

## Overview

ViSPE ADIN1110 is an Arduino library for the **Analog Devices ADIN1110** 10BASE-T1L
Single Pair Ethernet MAC-PHY. It provides a clean, hardware-ready API for sending and
receiving Ethernet frames over a single twisted pair at 10 Mbit/s, with full support
for the **Raspberry Pi Pico W** on the **VI SPE RP PICO 2** hardware platform.

## Hardware — VI SPE RP PICO 2 Pin Map

| ADIN1110 Signal | Pico W GPIO | SPI Bus  |
|-----------------|-------------|----------|
| SCLK            | GP10        | SPI1 SCK |
| SDI (MOSI)      | GP11        | SPI1 TX  |
| SDO (MISO/CFG0) | GP12        | SPI1 RX  |
| CS              | GP13        | —        |
| RESET           | GP7         | —        |
| INT             | GP6         | —        |

## Quick Start

```cpp
#include "Vi_SinglePairEthernet.h"

byte myMAC[6]   = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};
byte peerMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

ViSPE eth;

void setup() {
    Serial.begin(115200);
    if (!eth.begin(myMAC)) {
        Serial.println("ADIN1110 init failed");
        while (true);
    }
    while (!eth.getLinkStatus()); // wait for link
    Serial.println("Link UP");
}

void loop() {
    const char *msg = "Hello SPE";
    eth.sendData((byte*)msg, strlen(msg)+1, peerMAC);
    delay(1000);
}
```

## Board Setup

- **Board manager:** Raspberry Pi Pico/RP2040 by Earle F. Philhower III
- **Board target:** Raspberry Pi Pico W
- No extra configuration needed — SPI1 and pins are automatically selected.

## Classes

| Class | Description |
|-------|-------------|
| `ViSPE` | Simple send/receive API. Recommended for most projects. |
| `Vi_SPE_Advanced` | Full ADIN1110 register access, FIFO control, timestamps, frame gen/check. |

## Attribution

This library is built upon:
- **Analog Devices ADIN1110 MAC-PHY Driver** — Copyright © 2020, 2021 Analog Devices, Inc.
  (adin1110.c/h, adi_mac.c/h, adi_phy.c/h, hal.c/h and related files)
- **SparkFun ADIN1110 Arduino Library** — Copyright © SparkFun Electronics (MIT License)
  (Arduino board support layer and C++ wrapper — adapted and rebranded)

See `ATTRIBUTION.md` for full details.

## License

MIT License — see `LICENSE.md`.
