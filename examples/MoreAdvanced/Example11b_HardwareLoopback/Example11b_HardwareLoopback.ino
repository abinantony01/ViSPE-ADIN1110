/*
 * ============================================================================= 
 *  ViSPE ADIN1110 - Single Pair Ethernet Library  |  Version 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  Attribution:
 *   - Analog Devices ADIN1110 driver  (c) 2020,2021 Analog Devices, Inc.
 *   - Example adapted from SparkFun ADIN1110 Library (MIT)
 *  License  : MIT - see LICENSE.md
 * =============================================================================
 */

#include "Vi_SPE_Advanced.h"

/* Extra 4 bytes for FCS and 2 bytes for the frame header */
#define MAX_FRAME_BUF_SIZE  (MAX_FRAME_SIZE + 4 + 2)

#define TEST_FRAMES_COUNT   (2)

#define MAC_ADDR_0_0        (0x00)
#define MAC_ADDR_0_1        (0xE0)
#define MAC_ADDR_0_2        (0x22)
#define MAC_ADDR_0_3        (0xFE)
#define MAC_ADDR_0_4        (0xDA)
#define MAC_ADDR_0_5        (0xC9)

#define MAC_ADDR_1_0        (0x00)
#define MAC_ADDR_1_1        (0xE0)
#define MAC_ADDR_1_2        (0x22)
#define MAC_ADDR_1_3        (0xFE)
#define MAC_ADDR_1_4        (0xDA)
#define MAC_ADDR_1_5        (0xCA)

uint8_t macAddr[2][6] = {
    {MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5},
    {MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5},
};

Vi_SPE_Advanced adin1110;

/* Example configuration */
uint32_t heartbeatCheckTime = 0;

void setup() 
{
    adi_eth_Result_e        result;
    Serial.begin(115200);
    Serial.println("start");
    /****** System Init *****/
    result = adin1110.begin();
    if(result != ADI_ETH_SUCCESS) Serial.println("No MACPHY device found");
    else Serial.println("Adin1110 found!");

    result = adin1110.addAddressFilter(&macAddr[0][0], NULL, 0);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_AddAddressFilter");

    result = adin1110.addAddressFilter(&macAddr[1][0], NULL, 0);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_AddAddressFilter");

    result = adin1110.syncConfig();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_SyncConfig");

    result = adin1110.setLoopbackMode(ADI_PHY_LOOPBACK_MACIF_REMOTE);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_SetLoopbackMode");

    result = adin1110.enable();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_Enable");
}

void loop() 
{
    uint32_t now  = millis();
    /* Heartbeat pulse approximately once every 250ms. */
    if (now - heartbeatCheckTime >= 1000)
    {
        heartbeatCheckTime = now;
       BSP_HeartBeat();
    }
}