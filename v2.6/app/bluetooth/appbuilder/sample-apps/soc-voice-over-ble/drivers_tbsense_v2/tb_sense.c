/***************************************************************************//**
 * @file
 * @brief Event handling and application code for TBSense v1 VoBLE application
 * @version 5.2.1
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
#include <limits.h>
#include <gatt_db.h>
#include <filter.h>
#include "common.h"
#include "mic.h"
#include "adpcm.h"
#include "circular_buff.h"
#include "tb_sense.h"

/***************************************************************************//**
 * @defgroup TB_Sense Thunderboard Sense API functions
 * @{
 * @brief Specific Thunderboard Sense 2 function
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup TB_Sense_Locals TB Sense Local Variables
 * @{
 * @brief Thunderboard Sense 2 local variables
 ******************************************************************************/

/** @} {end defgroup TB_Sense_Locals} */

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup TB_Sense_Functions Thunderboard Sense 2 function definitions
 * @{
 * @brief Thunderboard Sense 2 public API function definitions
 ******************************************************************************/
void vobleInit(void)
{
  getVobleConfig()->sampleRate = sr_16k;
  getVobleConfig()->filter_enabled = true;
  getVobleConfig()->encoding_enabled = true;
  getVobleConfig()->transfer_status = true;
}

/***************************************************************************//**
 * @brief
 *    Sets ADC resolution in Voice over BLE configuration structure
 *
 * @return
 *    None
 ******************************************************************************/
void writeRequestAdcResolutionHandle(struct gecko_cmd_packet* evt)
{
  send_write_response(evt, gattdb_adc_resolution, bg_err_att_application);
}

/***************************************************************************//**
 * @brief
 *    Process data incoming from microphone. Data are filtered or not depending on
 *    voble_config.filter_enabled flag, stored in temporary buffer, encoded and finally
 *    written to audio data characteristic.
 *
 * @return
 *    None
 ******************************************************************************/
void processData(int16_t *pSrc)
{
  int16_t buffer[PROCESS_DATA_BUFFER_SIZE] = { 0 };
  uint16_t fill_buff[PROCESS_DATA_BUFFER_SIZE] = { 0 };
  adpcm_t *pEncoded;

  /* Select 16bits data */
  for ( uint16_t i = 0; i < PROCESS_DATA_BUFFER_SIZE; i++ ) {
    buffer[i] = (int16_t)pSrc[i * PROCESS_DATA_FACTOR];
  }

  if ( getVobleConfig()->filter_enabled) {
    /* Convert int16_t to uint16_t for filter function */
    for ( uint16_t i = 0; i < PROCESS_DATA_BUFFER_SIZE; i++ ) {
      fill_buff[i] = (uint16_t)(((int32_t)buffer[i]) - SHRT_MIN);
    }

    /* Filter data */
    FIL_filter(buffer, fill_buff, PROCESS_DATA_BUFFER_SIZE);
  }

  if ( getVobleConfig()->encoding_enabled) {
    /* Encode data */
    pEncoded = ADPCM_ima_encodeBuffer(buffer, PROCESS_DATA_BUFFER_SIZE);
    cb_push_buff(getCircularBuffer(), pEncoded->payload, pEncoded->payload_size);
  } else {
    cb_push_buff(getCircularBuffer(), (uint8_t *)buffer, (sizeof(buffer) / sizeof(uint16_t)) * 2);
  }

  gecko_external_signal(EXT_SIGNAL_READY_TO_SEND);

  return;
}

/** @} {end defgroup TB_Sense_Functions} */

/** @} {end defgroup TB_Sense} */
