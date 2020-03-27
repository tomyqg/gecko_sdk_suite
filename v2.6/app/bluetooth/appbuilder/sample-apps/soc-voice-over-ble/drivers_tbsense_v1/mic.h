/***************************************************************************//**
 * @file
 * @brief Driver for the SPV1840LR5H-B MEMS Microphone
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

#ifndef __MIC_H_
#define __MIC_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "mic_config.h"
#include "voble_config.h"

/***************************************************************************//**
 * @addtogroup Mic
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup Mic_Config_Settings MEMS Microphone Configuration Settings
 * @{
 * @brief MEMS microphone configuration setting macro definitions
 ******************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/
#define MIC_ADC_ACQ_TIME      MIC_CONFIG_ADC_ACQ_TIME    /**< ADC acquisition time  */
#define MIC_ADC_CLOCK_FREQ    MIC_CONFIG_ADC_CLOCK_FREQ  /**< ADC clock frequency   */

#define MIC_SAMPLE_BUFFER_SIZE    (112)                  /**< Sample buffer size. Maximum buffer size can be 492 due to audio data characteristic length,
                                                              however if audio data are sent in PCM format (not encoded)
                                                              maximum buffer should not be higher then 123 due to compression factor.*/
#define PROCESS_DATA_FACTOR       (1)                    /**< Factor required to keep compatibility with adpcm implementation.
                                                              1 means that ADPCM buffer does not reduce final data buffer size.
                                                              If there would be greater value encoded data will be cut off. */
#define MIC_SEND_BUFFER_SIZE      (224)                  /**< Buffer size used to send data via BLE */
#define PROCESS_DATA_BUFFER_SIZE  (MIC_SAMPLE_BUFFER_SIZE / PROCESS_DATA_FACTOR) /**< Buffer size required for selecting 16bits data from 32bits */

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/
typedef struct {
  uint16_t buffer0[MIC_SAMPLE_BUFFER_SIZE];              /**< DMA data Buffer0 */
  uint16_t buffer1[MIC_SAMPLE_BUFFER_SIZE];              /**< DMA data Buffer1 */
}audio_data_buff_t;

/** @} {end defgroup Mic_Config_Settings} */

/***************************************************************************//**
 * @defgroup Mic_External_Messages MEMS Microphone External Messages
 * @{
 * @brief MEMS microphone external message macro definitions
 ******************************************************************************/
#define EXT_SIGNAL_BUFFER0_READY  (1 << 2)  /**< External signal - Buffer0 ready for data processing */
#define EXT_SIGNAL_BUFFER1_READY  (1 << 3)  /**< External signal - Buffer1 ready for data processing */

/** @} {end defgroup Mic_External_Messages} */

/***************************************************************************//**
 * @defgroup Mic_Functions MEMS Microphone Functions
 * @{
 * @brief MEMS microphone driver and support functions
 ******************************************************************************/

uint32_t  MIC_init           (sample_rate_t sr, audio_data_buff_t *buffer);
void      MIC_deInit         (void);

uint16_t *MIC_getSampleBuffer0(void);
uint16_t *MIC_getSampleBuffer1(void);

/** @} {end defgroup Mic_Functions}*/

/** @} {end addtogroup Mic} */

#endif /* __MIC_H_ */
