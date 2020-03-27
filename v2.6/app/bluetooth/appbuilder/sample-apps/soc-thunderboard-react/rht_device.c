/***************************************************************************//**
 * @file
 * @brief Relative Humidity and Temperature Device
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

#include "rht_device.h"
/* standard library headers */
#include <stdlib.h>
#include "rd0057.h"
#include "si7013.h"

/***********************************************************************************************//**
 * @addtogroup app_hardware
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup rht-sensor
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

static bool si7013Detected = false;

/***************************************************************************************************
 * Local Function Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
void rhtDeviceInit(void)
{
  si7013Detected = Si7013_Detect(i2cInit.port, SI7021_ADDR, NULL);
}

void rhtDeviceDeInit(void)
{
  // nothing to do
}

uint8_t rhtDeviceTest(void)
{
  return si7013Detected ? 0 : 1;
}

void rhtDeviceSleep(void)
{
  // Nothing
}

void rhtDeviceConnectionOpened(void)
{
  // nothing to do
}

void rhtDeviceConnectionClosed(void)
{
  // nothing to do
}

void rhtDeviceHumidityMeasure(void (*humidityMeasurementDone)(uint16_t))
{
  uint32_t      rhData = 0;
  int32_t       tempData = 0;

  if ( si7013Detected) {
    Si7013_MeasureRHAndTemp(i2cInit.port, SI7021_ADDR, &rhData, &tempData);
  }
  // Limit the value to 100%.
  if (rhData > 100000) {
    rhData = 100000;
  }
  rhData /= 10;
  humidityMeasurementDone((uint16_t)rhData);
}

void rhtDeviceTemperatureMeasure(void (*temperatureMeasurementDone)(int16_t))
{
  uint32_t      rhData = 0;
  int32_t       tempData = 0;

  if ( si7013Detected) {
    Si7013_MeasureRHAndTemp(i2cInit.port, SI7021_ADDR, &rhData, &tempData);
  }
  tempData /= 10;
  temperatureMeasurementDone((int16_t)tempData);
}

/** @} (end addtogroup rht-sensor) */
/** @} (end addtogroup app_hardware) */
