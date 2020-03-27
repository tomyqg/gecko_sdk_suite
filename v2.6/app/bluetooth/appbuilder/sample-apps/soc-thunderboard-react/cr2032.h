/***************************************************************************//**
 * @file
 * @brief CR2032 Capacity Calculation
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

#ifndef CR2032_H
#define CR2032_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup tbr Thunderboard HW
 * \brief Thunderboard Hardware
 **************************************************************************************************/

/***********************************************************************************************//**
 * \defgroup cr2032 CR2032
 * \brief CR2032 Battery level calculations
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup tbr
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup cr2032
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  @brief
 *    Calculate the battery level based on the voltage.
 *    The routine assumes a very low current consumption.
 *  @param[in]  voltage
 *    The battery voltage in millivolts.
 *  @return
 *    The battery charge level in %.
 **************************************************************************************************/
uint8_t cr2032_CalculateLevel(uint16_t voltage);

/** @} (end addtogroup cr2032) */
/** @} (end addtogroup Thunderboard) */

#ifdef __cplusplus
};
#endif

#endif /* CR2032_H */
