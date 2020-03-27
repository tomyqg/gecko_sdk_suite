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

#ifndef RHT_DEVICE_H
#define RHT_DEVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup rht-sensor Relative Humidity And Temperature HW
 * \brief Relative Humidity And Temperature Sensor API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app_hardware
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup rht-sensor
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void rhtDeviceInit(void);

void rhtDeviceDeInit(void);

uint8_t rhtDeviceTest(void);

void rhtDeviceSleep(void);

void rhtDeviceConnectionOpened(void);

void rhtDeviceConnectionClosed(void);

void rhtDeviceHumidityMeasure(void (*humidityMeasurementDone)(uint16_t));

void rhtDeviceTemperatureMeasure(void (*temperatureMeasurementDone)(int16_t));

/** @} (end addtogroup rht-sensor) */
/** @} (end addtogroup app_hardware) */

#ifdef __cplusplus
};
#endif

#endif // RHT_DEVICE_H
