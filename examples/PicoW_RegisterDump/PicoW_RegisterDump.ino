/*
 * =============================================================================
 *  ViSPE ADIN1110 — Single Pair Ethernet Library  |  Version 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  Example  : PicoW_RegisterDump
 *  Hardware : VI SPE RP PICO 2 #1-R0 FNL
 *
 *  Reads and prints every readable SPI register in the ADIN1110 register map
 *  (Table 38, ADIN1110 datasheet Rev C), comparing each value against the
 *  documented reset default. Deviations are flagged with " <<< CHANGED".
 *
 *  Outputs a formatted table to Serial at 115200 baud.
 *  Does NOT require a link partner — runs immediately after MACPHY init.
 *
 *  Register access types:
 *    R   — readable, printed
 *    R/W — readable, printed
 *    W   — write-only, skipped (marked [WO])
 * -----------------------------------------------------------------------------
 *  Hardware connections:
 *    SCLK=GP10  MOSI=GP11  MISO=GP12  CS=GP13  RESET=GP7  INT=GP6
 *  Board: Raspberry Pi Pico W  (Earle Philhower RP2040 core)
 * =============================================================================
 */

#include <SPI.h>
#include "Vi_SinglePairEthernet.h"

// Use advanced class — no need for full data-path init
Vi_SPE_Advanced mac;

// ---------------------------------------------------------------------------
// Register descriptor
// ---------------------------------------------------------------------------
struct RegEntry {
    uint16_t    addr;
    const char *name;
    uint32_t    resetVal;   // from datasheet Table 38
    bool        writeOnly;  // W-only registers cannot be read
};

// ---------------------------------------------------------------------------
// Full ADIN1110 SPI register table (Table 38, Rev C)
// Ranges are expanded into individual entries.
// ---------------------------------------------------------------------------
static const RegEntry REGS[] = {
    // ---- Core identification / configuration ----------------------------
    { 0x00, "IDVER",              0x00000010, false },
    { 0x01, "PHYID",              0x0283BC91, false },
    { 0x02, "CAPABILITY",         0x000006C3, false },
    { 0x03, "RESET",              0x00000000, true  },  // W only
    { 0x04, "CONFIG0",            0x00000006, false },
    { 0x06, "CONFIG2",            0x00000800, false },
    { 0x08, "STATUS0",            0x00000040, false },
    { 0x09, "STATUS1",            0x00000000, false },
    { 0x0B, "BUFSTS",             0x00007700, false },
    { 0x0C, "IMASK0",             0x00001FBF, false },
    { 0x0D, "IMASK1",             0x43FA1F1A, false },

    // ---- Transmit timestamps -------------------------------------------
    { 0x10, "TTSCAH",             0x00000000, false },
    { 0x11, "TTSCAL",             0x00000000, false },
    { 0x12, "TTSCBH",             0x00000000, false },
    { 0x13, "TTSCBL",             0x00000000, false },
    { 0x14, "TTSCCH",             0x00000000, false },
    { 0x15, "TTSCCL",             0x00000000, false },

    // ---- MDIO access (0x20–0x27) ----------------------------------------
    { 0x20, "MDIOACC0",           0x8C000000, false },
    { 0x21, "MDIOACC1",           0x8C000000, false },
    { 0x22, "MDIOACC2",           0x8C000000, false },
    { 0x23, "MDIOACC3",           0x8C000000, false },
    { 0x24, "MDIOACC4",           0x8C000000, false },
    { 0x25, "MDIOACC5",           0x8C000000, false },
    { 0x26, "MDIOACC6",           0x8C000000, false },
    { 0x27, "MDIOACC7",           0x8C000000, false },

    // ---- MAC TX ----------------------------------------------------------
    { 0x30, "TX_FSIZE",           0x00000000, false },
    { 0x31, "TX",                 0x00000000, true  },  // W only
    { 0x32, "TX_SPACE",           0x00000FFF, false },
    { 0x34, "TX_THRESH",          0x00000041, false },
    { 0x36, "FIFO_CLR",           0x00000000, true  },  // W only

    // ---- Scratch registers (0x37–0x3A) ----------------------------------
    { 0x37, "SCRATCH0",           0x00000000, false },
    { 0x38, "SCRATCH1",           0x00000000, false },
    { 0x39, "SCRATCH2",           0x00000000, false },
    { 0x3A, "SCRATCH3",           0x00000000, false },

    // ---- System status / reset -----------------------------------------
    { 0x3B, "MAC_RST_STATUS",     0x00000003, false },
    { 0x3C, "SOFT_RST",           0x00000000, true  },  // W only
    { 0x3D, "SPI_INJ_ERR",        0x00000000, false },
    { 0x3E, "FIFO_SIZE",          0x00000464, false },
    { 0x3F, "TFC",                0x00000000, false },
    { 0x40, "TXSIZE",             0x00000000, false },
    { 0x41, "HTX_OVF_FRM_CNT",   0x00000000, false },
    { 0x42, "MECC_ERR_ADDR",      0x00000000, false },

    // ---- Corrected ECC error counters (0x43–0x49) ----------------------
    { 0x43, "CECC_ERR0",          0x00000000, false },
    { 0x44, "CECC_ERR1",          0x00000000, false },
    { 0x45, "CECC_ERR2",          0x00000000, false },
    { 0x46, "CECC_ERR3",          0x00000000, false },
    { 0x47, "CECC_ERR4",          0x00000000, false },
    { 0x48, "CECC_ERR5",          0x00000000, false },
    { 0x49, "CECC_ERR6",          0x00000000, false },

    // ---- MAC address filter upper (0x50,0x52,...,0x6E) — 16 entries ---
    { 0x50, "ADDR_FILT_UPR0",     0x00000000, false },
    { 0x52, "ADDR_FILT_UPR1",     0x00000000, false },
    { 0x54, "ADDR_FILT_UPR2",     0x00000000, false },
    { 0x56, "ADDR_FILT_UPR3",     0x00000000, false },
    { 0x58, "ADDR_FILT_UPR4",     0x00000000, false },
    { 0x5A, "ADDR_FILT_UPR5",     0x00000000, false },
    { 0x5C, "ADDR_FILT_UPR6",     0x00000000, false },
    { 0x5E, "ADDR_FILT_UPR7",     0x00000000, false },
    { 0x60, "ADDR_FILT_UPR8",     0x00000000, false },
    { 0x62, "ADDR_FILT_UPR9",     0x00000000, false },
    { 0x64, "ADDR_FILT_UPR10",    0x00000000, false },
    { 0x66, "ADDR_FILT_UPR11",    0x00000000, false },
    { 0x68, "ADDR_FILT_UPR12",    0x00000000, false },
    { 0x6A, "ADDR_FILT_UPR13",    0x00000000, false },
    { 0x6C, "ADDR_FILT_UPR14",    0x00000000, false },
    { 0x6E, "ADDR_FILT_UPR15",    0x00000000, false },

    // ---- MAC address filter lower (0x51,0x53,...,0x6F) — 16 entries ---
    { 0x51, "ADDR_FILT_LWR0",     0x00000000, false },
    { 0x53, "ADDR_FILT_LWR1",     0x00000000, false },
    { 0x55, "ADDR_FILT_LWR2",     0x00000000, false },
    { 0x57, "ADDR_FILT_LWR3",     0x00000000, false },
    { 0x59, "ADDR_FILT_LWR4",     0x00000000, false },
    { 0x5B, "ADDR_FILT_LWR5",     0x00000000, false },
    { 0x5D, "ADDR_FILT_LWR6",     0x00000000, false },
    { 0x5F, "ADDR_FILT_LWR7",     0x00000000, false },
    { 0x61, "ADDR_FILT_LWR8",     0x00000000, false },
    { 0x63, "ADDR_FILT_LWR9",     0x00000000, false },
    { 0x65, "ADDR_FILT_LWR10",    0x00000000, false },
    { 0x67, "ADDR_FILT_LWR11",    0x00000000, false },
    { 0x69, "ADDR_FILT_LWR12",    0x00000000, false },
    { 0x6B, "ADDR_FILT_LWR13",    0x00000000, false },
    { 0x6D, "ADDR_FILT_LWR14",    0x00000000, false },
    { 0x6F, "ADDR_FILT_LWR15",    0x00000000, false },

    // ---- Address mask ---------------------------------------------------
    { 0x70, "ADDR_MSK_UPR0",      0x0000FFFF, false },
    { 0x72, "ADDR_MSK_UPR1",      0x0000FFFF, false },
    { 0x71, "ADDR_MSK_LWR0",      0xFFFFFFFF, false },
    { 0x73, "ADDR_MSK_LWR1",      0xFFFFFFFF, false },

    // ---- Timestamping ---------------------------------------------------
    { 0x80, "TS_ADDEND",          0x85555555, false },
    { 0x81, "TS_1SEC_CMP",        0x3B9ACA00, false },
    { 0x82, "TS_SEC_CNT",         0x00000000, false },
    { 0x83, "TS_NS_CNT",          0x00000000, false },
    { 0x84, "TS_CFG",             0x00000000, false },
    { 0x85, "TS_TIMER_HI",        0x00000000, false },
    { 0x86, "TS_TIMER_LO",        0x00000000, false },
    { 0x87, "TS_TIMER_QE_CORR",   0x00000000, false },
    { 0x88, "TS_TIMER_START",     0x00000000, false },
    { 0x89, "TS_EXT_CAPT0",       0x00000000, false },
    { 0x8A, "TS_EXT_CAPT1",       0x00000000, false },
    { 0x8B, "TS_FREECNT_CAPT",    0x00000000, false },

    // ---- Port 1 RX data path --------------------------------------------
    { 0x90, "P1_RX_FSIZE",        0x00000000, false },
    { 0x91, "P1_RX",              0x00000000, false },

    // ---- Port 1 statistics counters ------------------------------------
    { 0xA0, "P1_RX_FRM_CNT",      0x00000000, false },
    { 0xA1, "P1_RX_BCAST_CNT",    0x00000000, false },
    { 0xA2, "P1_RX_MCAST_CNT",    0x00000000, false },
    { 0xA3, "P1_RX_UCAST_CNT",    0x00000000, false },
    { 0xA4, "P1_RX_CRC_ERR_CNT",  0x00000000, false },
    { 0xA5, "P1_RX_ALGN_ERR_CNT", 0x00000000, false },
    { 0xA6, "P1_RX_LS_ERR_CNT",   0x00000000, false },
    { 0xA7, "P1_RX_PHY_ERR_CNT",  0x00000000, false },
    { 0xA8, "P1_TX_FRM_CNT",      0x00000000, false },
    { 0xA9, "P1_TX_BCAST_CNT",    0x00000000, false },
    { 0xAA, "P1_TX_MCAST_CNT",    0x00000000, false },
    { 0xAB, "P1_TX_UCAST_CNT",    0x00000000, false },
    { 0xAC, "P1_RX_DROP_FULL_CNT",0x00000000, false },
    { 0xAD, "P1_RX_DROP_FILT_CNT",0x00000000, false },
    { 0xAE, "P1_RX_IFG_ERR_CNT",  0x00000000, false },

    // ---- Port 1 configuration -----------------------------------------
    { 0xB0, "P1_TX_IFG",          0x0000000B, false },
    { 0xB3, "P1_LOOP",            0x00000000, false },
    { 0xB4, "P1_RX_CRC_EN",       0x00000001, false },
    { 0xB5, "P1_RX_IFG",          0x0000000A, false },
    { 0xB6, "P1_RX_MAX_LEN",      0x00000618, false },
    { 0xB7, "P1_RX_MIN_LEN",      0x00000040, false },
    { 0xB8, "P1_LO_RFC",          0x00000000, false },
    { 0xB9, "P1_HI_RFC",          0x00000000, false },
    { 0xBA, "P1_LO_RXSIZE",       0x00000000, false },
    { 0xBB, "P1_HI_RXSIZE",       0x00000000, false },
};

static const int REG_COUNT = (int)(sizeof(REGS) / sizeof(REGS[0]));

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void printHex8(uint32_t v)
{
    // Always print full 8 hex digits
    for (int s = 28; s >= 0; s -= 4)
    {
        Serial.print((char)("0123456789ABCDEF"[(v >> s) & 0xF]));
    }
}

static void printLine(char c = '-', int n = 72)
{
    for (int i = 0; i < n; i++) Serial.print(c);
    Serial.println();
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(2000);

    printLine('=');
    Serial.println("  Vi Micro Systems Pvt Ltd");
    Serial.println("  ViSPE ADIN1110 — Full SPI Register Dump");
    Serial.println("  Table 38, ADIN1110 Datasheet Rev C");
    printLine('=');

    // Initialise MAC only — no link required for register reads
    Serial.println("  Initialising ADIN1110...");
    adi_eth_Result_e result = mac.begin();
    if (result != ADI_ETH_SUCCESS)
    {
        Serial.print("  ERROR: begin() failed  code=0x");
        Serial.println(result, HEX);
        Serial.println("  Check wiring: SCLK=GP10 MOSI=GP11 MISO=GP12 CS=GP13 RST=GP7 INT=GP6");
        while (true) { tight_loop_contents(); }
    }
    Serial.println("  ADIN1110 MACPHY : OK");
    Serial.println();

    // ---------------------------------------------------------------------------
    // Print table header
    // ---------------------------------------------------------------------------
    printLine();
    Serial.println("  Addr  Register Name         Reset Value  Read Value   Status");
    printLine();

    int changed = 0;
    int errors  = 0;
    int skipped = 0;

    for (int i = 0; i < REG_COUNT; i++)
    {
        const RegEntry &r = REGS[i];

        // Address column
        Serial.print("  0x");
        if (r.addr < 0x10) Serial.print('0');
        Serial.print(r.addr, HEX);
        Serial.print("  ");

        // Name column (padded to 22 chars)
        int nameLen = strlen(r.name);
        Serial.print(r.name);
        for (int p = nameLen; p < 22; p++) Serial.print(' ');

        if (r.writeOnly)
        {
            Serial.println("  [WO]                          Write-only");
            skipped++;
            continue;
        }

        // Reset value column
        Serial.print("  0x"); printHex8(r.resetVal); Serial.print("   ");

        // Read register
        uint32_t val = 0;
        adi_eth_Result_e res = mac.readRegister(r.addr, &val);

        if (res != ADI_ETH_SUCCESS)
        {
            Serial.print("  READ ERROR (0x");
            Serial.print(res, HEX);
            Serial.println(')');
            errors++;
            continue;
        }

        // Read value column
        Serial.print("0x"); printHex8(val);

        // Compare with reset default
        if (val != r.resetVal)
        {
            Serial.print("   <<< CHANGED");
            changed++;
        }
        else
        {
            Serial.print("   OK");
        }
        Serial.println();
    }

    // ---------------------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------------------
    printLine();
    Serial.print("  Total registers : "); Serial.println(REG_COUNT);
    Serial.print("  Readable        : "); Serial.println(REG_COUNT - skipped);
    Serial.print("  Write-only (skipped): "); Serial.println(skipped);
    Serial.print("  Read errors     : "); Serial.println(errors);
    Serial.print("  Changed from reset default: "); Serial.println(changed);
    printLine('=');

    if (errors == 0)
        Serial.println("  SPI communication : PASS — all readable registers returned data.");
    else
        Serial.println("  SPI communication : FAIL — check SPI wiring / power.");

    printLine('=');
    Serial.println("  Register dump complete. Loop idle.");
}

void loop()
{
    // Nothing — one-shot dump in setup()
    delay(10000);
}
