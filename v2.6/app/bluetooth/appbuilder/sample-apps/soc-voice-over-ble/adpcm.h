/***************************************************************************//**
 * @file
 * @brief ADPCM encoder
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

#ifndef ADPCM_H_
#define ADPCM_H_

#include <stdint.h>
#include "mic.h"

/***************************************************************************//**
 * @addtogroup ADPCM
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup ADPCM_Config_Settings ADPCM coder Configuration Settings
 * @{
 * @brief ADPCM coder configuration setting macro definitions
 ******************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/
#define AUDIO_CHANNEL_MONO (1)                                                 /**< Mono channel */
#define ADPCM_PAYLOAD_SIZE (MIC_SAMPLE_BUFFER_SIZE / 2 / PROCESS_DATA_FACTOR)  /**< ADPCM compress data by factor 2. */

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/
typedef struct {
  int8_t step;                                           /**< Index into adpcmStepTable */
  int16_t predictedSample;                               /**< Last predicted sample */
} adpcm_state_t;

typedef struct {
  adpcm_state_t comprStateBegin;                        /**< Compression begin state */
  adpcm_state_t comprStateCurrent;                      /**< Compression current state */
  uint8_t      payload[ADPCM_PAYLOAD_SIZE];             /**< Encoded data buffer */
  uint8_t      payload_size;                            /**< Encoded data buffer size */
  uint8_t      payloadIndex;                            /**< Data index in Encoded data buffer */
} adpcm_t;

/***************************************************************************//**
 * @defgroup ADPCM_Functions ADPCM coder Functions
 * @{
 * @brief ADPCM coder API functions
 ******************************************************************************/

void ADPCM_init(void);
adpcm_t *ADPCM_ima_encodeBuffer(int16_t *buffer, uint16_t buffer_len);

/** @} {end defgroup ADPCM_Functions}*/

/** @} {end addtogroup ADPCM} */

#endif /* ADPCM_H_ */
