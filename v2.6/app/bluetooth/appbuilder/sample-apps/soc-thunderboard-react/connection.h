/***************************************************************************//**
 * @file
 * @brief Header file related to connection
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

#ifndef CONNECTION_H
#define CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup connection Connection
 * \brief Connection API
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup connection
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Declarations
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Indicate that connection has started.
 *  \param[in]  connection  ConnectionId.
 *  \param[in]  bonding  Bonding ID.
 **************************************************************************************************/
void conConnectionStarted(uint8_t connection, uint8_t bonding);

/***********************************************************************************************//**
 *  \brief  Indicate that connection has closed.
 **************************************************************************************************/
void conConnectionClosed(void);

/***********************************************************************************************//**
 *  \brief  Indicate that connection parameters have changed.
 *  \param[in]  connection  ConnectionId.
 *  \param[in]  interval  Interval.
 *  \param[in]  latency  Latency.
 *  \param[in]  timeout  Timeout.
 *  \param[in]  security_mode  Security mode.
 **************************************************************************************************/
void conConnectionParameters(uint8_t connection,
                             uint16_t interval,
                             uint16_t latency,
                             uint16_t timeout,
                             uint8_t security_mode);

/***********************************************************************************************//**
 *  \brief  Get connection Id.
 *  \return  Connection Id.
 **************************************************************************************************/
uint8_t conGetConnectionId(void);

/***********************************************************************************************//**
 *  \brief  Init connection parameters (bonding, etc).
 **************************************************************************************************/
void conConnectionInit();

/** @} (end addtogroup connection) */
/** @} (end addtogroup Features) */

#ifdef __cplusplus
};
#endif

#endif /* CONNECTION_H */
