/***************************************************************************//**
 * @file
 * @brief Voice over Bluetooth Low Energy configuration
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

#ifndef VOBLE_CONFIG_H_
#define VOBLE_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/
typedef enum {
  adc_8bit = 8,
  adc_12bit = 12
}adc_resolution_t;

typedef enum {
  sr_8k = 8,
  sr_16k = 16,
}sample_rate_t;

typedef struct {
  adc_resolution_t adcResolution;
  sample_rate_t sampleRate;
  bool filter_enabled;
  bool encoding_enabled;
  bool transfer_status;
}voble_config_t;

#endif /* VOBLE_CONFIG_H_ */
