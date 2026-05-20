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
 *   - Arduino wrapper adapted from SparkFun ADIN1110 Library (MIT)
 *  License  : MIT  —  see LICENSE.md
 * =============================================================================
 */

#include "Vi_SinglePairEthernet.h"

/* =========================================================================
 * Static C-compatible shims
 * These allow a plain C function pointer to be stored in the driver while
 * still dispatching to the correct C++ object instance.
 * ====================================================================== */

void ViSPE::linkCallback_C(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    ViSPE *self = reinterpret_cast<ViSPE *>(adin1110_GetUserContext(device));
    if (self) self->linkCallback(pCBParam, Event, pArg);
}

void ViSPE::txCallback_C(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    ViSPE *self = reinterpret_cast<ViSPE *>(adin1110_GetUserContext(device));
    if (self) self->txCallback(pCBParam, Event, pArg);
}

void ViSPE::rxCallback_C(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    ViSPE *self = reinterpret_cast<ViSPE *>(adin1110_GetUserContext(device));
    if (self) self->rxCallback(pCBParam, Event, pArg);
}

/* =========================================================================
 * Internal callbacks (member functions)
 * ====================================================================== */

void ViSPE::txCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t *pTxBufDesc = (adi_eth_BufDesc_t *)pArg;
    for (uint32_t i = 0; i < VISPE_NUM_BUFS; i++)
    {
        if (&txBuf[i][0] == pTxBufDesc->pBuf)
        {
            txBufAvailable[i] = true;
            break;
        }
    }
}

void ViSPE::rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t *pRxBufDesc = (adi_eth_BufDesc_t *)pArg;
    rxSinceLastCheck = true;

    uint32_t i;
    for (i = 0; i < VISPE_NUM_BUFS; i++)
    {
        if (&rxBuf[i][0] == pRxBufDesc->pBuf)
        {
            rxBufAvailable[i] = true;
            break;
        }
    }

    if (userRxCallback && (pRxBufDesc->trxSize > VISPE_FRAME_HEADER_SIZE))
    {
        userRxCallback(&pRxBufDesc->pBuf[VISPE_FRAME_HEADER_SIZE],
                       (pRxBufDesc->trxSize - VISPE_FRAME_HEADER_SIZE),
                       &pRxBufDesc->pBuf[VISPE_MAC_SIZE]);
        submitRxBuffer(pRxBufDesc);
        rxBufAvailable[i] = false;
    }
}

void ViSPE::linkCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    linkStatus = *(adi_eth_LinkStatus_e *)pArg;
    if (userLinkCallback)
    {
        userLinkCallback(linkStatus == ADI_ETH_LINK_STATUS_UP);
    }
}

/* =========================================================================
 * begin() — single chip-select form (uses boardsupport.h defaults)
 * ====================================================================== */
bool ViSPE::begin(uint8_t *mac, uint8_t cs_pin)
{
    adi_eth_Result_e result;
    if (mac) setMac(mac);
    result = Vi_SPE_Advanced::begin(cs_pin);
    setUserContext((void *)this);
    if (result == ADI_ETH_SUCCESS) result = enableDefaultBehavior();
    return (result == ADI_ETH_SUCCESS);
}

/* =========================================================================
 * begin() — explicit pin form
 * ====================================================================== */
bool ViSPE::begin(uint8_t *mac, uint8_t status, uint8_t interrupt,
                  uint8_t reset, uint8_t chip_select)
{
    adi_eth_Result_e result;
    if (mac) setMac(mac);
    result = Vi_SPE_Advanced::begin(status, interrupt, reset, chip_select);
    setUserContext((void *)this);
    if (result == ADI_ETH_SUCCESS) result = enableDefaultBehavior();
    return (result == ADI_ETH_SUCCESS);
}

/* =========================================================================
 * enableDefaultBehavior — sets up filters, callbacks, and Rx buffers
 * ====================================================================== */
adi_eth_Result_e ViSPE::enableDefaultBehavior()
{
    adi_eth_Result_e result = ADI_ETH_SUCCESS;

    if (result == ADI_ETH_SUCCESS)
        result = addAddressFilter(macAddr, NULL, 0);

    if (result == ADI_ETH_SUCCESS)
        result = syncConfig();

    if (result == ADI_ETH_SUCCESS)
        result = registerCallback(linkCallback_C, ADI_MAC_EVT_LINK_CHANGE);

    for (uint32_t i = 0; i < VISPE_NUM_BUFS; i++)
    {
        if (result != ADI_ETH_SUCCESS) break;

        txBufAvailable[i] = true;
        rxBufAvailable[i] = false;
        rxBufDesc[i].pBuf    = &rxBuf[i][0];
        rxBufDesc[i].bufSize = VISPE_MAX_BUF_FRAME_SIZE;
        rxBufDesc[i].cbFunc  = rxCallback_C;

        result = submitRxBuffer(&rxBufDesc[i]);
    }

    if (result == ADI_ETH_SUCCESS)
        result = enable();

    return result;
}

/* =========================================================================
 * sendData
 * ====================================================================== */
bool ViSPE::sendData(uint8_t *data, int dataLen)
{
    return sendData(data, dataLen, destMacAddr);
}

bool ViSPE::sendData(uint8_t *data, int dataLen, uint8_t *destMac)
{
    adi_eth_Result_e result;

    if ((dataLen + VISPE_FRAME_HEADER_SIZE > VISPE_FRAME_SIZE) || !destMac)
        return false;

    int txLen = 0;
    memcpy(&txBuf[txBufIdx][txLen], destMac,  VISPE_MAC_SIZE); txLen += VISPE_MAC_SIZE;
    memcpy(&txBuf[txBufIdx][txLen], macAddr,  VISPE_MAC_SIZE); txLen += VISPE_MAC_SIZE;
    txBuf[txBufIdx][txLen++] = VISPE_ETHERTYPE_IPV4_B0;
    txBuf[txBufIdx][txLen++] = VISPE_ETHERTYPE_IPV4_B1;
    memcpy(&txBuf[txBufIdx][txLen], data, dataLen);  txLen += dataLen;
    while (txLen < VISPE_MIN_PAYLOAD_SIZE) txBuf[txBufIdx][txLen++] = 0;

    txBufDesc[txBufIdx].pBuf      = &txBuf[txBufIdx][0];
    txBufDesc[txBufIdx].trxSize   = txLen;
    txBufDesc[txBufIdx].bufSize   = VISPE_MAX_BUF_FRAME_SIZE;
    txBufDesc[txBufIdx].egressCapt = ADI_MAC_EGRESS_CAPTURE_NONE;
    txBufDesc[txBufIdx].cbFunc    = txCallback_C;
    txBufAvailable[txBufIdx]      = false;

    result = submitTxBuffer(&txBufDesc[txBufIdx]);
    if (result == ADI_ETH_SUCCESS)
    {
        txBufIdx = (txBufIdx + 1) % VISPE_NUM_BUFS;
        setDestMac(destMac);
    }
    else
    {
        txBufAvailable[txBufIdx] = true;
    }
    return (result == ADI_ETH_SUCCESS);
}

/* =========================================================================
 * getRxData
 * ====================================================================== */
int ViSPE::getRxData(uint8_t *data, int dataLen, uint8_t *senderMac)
{
    bool rxDataAvailable = false;
    for (int i = 0; i < VISPE_NUM_BUFS; i++)
    {
        if (rxBufAvailable[rxBufIdx]) { rxDataAvailable = true; break; }
        rxBufIdx = (rxBufIdx + 1) % VISPE_NUM_BUFS;
    }
    if (rxDataAvailable)
    {
        int cpyLen = rxBufDesc[rxBufIdx].trxSize - VISPE_FRAME_HEADER_SIZE;
        cpyLen = (cpyLen < dataLen) ? cpyLen : dataLen;
        memcpy(senderMac, &rxBufDesc[rxBufIdx].pBuf[VISPE_MAC_SIZE], VISPE_MAC_SIZE);
        memcpy(data,      &rxBufDesc[rxBufIdx].pBuf[VISPE_FRAME_HEADER_SIZE], cpyLen);
        submitRxBuffer(&rxBufDesc[rxBufIdx]);
        rxBufAvailable[rxBufIdx] = false;
        rxSinceLastCheck         = false;
        return cpyLen;
    }
    return 0;
}

bool ViSPE::getRxAvailable() { return rxSinceLastCheck; }

/* =========================================================================
 * MAC address helpers
 * ====================================================================== */
void ViSPE::setMac(uint8_t *mac)     { if (mac) memcpy(macAddr,     mac, VISPE_MAC_SIZE); }
void ViSPE::getMac(uint8_t *mac)     { if (mac) memcpy(mac, macAddr,     VISPE_MAC_SIZE); }
void ViSPE::setDestMac(uint8_t *mac) { if (mac) memcpy(mac, destMacAddr, VISPE_MAC_SIZE); }

bool ViSPE::indenticalMacs(uint8_t *mac1, uint8_t *mac2)
{
    if (!mac1 || !mac2) return false;
    return (mac1[0]==mac2[0] && mac1[1]==mac2[1] && mac1[2]==mac2[2] &&
            mac1[3]==mac2[3] && mac1[4]==mac2[4] && mac1[5]==mac2[5]);
}

/* =========================================================================
 * Callbacks and link status
 * ====================================================================== */
void ViSPE::setRxCallback  (void (*cbFunc)(uint8_t *, int, uint8_t *)) { userRxCallback   = cbFunc; }
void ViSPE::setLinkCallback(void (*cbFunc)(bool))                       { userLinkCallback = cbFunc; }
bool ViSPE::getLinkStatus  ()                                           { return (linkStatus == ADI_ETH_LINK_STATUS_UP); }
