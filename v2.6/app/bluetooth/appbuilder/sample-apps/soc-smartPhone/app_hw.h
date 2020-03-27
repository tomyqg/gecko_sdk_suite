/***************************************************************************//**
 * @file
 * @brief Hardware specific application header file
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

#ifndef APP_HW_H
#define APP_HW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/***********************************************************************************************//**
 * \defgroup app_hw Application Hardware Specific
 * \brief Hardware specific application file.
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Data Types
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Initialize buttons and Temperature sensor.
 **************************************************************************************************/
void appHwInit(void);

/***********************************************************************************************//**
 *  \brief  Perform a temperature measurement.  Return the measurement data.
 *  \param[out]  tempData  Result of temperature conversion.
 *  \return  0 if Temp Read successful, otherwise -1
 **************************************************************************************************/
int32_t appHwReadTm(int32_t* tempData);

/***********************************************************************************************//**
 *  \brief  Initialise temperature measurement.
 *  \return  true if a Si7013 is detected, false otherwise
 **************************************************************************************************/
bool appHwInitTempSens(void);

/** @} (end addtogroup app_hw) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* APP_HW_H */
