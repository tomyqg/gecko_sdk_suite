/***************************************************************************//**
 * @file
 * @brief Ambient Light Service
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

#include "amblight_service.h"

/* standard library headers */
#include <stdint.h>

/* BG stack headers */
#include "gatt_db.h"
#include "native_gecko.h"
#include "infrastructure.h"
#include "connection.h"

/* application specific headers */
//#include "app_hw.h"
#include "aluv_device.h"

/***********************************************************************************************//**
 * @addtogroup Features
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup amblight
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

#define AMBLIGHT_PAYLOAD_LEN         4

/***************************************************************************************************
 * Local Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/
static void luxReadReady(uint32_t amblight)
{
  uint8_t buffer[AMBLIGHT_PAYLOAD_LEN];
  uint8_t *p = buffer;

  UINT32_TO_BITSTREAM(p, amblight);
  gecko_cmd_gatt_server_send_user_read_response(conGetConnectionId(),
                                                gattdb_amblight_lux,
                                                0,
                                                AMBLIGHT_PAYLOAD_LEN,
                                                buffer);
}

/***************************************************************************************************
 * Public Variable Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
void amblightServiceInit(void)
{
}

void amblightServiceRead(void)
{
  aluvDeviceLuxMeasure(&luxReadReady);
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

/** @} (end addtogroup amblight) */
/** @} (end addtogroup Features) */
