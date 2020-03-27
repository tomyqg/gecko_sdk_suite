/***************************************************************************//**
 * @file
 * @brief Automation I/O Device
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

#include <stdint.h>
#include "aio_device.h"
#include "rd0057.h"

/***********************************************************************************************//**
 * @addtogroup app_hardware
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup aio-hw
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
void aioDeviceInit(void)
{
}

void aioDeviceSleep(void)
{
}

void aioDeviceDigitalOutWrite(AioDigitalState_t *aioDigitalOutStates)
{
  uint8_t i;

  for (i = 0; i < AIO_NUMBER_OF_DIGITAL_OUTPUTS; i++) {
    switch (aioDigitalOutStates[i]) {
      case AIO_DIGITAL_INACTIVE:
        boardLedOff(i);
        break;

      case AIO_DIGITAL_ACTIVE:
        boardLedOn(i);
        break;
      default:
        break;
    }
  }
}

void aioDeviceDigitalOutRead(AioDigitalState_t *aioDigitalOutStates)
{
  uint8_t i;

  for (i = 0; i < AIO_NUMBER_OF_DIGITAL_OUTPUTS; i++) {
    aioDigitalOutStates[i] = boardLedGet(i)
                             ? AIO_DIGITAL_ACTIVE
                             : AIO_DIGITAL_INACTIVE;
  }
}

void aioDeviceDigitalInRead(AioDigitalState_t *aioDigitalInStates)
{
  uint8_t i;
  for (i = 0; i < AIO_NUMBER_OF_DIGITAL_INPUTS; i++) {
    aioDigitalInStates[i] = boardButtonGet(i)
                            ? AIO_DIGITAL_ACTIVE
                            : AIO_DIGITAL_INACTIVE;
  }
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

/** @} (end addtogroup aio-hw) */
/** @} (end addtogroup app_hardware) */
