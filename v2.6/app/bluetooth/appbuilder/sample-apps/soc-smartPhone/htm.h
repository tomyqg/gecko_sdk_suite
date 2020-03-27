/***************************************************************************//**
 * @file
 * @brief Health Thermometer Service
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

#ifndef HTM_H
#define HTM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup htm Health Thermometer
 * \brief Health Thermometer Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Services
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup htm
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Initialise Health Thermometer Service.
 *  \details  Initialise the connection ID, the configuration flags of the temperature measurement
 *  and stop temperature measurement timer.
 **************************************************************************************************/
void htmInit(void);

/***********************************************************************************************//**
 *  \brief  Temperature CCCD has changed event handler function.
 *  \param[in]  connection  Connection ID.
 *  \param[in]  clientConfig  New value of CCCD.
 **************************************************************************************************/
void htmTemperatureCharStatusChange(uint8_t connection, uint16_t clientConfig);

/***********************************************************************************************//**
 *  \brief  Make one temperature measurement.
 **************************************************************************************************/
void htmTemperatureMeasure(void);

/** @} (end addtogroup htm) */
/** @} (end addtogroup Services) */

#ifdef __cplusplus
};
#endif

#endif /* HTM_H */
