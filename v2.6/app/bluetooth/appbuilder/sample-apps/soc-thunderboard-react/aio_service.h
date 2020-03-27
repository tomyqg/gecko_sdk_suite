/***************************************************************************//**
 * @file
 * @brief Automation I/O Service
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

#ifndef AIO_SERVICE_H
#define AIO_SERVICE_H

#include "bg_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup aio Automation IO
 * \brief Automation IO Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup aio
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/**********************************************************************************************//**
 * @brief
 *   Initialize the module.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceInit(void);

/**********************************************************************************************//**
 * @brief
 *   De-initialize the module.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDeInit(void);

/**********************************************************************************************//**
 * @brief
 *   Must be called whenever the connection is opened.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceConnectionOpened(void);

/**********************************************************************************************//**
 * @brief
 *   Must be called whenever the connection is closed.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceConnectionClosed(void);

/**********************************************************************************************//**
 * @brief
 *   Digital input characteristics changed event handler function.
 * @param[in]  connection
 *   Connection ID.
 * @param[in]  clientConfig
 *   New value of characteristics.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDigitalInCharStatusChange(uint8_t connection, uint16_t clientConfig);

/**********************************************************************************************//**
 * @brief
 *   Digital output write, used to set digital outputs.
 * @param[in]  writeValue
 *   Array of 2 2-bit values.
 *   0b00 = Inactive state, 0b01 = Active state, 0b10 = Tri state, 0b11 = Unknown
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDigitalOutWrite(uint8array *writeValue);

/**********************************************************************************************//**
 * @brief
 *   Digital output read, used to get digital inputs.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDigitalOutRead(void);

/**********************************************************************************************//**
 * @brief
 *   Must be called whenever a digital input changes state. Will notify
 *   subscribers about the current state.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDigitalInUpdate(void);

/**********************************************************************************************//**
 * @brief
 *   Digital input read, used to get digital inputs.
 * @return
 *   None
 *************************************************************************************************/
void aioServiceDigitalInRead(void);

/** @} (end addtogroup aio) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif // AIO_SERVICE_H
