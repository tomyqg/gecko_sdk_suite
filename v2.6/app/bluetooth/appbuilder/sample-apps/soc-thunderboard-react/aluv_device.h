/***************************************************************************//**
 * @file
 * @brief Ambient Light and UV Device
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

#ifndef ALUV_DEVICE_H
#define ALUV_DEVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup aluv-sensor Ambient Light And UV HW
 * \brief Ambient Light and UV Sensor API
 *
 * This is the module that handles ambient light and UV-index requests in the
 * application. It uses the si1133 interface library to do the actual low level
 * configuration and measurements of the sensor.
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app_hardware
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup aluv-sensor
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 * @brief
 *   The initialization routine for the sensor.
 **************************************************************************************************/
void aluvDeviceInit(void);

/***********************************************************************************************//**
 * @brief
 *   The de-initialization routine for the sensor.
 **************************************************************************************************/
void aluvDeviceDeInit(void);

/***********************************************************************************************//**
 * @brief
 *   Function to test if the sensor is detected.
 * @return
 *   0 if a si1133 is detected, 1 otherwise.
 **************************************************************************************************/
uint8_t aluvDeviceTest(void);

/***********************************************************************************************//**
 * @brief
 *   Put the sensor into sleep mode.
 *   Does nothing for the moment because the sensor is off when not in use.
 **************************************************************************************************/
void aluvDeviceSleep(void);

/***********************************************************************************************//**
 * @brief
 *   Function that is called when the connection is opened.
 **************************************************************************************************/
void aluvDeviceConnectionOpened(void);

/***********************************************************************************************//**
 * @brief
 *   Function that is called when the connection is closed.
 **************************************************************************************************/
void aluvDeviceConnectionClosed(void);

/***********************************************************************************************//**
 * @brief
 *   Start a lux measurement.
 *   The callback function will be called when the measurement is done.
 **************************************************************************************************/
void aluvDeviceLuxMeasure(void (*luxMeasurementDone)(uint32_t));

/***********************************************************************************************//**
 * @brief
 *   Start a UV-index measurement.
 *   The callback function will be called when the measurement is done.
 **************************************************************************************************/
void aluvDeviceUviMeasure(void (*uviMeasurementDone)(uint8_t));

/***********************************************************************************************//**
 * @brief
 *   Triggered by an interrupt from the ALUV device
 **************************************************************************************************/
void aluvDeviceInterruptEvtHandler(void);

/** @} (end addtogroup aluv-sensor) */
/** @} (end addtogroup app_hardware) */

#ifdef __cplusplus
};
#endif

#endif // ALUV_DEVICE_H
