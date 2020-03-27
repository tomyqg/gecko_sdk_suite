/***************************************************************************//**
 * @file fem-control.c
 * @brief This file implements a simple API for configuring FEM control signals via PRS.
 * @copyright Copyright 2018 Silicon Laboratories, Inc. www.silabs.com
 ******************************************************************************/

#include "hal-config.h"
#include "em_device.h"
#include "em_assert.h"
#include "em_gpio.h"
#include "em_bus.h"
#include "em_cmu.h"
#include "em_prs.h"

#ifdef _SILICON_LABS_32B_SERIES_1
#if (!defined(BSP_FEM_RX_CHANNEL) \
  || !defined(BSP_FEM_RX_PORT)    \
  || !defined(BSP_FEM_RX_PIN)     \
  || !defined(BSP_FEM_RX_LOC))
  #error "BSP_FEM_RX_CHANNEL/PORT/PIN/LOC must be defined."
#endif
#else //!_SILICON_LABS_32B_SERIES_1
#if (!defined(BSP_FEM_RX_CHANNEL) \
  || !defined(BSP_FEM_RX_PORT)    \
  || !defined(BSP_FEM_RX_PIN))
  #error "BSP_FEM_RX_CHANNEL/PORT/PIN must be defined."
#endif
#endif //!_SILICON_LABS_32B_SERIES_1

// if no separate CTX pin is defined, CRX is a combined rx tx pin
#ifndef BSP_FEM_TX_CHANNEL
  #define BSP_FEM_TX_PORT BSP_FEM_RX_PORT
  #define BSP_FEM_TX_PIN BSP_FEM_RX_PIN
  #define BSP_FEM_TX_LOC BSP_FEM_RX_LOC
  #define BSP_FEM_TX_CHANNEL BSP_FEM_RX_CHANNEL
#endif

#if HAL_FEM_TX_ACTIVE == 1 && HAL_FEM_RX_ACTIVE == 1
  #if BSP_FEM_RX_CHANNEL == BSP_FEM_TX_CHANNEL
    #error The RX and TX PRS channels cannot be equal.
  #endif
#endif

#if HAL_FEM_RX_ACTIVE && defined(BSP_FEM_SLEEP_CHANNEL)
#if (BSP_FEM_RX_CHANNEL + 1) != BSP_FEM_SLEEP_CHANNEL
  #error "Sleep pin channel must immediately follow RX channel"
#endif
#endif // HAL_FEM_RX_ACTIVE

#ifndef PRS_CHAN_COUNT
#define PRS_CHAN_COUNT PRS_ASYNC_CHAN_COUNT
#endif

#if defined(BSP_FEM_SLEEP_CHANNEL) && BSP_FEM_SLEEP_CHANNEL >= PRS_CHAN_COUNT
  #error "FEM_SLEEP channel number higher than number of PRS channels"
#endif

#if HAL_FEM_TX_ACTIVE
#if BSP_FEM_TX_CHANNEL >= PRS_CHAN_COUNT
  #error "FEM_TX channel number higher than number of PRS channels"
#endif
#endif

#ifdef _SILICON_LABS_32B_SERIES_1
#define FEM_TX_LOGIC (0U)
#define FEM_RX_LOGIC (0U)
#if HAL_FEM_RX_ACTIVE
#define FEM_SLEEP_LOGIC (PRS_CH_CTRL_ORPREV)
#else //!HAL_FEM_RX_ACTIVE
#define FEM_SLEEP_LOGIC (0U)
#endif //HAL_FEM_RX_ACTIVE
#else //!_SILICON_LABS_32B_SERIES_1
#define FEM_TX_LOGIC      (prsLogic_A)
#define FEM_RX_LOGIC      (prsLogic_A)
#define BSP_FEM_RX_LOC    (0U)
#define BSP_FEM_TX_LOC    (0U)
#define BSP_FEM_SLEEP_LOC (0U)
#if HAL_FEM_RX_ACTIVE
#define FEM_SLEEP_LOGIC (prsLogic_A_OR_B)
#else //!HAL_FEM_RX_ACTIVE
#define FEM_SLEEP_LOGIC (prsLogic_A)
#endif //HAL_FEM_RX_ACTIVE
#endif //_SILICON_LABS_32B_SERIES_1

static void configPrs(GPIO_Port_TypeDef port,
                      unsigned int pin,
                      uint32_t channel,
                      uint32_t location,
                      uint32_t signal,
                      uint32_t logic)
{
#ifdef _SILICON_LABS_32B_SERIES_1
  //Setup the PRS to output on the channel and location chosen.
  volatile uint32_t * routeRegister;

  PRS->CH[channel].CTRL = signal | logic;

  // Configure PRS output to selected channel and location
  if (channel < 4) {
    routeRegister = &PRS->ROUTELOC0;
  } else if (channel < 8) {
    routeRegister = &PRS->ROUTELOC1;
  } else if (channel < 12) {
    routeRegister = &PRS->ROUTELOC2;
  } else {
    EFM_ASSERT(0);
    return; // error
  }

  BUS_RegMaskedWrite(routeRegister,
                     0xFF << ((channel % 4) * 8),
                     location << ((channel % 4) * 8));

  BUS_RegMaskedSet(&PRS->ROUTEPEN, (1 << channel));
#else //!_SILICON_LABS_32B_SERIES_1
  (void)location;
  PRS_SourceAsyncSignalSet(channel,
                           0U,
                           signal);
  PRS_Combine(channel,
              channel - 1,
              logic);
  PRS_PinOutput(channel, prsTypeAsync, port, pin);
#endif //_SILICON_LABS_32B_SERIES_1
  //Enable the output based on a specific port and pin
  //Configure the gpio to be an output
  GPIO_PinModeSet(port, pin, gpioModePushPull, 0U);
}

void initFem(void)
{
  // Turn on the GPIO clock so that we can turn on GPIOs
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_PRS, true);

#if HAL_FEM_TX_ACTIVE == 1
  configPrs(BSP_FEM_TX_PORT,
            BSP_FEM_TX_PIN,
            BSP_FEM_TX_CHANNEL,
            BSP_FEM_TX_LOC,
            PRS_RAC_PAEN,
            FEM_TX_LOGIC);
#endif //HAL_FEM_TX_ACTIVE == 1

#if HAL_FEM_RX_ACTIVE == 1
  configPrs(BSP_FEM_RX_PORT,
            BSP_FEM_RX_PIN,
            BSP_FEM_RX_CHANNEL,
            BSP_FEM_RX_LOC,
            PRS_RAC_LNAEN,
            FEM_RX_LOGIC);
#endif //HAL_FEM_RX_ACTIVE == 1

#if defined(BSP_FEM_SLEEP_CHANNEL)
  configPrs(BSP_FEM_RX_PORT,
            BSP_FEM_RX_PIN,
            BSP_FEM_RX_CHANNEL,
            BSP_FEM_SLEEP_LOC,
            PRS_RAC_PAEN,
            FEM_SLEEP_LOGIC);
#endif //defined(BSP_FEM_SLEEP_CHANNEL)

// if fem has a bypass pin (FEM pin CPS)
#ifdef BSP_FEM_BYPASS_PORT
  // set up bypass pin
  #if HAL_FEM_BYPASS_ENABLE
  GPIO_PinModeSet(BSP_FEM_BYPASS_PORT, BSP_FEM_BYPASS_PIN, gpioModePushPull, 1);
  #else
  GPIO_PinModeSet(BSP_FEM_BYPASS_PORT, BSP_FEM_BYPASS_PIN, gpioModePushPull, 0);
  #endif
#endif

// if fem has a tx power pin (FEM pin CHL)
#ifdef BSP_FEM_TXPOWER_PORT
  // set up tx power pin
  #if HAL_FEM_TX_HIGH_POWER
  GPIO_PinModeSet(BSP_FEM_TXPOWER_PORT, BSP_FEM_TXPOWER_PIN, gpioModePushPull, 1);
  #else
  GPIO_PinModeSet(BSP_FEM_TXPOWER_PORT, BSP_FEM_TXPOWER_PIN, gpioModePushPull, 0);
  #endif
#endif
}

void wakeupFem(void)
{
// if fem has a bypass pin (FEM pin CPS)
#ifdef BSP_FEM_BYPASS_PORT
  #if HAL_FEM_BYPASS_ENABLE
  GPIO_PinOutSet(BSP_FEM_BYPASS_PORT, BSP_FEM_BYPASS_PIN);
  #else
  GPIO_PinOutClear(BSP_FEM_BYPASS_PORT, BSP_FEM_BYPASS_PIN);
  #endif
#endif

// if fem has a tx power pin (FEM pin CHL)
#ifdef BSP_FEM_TXPOWER_PORT
  #if HAL_FEM_TX_HIGH_POWER
  GPIO_PinOutSet(BSP_FEM_TXPOWER_PORT, BSP_FEM_TXPOWER_PIN);
  #else
  GPIO_PinOutClear(BSP_FEM_TXPOWER_PORT, BSP_FEM_TXPOWER_PIN);
  #endif
#endif
}

void shutdownFem(void)
{
  #ifdef BSP_FEM_BYPASS_PORT
  GPIO_PinOutClear(BSP_FEM_BYPASS_PORT, BSP_FEM_BYPASS_PIN);
  #endif
  #ifdef BSP_FEM_TXPOWER_PORT
  GPIO_PinOutClear(BSP_FEM_TXPOWER_PORT, BSP_FEM_TXPOWER_PIN);
  #endif
}
