/***************************************************************************//**
 * @file
 * @brief Infrastructure for commonly included files in the BLE SDK
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

#ifndef __COMMON_INCLUDES__
#define __COMMON_INCLUDES__

#include <string.h>
#include <stdio.h>

// Common defines for NCP host applications
#if defined(BLE_NCP_HOST)
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include "gecko_bglib.h"
#include "uart.h"
#include <unistd.h>

// Common defines for SOC and NCP target applications
#else // BLE_NCP_HOST

// GATT database
#include "gatt_db.h"

// Board Headers
#include "ble-configuration.h"
#include "board_features.h"

// Bluetooth stack headers
#include "bg_types.h"
#include "native_gecko.h"
#include "infrastructure.h"

// EM library (EMlib)
#include "em_system.h"
#include "em_gpio.h"

// Libraries containing default Gecko configuration values
#include "em_emu.h"
#include "em_cmu.h"

#ifdef FEATURE_BOARD_DETECTED
#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif
#include "pti.h"
#else // FEATURE_BOARD_DETECTED
#error This sample app only works with a Silicon Labs Board
#endif // FEATURE_BOARD_DETECTED

#include "bsp.h"

#ifdef FEATURE_IOEXPANDER
#include "bsp_stk_ioexp.h"
#endif // FEATURE_IOEXPANDER

#endif //BLE_NCP_HOST

#include "infrastructure.h"
#include "stack_bridge.h"
#include "ble-callbacks.h"

#endif // __COMMON_INCLUDES__
