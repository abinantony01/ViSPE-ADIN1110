/*
 * =============================================================================
 *  ViSPE ADIN1110 — Single Pair Ethernet Library  |  Version 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  Example  : PicoW_BasicSendReceive
 *  Hardware : VI SPE RP PICO 2 #1-R0 FNL
 * -----------------------------------------------------------------------------
 *  Hardware connections:
 *  ┌─────────────────┬──────────┬───────────┐
 *  │ ADIN1110 Signal │ Pico W   │ SPI Bus   │
 *  ├─────────────────┼──────────┼───────────┤
 *  │ SCLK            │ GP10     │ SPI1 SCK  │
 *  │ SDI (MOSI)      │ GP11     │ SPI1 TX   │
 *  │ SDO (MISO/CFG0) │ GP12     │ SPI1 RX   │
 *  │ CS              │ GP13     │ —         │
 *  │ RESET           │ GP7      │ —         │
 *  │ INT             │ GP6      │ —         │
 *  └─────────────────┴──────────┴───────────┘
 *
 *  Board manager : Raspberry Pi Pico/RP2040 by Earle F. Philhower III
 *  Board target  : Raspberry Pi Pico W
 * -----------------------------------------------------------------------------
 *  Attribution:
 *   - Analog Devices ADIN1110 driver © 2020,2021 Analog Devices, Inc.
 *   - Arduino port adapted from SparkFun ADIN1110 Library (MIT)
 *  License: MIT — see LICENSE.md
 * =============================================================================
 */

#include <SPI.h>
#include "Vi_SinglePairEthernet.h"

// ---------------------------------------------------------------------------
// MAC addresses — assign a unique address per node
// ---------------------------------------------------------------------------
byte myMAC[6]   = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};   // THIS board
byte peerMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};   // REMOTE board

ViSPE eth;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void printMac(const byte *mac)
{
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 0x10) Serial.print('0');
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(':');
    }
}

static void printLine(char c = '-', int n = 52)
{
    for (int i = 0; i < n; i++) Serial.print(c);
    Serial.println();
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
static unsigned long rxCount = 0;

static void onRxData(byte *data, int dataLen, byte *senderMac)
{
    rxCount++;
    Serial.print("\n[RX #"); Serial.print(rxCount);
    Serial.print("] from "); printMac(senderMac);
    Serial.print("  len="); Serial.print(dataLen);
    Serial.print("  \"");
    for (int i = 0; i < min(dataLen, 80); i++)
    {
        char ch = (char)data[i];
        Serial.print((ch >= 0x20 && ch < 0x7F) ? ch : '.');
    }
    Serial.println("\"");
}

static void onLinkChange(bool connected)
{
    Serial.println();
    printLine('=');
    Serial.print("  LINK: "); Serial.println(connected ? "*** UP ***" : "*** DOWN ***");
    printLine('=');
}

// ---------------------------------------------------------------------------
// Print full device identity
// ---------------------------------------------------------------------------
static void printDeviceId()
{
    adin1110_DeviceId_t devId;
    if (eth.getDeviceId(&devId) != ADI_ETH_SUCCESS)
    {
        Serial.println("  [Device ID read failed]");
        return;
    }
    printLine();
    Serial.println("  ADIN1110 Device Identity");
    printLine();
    Serial.print("  PHY ID Raw    : 0x"); Serial.println(devId.phyId, HEX);
    Serial.print("  OUI           : 0x"); Serial.println((uint32_t)(devId.oui) << 2, HEX);
    Serial.print("  Model Num     : 0x"); Serial.print(devId.modelNum, HEX);
    Serial.print("  ("); Serial.print(devId.modelNum, DEC); Serial.println(')');
    Serial.print("  PHY Revision  : "); Serial.println(devId.revNum, DEC);
    Serial.print("  Digital Rev   : 0x"); Serial.println(devId.digRevNum, HEX);
    Serial.print("  Package       : ");
    switch (devId.pkgType)
    {
        case 0x0: Serial.println("32-LFCSP"); break;
        case 0x1: Serial.println("24-LFCSP"); break;
        default:  Serial.println("Unknown");  break;
    }
    printLine();
}

// ---------------------------------------------------------------------------
// Print link quality (MSE / SQI)
// ---------------------------------------------------------------------------
static void printLinkQuality()
{
    adi_phy_MseLinkQuality_t mse;
    if (eth.getMseLinkQuality(&mse) == ADI_ETH_SUCCESS)
    {
        Serial.print("  Link Quality  MSE=0x"); Serial.print(mse.mseVal, HEX);
        Serial.print("  SQI="); Serial.print(mse.sqi, DEC);
        Serial.print("  Quality=");
        switch (mse.linkQuality)
        {
            case ADI_PHY_LINK_QUALITY_GOOD:     Serial.println("GOOD");     break;
            case ADI_PHY_LINK_QUALITY_MARGINAL: Serial.println("MARGINAL"); break;
            case ADI_PHY_LINK_QUALITY_POOR:     Serial.println("POOR");     break;
            default:                            Serial.println("UNKNOWN");  break;
        }
    }
}

// ---------------------------------------------------------------------------
// Print MAC statistics
// ---------------------------------------------------------------------------
static void printStats()
{
    adi_eth_MacStatCounters_t s;
    if (eth.getStatCounters(&s) != ADI_ETH_SUCCESS) return;
    printLine('-', 42);
    Serial.println("  MAC Statistics");
    printLine('-', 42);
    Serial.print("  TX Frames        : "); Serial.println(s.TX_FRM_CNT);
    Serial.print("  TX Unicast       : "); Serial.println(s.TX_UCAST_CNT);
    Serial.print("  TX Multicast     : "); Serial.println(s.TX_MCAST_CNT);
    Serial.print("  TX Broadcast     : "); Serial.println(s.TX_BCAST_CNT);
    Serial.print("  RX Frames        : "); Serial.println(s.RX_FRM_CNT);
    Serial.print("  RX Unicast       : "); Serial.println(s.RX_UCAST_CNT);
    Serial.print("  RX Multicast     : "); Serial.println(s.RX_MCAST_CNT);
    Serial.print("  RX Broadcast     : "); Serial.println(s.RX_BCAST_CNT);
    Serial.print("  RX CRC Errors    : "); Serial.println(s.RX_CRC_ERR_CNT);
    Serial.print("  RX Align Errors  : "); Serial.println(s.RX_ALGN_ERR_CNT);
    Serial.print("  RX Drop (Full)   : "); Serial.println(s.RX_DROP_FULL_CNT);
    Serial.print("  RX Drop (Filter) : "); Serial.println(s.RX_DROP_FILT_CNT);
    printLine('-', 42);
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(2000);

    printLine('=');
    Serial.println("  Vi Micro Systems Pvt Ltd");
    Serial.println("  ViSPE ADIN1110 — Single Pair Ethernet");
    Serial.print  ("  Library v");
    Serial.print  (VISPE_VERSION_MAJOR); Serial.print('.');
    Serial.print  (VISPE_VERSION_MINOR); Serial.print('.');
    Serial.println(VISPE_VERSION_PATCH);
    Serial.println("  Hardware: VI SPE RP PICO 2 #1-R0 FNL");
    printLine('=');

    // Pin map
    printLine();
    Serial.println("  Pin Map");
    printLine();
    Serial.println("  SCLK  GP10  SPI1 SCK");
    Serial.println("  MOSI  GP11  SPI1 TX");
    Serial.println("  MISO  GP12  SPI1 RX (SPI_CFG0)");
    Serial.println("  CS    GP13");
    Serial.println("  RESET GP7");
    Serial.println("  INT   GP6");
    printLine();
    Serial.print("  Node MAC : "); printMac(myMAC);   Serial.println();
    Serial.print("  Peer MAC : "); printMac(peerMAC); Serial.println();
    printLine();

    // Initialise — SPI1 and pins are auto-selected for Pico W
    Serial.println("  Initialising ADIN1110...");
    if (!eth.begin(myMAC))
    {
        Serial.println("\n  ERROR: ADIN1110 initialisation failed!");
        Serial.println("  Check wiring against VI SPE RP PICO 2 #1-R0 FNL schematic.");
        while (true) { tight_loop_contents(); }
    }
    Serial.println("  ADIN1110 MACPHY : OK");

    printDeviceId();

    eth.setRxCallback(onRxData);
    eth.setLinkCallback(onLinkChange);

    // Wait for link
    Serial.print("  Waiting for SPE link");
    unsigned long t0 = millis();
    int dots = 0;
    while (!eth.getLinkStatus())
    {
        delay(250);
        Serial.print('.');
        if (++dots % 40 == 0)
        {
            Serial.print(" ["); Serial.print((millis()-t0)/1000); Serial.println("s]");
        }
    }
    Serial.println();
    Serial.print("  Link UP  — time to link: ");
    Serial.print((millis()-t0)/1000.0f, 2);
    Serial.println(" s");
    printLinkQuality();
    printLine('=');
    Serial.println("  Ready — entering TX/RX loop.");
    printLine('=');
}

// ---------------------------------------------------------------------------
static unsigned long txCount = 0;

void loop()
{
    if (eth.getLinkStatus())
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "ViSPE msg#%lu  uptime=%lus  node=00:E0:22:FE:DA:C9",
                 txCount, millis()/1000UL);

        bool ok = eth.sendData(reinterpret_cast<byte*>(buf),
                               static_cast<int>(strlen(buf)+1),
                               peerMAC);

        Serial.print("[TX #"); Serial.print(txCount);
        Serial.print("] \""); Serial.print(buf);
        Serial.print("\"  "); Serial.println(ok ? "OK" : "FAILED");

        if (ok) txCount++;

        if (txCount > 0 && txCount % 5 == 0)
        {
            printStats();
            printLinkQuality();
        }
    }
    else
    {
        Serial.println("[LOOP] Link DOWN — waiting...");
    }
    delay(3000);
}
