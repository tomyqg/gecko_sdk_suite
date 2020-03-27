/***************************************************************************//**
 * @file
 * @brief Immediate Alert Service
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

#ifndef IA_H
#define IA_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup ia Immediate Alert
 * \brief Immediate Alert Service API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Services
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup ia
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Immediate Alert Write request with new Alert Level data.
 *  \param[in]  writeValue  Pointer to generic array holding written value.
 **************************************************************************************************/
void iaImmediateAlertWrite(uint8array *writeValue);

/** @} (end addtogroup ia) */
/** @} (end addtogroup Services) */

#ifdef __cplusplus
};
#endif

#endif /* IA_H */
