/*
 * =============================================================================
 *  ViSPE ADIN1110 — Single Pair Ethernet Library
 *  Version  : 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  Target   : Raspberry Pi Pico W  (VI SPE RP PICO 2 hardware)
 *             SPI1 — SCK=GP10, MOSI=GP11, MISO=GP12
 *             CS=GP13  RESET=GP7  INT=GP6
 * -----------------------------------------------------------------------------
 *  Attribution:
 *   - Analog Devices ADIN1110 driver  © 2020,2021 Analog Devices, Inc.
 *   - Arduino wrapper adapted from SparkFun ADIN1110 Library (MIT)
 *     © SparkFun Electronics / Kyle Wenner
 *  See ATTRIBUTION.md for full details.
 * -----------------------------------------------------------------------------
 *  License  : MIT  —  see LICENSE.md
 * =============================================================================
 */

#ifndef VI_SINGLE_PAIR_ETHERNET_H
#define VI_SINGLE_PAIR_ETHERNET_H

#include "Vi_SPE_Advanced.h"

/* -------------------------------------------------------------------------
 * Library version
 * ---------------------------------------------------------------------- */
#define VISPE_VERSION_MAJOR  (1)
#define VISPE_VERSION_MINOR  (0)
#define VISPE_VERSION_PATCH  (0)

/* -------------------------------------------------------------------------
 * Default MAC addresses (Analog Devices example values — change per node)
 * ---------------------------------------------------------------------- */
#define VISPE_MAC_ADDR_0_0   (0x00)
#define VISPE_MAC_ADDR_0_1   (0xE0)
#define VISPE_MAC_ADDR_0_2   (0x22)
#define VISPE_MAC_ADDR_0_3   (0xFE)
#define VISPE_MAC_ADDR_0_4   (0xDA)
#define VISPE_MAC_ADDR_0_5   (0xC9)

#define VISPE_MAC_ADDR_1_0   (0x00)
#define VISPE_MAC_ADDR_1_1   (0xE0)
#define VISPE_MAC_ADDR_1_2   (0x22)
#define VISPE_MAC_ADDR_1_3   (0xFE)
#define VISPE_MAC_ADDR_1_4   (0xDA)
#define VISPE_MAC_ADDR_1_5   (0xCA)

/* -------------------------------------------------------------------------
 * Frame size constants
 * ---------------------------------------------------------------------- */
const int VISPE_NUM_BUFS          = 4;
const int VISPE_FRAME_SIZE        = 1518;
const int VISPE_MAX_BUF_FRAME_SIZE = (VISPE_FRAME_SIZE + 4 + 2);
const int VISPE_MIN_PAYLOAD_SIZE  = 46;
const int VISPE_MAC_SIZE          = 6;
const int VISPE_FRAME_HEADER_SIZE = (2 * VISPE_MAC_SIZE + 2);
const int VISPE_ETHERTYPE_IPV4_B0 = 0x80;
const int VISPE_ETHERTYPE_IPV4_B1 = 0x00;

/* =========================================================================
 * ViSPE — simple send/receive class
 * Inherits the full ADIN1110 register API from Vi_SPE_Advanced.
 * ====================================================================== */
class ViSPE : public Vi_SPE_Advanced
{
private:
    adi_eth_Result_e    enableDefaultBehavior   ();

    uint8_t rxBuf[VISPE_NUM_BUFS][VISPE_MAX_BUF_FRAME_SIZE] HAL_ALIGNED_ATTRIBUTE(4);
    uint8_t txBuf[VISPE_NUM_BUFS][VISPE_MAX_BUF_FRAME_SIZE] HAL_ALIGNED_ATTRIBUTE(4);

    adi_eth_BufDesc_t rxBufDesc[VISPE_NUM_BUFS];
    adi_eth_BufDesc_t txBufDesc[VISPE_NUM_BUFS];

    bool txBufAvailable[VISPE_NUM_BUFS];
    bool rxBufAvailable[VISPE_NUM_BUFS];

    uint32_t txBufIdx;
    uint32_t rxBufIdx;
    bool     rxSinceLastCheck;

    uint8_t macAddr[VISPE_MAC_SIZE]     = {VISPE_MAC_ADDR_0_0, VISPE_MAC_ADDR_0_1,
                                           VISPE_MAC_ADDR_0_2, VISPE_MAC_ADDR_0_3,
                                           VISPE_MAC_ADDR_0_4, VISPE_MAC_ADDR_0_5};
    uint8_t destMacAddr[VISPE_MAC_SIZE] = {VISPE_MAC_ADDR_1_0, VISPE_MAC_ADDR_1_1,
                                           VISPE_MAC_ADDR_1_2, VISPE_MAC_ADDR_1_3,
                                           VISPE_MAC_ADDR_1_4, VISPE_MAC_ADDR_1_5};

    volatile adi_eth_LinkStatus_e linkStatus;

public:
    /* ---- Initialisation ------------------------------------------------ */
    bool begin (uint8_t *mac, uint8_t cs_pin = DEFAULT_ETH_SPI_CS_Pin);
    bool begin (uint8_t *mac, uint8_t status, uint8_t interrupt,
                uint8_t reset, uint8_t chip_select);

    /* ---- Data transfer ------------------------------------------------- */
    bool sendData   (uint8_t *data, int dataLen, uint8_t *destMac);
    bool sendData   (uint8_t *data, int dataLen);
    int  getRxData  (uint8_t *data, int dataLen, uint8_t *senderMac);
    bool getRxAvailable ();

    /* ---- MAC address helpers ------------------------------------------- */
    void setMac         (uint8_t *mac);
    void getMac         (uint8_t *mac);
    void setDestMac     (uint8_t *mac);
    bool identicalMacs (uint8_t *mac1, uint8_t *mac2);

    /* ---- Callbacks ------------------------------------------------------ */
    void setRxCallback   (void (*cbFunc)(uint8_t *, int, uint8_t *));
    void setLinkCallback (void (*cbFunc)(bool));
    bool getLinkStatus   ();

    /* User-supplied callback pointers */
    void (*userRxCallback)  (uint8_t *data, int dataLen, uint8_t *senderMac);
    void (*userLinkCallback)(bool connected);

    /* Internal C-compatible static shims */
    static void txCallback_C   (void *pCBParam, uint32_t Event, void *pArg);
    static void rxCallback_C   (void *pCBParam, uint32_t Event, void *pArg);
    static void linkCallback_C (void *pCBParam, uint32_t Event, void *pArg);

    void txCallback   (void *pCBParam, uint32_t Event, void *pArg);
    void rxCallback   (void *pCBParam, uint32_t Event, void *pArg);
    void linkCallback (void *pCBParam, uint32_t Event, void *pArg);
};

#endif /* VI_SINGLE_PAIR_ETHERNET_H */
