/***************************************************************************//**
 * @file
 * @brief Acceleration and Orientation Service
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

#ifndef ACCOR_H
#define ACCOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "bg_types.h"

/***********************************************************************************************//**
 * \defgroup accor Acceleration and Orientation
 * \brief Acceleration and Orientation Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup accor
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/**********************************************************************************************//**
 * \brief  Initialize the module.
 *************************************************************************************************/
void accoriServiceInit(void);

/**********************************************************************************************//**
 * \brief  De-initialize the module.
 *************************************************************************************************/
void accoriServiceDeInit(void);

/**********************************************************************************************//**
 * \brief  Must be called whenever the connection is opened.
 *************************************************************************************************/
void accoriServiceConnectionOpened(void);

/**********************************************************************************************//**
 * \brief  Must be called whenever the connection is closed.
 *************************************************************************************************/
void accoriServiceConnectionClosed(void);

/**********************************************************************************************//**
 * \brief  Acceleration characteristics changed event handler function.
 * \param[in]  connection  Connection ID.
 * \param[in]  clientConfig  New value of characteristics.
 *************************************************************************************************/
void accoriServiceAccelerationCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * \brief  Orientation characteristics changed event handler function.
 * \param[in]  connection  Connection ID.
 * \param[in]  clientConfig  New value of characteristics.
 *************************************************************************************************/
void accoriServiceOrientationCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * \brief  Control Point characteristics changed event handler function.
 * \param[in]  connection  Connection ID.
 * \param[in]  clientConfig  New value of characteristics.
 *************************************************************************************************/
void accoriServiceCpCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * \brief  Control Point write, used to start a control point function.
 * \param[in]  writeValue  The function ID. 0x01=Start calibration, 0x02=Reset orientation
 *************************************************************************************************/
void accoriServiceCpWrite(uint8array *writeValue);

/**********************************************************************************************//**
 * \brief  Event to handle periodic acceleration measurements
 *************************************************************************************************/
void accoriServiceAccelerationTimerEvtHandler(void);

/**********************************************************************************************//**
 * \brief  Event to handle periodic orientation measurements
 *************************************************************************************************/
void accoriServiceOrientationTimerEvtHandler(void);

/** @} (end addtogroup accor) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif /* ACCOR_H */
