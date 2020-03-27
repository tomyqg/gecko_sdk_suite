/***************************************************************************//**
 * @file
 * @brief Range Test static configuration source.
 * This file contains the different application configurations for each
 * separate radio cards.
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "rangeTest.h"
#include "rail_config.h"
#include "rangetestconfig.h"

char rangeTestPhys[10][10];

void initRangeTestPhys(uint8_t phyCnt)
{
  int i = 0;
  for (i = 0; i < phyCnt; ++i) {
    sprintf(rangeTestPhys[i], "custom_%d", i);
  }
}
