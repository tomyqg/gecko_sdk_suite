/***************************************************************************//**
 * @file
 * @brief Battery Service
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

/* standard library headers */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* BG stack headers */
#include "bg_types.h"
#include "gatt_db.h"
#include "native_gecko.h"
#include "infrastructure.h"

#include "battery_device.h"
#include "app_timer.h"
#include "connection.h"

/* Own header*/
#include "battery_service.h"

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup battery
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

#define SILABS_AF_PLUGIN_BATT_NOTIFICATION_INT 10000

/** Battery measurement period in ms. */
#define BATT_IND_TIMEOUT                SILABS_AF_PLUGIN_BATT_NOTIFICATION_INT

/** Indicates currently there is no active connection using this service. */
#define BATT_NO_CONNECTION                   0xFF

/***************************************************************************************************
 * Local Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/

static uint8 batteryLevel; /* Battery Level */

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

/***************************************************************************************************
 * Public Variable Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
void batteryServiceInit(void)
{
  gecko_cmd_hardware_set_soft_timer(0, BATT_SERVICE_TIMER, false);
}

void batteryServiceCharStatusChange(uint8_t connection, uint16_t clientConfig)
{
  /* if the new value of CCC is not 0 (either indication or notification enabled)
   *  start battery level measurement */
  if (clientConfig) {
    batteryServiceMeasure(); /* make an initial measurement */
    gecko_cmd_hardware_set_soft_timer(TIMER_MS_2_TIMERTICK(BATT_IND_TIMEOUT), BATT_SERVICE_TIMER, false);
  } else {
    gecko_cmd_hardware_set_soft_timer(0, BATT_SERVICE_TIMER, false);
  }
}

void batteryServiceMeasure(void)
{
  /* Update battery level based on battery level sensor */
  batteryLevel = appHwReadBatteryLevel();

  /* Send notification */
  gecko_cmd_gatt_server_send_characteristic_notification(
    conGetConnectionId(), gattdb_battery_measurement, sizeof(batteryLevel), &batteryLevel);
}

void batteryServiceRead(void)
{
  /* Update battery level based on battery level sensor */
  batteryLevel = appHwReadBatteryLevel();
  /* Send response to read request */
  gecko_cmd_gatt_server_send_user_read_response(conGetConnectionId(), gattdb_battery_measurement, 0,
                                                sizeof(batteryLevel), &batteryLevel);
}

/** @} (end addtogroup battery) */
/** @} (end addtogroup Features) */
