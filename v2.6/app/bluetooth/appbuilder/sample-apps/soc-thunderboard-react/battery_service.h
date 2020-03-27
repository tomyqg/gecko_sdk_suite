/***************************************************************************//**
 * @file
 * @brief Battery Service
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

#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup battery Battery
 * \brief Battery Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup battery
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Initialise Battery Service.
 *  \details  Initialise the connection ID, the configuration flags of the temperature measurement
 *  and stop temperature measurement timer.
 **************************************************************************************************/
void batteryServiceInit(void);

/***********************************************************************************************//**
 *  \brief  Battery CCCD has changed event handler function.
 *  \param[in]  connection  Connection ID.
 *  \param[in]  clientConfig  New value of CCCD.
 **************************************************************************************************/
void batteryServiceCharStatusChange(uint8_t connection, uint16_t clientConfig);

/***********************************************************************************************//**
 *  \brief  Make one battery measurement.
 **************************************************************************************************/
void batteryServiceMeasure(void);

/***********************************************************************************************//**
 *  \brief  Read battery measurement.
 **************************************************************************************************/
void batteryServiceRead(void);

/** @} (end addtogroup hr) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif /* BATTERY_SERVICE_H */
