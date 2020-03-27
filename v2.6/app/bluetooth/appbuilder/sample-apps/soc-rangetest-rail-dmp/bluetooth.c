/***************************************************************************//**
 * @file
 * @brief bluetooth.c
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

#include <common/include/common.h>
#include <common/include/logging.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/rtos_prio.h>
#include <common/source/logging/logging_priv.h>
#include <cpu/include/cpu.h>
#include <kernel/include/os.h>

#include <stdio.h>

#include "hal-config.h"

#include "rtos_bluetooth.h"
#include "rtos_gecko.h"
#include "gatt_db.h"

#include "board_features.h"
#include "rail_config.h"
#include "rangeTest.h"
#include "bluetooth.h"
#include "menu.h"
#include "response_print.h"
#include "infrastructure.h"

#define BLUETOOTH_EVENT_FLAG_APP_INDICATE ((OS_FLAGS)64)

#define BLUETOOTH_EVENT_FLAG_APP_MASK     (BLUETOOTH_EVENT_FLAG_APP_INDICATE)
#define BLUETOOTH_EVENT_FLAG_STK_MASK     (BLUETOOTH_EVENT_FLAG_EVT_WAITING)

typedef enum {
  idle    = 0,
  success = 1,
  failure = 2,
  ongoing = 3
} IndicateStatus;

typedef struct {
  uint16_t ccc;
} UserData;

typedef struct {
  uint16_t id;
  UserData data;
} CharUserData;

static CharUserData charUserDb[] = {
  { gattdb_pktsSent, { 0 } },
  { gattdb_pktsReq, { 0 } },
  { gattdb_channel, { 0 } },
  { gattdb_phy, { 0 } },
  { gattdb_radioMode, { 0 } },
  { gattdb_txPower, { 0 } },
  { gattdb_destID, { 0 } },
  { gattdb_srcID, { 0 } },
  { gattdb_payload, { 0 } },
  { gattdb_maSize, { 0 } },
  { gattdb_log, { 0 } },
  { gattdb_isRunning, { 0 } }
};

static IndicateStatus indicateStatus = idle;
static charIndList charInds = { { { 0 } } };

// Bluetooth stack
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
/* Configuration parameters (see gecko_configuration.h) */
static const gecko_configuration_t bluetooth_config =
{
  .config_flags = GECKO_CONFIG_FLAG_RTOS,
  .sleep.flags = 0,
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .gattdb = &bg_gattdb_data,
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
  .scheduler_callback = BluetoothLLCallback,
  .stack_schedule_callback = BluetoothUpdate,
#if (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
  .pa.config_enable = 1, // Enable high power PA
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#endif // (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
  .mbedtls.flags = GECKO_MBEDTLS_FLAGS_NO_MBEDTLS_DEVICE_INIT,
  .mbedtls.dev_number = 0,
};
static ConnClosedReason connClosedReason = unknown;
static uint8_t connHandle = 0xFF;
static OS_FLAGS flags = 0;

static inline void bluetoothWaitFlag(void);
static inline void bluetoothSetFlag(OS_FLAGS flag);
static inline bool bluetoothChkClrFlag(OS_FLAGS flag);
static inline bool bluetoothChkAppFlag(void);
static inline bool bluetoothChkStkFlag(void);
static void bStartTimer(void);
static void bStopTimer(void);
static bool setCharUserData(uint16_t id, UserData* data);
static bool getCharUserData(uint16_t id, UserData* data);
static void clearUserData(void);
static void clearIndications(void);
static bool dequeueIndication(charInd* bci);
static void sendIndication(charInd* bci);
static void startAdvertising(void);
static void stopAdvertising(void);

static OS_TMR bTimer;
static void bTimerCallback(void* p_tmr, void* p_arg);

/***************************************************************************//**
 * Setup the bluetooth init function.
 *
 * @return error code for the gecko_init function
 *
 * All bluetooth specific initialization code should be here like gecko_init(),
 * gecko_init_whitelisting(), gecko_init_multiprotocol() and so on.
 ******************************************************************************/
static errorcode_t initialize_bluetooth()
{
  errorcode_t err = gecko_init(&bluetooth_config);
  if (err == bg_err_success) {
    gecko_init_multiprotocol(NULL);
  }
  APP_RTOS_ASSERT_DBG((err == bg_err_success), 1);
  return err;
}

/**************************************************************************//**
 * Bluetooth Application task.
 *
 * @param p_arg Pointer to an optional data area which can pass parameters to
 *              the task when the task executes.
 *
 * This is a minimal Bluetooth Application task that starts advertising after
 * boot and supports OTA upgrade feature.
 *****************************************************************************/
void bluetoothAppTask(void *p_arg)
{
  PP_UNUSED_PARAM(p_arg);
  RTOS_ERR err;
  uint16_t val;
  MenuItemChange mic;
  MenuItemValue miv;
  char buf[8] = { 0 };
  UserData userData = { 0 };
  struct gecko_msg_gatt_server_write_attribute_value_rsp_t* gswavrsp = NULL;
  struct gecko_msg_gatt_server_send_user_read_response_rsp_t* gssurrrsp = NULL;
  struct gecko_msg_gatt_server_send_user_write_response_rsp_t* gssuwrrsp = NULL;

  OSTmrCreate(&bTimer,
              "Bluetooth Timer",
              0,
              100,
              OS_OPT_TMR_PERIODIC,
              &bTimerCallback,
              DEF_NULL,
              &err);
  APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
  // Start link layer and stack tasks.
  bluetooth_start(APP_CFG_TASK_BLUETOOTH_LL_PRIO,
                  APP_CFG_TASK_BLUETOOTH_STACK_PRIO,
                  initialize_bluetooth);

  while (DEF_TRUE) {
    // Wait for Bluetooth application and stack events.
    bluetoothWaitFlag();

    // Handle application events.
    if (bluetoothChkAppFlag()) {
      // App: Indicate event
      if (bluetoothChkClrFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE)) {
        charInd bci;
        if (!dequeueIndication(&bci)) {
          sendIndication(&bci);
        }
      }
    }

    // Handle stack events.
    if (bluetoothChkStkFlag()) {
      // Stack: event waiting to be handled.
      if (bluetoothChkClrFlag(BLUETOOTH_EVENT_FLAG_EVT_WAITING)) {
        switch (BGLIB_MSG_ID(bluetooth_evt->header)) {
          // This boot event is generated when the system boots up after reset.
          // Do not call any stack commands before receiving the boot event.
          // Write  the system is set to start advertising immediately after boot
          // procedure.
          case gecko_evt_system_boot_id:
            connHandle = 0xFF;
            APP_LOG("[info] [B] Booted\n");
            // Initialise Serial Number String characteristic based on the 16-bit device ID.
            snprintf(buf, 5, "%04x", *(uint16_t*)(gecko_cmd_system_get_bt_address()->address.addr));
            gswavrsp = gecko_cmd_gatt_server_write_attribute_value(gattdb_serial_number_string,
                                                                   0,
                                                                   4,
                                                                   (uint8_t*)buf);
            APP_ASSERT_DBG(!gswavrsp->result, gswavrsp->result);
            // Initialise Valid Range Descriptor of channel Characteristic based on RAIL config.
            buf[0] = (uint8_t)(channelConfigs[rangeTest.phy]->configs[0u].channelNumberStart & 0x00FF);
            buf[1] = (uint8_t)((channelConfigs[rangeTest.phy]->configs[0u].channelNumberStart >> 8) & 0x00FF);
            buf[2] = (uint8_t)(channelConfigs[rangeTest.phy]->configs[0u].channelNumberEnd & 0x00FF);
            buf[3] = (uint8_t)((channelConfigs[rangeTest.phy]->configs[0u].channelNumberEnd >> 8) & 0x00FF);
            gswavrsp = gecko_cmd_gatt_server_write_attribute_value(gattdb_channel_valid_range,
                                                                   0,
                                                                   4,
                                                                   (uint8_t*)buf);
            APP_ASSERT_DBG(!gswavrsp->result, gswavrsp->result);
            // Start advertising.
            startAdvertising();
            break;

          case gecko_evt_le_connection_opened_id:
            connHandle = bluetooth_evt->data.evt_le_connection_opened.connection;
            connClosedReason = unknown;
            APP_LOG("[info] [B] Connection opened\n");
            // Clear indications.
            clearIndications();
            // Reset CCC values.
            clearUserData();
            break;

          case gecko_evt_le_connection_closed_id:
            connHandle = 0xFF;
            APP_LOG("[info] [B] Connection closed\n");
            // Disable periodic pktSent indications
            bStopTimer();
            // Check the reason for the connection closed event
            if (boot_to_dfu == connClosedReason) {
              // Enter to DFU OTA mode.
              gecko_cmd_system_reset(2);
            } else if (deactivated == connClosedReason) {
              // Do not start advertising
            } else {
              // Restart advertising after client has disconnected.
              startAdvertising();
            }
            break;

          /* This event indicates that a remote GATT client is attempting to read a value of an
           *  attribute from the local GATT database, where the attribute was defined in the GATT
           *  XML firmware configuration file to have type="user". */
          case gecko_evt_gatt_server_user_read_request_id:
            if (gattdb_PER == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              uint16_t per = (uint16_t)(rangeTest.PER * 10);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_PER,
                0,
                2,
                (uint8_t*)&per);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_MA == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              uint16_t ma = (uint16_t)(rangeTest.MA * 10);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_MA,
                0,
                2,
                (uint8_t*)&ma);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_pktsSent == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_pktsSent,
                0,
                2,
                (uint8_t*)&rangeTest.pktsSent);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_pktsCnt == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_pktsCnt,
                0,
                2,
                (uint8_t*)&rangeTest.pktsCnt);
            } else if (gattdb_pktsRcvd == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_pktsRcvd,
                0,
                2,
                (uint8_t*)&rangeTest.pktsRcvd);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_pktsReq == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetPktsNum);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_pktsReq,
                0,
                2,
                (uint8_t*)&val);
            } else if (gattdb_channel == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetChannel);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_channel,
                0,
                2,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_phy == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetPhy);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_phy,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_phyList == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = getPhyList(&miv);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_phyList,
                0,
                miv.size,
                miv.data);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_radioMode == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetRFMode);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_radioMode,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_frequency == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetRFFrequency);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_frequency,
                0,
                2,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_txPower == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetRFPA);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_txPower,
                0,
                2,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_destID == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetIDDest);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_destID,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_srcID == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetIDSrc);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_srcID,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_payload == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetPktsLen);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_payload,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_maSize == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetMAWindow);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_maSize,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_log == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_SetLogMode);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_log,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else if (gattdb_isRunning == bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic) {
              val = menuItemValueGet(eAppMenu_RunDemo);
              // Send response to read request.
              gssurrrsp = gecko_cmd_gatt_server_send_user_read_response(
                bluetooth_evt->data.evt_gatt_server_user_read_request.connection,
                gattdb_isRunning,
                0,
                1,
                (uint8_t*)&val);
              APP_ASSERT_DBG(!gssurrrsp->result, gssurrrsp->result);
            } else {
              APP_LOG("[warning] [B] Unhandled characteristic read!");
            }
            APP_LOG("[info] [B] GATT read: ");
            snprintf(buf, 7, "%u\n", bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic);
            APP_LOG(buf);
            break;

          /* This event indicates that a remote GATT client is attempting to write a value of an
           * attribute into the local GATT database, where the attribute was defined in the GATT
           * XML firmware configuration file to have type="user". */
          case gecko_evt_gatt_server_user_write_request_id:
            if (gattdb_pktsReq == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              mic.index = eAppMenu_SetPktsNum;
              mic.value = *(uint16_t*)bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              menuItemChangeEnqueue(&mic);
              // Send response to write request.
              gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                gattdb_pktsReq,
                0);
              APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
            } else if (gattdb_channel == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_channel_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              uint16_t value = *(uint16_t*)bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if (channelConfigs[rangeTest.phy]->configs[0u].channelNumberStart <= value && value <= channelConfigs[rangeTest.phy]->configs[0u].channelNumberEnd) {
                mic.index = eAppMenu_SetChannel;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_channel,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_channel,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_phy == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              uint8_t value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              mic.index = eAppMenu_SetPhy;
              mic.value = (uint8_t)value;
              // Value is valid
              if (value < phyCnt) {
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_phy,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_phy,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_radioMode == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_radioMode_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              uint8_t value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if (gsravrsp->value.data[0] <= value && value <= gsravrsp->value.data[1]) {
                mic.index = eAppMenu_SetRFMode;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_radioMode,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_radioMode,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_txPower == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_txPower_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              int16_t value = *(int16_t*)bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if (*(int16_t*)gsravrsp->value.data <= value && value <= *(int16_t*)&gsravrsp->value.data[2]) {
                mic.index = eAppMenu_SetRFPA;
                mic.value = (uint16_t)value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_txPower,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_txPower,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_destID == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              mic.index = eAppMenu_SetIDDest;
              mic.value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              menuItemChangeEnqueue(&mic);
              // Send response to write request.
              gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                gattdb_destID,
                0);
              APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
            } else if (gattdb_srcID == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              mic.index = eAppMenu_SetIDSrc;
              mic.value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              menuItemChangeEnqueue(&mic);
              // Send response to write request.
              gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                gattdb_srcID,
                0);
              APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
            } else if (gattdb_payload == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_payload_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              uint8_t value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if (gsravrsp->value.data[0] <= value && value <= gsravrsp->value.data[1]) {
                mic.index = eAppMenu_SetPktsLen;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_payload,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_payload,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_maSize == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_maSize_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              uint8_t value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if (gsravrsp->value.data[0] <= value && value <= gsravrsp->value.data[1]) {
                mic.index = eAppMenu_SetMAWindow;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_maSize,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_maSize,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_log == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_log_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              bool value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if ((gsravrsp->value.data[0] <= ((uint8_t)value)) && (((uint8_t)value) <= gsravrsp->value.data[1])) {
                mic.index = eAppMenu_SetLogMode;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_log,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_log,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            } else if (gattdb_isRunning == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Check if value is within valid range
              struct gecko_msg_gatt_server_read_attribute_value_rsp_t* gsravrsp = \
                gecko_cmd_gatt_server_read_attribute_value(gattdb_isRunning_valid_range, 0);
              APP_ASSERT_DBG(!gsravrsp->result, gsravrsp->result);
              bool value = *bluetooth_evt->data.evt_gatt_server_user_write_request.value.data;
              // Value is in range
              if ((gsravrsp->value.data[0] <= ((uint8_t)value)) && (((uint8_t)value) <= gsravrsp->value.data[1])) {
                mic.index = eAppMenu_RunDemo;
                mic.value = value;
                menuItemChangeEnqueue(&mic);
                // Send response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_isRunning,
                  0);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
                // Value out of range
              } else {
                // Send Out Of Range error response to write request.
                gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                  bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                  gattdb_isRunning,
                  0xFF);
                APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              }
            }
            // Check if the user-type OTA Control Characteristic was written.
            // If ota_control was written, boot the device into Device Firmware
            // Upgrade (DFU) mode.
            else if (gattdb_ota_control == bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic) {
              // Set flag to enter to OTA mode.
              connClosedReason = boot_to_dfu;
              // Send response to Write Request.
              gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(
                bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
                gattdb_ota_control,
                bg_err_success);
              APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
              // Close connection to enter to DFU OTA mode.
              gecko_cmd_le_connection_close(bluetooth_evt->data.evt_gatt_server_user_write_request.connection);
            } else {
              APP_LOG("[warning] [B] Unhandled characteristic write!");
            }
            APP_LOG("[info] [B] GATT write: ");
            snprintf(buf, 7, "%u\n", bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic);
            APP_LOG(buf);
            break;

          /* This event indicates either that a local Client Characteristic Configuration descriptor
           * has been changed by the remote GATT client, or that a confirmation from the remote GATT
           * client was received upon a successful reception of the indication. */
          case gecko_evt_gatt_server_characteristic_status_id:
            if (gattdb_pktsSent == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_pktsSent, &userData), 0);
                if (rangeTest.isRunning && userData.ccc) {
                  // Enable periodic pktSent indications
                  bStartTimer();
                } else {
                  // Disable periodic pktSent indications
                  bStopTimer();
                }
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_pktsReq == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_pktsReq, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_channel == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_channel, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_phy == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_phy, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_radioMode == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_radioMode, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_txPower == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_txPower, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_destID == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_destID, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_srcID == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_srcID, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_payload == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_payload, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_maSize == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_maSize, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_log == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_log, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else if (gattdb_isRunning == bluetooth_evt->data.evt_gatt_server_characteristic_status.characteristic) {
              // confirmation of indication received from remote GATT client
              if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                indicateStatus = success;
                bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
                // client characteristic configuration changed by remote GATT client
              } else if (gatt_server_client_config == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
                userData.ccc = bluetooth_evt->data.evt_gatt_server_characteristic_status.client_config_flags;
                APP_ASSERT_DBG(setCharUserData(gattdb_isRunning, &userData), 0);
                // unhandled event
              } else {
                APP_ASSERT_DBG(false, 0);
              }
            } else {
              APP_LOG("[warning] [B] Unhandled indication confirmation or CCC change!\n");
            }
            if (gatt_server_confirmation == (enum gatt_server_characteristic_status_flag)bluetooth_evt->data.evt_gatt_server_characteristic_status.status_flags) {
              APP_LOG("[info] [B] Indication confirmation: ");
            } else {
              APP_LOG("[info] [B] CCC change: ");
            }
            snprintf(buf, 7, "%u\n", bluetooth_evt->data.evt_gatt_server_user_read_request.characteristic);
            APP_LOG(buf);
            break;

          case gecko_evt_gatt_procedure_completed_id:
            if (indicateStatus == ongoing
                && bg_err_gatt_connection_timeout == bluetooth_evt->data.evt_gatt_procedure_completed.result) {
              indicateStatus = failure;
              bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
            }
            break;

          default:
            break;
        }
      }
    }
    // Post to stack task that event has been handled.
    bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_EVT_HANDLED);
  }
}

// Private Functions

static inline void bluetoothWaitFlag(void)
{
  RTOS_ERR err;
  // Do not consume flags!
  flags |= OSFlagPend(&bluetooth_event_flags,
                      BLUETOOTH_EVENT_FLAG_STK_MASK
                      + BLUETOOTH_EVENT_FLAG_APP_MASK,
                      0,
                      OS_OPT_PEND_BLOCKING \
                      + OS_OPT_PEND_FLAG_SET_ANY,
                      NULL,
                      &err);
  APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
}

static inline void bluetoothSetFlag(OS_FLAGS flag)
{
  RTOS_ERR err;
  OSFlagPost(&bluetooth_event_flags,
             flag,
             OS_OPT_POST_FLAG_SET,
             &err);
  APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
}

static inline bool bluetoothChkClrFlag(OS_FLAGS flag)
{
  RTOS_ERR err;
  bool ret = false;
  if (flags & flag) {
    flags &= ~flag;
    OSFlagPost(&bluetooth_event_flags,
               flag,
               OS_OPT_POST_FLAG_CLR,
               &err);
    APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
    ret = true;
  }
  return ret;
}

static inline bool bluetoothChkAppFlag(void)
{
  return (bool)(flags & BLUETOOTH_EVENT_FLAG_APP_MASK);
}

static inline bool bluetoothChkStkFlag(void)
{
  return (bool)(flags & BLUETOOTH_EVENT_FLAG_STK_MASK);
}

static void bTimerCallback(void *p_tmr,
                           void *p_arg)
{
  /* Called when timer expires:                            */
  /*   'p_tmr' is pointer to the user-allocated timer.     */
  /*   'p_arg' is argument passed when creating the timer. */
  PP_UNUSED_PARAM(p_tmr);
  PP_UNUSED_PARAM(p_arg);
  // Send packet sent indications periodically with about 1Hz frequency.
  charInd bci = {
    .id = gattdb_pktsSent,
    .value = rangeTest.pktsSent
  };
  bluetoothEnqueueIndication(&bci);
}

static void bStartTimer(void)
{
  RTOS_ERR err;
  OS_STATE tmrState = OSTmrStateGet(&bTimer, &err);
  APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
  APP_LOG("bStartTimer: ");
  if (OS_TMR_STATE_RUNNING == tmrState) {
    APP_LOG("already started\n");
  } else {
    OSTmrStart(&bTimer, &err);
    APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
    APP_LOG("started\n");
  }
}

static void bStopTimer(void)
{
  RTOS_ERR err;
  OS_STATE tmrState = OSTmrStateGet(&bTimer, &err);
  APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
  APP_LOG("bStopTimer: ");
  if (OS_TMR_STATE_RUNNING == tmrState) {
    OSTmrStop(&bTimer, OS_OPT_TMR_NONE, DEF_NULL, &err);
    APP_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_CODE_GET(err));
    APP_LOG("stopped\n");
  } else {
    APP_LOG("already stopped\n");
  }
}

static bool setCharUserData(uint16_t id, UserData* data)
{
  for (uint8_t i = 0; i < (sizeof(charUserDb) / sizeof(CharUserData)); i++) {
    if (id == charUserDb[i].id) {
      memcpy((void*)&charUserDb[i].data, (void*)data, sizeof(UserData));
      return true;
    }
  }
  return false;
}

static bool getCharUserData(uint16_t id, UserData* data)
{
  for (uint8_t i = 0; i < (sizeof(charUserDb) / sizeof(CharUserData)); i++) {
    if (id == charUserDb[i].id) {
      memcpy((void*)data, (void*)&charUserDb[i].data, sizeof(UserData));
      return true;
    }
  }
  return false;
}

static void clearUserData(void)
{
  for (uint8_t i = 0; i < (sizeof(charUserDb) / sizeof(CharUserData)); i++) {
    memset((void*)&charUserDb[i].data, 0, sizeof(UserData));
  }
}

static void clearIndications(void)
{
  memset((void*)&charInds, 0, sizeof(charInds));
}

static bool dequeueIndication(charInd* bci)
{
  if (indicateStatus == ongoing) {
    return true;
  }
  if (indicateStatus == failure) {
    APP_ASSERT_DBG(false, 0);
  }
  if (charInds.size <= 0) {
    return true;
  }
  if (indicateStatus == success) {
    charInds.tail = (charInds.tail < CHAR_IND_LIST_SIZE - 1) \
                    ? (charInds.tail + 1) : 0;
    charInds.size--;
  }
  bci->id = charInds.items[charInds.tail].id;
  bci->value = charInds.items[charInds.tail].value;
  indicateStatus = idle;
  if (charInds.size == 0) {
    return true;
  }
  return false;
}

static void sendIndication(charInd* bci)
{
  uint8_t len = 0;
  uint8_t buffer[8] = { 0 };

  // Only send indication if we are connected.
  if (connHandle == 0xFF) {
    return;
  }

  switch (bci->id) {
    case gattdb_pktsSent:
    case gattdb_pktsReq:
    case gattdb_channel:
    case gattdb_phy:
    case gattdb_txPower:
      len = 2;
      break;

    case gattdb_radioMode:
    case gattdb_destID:
    case gattdb_srcID:
    case gattdb_payload:
    case gattdb_maSize:
    case gattdb_log:
    case gattdb_isRunning:
      len = 1;
      break;

    default:
      APP_ASSERT_DBG(false, 0);
      break;
  }
  memcpy((void*)buffer, (void*)&bci->value, len);
  struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t* gsscnrsp = \
    gecko_cmd_gatt_server_send_characteristic_notification(connHandle,
                                                           bci->id,
                                                           len,
                                                           buffer);
  APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);
  indicateStatus = ongoing;
  APP_LOG("[info] [B] Send indication: ");
  snprintf((char*)buffer, 7, "%u\n", bci->id);
  APP_LOG((char*)buffer);
}

static void startAdvertising(void)
{
  // Range Test RAIL DMP UUID defined by marketing.
  const uint8_t uuid[16] = { 0x63, 0x7e, 0x17, 0x3b, 0x39, 0x9e, 0x20, 0x9f,
                             0x62, 0x4d, 0xe6, 0x17, 0x49, 0xa6, 0x0a, 0x53 };
  char sLocalName[] = "DMP0000";
  uint8_t i = 0;
  uint8_t buf[31] = { 0 };
  // Construct advertisement structure
  // AD Structure: Flags
  buf[i++] = 2;               // Length of field: Type + Flags
  buf[i++] = 0x01;            // Type of field: Flags
  buf[i++] = 0x04 | 0x02;     // Flags: BR/EDR is disabled, LE General Discoverable Mode
  // AD Structure: Shortened Local Name, e.g.: DMP1234
  snprintf(&sLocalName[3], 5, "%04X", *(uint16_t*)(gecko_cmd_system_get_bt_address()->address.addr));
  buf[i++] = 1 + sizeof(sLocalName) - 1;  // Length of field: Type + Shortened Local Name
  buf[i++] = 0x08;            // Shortened Local Name
  memcpy(&buf[i], (uint8_t*)sLocalName, sizeof(sLocalName) - 1); i += (sizeof(sLocalName) - 1);
  // AD Structure: Range Test DMP UUID
  buf[i++] = 1 + sizeof(uuid);
  buf[i++] = 0x06;            // Incomplete List of 128-bit Service Class UUID
  memcpy(&buf[i], uuid, sizeof(uuid)); i += sizeof(uuid);
  struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t* lgbsadrsp = \
    gecko_cmd_le_gap_bt5_set_adv_data(0, 0, i, buf);
  APP_ASSERT_DBG(!lgbsadrsp->result, lgbsadrsp->result);
  // Set advertising interval to 100ms.
  struct gecko_msg_le_gap_set_advertise_timing_rsp_t* lgsatrsp = \
    gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);
  APP_ASSERT_DBG(!lgsatrsp->result, lgsatrsp->result);
  // Start advertising and enable connections.
  struct gecko_msg_le_gap_start_advertising_rsp_t* lgsarsp = \
    gecko_cmd_le_gap_start_advertising(0,
                                       le_gap_user_data,
                                       le_gap_connectable_scannable);
  APP_ASSERT_DBG(!lgsarsp->result, lgsarsp->result);
  APP_LOG("[info] [B] Start advertising\n");
}

static void stopAdvertising(void)
{
  struct gecko_msg_le_gap_stop_advertising_rsp_t* rsp = gecko_cmd_le_gap_stop_advertising(0);
  APP_ASSERT_DBG(!rsp->result, rsp->result);
  APP_LOG("[info] [B] Stop advertising\n");
}

// Public Functions

void bluetoothActivate(void)
{
  // no connection: start advertising again
  if (connHandle == 0xFF) {
    stopAdvertising();
    startAdvertising();
  }
}

void bluetoothDeactivate(void)
{
  // close connection
  if (connHandle != 0xFF) {
    struct gecko_msg_le_connection_close_rsp_t* rsp = gecko_cmd_le_connection_close(connHandle);
    APP_ASSERT_DBG(!rsp->result, rsp->result);
    connClosedReason = deactivated;
  }
  // stop advertisement
  else {
    stopAdvertising();
  }
}

void bluetoothAdvertiseRealtimeData(uint8_t rssi, uint16_t cnt, uint16_t rcvd)
{
  // Company Identifier of Silicon Labs
  uint16_t compId = 0x02FF;
  char sLocalName[] = "DMP0000";
  uint8_t i = 0;
  uint8_t buf[31] = { 0 };
  // Construct advertisement structure
  // AD Structure: Flags
  buf[i++] = 2;               // Length of field: Type + Flags
  buf[i++] = 0x01;            // Type of field: Flags
  buf[i++] = 0x04 | 0x02;     // Flags: BR/EDR is disabled, LE General Discoverable Mode
  // AD Structure: Shortened Local Name, e.g.: DMP1234
  snprintf(&sLocalName[3], 5, "%04X", *(uint16_t*)(gecko_cmd_system_get_bt_address()->address.addr));
  buf[i++] = 1 + sizeof(sLocalName) - 1;  // Length of field: Type + Shortened Local Name
  buf[i++] = 0x08;            // Shortened Local Name
  memcpy(&buf[i], (uint8_t*)sLocalName, sizeof(sLocalName) - 1); i += (sizeof(sLocalName) - 1);
  // AD Structure: Manufacturer specific
  buf[i++] = 9;               // Length of structure
  buf[i++] = 0xFF;            // Manufacturer Specific Data
  buf[i++] = (uint8_t)(compId & 0x00FF);        // Company ID
  buf[i++] = (uint8_t)((compId >> 8) & 0x00FF); // Company ID
  buf[i++] = 0x00;            // Structure type; used for backward compatibility
  buf[i++] = rssi;            // RSSI
  buf[i++] = (uint8_t)(cnt & 0x00FF);           // Packet counter
  buf[i++] = (uint8_t)((cnt >> 8) & 0x00FF);    // Packet counter
  buf[i++] = (uint8_t)(rcvd & 0x00FF);          // Number of received packets
  buf[i++] = (uint8_t)((rcvd >> 8) & 0x00FF);   // Number of received packets
  struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t* lgbsadrsp = \
    gecko_cmd_le_gap_bt5_set_adv_data(0, 0, i, buf);
  APP_ASSERT_DBG(!lgbsadrsp->result, lgbsadrsp->result);
  // Configure advertising to send out only 1 packet.
  struct gecko_msg_le_gap_set_advertise_timing_rsp_t* lgsatrsp = \
    gecko_cmd_le_gap_set_advertise_timing(0, 32, 32, 0, 1);
  APP_ASSERT_DBG(!lgsatrsp->result, lgsatrsp->result);
  // Start advertising.
  struct gecko_msg_le_gap_start_advertising_rsp_t* lgsarsp = \
    gecko_cmd_le_gap_start_advertising(0,
                                       le_gap_user_data,
                                       le_gap_non_connectable);
  APP_ASSERT_DBG(!lgsarsp->result, lgsarsp->result);
//  APP_LOG("[info] [B] Advertise RSSI\n");
}

bool bluetoothEnqueueIndication(charInd* bci)
{
  UserData userData;
  if (0xFF == connHandle
      || !getCharUserData(bci->id, &userData)
      || gatt_indication != (enum gatt_client_config_flag)userData.ccc) {
    return true;
  }
  if (charInds.size >= CHAR_IND_LIST_SIZE) {
    APP_ASSERT_DBG(false, 0);
  }
  charInds.items[charInds.head].id = bci->id;
  charInds.items[charInds.head].value = bci->value;
  charInds.head = (charInds.head < CHAR_IND_LIST_SIZE - 1) \
                  ? (charInds.head + 1) : 0;
  charInds.size++;
  bluetoothSetFlag(BLUETOOTH_EVENT_FLAG_APP_INDICATE);
  return false;
}

void bluetoothPktsSentIndications(bool enable)
{
  UserData userData = { 0 };
  APP_ASSERT_DBG(getCharUserData(gattdb_pktsSent, &userData), 0);
  // Start periodic pktsSent indications if its CCC is enabled.
  if (enable && userData.ccc) {
    bStartTimer();
  } else {
    bStopTimer();
  }
}
