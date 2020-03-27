/***************************************************************************//**
 * @file
 * @brief Common definitions and functions
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
#ifndef __COMMON_H_
#define __COMMON_H_

#include "bg_types.h"
#include "native_gecko.h"
#include "voble_config.h"
#include "circular_buff.h"

/***************************************************************************//**
 * @addtogroup Common
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup Common_Config Common configuration and definitions
 * @{
 * @brief Common configuration and definitions
 ******************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/
#define TRANSFER_STOP  (48)    /**< O in ASCII */
#define TRANSFER_START (49)    /**< 1 in ASCII */

#define EXT_SIGNAL_READY_TO_SEND    (1 << 4)  /**< External signal - Data ready to send in circular buffer */

#define U2K(val) ((val) * 1000) /**< Multiplay value by 1000 */

/** @} {end defgroup Common_Config} */

/***************************************************************************//**
 * @defgroup Common_Functions Common Functions declaration
 * @{
 * @brief Common functions for TBSense v1 and TBSense v2
 ******************************************************************************/

uint32_t setSampleRate(sample_rate_t sr);
voble_config_t * getVobleConfig(void);
circular_buffer_t * getCircularBuffer(void);
struct gecko_msg_gatt_server_send_user_write_response_rsp_t* send_write_response(struct gecko_cmd_packet* evt, uint16 characteristic, uint16 att_errorcode);

/** @} {end defgroup Common_Functions}*/

/** @} {end addtogroup Common} */

#endif /* __COMMON_H_ */
