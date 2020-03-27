/***************************************************************************//**
 * @file
 * @brief power.c
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

#include "power.h"

/* BG stack headers */
#include "gatt_db.h"
#include "native_gecko.h"
#include "infrastructure.h"

#include "thunderboard/util.h"
#include "connection.h"

void powerInit(void)
{
  return;
}

void powerDeInit(void)
{
  return;
}

void powerSourceTypeRead(void)
{
  uint8_t sourceType = UTIL_supplyGetType();
  struct gecko_msg_gatt_server_send_user_read_response_rsp_t* gssurrrsp = NULL;
  gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(conGetConnectionId(),
                                                            gattdb_power_source_type,
                                                            0,
                                                            1,
                                                            &sourceType);
  APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
}
