/***************************************************************************//**
 * @file
 * @brief Application header file
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

#ifndef APP_H
#define APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "native_gecko.h"

/***********************************************************************************************//**
 * \defgroup tbr_app Application
 * \brief Sample Application Implementation
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Thunderboard
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup tbr_app
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/**********************************************************************************************//**
 * @brief
 *   Top level initialization of application
 *  return None
 *************************************************************************************************/
void appInit (void);

/**********************************************************************************************//**
 * @brief
 *   Function called by app-ble.c when a connection is opened
 *  param[in]  connection  Connection ID.
 *  param[in]  bonding  Bonding ID.
 *  return  None.
 *************************************************************************************************/
void appConnectionOpenedEvent(uint8_t connection, uint8_t bonding);

/**********************************************************************************************//**
 * @brief
 *   Function called by app-ble.c when a connection is closed
 *  param[in]  connection  Connection ID.
 *  param[in]  reason  Reason for lost connection.
 *  return None
 *************************************************************************************************/
void appConnectionClosedEvent(uint8_t connection, uint16_t reason);

/**********************************************************************************************//**
 * @brief
 *   Function called when advertisement should be stopped
 *  return None
 *************************************************************************************************/
void appAdvTimeoutEvtHandler(void);

/** @} (end addtogroup tbr_app) */
/** @} (end addtogroup Thunderboard) */

#ifdef __cplusplus
};
#endif

#endif // APP_H
