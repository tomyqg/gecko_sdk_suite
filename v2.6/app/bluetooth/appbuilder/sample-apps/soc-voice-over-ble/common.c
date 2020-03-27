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
#include "common.h"

/***************************************************************************//**
 * @defgroup Common - Common TBSense API functions
 * @{
 * @brief Common TBSense API functions.
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup Common_Locals Common local variables
 * @{
 * @brief Common local variables
 ******************************************************************************/

voble_config_t    voble_config;
circular_buffer_t circular_buffer;

/** @} {end defgroup Common_Locals} */

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup Common_Functions Common Functions
 * @{
 * @brief Common support functions
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *    Get VoBLE configuration data
 *
 * @return
 *    Pointer to VoBLE configuration structure.
 ******************************************************************************/
voble_config_t * getVobleConfig(void)
{
  return &voble_config;
}

/***************************************************************************//**
 * @brief
 *    Get circular buffer
 *
 * @return
 *    Pointer to circular buffer structure.
 ******************************************************************************/
circular_buffer_t * getCircularBuffer(void)
{
  return &circular_buffer;
}

/***************************************************************************//**
 * @brief
 *    Set sample rate base on ADC sample rate characteristic value
 *
 * @return
 *    Sample rate
 ******************************************************************************/
uint32_t setSampleRate(sample_rate_t sr)
{
  uint32_t fs;

  switch (sr) {
    case sr_16k:
    case sr_8k:
      fs = U2K(sr);
      break;
    default:
      fs = U2K(voble_config.sampleRate);
      break;
  }
  return fs;
}

/***************************************************************************//**
 * @brief
 *    Send response to Write Request
 *
 * @return
 *    Write response result
 ******************************************************************************/
struct gecko_msg_gatt_server_send_user_write_response_rsp_t* send_write_response(struct gecko_cmd_packet* evt, uint16 characteristic, uint16 att_errorcode)
{
  return gecko_cmd_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection, characteristic, att_errorcode);
}

/** @} {end defgroup Common_Functions} */

/** @} {end defgroup Common} */
