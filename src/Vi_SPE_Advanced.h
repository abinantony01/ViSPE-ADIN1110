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
 *  Vi_SPE_Advanced.h — Full ADIN1110 register-level access class.
 *  Inherit from this for custom low-level control of the MAC-PHY.
 * -----------------------------------------------------------------------------
 *  Attribution:
 *   - Analog Devices ADIN1110 driver  © 2020,2021 Analog Devices, Inc.
 *   - Adapted from sfe_spe_advanced.h by SparkFun Electronics (MIT)
 *  License  : MIT  —  see LICENSE.md
 * =============================================================================
 */

#ifndef VI_SPE_ADVANCED_H
#define VI_SPE_ADVANCED_H

#include <SPI.h>
#include "adin1110.h"
#include "boardsupport.h"

#if defined(ARDUINO) && ARDUINO >= 100
#  include "Arduino.h"
#else
#  include "WProgram.h"
#endif

/* =========================================================================
 * Vi_SPE_Advanced
 * Thin C++ wrapper around the Analog Devices ADIN1110 C driver.
 * All ADIN1110 functions are exposed as member methods.
 * ====================================================================== */
class Vi_SPE_Advanced
{
private:
    uint8_t devMem[ADIN1110_DEVICE_SIZE];
    adin1110_DriverConfig_t drvConfig = {
        .pDevMem    = (void *)devMem,
        .devMemSize = sizeof(devMem),
        .fcsCheckEn = false,
    };

public:
    /* ---- Initialisation ------------------------------------------------ */
    adi_eth_Result_e begin (uint8_t cs_pin = DEFAULT_ETH_SPI_CS_Pin);
    adi_eth_Result_e begin (uint8_t status, uint8_t interrupt,
                            uint8_t reset, uint8_t chip_select);

    /**
     * @brief Select the SPI bus (e.g. &SPI1 for Pico W). Call BEFORE begin().
     */
    void setSPIInstance (SPIClass *spiInstance);

    /**
     * @brief Override SPI pin assignments (RP2040 / Pico W). Call BEFORE begin().
     * @param sck   GPIO number for SCK  (e.g. 10 for GP10)
     * @param mosi  GPIO number for MOSI (e.g. 11 for GP11)
     * @param miso  GPIO number for MISO (e.g. 12 for GP12)
     */
    void setSPIPins (int8_t sck, int8_t mosi, int8_t miso);

    /* ---- Device control ----------------------------------------------- */
    adi_eth_Result_e init           (adin1110_DriverConfig_t *pCfg);
    adi_eth_Result_e unInit         ();
    adi_eth_Result_e getDeviceId    (adin1110_DeviceId_t *pDevId);
    adi_eth_Result_e enable         ();
    adi_eth_Result_e disable        ();
    adi_eth_Result_e reset          (adi_eth_ResetType_e resetType);
    adi_eth_Result_e syncConfig     ();
    adi_eth_Result_e getLinkStatus  (adi_eth_LinkStatus_e *linkStatus);
    adi_eth_Result_e getStatCounters(adi_eth_MacStatCounters_t *stat);
    adi_eth_Result_e ledEn          (bool enable);
    adi_eth_Result_e setLoopbackMode(adi_phy_LoopbackMode_e loopbackMode);
    adi_eth_Result_e setTestMode    (adi_phy_TestMode_e testMode);

    /* ---- Address filtering -------------------------------------------- */
    adi_eth_Result_e addAddressFilter  (uint8_t *macAddr, uint8_t *macAddrMask, uint32_t priority);
    adi_eth_Result_e clearAddressFilter(uint32_t addrIndex);

    /* ---- Data path ------------------------------------------------------- */
    adi_eth_Result_e submitTxBuffer (adi_eth_BufDesc_t *pBufDesc);
    adi_eth_Result_e submitRxBuffer (adi_eth_BufDesc_t *pBufDesc);
#if defined(ADI_MAC_ENABLE_RX_QUEUE_HI_PRIO)
    adi_eth_Result_e submitRxBufferHp(adi_eth_BufDesc_t *pBufDesc);
#endif

    /* ---- Promiscuous mode ------------------------------------------------ */
    adi_eth_Result_e setPromiscuousMode(bool bFlag);
    adi_eth_Result_e getPromiscuousMode(bool *pFlag);

    /* ---- SPI Open Alliance (OA) ---------------------------------------- */
#if defined(SPI_OA_EN)
    adi_eth_Result_e setChunkSize(adi_mac_OaCps_e cps);
    adi_eth_Result_e getChunkSize(adi_mac_OaCps_e *pCps);
#endif

    /* ---- Cut-through / FIFO --------------------------------------------- */
    adi_eth_Result_e setCutThroughMode(bool txcte, bool rxcte);
    adi_eth_Result_e getCutThroughMode(bool *pTxcte, bool *pRxcte);
    adi_eth_Result_e setFifoSizes     (adi_mac_FifoSizes_t fifoSizes);
    adi_eth_Result_e getFifoSizes     (adi_mac_FifoSizes_t *pFifoSizes);
    adi_eth_Result_e clearFifos       (adi_mac_FifoClrMode_e clearMode);

    /* ---- Timestamping --------------------------------------------------- */
    adi_eth_Result_e tsEnable             (adi_mac_TsFormat_e format);
    adi_eth_Result_e tsClear              ();
    adi_eth_Result_e tsTimerStart         (adi_mac_TsTimerConfig_t *pTimerConfig);
    adi_eth_Result_e tsTimerStop          ();
    adi_eth_Result_e tsSetTimerAbsolute   (uint32_t seconds, uint32_t nanoseconds);
    adi_eth_Result_e tsSyncClock          (int64_t tError, uint64_t refNsDiff, uint64_t localNsDiff);
    adi_eth_Result_e tsGetExtCaptTimestamp(adi_mac_TsTimespec_t *pCapturedTimespec);
    adi_eth_Result_e tsGetEgressTimestamp (adi_mac_EgressCapture_e egressReg, adi_mac_TsTimespec_t *pCapturedTimespec);
    adi_eth_Result_e tsConvert            (uint32_t tsLow, uint32_t tsHigh, adi_mac_TsFormat_e format, adi_mac_TsTimespec_t *pTimespec);
    int64_t          tsSubtract           (adi_mac_TsTimespec_t *pTsA, adi_mac_TsTimespec_t *pTsB);

    /* ---- Callbacks and context ------------------------------------------ */
    adi_eth_Result_e registerCallback(adi_eth_Callback_t cbFunc, adi_mac_InterruptEvt_e cbEvent);
    adi_eth_Result_e setUserContext  (void *pContext);
    void *           getUserContext  ();

    /* ---- Register access ----------------------------------------------- */
    adi_eth_Result_e writeRegister(uint16_t regAddr, uint32_t regData);
    adi_eth_Result_e readRegister (uint16_t regAddr, uint32_t *regData);
    adi_eth_Result_e phyWrite     (uint32_t regAddr, uint16_t regData);
    adi_eth_Result_e phyRead      (uint32_t regAddr, uint16_t *regData);

    /* ---- PHY diagnostics ----------------------------------------------- */
    adi_eth_Result_e getMseLinkQuality     (adi_phy_MseLinkQuality_t *mseLinkQuality);
    adi_eth_Result_e frameGenEn            (bool enable);
    adi_eth_Result_e frameGenSetMode       (adi_phy_FrameGenMode_e mode);
    adi_eth_Result_e frameGenSetFrameCnt   (uint32_t frameCnt);
    adi_eth_Result_e frameGenSetFramePayload(adi_phy_FrameGenPayload_e payload);
    adi_eth_Result_e frameGenSetFrameLen   (uint16_t frameLen);
    adi_eth_Result_e frameGenSetIfgLen     (uint16_t ifgLen);
    adi_eth_Result_e frameGenRestart       ();
    adi_eth_Result_e frameGenDone          (bool *fgDone);
    adi_eth_Result_e frameChkEn            (bool enable);
    adi_eth_Result_e frameChkSourceSelect  (adi_phy_FrameChkSource_e source);
    adi_eth_Result_e frameChkReadFrameCnt  (uint32_t *cnt);
    adi_eth_Result_e frameChkReadRxErrCnt  (uint16_t *cnt);
    adi_eth_Result_e frameChkReadErrorCnt  (adi_phy_FrameChkErrorCounters_t *cnt);

    /* ---- Device handle (public so inherited class can use it) ---------- */
    adin1110_DeviceStruct_t dev;
    adin1110_DeviceHandle_t hDevice = &dev;
};

#endif /* VI_SPE_ADVANCED_H */
