/*
 * =============================================================================
 *  ViSPE ADIN1110 — Single Pair Ethernet Library  |  Version 1.0.0
 * =============================================================================
 *  Author   : Abin Antony
 *  Email    : abinantony.dev@gmail.com
 *  Role     : R&D Engineer
 *  Company  : Vi Micro Systems Pvt Ltd
 * -----------------------------------------------------------------------------
 *  boardsupport_ard.cpp — Generic Arduino board support implementation.
 *  Handles SPI, GPIO, interrupts, and hardware reset for the ADIN1110.
 *  Includes full Raspberry Pi Pico W (SPI1, GP6/7/10/11/12/13) support.
 * -----------------------------------------------------------------------------
 *  Attribution:
 *   - Core driver  © 2020,2021 Analog Devices, Inc. (All Rights Reserved)
 *   - Arduino port adapted from SparkFun ADIN1110 Library (MIT)
 *     SparkFun Electronics / Kyle Wenner
 *  License: MIT  —  see LICENSE.md
 * =============================================================================
 */

#include "boardsupport.h"

#include <string.h>

#include "Arduino.h"
#include <SPI.h>

// Enable this define to print all spi messages, note this will severely impact performance
// #define DEBUG_SPI

#if defined(ARDUINO_ARCH_MBED) 
#include <mbed.h>
rtos::Thread thread;
rtos::Semaphore updates(0);
#endif

/* ---------------------------------------------------------------
 * Pico W / RP2040: default to SPI1 (GP10 SCK, GP11 MOSI, GP12 MISO)
 * --------------------------------------------------------------- */
#if defined(DEFAULT_ETH_USE_SPI1)
static SPIClass* SPI_instance = &SPI1;
#else
static SPIClass* SPI_instance = &SPI;
#endif

/* Optional explicit SPI pin overrides (-1 = use hardware defaults) */
static int8_t spi_sck_pin  = -1;
static int8_t spi_mosi_pin = -1;
static int8_t spi_miso_pin = -1;

#if defined(DEFAULT_ETH_SPI_SCK_Pin)
/* Populate compile-time defaults from boardsupport.h */
namespace {
  struct _SpiPinInit {
    _SpiPinInit() {
      spi_sck_pin  = DEFAULT_ETH_SPI_SCK_Pin;
      spi_mosi_pin = DEFAULT_ETH_SPI_MOSI_Pin;
      spi_miso_pin = DEFAULT_ETH_SPI_MISO_Pin;
    }
  } _spiPinInit;
}
#endif

#define RESET_DELAY       (1)
#define AFTER_RESET_DELAY (100)

static          ADI_CB gpfSpiCallback = NULL;
static void     *gpSpiCBParam = NULL;

static          ADI_CB gpfGPIOIntCallback = NULL;
static void     *gpGPIOIntCBParam = NULL;

static uint8_t status_led_pin = LED_BUILTIN;
static uint8_t interrupt_pin = DEFAULT_ETH_INT_Pin;
static uint8_t reset_pin = DEFAULT_ETH_RESET_Pin;
static uint8_t chip_select_pin = DEFAULT_ETH_SPI_CS_Pin;

void SPI_TxRxCpltCallback(void);
void BSP_IRQCallback(void);

/*
* Functions that are part of the driver, that do nothing in the arduino port
*/
void BSP_ErrorLed(bool on) { /*NO ERROR LED*/ }

void BSP_FuncLed1(bool on) { /* NO FuncLed1 LED */ }

void BSP_FuncLed1Toggle(void) { /* NO FuncLed1 LED */ }

void BSP_FuncLed2(bool on) { /* NO FuncLed2 LED */ }

void BSP_FuncLed2Toggle(void) { /* NO FuncLed2 LED */ }

void BSP_getConfigPins(uint16_t *value) { /* This board has no config pins, so odnt do anything */ }


void BSP_disableInterrupts(void)
{
    #if defined(ARDUINO_ARCH_APOLLO3) || defined(ARDUINO_SPARKFUN_THINGPLUS_RP2040) || defined(ARDUINO_SPARKFUN_MICROMOD_RP2040) || defined(ARDUINO_ARCH_RP2040)
        //These architectures have problems with entering/exiting critical section so just don't do it
    #else
        noInterrupts();
    #endif
}


void BSP_enableInterrupts(void)
{
    #if defined(ARDUINO_ARCH_APOLLO3) || defined(ARDUINO_SPARKFUN_THINGPLUS_RP2040) || defined(ARDUINO_SPARKFUN_MICROMOD_RP2040) || defined(ARDUINO_ARCH_RP2040)
        //These architectures have problems with entering/exiting critical section so just don't do it
    #else
        interrupts();  // Fix: was incorrectly calling noInterrupts()
    #endif
}

/*
 * Blocking delay function
 */
void BSP_delayMs(uint32_t delay)
{
    volatile uint32_t now;
    uint32_t checkTime  = BSP_SysNow();
    /* Read SysTick Timer every Ms*/
    while (1)
    {
      now  = BSP_SysNow();
       if (now - checkTime >= delay)
       {
          break;
       }
    }
}

/*
 * Hardware reset to DUT
 */
void BSP_HWReset(bool set)
{
    digitalWrite(reset_pin, LOW);
    BSP_delayMs(RESET_DELAY);
    digitalWrite(reset_pin, HIGH);
    BSP_delayMs(AFTER_RESET_DELAY);
}

/* LED functions */
static void bspLedSet(uint16_t pin, bool on)
{
    if (on)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        digitalWrite(pin, LOW);
    }
}

static void bspLedToggle(uint16_t pin)
{
    digitalWrite(pin, !digitalRead(pin));
}

/*
 * Heartbeat LED, On arduino we just default this to LED_BUILTIN
 */
void BSP_HeartBeat(void)
{
    bspLedToggle(status_led_pin);
}

/*
 * HeartBeat LED, On arduino we just default this to LED_BUILTIN
 */
void BSP_HeartBeatLed(bool on)
{
    bspLedSet(status_led_pin, on);
}

/* All LEDs toggle, used to indicate hardware failure on the board */
void BSP_LedToggleAll(void)
{
    bspLedSet(status_led_pin, HIGH);
}

uint32_t BSP_spi2_write_and_read(uint8_t *pBufferTx, uint8_t *pBufferRx, uint32_t nbBytes, bool useDma)
{
    //Validate parameters
    if(!pBufferTx || !pBufferRx)
    {
        return 1;
    }
    if(useDma)
    { //no DMA support for arduino
        return 1;
    }

    #ifdef DEBUG_SPI
    Serial.printf("writing numbytes = %d: ", nbBytes);
    for(int i = 0; i < nbBytes; i++)
    {
        Serial.printf(" %02X", pBufferTx[i]);
    }
    Serial.println();
    #endif

    memcpy(pBufferRx, pBufferTx, nbBytes);
    SPI_instance->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    bspLedSet(chip_select_pin, LOW);
    SPI_instance->transfer(pBufferRx, nbBytes);
    //Driver expects there to be an interrupt that fires after completion
    //Since SPI is blocking for arduino this is overly complicated, just call the "callback" function now
    SPI_TxRxCpltCallback();

    #ifdef DEBUG_SPI
    Serial.printf("read: ");
    for(int i = 0; i < nbBytes; i++)
    {
        Serial.printf(" %02X", pBufferRx[i]);
    }
    Serial.println();
    #endif

    return 0;
}

//Function called on SPI transaction completion
void SPI_TxRxCpltCallback(void)
{
    bspLedSet(chip_select_pin, HIGH);
    SPI_instance->endTransaction();
    (*gpfSpiCallback)(gpSpiCBParam, 0, NULL);
}

// Register the SPI callback, in the driver the following macro is used:
// extern uint32_t HAL_SPI_Register_Callback(ADI_CB const *pfCallback, void *const pCBParam);
uint32_t BSP_spi2_register_callback(ADI_CB const *pfCallback, void *const pCBParam)
{
    gpfSpiCallback = (ADI_CB)pfCallback;
    gpSpiCBParam = pCBParam ;
    return 0;
}

/*
* Set the Chip select to desired level, true for HIGH or false for LOW
 */
void setSPI2Cs(bool set)
{
    if(set == true)
    {
        bspLedSet(chip_select_pin, HIGH);
    }
    else
    {
        bspLedSet(chip_select_pin, LOW);
    }
}

// Register the callback for the interrupt pin, in the driver the following macro is used:
// extern uint32_t HAL_INT_N_Register_Callback(ADI_CB const *pfCallback, void *const pCBParam);
uint32_t BSP_RegisterIRQCallback(ADI_CB const *intCallback, void * hDevice)
{
    gpfGPIOIntCallback = (ADI_CB)intCallback;
    gpGPIOIntCBParam = hDevice ;

    attachInterrupt(interrupt_pin, BSP_IRQCallback, FALLING);
    return 0;
}

#if defined(ARDUINO_ARCH_MBED) 
// MBED will not allow SPI calls during ISR, which the callbacks may do
// So instead of directly calling in this function we start a thread that will call the callback after signalled to by this function
int num_int = 0;
void BSP_IRQCallback()
{
    //Signal to thread that this function was called
    updates.release();
}

void thread_fn( void ){  
    while(1)
    {
        //Once acquired, the IRQCallback function has run
        updates.acquire();

        if (gpfGPIOIntCallback)
        {
            (*gpfGPIOIntCallback)(gpGPIOIntCBParam, 0, NULL);
        }
    }
}
#else
//Outside of mbed cores, we just call the callback within the ISR
void BSP_IRQCallback()
{
    if (gpfGPIOIntCallback)
    {
        (*gpfGPIOIntCallback)(gpGPIOIntCBParam, 0, NULL);
    }
}
#endif

uint32_t BSP_SysNow(void)
{
    return millis();
}

uint32_t BSP_InitSystem(void)
{
#if defined(ARDUINO_ARCH_MBED)
    //Start a thread to handle the IRQ callback
    thread.start(thread_fn);
    thread.set_priority(osPriorityHigh);
#endif

    /* Configure SPI pins before calling begin() on RP2040 / Pico W targets.
     *
     * setRX / setTX / setSCK are defined on SPIClassRP2040 (the concrete type
     * from the Earle Philhower RP2040 core), but our SPI_instance pointer is
     * typed as SPIClass* (arduino::HardwareSPI – the abstract base), so the
     * compiler cannot find those methods through the pointer.
     *
     * Work-around: call the pin setters directly on the well-known global
     * objects (SPI1 / SPI), which keep their full concrete type. */
#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
  #if defined(DEFAULT_ETH_USE_SPI1)
    /* --- SPI1 bus (default for Pico W, VI SPE RP PICO 2 schematic) --- */
    SPI1.setRX (spi_miso_pin >= 0 ? (pin_size_t)spi_miso_pin : (pin_size_t)DEFAULT_ETH_SPI_MISO_Pin);
    SPI1.setTX (spi_mosi_pin >= 0 ? (pin_size_t)spi_mosi_pin : (pin_size_t)DEFAULT_ETH_SPI_MOSI_Pin);
    SPI1.setSCK(spi_sck_pin  >= 0 ? (pin_size_t)spi_sck_pin  : (pin_size_t)DEFAULT_ETH_SPI_SCK_Pin);
  #else
    /* --- SPI0 bus with optional pin override --- */
    if (spi_miso_pin >= 0) SPI.setRX ((pin_size_t)spi_miso_pin);
    if (spi_mosi_pin >= 0) SPI.setTX ((pin_size_t)spi_mosi_pin);
    if (spi_sck_pin  >= 0) SPI.setSCK((pin_size_t)spi_sck_pin);
  #endif
#endif

    SPI_instance->begin();
    pinMode(status_led_pin, OUTPUT);
    pinMode(interrupt_pin, INPUT);
    pinMode(reset_pin, OUTPUT);
    pinMode(chip_select_pin, OUTPUT);
    digitalWrite(chip_select_pin, HIGH);
    digitalWrite(reset_pin, HIGH);
    return 0;
}

//Set all the pins that are uysed throughout this module
uint32_t BSP_ConfigSystem(uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
{
    status_led_pin = status;
    interrupt_pin = interrupt;
    reset_pin = reset;
    chip_select_pin = chip_select;
    return 0;
}

//Change just the chip select pin
uint32_t BSP_ConfigSystemCS(uint8_t chip_select)
{
    chip_select_pin = chip_select;
    return 0;
}

/**
 * @brief  Override the SPI peripheral instance (e.g. pass &SPI1 for Pico W).
 * @param  spiInstance  Pointer to an SPIClass object (cast from void*).
 */
void BSP_SetSPIInstance(void *spiInstance)
{
    if (spiInstance)
    {
        SPI_instance = reinterpret_cast<SPIClass *>(spiInstance);
    }
}

/**
 * @brief  Override the SPI bus pin assignments.
 *         Call before begin() / BSP_InitSystem().
 * @param  sck   SCK  GPIO number  (e.g. 10 for GP10 on Pico W)
 * @param  mosi  MOSI GPIO number  (e.g. 11 for GP11 on Pico W)
 * @param  miso  MISO GPIO number  (e.g. 12 for GP12 on Pico W)
 */
void BSP_SetSPIPins(int8_t sck, int8_t mosi, int8_t miso)
{
    spi_sck_pin  = sck;
    spi_mosi_pin = mosi;
    spi_miso_pin = miso;
}

//User in the functions below, prints debug and error messages from within the driver
uint32_t msgWrite(char * ptr)
{
    Serial.print(ptr);
    return 0;
}

char aDebugString[150u];

void common_Fail(char *FailureReason)
{
    char fail[] = "Failed: ";
    char term[] = "\n\r";

    /* Ignore return codes since there's nothing we can do if it fails */
    msgWrite(fail);
    msgWrite(FailureReason);
    msgWrite(term);
 }

void common_Perf(char *InfoString)
{
    char term[] = "\n\r";

    /* Ignore return codes since there's nothing we can do if it fails */
    msgWrite(InfoString);
    msgWrite(term);
}

