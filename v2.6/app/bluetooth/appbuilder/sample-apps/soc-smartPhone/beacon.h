/***************************************************************************//**
 * @file
 * @brief Beacon advertisement
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

#ifndef BEACON_H
#define BEACON_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup beacon Beacon
 * \brief Beacon Advertisement Service API
 *
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Services
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup beacon
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Setup advertising for Beaconing mode
 **************************************************************************************************/
void bcnSetupAdvBeaconing(void);

/** @} (end addtogroup beacon) */
/** @} (end addtogroup Services) */

#ifdef __cplusplus
};
#endif

#endif /* BEACON_H */
