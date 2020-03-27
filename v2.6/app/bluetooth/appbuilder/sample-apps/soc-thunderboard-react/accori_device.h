/***************************************************************************//**
 * @file
 * @brief Acceleration and Orientation Device
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

#ifndef ACCGYRO_SENSOR_H
#define ACCGYRO_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************//**
 * \defgroup accgyro-sensor Acceleration and Orientation HW
 * \brief Acceleration and Orientation API.
 *
 * The accgyro-sensor module is the ble application interface to the
 * acceleration and orientation routines supported by underlaying hardware and
 * software libraries.
 *************************************************************************************************/

/**********************************************************************************************//**
 * @addtogroup app_hardware
 * @{
 *************************************************************************************************/

/**********************************************************************************************//**
 * @addtogroup accgyro-sensor
 * @{
 *************************************************************************************************/

/**************************************************************************************************
 * Public Macros and Definitions
 *************************************************************************************************/

/**************************************************************************************************
 * Public Type declarations
 *************************************************************************************************/

/**************************************************************************************************
 * Function Declarations
 *************************************************************************************************/

/**********************************************************************************************//**
 * \brief  Initialize the module.
 *************************************************************************************************/
void accoriDeviceInit(void);

/**********************************************************************************************//**
 * \brief  De-initialize the module.
 *************************************************************************************************/
void accoriDeviceDeInit(void);

/**********************************************************************************************//**
 * \brief  Test if the hardware is reay.
 * @return  0 if no errors detected, else 1.
 *************************************************************************************************/
uint8_t accoriDeviceTest(void);

/**********************************************************************************************//**
 * \brief  Put the hardware in sleep mode (for low power).
 *************************************************************************************************/
void accoriDeviceSleep(void);

/**********************************************************************************************//**
 * \brief  Prepare the module for requests.
 *************************************************************************************************/
void accoriDeviceConnectionOpened(void);

/**********************************************************************************************//**
 * \brief  Prepare the module for no more requests.
 *************************************************************************************************/
void accoriDeviceConnectionClosed(void);

/**********************************************************************************************//**
 * \brief  Read accelerometer data.
 * \param[out] accX  Accelerometer X value.
 * \param[out] accY  Accelerometer Y value.
 * \param[out] accZ  Accelerometer Z value.
 *************************************************************************************************/
void accoriDeviceAccelerationRead(int16_t *accX, int16_t *accY, int16_t *accZ);

/**********************************************************************************************//**
 * \brief  Read orientation data.
 * \param[out] oriX  Orientation X value.
 * \param[out] oriY  Orientation Y value.
 * \param[out] oriZ  Orientation Z value.
 *************************************************************************************************/
void accoriDeviceOrientationRead(int16_t *oriX, int16_t *oriY, int16_t *oriZ);

/**********************************************************************************************//**
 * \brief  Enable or disable accelerometers depending on the characteristics.
 * \param[in]  connection  Connection ID.
 * \param[in] clientConfig  Acceleration characteristic.
 *************************************************************************************************/
void accoriDeviceAccelerationCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * \brief  Enable or disable gyrometers depending on the characteristics.
 * \param[in]  connection  Connection ID.
 * \param[in] clientConfig  Orientation characteristic.
 *************************************************************************************************/
void accoriDeviceOrientationCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * \brief  Start a gyrometer calibration.
 * \param[in] calibrateDoneCallback  Function that will be called when the calibration is completed.
 *************************************************************************************************/
void accoriDeviceCalibrate(void (*calibrateDoneCallback)(void));

/**********************************************************************************************//**
 * \brief  Reset gyrometer calibration.
 *************************************************************************************************/
void accoriDeviceCalibrateReset(void);

/**********************************************************************************************//**
 * \brief  Reset the z-axis for the orientation.
 *************************************************************************************************/
void accoriDeviceOrientationReset(void);

/**********************************************************************************************//**
 * \brief  Event handler that is triggered from the interrupt handler to avoid processing in ISR
 *************************************************************************************************/
void accoriDeviceInterruptEvtHandler(void);

/** @} (end addtogroup accgyro-sensor) */
/** @} (end addtogroup app_hardware) */

#ifdef __cplusplus
};
#endif

#endif /* ACCGYRO_SENSOR_H */
