/***************************************************************************//**
 * @file
 * @brief Ambient Light Service
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

#ifndef AMBLIGHT_SERVICE_H
#define AMBLIGHT_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup amblight Ambient Light
 * \brief Ambient Light Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup amblight
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
 *************************************************************************************************/
void amblightServiceInit(void);

/**********************************************************************************************//**
 * @brief
 *   Read the ambient light.
 *************************************************************************************************/
void amblightServiceRead(void);

/** @} (end addtogroup amblight) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif // AMBLIGHT_SERVICE_H
