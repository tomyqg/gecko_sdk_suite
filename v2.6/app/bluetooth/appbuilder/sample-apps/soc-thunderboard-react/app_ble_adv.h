/***************************************************************************//**
 * @file
 * @brief Application BLE Advertisement
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

#ifndef APP_BLE_ADV_H
#define APP_BLE_ADV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup app_ble_adv Application BLE Advertisement
 * \brief Application BLE Advertisement API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Thunderboard
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app_ble_adv
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Declarations
 **************************************************************************************************/

/**********************************************************************************************//**
 * @brief
 *   Function to initialize advertisement. Should be run once at startup.
 *  return None
 *************************************************************************************************/
void appBleAdvInit(void);

/**********************************************************************************************//**
 * @brief
 *   Function called to set a custom device name as local name in the
 *  advertisement paylaod.
 *  param[in]  devName Name to use as local name in advertisement
 *  return None
 *************************************************************************************************/
void appBleAdvSetDevName(char *devName);

/**********************************************************************************************//**
 * @brief
 *   Function called to set the uniqueId that is used as the major/minor values
 *  in the iBeacon payload
 *  param[in]  uniqueId 24-bit Unique ID
 *  return None
 *************************************************************************************************/
void appBleAdvSetId(uint32_t uniqueId);

/**********************************************************************************************//**
 * @brief
 *   Function to start advertisement. Advertisement alternates each second
 *  between an iBeacon and regular advertisment payload. appBleAdvInit, must be
 *  called before this function is called.
 *  return None
 *************************************************************************************************/
void appBleAdvStart(void);

/**********************************************************************************************//**
 * @brief
 *   Function to stop advertisement.
 *  return None
 *************************************************************************************************/
void appBleAdvStop(void);

/**********************************************************************************************//**
 * @brief
 *   Function to alternate advertisement data periodically
 *  return None
 *************************************************************************************************/
void appBleAdvAlternateEvtHandler(void);

/** @} (end addtogroup app_ble_adv) */
/** @} (end addtogroup Thunderboard) */

#endif /* APP_BLE_ADV_H */
