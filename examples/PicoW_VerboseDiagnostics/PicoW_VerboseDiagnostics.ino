/*
 * =============================================================================
 *  ViSPE ADIN1110 — Single Pair Ethernet Library  |  Version 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  Example  : PicoW_VerboseDiagnostics
 *  Hardware : VI SPE RP PICO 2 #1-R0 FNL
 *
 *  Prints full ADIN1110 identity at boot:
 *    - SPI pin map
 *    - PHY ID, OUI, Model Number, Revision, Digital Rev, Package Type
 *    - Link wait heartbeat with elapsed timer
 *    - MSE Link Quality + SQI on link-up
 *    - Per-message TX log
 *    - Full MAC statistics every 5 messages
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
 *   - Analog Devices ADIN1110 driver  © 2020,2021 Analog Devices, Inc.
 *   - Arduino port adapted from SparkFun ADIN1110 Library (MIT)
 *  License: MIT — see LICENSE.md
 * =============================================================================
 */

#include <SPI.h>
#include "Vi_SinglePairEthernet.h"

// ---------------------------------------------------------------------------
// LED configuration
// ---------------------------------------------------------------------------
#define LED_STATUS      LED_BUILTIN  // Pico W onboard LED (GP25)
#define BLINK_NO_LINK   200          // ms — fast blink while searching for link
#define BLINK_LINK_UP   1000         // ms — slow blink when connected
#define BLINK_TX_FLASH  50           // ms — brief flash on each TX

static unsigned long ledLastToggle = 0;
static bool          ledState      = false;
static bool          txFlash       = false;
static unsigned long txFlashEnd    = 0;

static void triggerTxFlash()
{
    digitalWrite(LED_STATUS, HIGH);
    txFlash    = true;
    txFlashEnd = millis() + BLINK_TX_FLASH;
}

static void updateLED(bool linked)
{
    unsigned long now = millis();
    if (txFlash)
    {
        if (now >= txFlashEnd) { txFlash = false; ledState = false; digitalWrite(LED_STATUS, LOW); }
        return;
    }
    unsigned long period = linked ? BLINK_LINK_UP : BLINK_NO_LINK;
    if (now - ledLastToggle >= period)
    {
        ledLastToggle = now;
        ledState = !ledState;
        digitalWrite(LED_STATUS, ledState ? HIGH : LOW);
    }
}

// ---------------------------------------------------------------------------
// MAC addresses — one unique address per node
// ---------------------------------------------------------------------------
byte deviceMAC[6]      = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};  // THIS node
byte destinationMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};  // REMOTE node

ViSPE adin1110;

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

static void printSeparator(char c = '-', int len = 50)
{
    for (int i = 0; i < len; i++) Serial.print(c);
    Serial.println();
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------
static unsigned long rxCount = 0;

static void rxCallback(byte *data, int dataLen, byte *senderMac)
{
    rxCount++;
    Serial.print("\n[RX #");
    Serial.print(rxCount);
    Serial.print("]  from ");
    printMac(senderMac);
    Serial.print("  len=");
    Serial.print(dataLen);
    Serial.print("  \"");
    for (int i = 0; i < min(dataLen, 80); i++)
    {
        char ch = (char)data[i];
        Serial.print((ch >= 0x20 && ch < 0x7F) ? ch : '.');
    }
    Serial.println("\"");
}

static void linkCallback(bool connected)
{
    Serial.println();
    printSeparator('=');
    Serial.print("  LINK STATUS: ");
    Serial.println(connected ? "*** UP ***" : "*** DOWN ***");
    printSeparator('=');
}

// ---------------------------------------------------------------------------
// Pin map
// ---------------------------------------------------------------------------
static void printPinMap()
{
    printSeparator();
    Serial.println("  Pin Map  (VI SPE RP PICO 2 #1-R0 FNL schematic)");
    printSeparator();
    Serial.println("  Signal        GPIO   SPI bus");
    Serial.println("  SCLK          GP10   SPI1 SCK");
    Serial.println("  SDI / MOSI    GP11   SPI1 TX");
    Serial.println("  SDO / MISO    GP12   SPI1 RX  (SPI_CFG0)");
    Serial.println("  CS            GP13");
    Serial.println("  RESET         GP7");
    Serial.println("  INT           GP6");
    printSeparator();
}

// ---------------------------------------------------------------------------
// ADIN1110 device identity registers
// ---------------------------------------------------------------------------
static void printDeviceId()
{
    adin1110_DeviceId_t devId;
    adi_eth_Result_e result = adin1110.getDeviceId(&devId);

    printSeparator();
    Serial.println("  ADIN1110 Device Identity");
    printSeparator();

    if (result != ADI_ETH_SUCCESS)
    {
        Serial.print("  ERROR reading device ID: 0x");
        Serial.println(result, HEX);
        return;
    }

    // PHY ID (32-bit combined register)
    Serial.print("  PHY ID Raw    : 0x");
    if (devId.phyId < 0x10000000UL) Serial.print('0');
    Serial.println(devId.phyId, HEX);

    // OUI — 22-bit field, shift left 2 to recover IEEE 24-bit OUI
    uint32_t oui24 = (uint32_t)(devId.oui) << 2;
    Serial.print("  OUI           : 0x");
    if (oui24 < 0x100000UL) Serial.print('0');
    Serial.println(oui24, HEX);

    // Model number (6 bits — 9 = ADIN1110)
    Serial.print("  Model Num     : 0x");
    Serial.print(devId.modelNum, HEX);
    Serial.print("  (");
    Serial.print(devId.modelNum, DEC);
    Serial.println(')');

    // PHY revision (4 bits)
    Serial.print("  PHY Rev       : ");
    Serial.println(devId.revNum, DEC);

    // Digital revision
    Serial.print("  Digital Rev   : 0x");
    Serial.println(devId.digRevNum, HEX);

    // Package type
    Serial.print("  Package Type  : 0x");
    Serial.print(devId.pkgType, HEX);
    switch (devId.pkgType)
    {
        case 0x0: Serial.println("  (32-LFCSP)"); break;
        case 0x1: Serial.println("  (24-LFCSP)"); break;
        default:  Serial.println("  (unknown)");  break;
    }
    printSeparator();
}

// ---------------------------------------------------------------------------
// MSE link quality — available once link is UP
// ---------------------------------------------------------------------------
static void printLinkQuality()
{
    adi_phy_MseLinkQuality_t mse;
    adi_eth_Result_e result = adin1110.getMseLinkQuality(&mse);
    if (result == ADI_ETH_SUCCESS)
    {
        Serial.print("  Link Quality  MSE=0x");
        Serial.print(mse.mseVal, HEX);
        Serial.print("  SQI=");
        Serial.print(mse.sqi, DEC);
        Serial.print("  Quality=");
        switch (mse.linkQuality)
        {
            case ADI_PHY_LINK_QUALITY_GOOD:     Serial.println("GOOD");     break;
            case ADI_PHY_LINK_QUALITY_MARGINAL: Serial.println("MARGINAL"); break;
            case ADI_PHY_LINK_QUALITY_POOR:     Serial.println("POOR");     break;
            default:                            Serial.println("UNKNOWN");  break;
        }
    }
    else
    {
        Serial.print("  [MSE read failed: 0x");
        Serial.print(result, HEX);
        Serial.println(']');
    }
}

// ---------------------------------------------------------------------------
// Full MAC statistics counter dump
// ---------------------------------------------------------------------------
static void printStatCounters()
{
    adi_eth_MacStatCounters_t stat;
    adi_eth_Result_e result = adin1110.getStatCounters(&stat);
    if (result != ADI_ETH_SUCCESS)
    {
        Serial.print("  [Stat read failed: 0x");
        Serial.print(result, HEX);
        Serial.println(']');
        return;
    }

    printSeparator('-', 40);
    Serial.println("  MAC Counters");
    printSeparator('-', 40);
    Serial.print("  TX Frames        : "); Serial.println(stat.TX_FRM_CNT);
    Serial.print("  TX Unicast       : "); Serial.println(stat.TX_UCAST_CNT);
    Serial.print("  TX Multicast     : "); Serial.println(stat.TX_MCAST_CNT);
    Serial.print("  TX Broadcast     : "); Serial.println(stat.TX_BCAST_CNT);
    Serial.print("  RX Frames        : "); Serial.println(stat.RX_FRM_CNT);
    Serial.print("  RX Unicast       : "); Serial.println(stat.RX_UCAST_CNT);
    Serial.print("  RX Multicast     : "); Serial.println(stat.RX_MCAST_CNT);
    Serial.print("  RX Broadcast     : "); Serial.println(stat.RX_BCAST_CNT);
    Serial.print("  RX CRC Errors    : "); Serial.println(stat.RX_CRC_ERR_CNT);
    Serial.print("  RX Align Errors  : "); Serial.println(stat.RX_ALGN_ERR_CNT);
    Serial.print("  RX Len/Short Err : "); Serial.println(stat.RX_LS_ERR_CNT);
    Serial.print("  RX PHY Errors    : "); Serial.println(stat.RX_PHY_ERR_CNT);
    Serial.print("  RX Drop (Full)   : "); Serial.println(stat.RX_DROP_FULL_CNT);
    Serial.print("  RX Drop (Filter) : "); Serial.println(stat.RX_DROP_FILT_CNT);
    printSeparator('-', 40);
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(2000);   // Wait for USB serial to enumerate on Pico W

    printSeparator('=');
    Serial.println("  Vi Micro Systems Pvt Ltd");
    Serial.println("  ViSPE ADIN1110 — Verbose Diagnostics");
    Serial.print  ("  Library v");
    Serial.print  (VISPE_VERSION_MAJOR); Serial.print('.');
    Serial.print  (VISPE_VERSION_MINOR); Serial.print('.');
    Serial.println(VISPE_VERSION_PATCH);
    Serial.print  ("  ADIN1110 Driver v");
    Serial.print  (ADIN1110_VERSION_MAJOR); Serial.print('.');
    Serial.print  (ADIN1110_VERSION_MINOR); Serial.print('.');
    Serial.println(ADIN1110_VERSION_PATCH);
    printSeparator('=');

    // ---- Pin map ----
    printPinMap();

    // ---- MAC addresses ----
    Serial.print("  Device MAC      : "); printMac(deviceMAC);      Serial.println();
    Serial.print("  Destination MAC : "); printMac(destinationMAC); Serial.println();
    printSeparator();

    // LED setup
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);

    // ---- Initialise — SPI1 + pins auto-selected for Pico W ----
    Serial.println("  Initialising ADIN1110...");
    if (!adin1110.begin(deviceMAC))
    {
        digitalWrite(LED_STATUS, HIGH);  // solid = fatal error
        Serial.println("\n  ERROR: begin() failed. Check:");
        Serial.println("    - Wiring matches VI SPE RP PICO 2 #1-R0 FNL schematic");
        Serial.println("    - 3.3 V power on ADIN1110 VDD");
        Serial.println("    - SPI_CFG0 (GP12) pulled LOW for generic SPI mode");
        while (true) { tight_loop_contents(); }
    }

    Serial.println("  ADIN1110 MACPHY : OK");

    // Enable ADIN1110 hardware LED pins (LED_0 = link, LED_1 = activity)
    adin1110.ledEn(true);
    Serial.println("  ADIN1110 LEDs   : enabled");

    // ---- Full device identity dump ----
    printDeviceId();

    // ---- Register callbacks ----
    adin1110.setRxCallback(rxCallback);
    adin1110.setLinkCallback(linkCallback);

    // ---- Wait for SPE link with live heartbeat ----
    Serial.print("  Waiting for SPE link");
    unsigned long t0 = millis();
    int dots = 0;
    while (!adin1110.getLinkStatus())
    {
        updateLED(false);   // fast blink while no link
        delay(250);
        Serial.print('.');
        if (++dots % 40 == 0)
        {
            Serial.print("  [");
            Serial.print((millis() - t0) / 1000);
            Serial.println("s]");
        }
    }
    Serial.println();
    Serial.print  ("  Link UP  —  time to link: ");
    Serial.print  ((millis() - t0) / 1000.0f, 2);
    Serial.println(" s");

    printLinkQuality();
    printSeparator('=');
    Serial.println("  Setup complete. Entering TX/RX loop.");
    printSeparator('=');
}

// ---------------------------------------------------------------------------
static unsigned long txCount  = 0;
static unsigned long loopIter = 0;

void loop()
{
    loopIter++;
    bool linked = adin1110.getLinkStatus();
    updateLED(linked);   // keep LED blinking non-blocking

    if (linked)
    {
        // Build payload
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "ViSPE msg#%lu  uptime=%lus  iter=%lu",
                 txCount, millis() / 1000UL, loopIter);

        // Transmit
        bool ok = adin1110.sendData(
            reinterpret_cast<byte *>(buf),
            static_cast<int>(strlen(buf) + 1),
            destinationMAC);

        Serial.print("[TX #");
        Serial.print(txCount);
        Serial.print("]  \"");
        Serial.print(buf);
        Serial.print("\"  ");
        Serial.println(ok ? "OK" : "FAILED (TX queue full?)");

        if (ok) { txCount++; triggerTxFlash(); }

        // Print full stats + link quality every 5 transmitted messages
        if (txCount > 0 && txCount % 5 == 0)
        {
            printStatCounters();
            printLinkQuality();
        }
    }
    else
    {
        Serial.println("[LOOP] Link is DOWN — waiting...");
    }

    delay(3000);
}
