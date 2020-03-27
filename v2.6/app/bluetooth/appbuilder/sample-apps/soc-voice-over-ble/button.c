/***************************************************************************//**
 * @file
 * @brief Button definitions
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <stdbool.h>
#include "bg_types.h"
#include "gpiointerrupt.h"
#include "em_gpio.h"
#include "native_gecko.h"
#include "InitDevice.h"
#include "button.h"

/***************************************************************************//**
 * @defgroup Button Buttons
 * @{
 * @brief Buttons driver
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup Button_Functions Button Functions
 * @{
 * @brief Button driver and support functions
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Definition of interrupts binded to button pressed and released states. */
#define BUTTON_SW1_PRESSED  (BUTTON_SW1_PIN)
#define BUTTON_SW1_RELEASED (BUTTON_SW2_PIN)

static void buttonIrqGpioHandler(uint8_t pin);
static void buttonEnableIRQ(bool enable);
static void buttonConfig(void);

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Initializes button GPIOs and IRQ
 *
 * @return
 *    None
 ******************************************************************************/
void BUTTON_Init(void)
{
  buttonConfig();
  buttonEnableIRQ(true);
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Emits external signal depending on released button.
 *
 * @param[in] pin
 *    Pin number for released button
 *
 * @return
 *    None
 ******************************************************************************/
static void buttonIrqGpioHandler(uint8_t pin)
{
  switch (pin) {
    case BUTTON_SW1_PRESSED:
      gecko_external_signal(EXTSIG_BUTTON_SW1_PRESSED);
      break;
    case BUTTON_SW1_RELEASED:
      gecko_external_signal(EXTSIG_BUTTON_SW1_RELEASED);
      break;
    default:
      break;
  }
}

/***************************************************************************//**
 * @brief
 *    Enable/Disable button IRQ.
 *
 * @param[in] enable
 *    Enable/Disable interrupts.
 *
 * @return
 *    None
 ******************************************************************************/
static void buttonEnableIRQ(bool enable)
{
  GPIOINT_Init();

  GPIOINT_CallbackRegister(BUTTON_SW1_PRESSED, buttonIrqGpioHandler);
  GPIOINT_CallbackRegister(BUTTON_SW1_RELEASED, buttonIrqGpioHandler);

  GPIO_ExtIntConfig(gpioPortD, BUTTON_SW1_PIN, BUTTON_SW1_PRESSED, false, true, enable);
  GPIO_ExtIntConfig(gpioPortD, BUTTON_SW1_PIN, BUTTON_SW1_RELEASED, true, false, enable);
}

/***************************************************************************//**
 * @brief
 *    Button GPIO configuration
 *
 * @return
 *    None
 ******************************************************************************/
static void buttonConfig(void)
{
  GPIO_PinModeSet(BUTTON_SW1_PORT, BUTTON_SW1_PIN, gpioModeInput, 0);
}

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** @} {end defgroup Button_Functions} */

/** @} {end defgroup Button} */
