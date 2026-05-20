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
 *  Attribution:
 *   - Analog Devices ADIN1110 driver  © 2020,2021 Analog Devices, Inc.
 *   - Adapted from sfe_spe_advanced.cpp by SparkFun Electronics (MIT)
 *  License  : MIT  —  see LICENSE.md
 * =============================================================================
 */

#include "Vi_SPE_Advanced.h"
#include "boardsupport.h"

static const int ADIN1110_INIT_ITER = 5;

/* =========================================================================
 * SPI configuration helpers
 * ====================================================================== */
void Vi_SPE_Advanced::setSPIInstance(SPIClass *spiInstance)
{
    BSP_SetSPIInstance(reinterpret_cast<void *>(spiInstance));
}

void Vi_SPE_Advanced::setSPIPins(int8_t sck, int8_t mosi, int8_t miso)
{
    BSP_SetSPIPins(sck, mosi, miso);
}

/* =========================================================================
 * begin() — single CS pin form
 * ====================================================================== */
adi_eth_Result_e Vi_SPE_Advanced::begin(uint8_t cs_pin)
{
    adi_eth_Result_e result;
    BSP_ConfigSystemCS(cs_pin);
    if (BSP_InitSystem()) return ADI_ETH_DEVICE_UNINITIALIZED;
    BSP_HWReset(true);
    for (uint32_t i = 0; i < ADIN1110_INIT_ITER; i++)
    {
        result = init(&drvConfig);
        if (result == ADI_ETH_SUCCESS) break;
    }
    return result;
}

/* =========================================================================
 * begin() — explicit pin form
 * ====================================================================== */
adi_eth_Result_e Vi_SPE_Advanced::begin(uint8_t status, uint8_t interrupt,
                                        uint8_t reset, uint8_t chip_select)
{
    adi_eth_Result_e result;
    BSP_ConfigSystem(status, interrupt, reset, chip_select);
    if (BSP_InitSystem()) return ADI_ETH_DEVICE_UNINITIALIZED;
    BSP_HWReset(true);
    for (uint32_t i = 0; i < ADIN1110_INIT_ITER; i++)
    {
        result = init(&drvConfig);
        if (result == ADI_ETH_SUCCESS) break;
    }
    return result;
}

/* =========================================================================
 * ADIN1110 driver wrappers
 * ====================================================================== */
adi_eth_Result_e Vi_SPE_Advanced::init           (adin1110_DriverConfig_t *pCfg) { return adin1110_Init(hDevice, pCfg); }
adi_eth_Result_e Vi_SPE_Advanced::unInit         ()                              { return adin1110_UnInit(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::getDeviceId    (adin1110_DeviceId_t *pDevId)  { return adin1110_GetDeviceId(hDevice, pDevId); }
adi_eth_Result_e Vi_SPE_Advanced::enable         ()                              { return adin1110_Enable(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::disable        ()                              { return adin1110_Disable(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::reset          (adi_eth_ResetType_e t)        { return adin1110_Reset(hDevice, t); }
adi_eth_Result_e Vi_SPE_Advanced::syncConfig     ()                              { return adin1110_SyncConfig(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::getLinkStatus  (adi_eth_LinkStatus_e *ls)     { return adin1110_GetLinkStatus(hDevice, ls); }
adi_eth_Result_e Vi_SPE_Advanced::getStatCounters(adi_eth_MacStatCounters_t *s) { return adin1110_GetStatCounters(hDevice, s); }
adi_eth_Result_e Vi_SPE_Advanced::ledEn          (bool en)                      { return adin1110_LedEn(hDevice, en); }
adi_eth_Result_e Vi_SPE_Advanced::setLoopbackMode(adi_phy_LoopbackMode_e m)     { return adin1110_SetLoopbackMode(hDevice, m); }
adi_eth_Result_e Vi_SPE_Advanced::setTestMode    (adi_phy_TestMode_e m)         { return adin1110_SetTestMode(hDevice, m); }

adi_eth_Result_e Vi_SPE_Advanced::addAddressFilter  (uint8_t *mac, uint8_t *mask, uint32_t pri) { return adin1110_AddAddressFilter(hDevice, mac, mask, pri); }
adi_eth_Result_e Vi_SPE_Advanced::clearAddressFilter(uint32_t idx)                              { return adin1110_ClearAddressFilter(hDevice, idx); }
adi_eth_Result_e Vi_SPE_Advanced::submitTxBuffer    (adi_eth_BufDesc_t *p)                      { return adin1110_SubmitTxBuffer(hDevice, p); }
adi_eth_Result_e Vi_SPE_Advanced::submitRxBuffer    (adi_eth_BufDesc_t *p)                      { return adin1110_SubmitRxBuffer(hDevice, p); }

#if defined(ADI_MAC_ENABLE_RX_QUEUE_HI_PRIO)
adi_eth_Result_e Vi_SPE_Advanced::submitRxBufferHp(adi_eth_BufDesc_t *p) { return adin1110_SubmitRxBufferHp(hDevice, p); }
#endif

adi_eth_Result_e Vi_SPE_Advanced::setPromiscuousMode(bool f)     { return adin1110_SetPromiscuousMode(hDevice, f); }
adi_eth_Result_e Vi_SPE_Advanced::getPromiscuousMode(bool *f)    { return adin1110_GetPromiscuousMode(hDevice, f); }

#if defined(SPI_OA_EN)
adi_eth_Result_e Vi_SPE_Advanced::setChunkSize(adi_mac_OaCps_e c)  { return adin1110_SetChunkSize(hDevice, c); }
adi_eth_Result_e Vi_SPE_Advanced::getChunkSize(adi_mac_OaCps_e *c) { return adin1110_GetChunkSize(hDevice, c); }
#endif

adi_eth_Result_e Vi_SPE_Advanced::setCutThroughMode(bool tx, bool rx)         { return adin1110_SetCutThroughMode(hDevice, tx, rx); }
adi_eth_Result_e Vi_SPE_Advanced::getCutThroughMode(bool *tx, bool *rx)       { return adin1110_GetCutThroughMode(hDevice, tx, rx); }
adi_eth_Result_e Vi_SPE_Advanced::setFifoSizes     (adi_mac_FifoSizes_t s)    { return adin1110_SetFifoSizes(hDevice, s); }
adi_eth_Result_e Vi_SPE_Advanced::getFifoSizes     (adi_mac_FifoSizes_t *s)   { return adin1110_GetFifoSizes(hDevice, s); }
adi_eth_Result_e Vi_SPE_Advanced::clearFifos       (adi_mac_FifoClrMode_e m)  { return adin1110_ClearFifos(hDevice, m); }

adi_eth_Result_e Vi_SPE_Advanced::tsEnable             (adi_mac_TsFormat_e f)                                                      { return adin1110_TsEnable(hDevice, f); }
adi_eth_Result_e Vi_SPE_Advanced::tsClear              ()                                                                           { return adin1110_TsClear(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::tsTimerStart         (adi_mac_TsTimerConfig_t *p)                                                { return adin1110_TsTimerStart(hDevice, p); }
adi_eth_Result_e Vi_SPE_Advanced::tsTimerStop          ()                                                                           { return adin1110_TsTimerStop(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::tsSetTimerAbsolute   (uint32_t s, uint32_t ns)                                                   { return adin1110_TsSetTimerAbsolute(hDevice, s, ns); }
adi_eth_Result_e Vi_SPE_Advanced::tsSyncClock          (int64_t te, uint64_t rNs, uint64_t lNs)                                   { return adin1110_TsSyncClock(hDevice, te, rNs, lNs); }
adi_eth_Result_e Vi_SPE_Advanced::tsGetExtCaptTimestamp(adi_mac_TsTimespec_t *p)                                                   { return adin1110_TsGetExtCaptTimestamp(hDevice, p); }
adi_eth_Result_e Vi_SPE_Advanced::tsGetEgressTimestamp (adi_mac_EgressCapture_e e, adi_mac_TsTimespec_t *p)                       { return adin1110_TsGetEgressTimestamp(hDevice, e, p); }
adi_eth_Result_e Vi_SPE_Advanced::tsConvert            (uint32_t l, uint32_t h, adi_mac_TsFormat_e f, adi_mac_TsTimespec_t *p)   { return adin1110_TsConvert(l, h, f, p); }
int64_t          Vi_SPE_Advanced::tsSubtract           (adi_mac_TsTimespec_t *a, adi_mac_TsTimespec_t *b)                         { return adin1110_TsSubtract(a, b); }

adi_eth_Result_e Vi_SPE_Advanced::registerCallback(adi_eth_Callback_t cb, adi_mac_InterruptEvt_e ev) { return adin1110_RegisterCallback(hDevice, cb, ev); }
adi_eth_Result_e Vi_SPE_Advanced::setUserContext  (void *p)      { return adin1110_SetUserContext(hDevice, p); }
void *           Vi_SPE_Advanced::getUserContext  ()             { return adin1110_GetUserContext(hDevice); }

adi_eth_Result_e Vi_SPE_Advanced::writeRegister(uint16_t a, uint32_t d)  { return adin1110_WriteRegister(hDevice, a, d); }
adi_eth_Result_e Vi_SPE_Advanced::readRegister (uint16_t a, uint32_t *d) { return adin1110_ReadRegister(hDevice, a, d); }
adi_eth_Result_e Vi_SPE_Advanced::phyWrite     (uint32_t a, uint16_t d)  { return adin1110_PhyWrite(hDevice, a, d); }
adi_eth_Result_e Vi_SPE_Advanced::phyRead      (uint32_t a, uint16_t *d) { return adin1110_PhyRead(hDevice, a, d); }

adi_eth_Result_e Vi_SPE_Advanced::getMseLinkQuality      (adi_phy_MseLinkQuality_t *p)       { return adin1110_GetMseLinkQuality(hDevice, p); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenEn             (bool en)                            { return adin1110_FrameGenEn(hDevice, en); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenSetMode        (adi_phy_FrameGenMode_e m)           { return adin1110_FrameGenSetMode(hDevice, m); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenSetFrameCnt    (uint32_t c)                         { return adin1110_FrameGenSetFrameCnt(hDevice, c); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenSetFramePayload(adi_phy_FrameGenPayload_e p)        { return adin1110_FrameGenSetFramePayload(hDevice, p); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenSetFrameLen    (uint16_t l)                         { return adin1110_FrameGenSetFrameLen(hDevice, l); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenSetIfgLen      (uint16_t l)                         { return adin1110_FrameGenSetIfgLen(hDevice, l); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenRestart        ()                                   { return adin1110_FrameGenRestart(hDevice); }
adi_eth_Result_e Vi_SPE_Advanced::frameGenDone           (bool *d)                            { return adin1110_FrameGenDone(hDevice, d); }
adi_eth_Result_e Vi_SPE_Advanced::frameChkEn             (bool en)                            { return adin1110_FrameChkEn(hDevice, en); }
adi_eth_Result_e Vi_SPE_Advanced::frameChkSourceSelect   (adi_phy_FrameChkSource_e s)         { return adin1110_FrameChkSourceSelect(hDevice, s); }
adi_eth_Result_e Vi_SPE_Advanced::frameChkReadFrameCnt   (uint32_t *c)                        { return adin1110_FrameChkReadFrameCnt(hDevice, c); }
adi_eth_Result_e Vi_SPE_Advanced::frameChkReadRxErrCnt   (uint16_t *c)                        { return adin1110_FrameChkReadRxErrCnt(hDevice, c); }
adi_eth_Result_e Vi_SPE_Advanced::frameChkReadErrorCnt   (adi_phy_FrameChkErrorCounters_t *c) { return adin1110_FrameChkReadErrorCnt(hDevice, c); }
