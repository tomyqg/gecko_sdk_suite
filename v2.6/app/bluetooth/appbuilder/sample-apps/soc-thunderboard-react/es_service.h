/***************************************************************************//**
 * @file
 * @brief Environmental Sensing Service
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

#ifndef ES_SERVICE_H
#define ES_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup es Environmental Sensing
 * \brief Environmental Sensing Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup es
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void esServiceInit(void);
void esServiceHumidityRead(void);
void esServiceTemperatureRead(void);
void esServiceUvIndexRead(void);

/** @} (end addtogroup es) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif // ES_SERVICE_H
