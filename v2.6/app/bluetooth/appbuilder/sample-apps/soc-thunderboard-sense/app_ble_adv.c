/***************************************************************************//**
 * @file
 * @brief app_ble_adv.c
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

#include <stdbool.h>

/* BG stack headers */
#include "bg_types.h"
//#include "infrastructure.h"
#include "native_gecko.h"

/* BG stack headers */
#include "gatt_db.h"

/* application specific headers */
#include "app_timer.h"

/* features */
#include "connection.h"
#include "infrastructure.h"

/* BLE services */
#include "batt.h"
#include "iaq.h"
#include "es.h"
#include "ui.h"

#include "advertisement-callback.h"

/* Own header */
#include "app_ble_adv.h"

/*******************************************************************************
 * Local Variables
 ******************************************************************************/
static APP_BLE_ADV_iBeaconData  iBeaconData;
static APP_BLE_ADV_ScanResponse advData;

static bool beaconData = false;
static bool dataConfigured = false;
static bool advActive = false;

static const uint8_t iBeaconUUID[] = { APP_ADV_BLE_IBEACON_UUID };

/*******************************************************************************
 * Public Variable Definitions
 ******************************************************************************/

void appBleDataConfig(void);
void appBleAdvSetDevName(char deviceName[]);

void appBleDataConfig(void)
{
  if ( dataConfigured ) {
    return;
  }

  /* Adv payload init */
  advData.flagsLength         = APP_BLE_ADV_ADV_FLAGS_LENGTH;
  advData.flagsType           = APP_BLE_ADV_ADV_FLAGS_TYPE;
  advData.flags               = APP_BLE_ADV_ADV_FLAGS_LE_GENERAL_DISCOVERABLE | APP_BLE_ADV_ADV_FLAGS_BR_EDR_NOT_SUPPORTED;
  advData.mandatoryDataLength = APP_BLE_ADV_MANDATORY_DATA_LENGTH;
  advData.mandatoryDataType   = APP_BLE_ADV_MANDATORY_DATA_TYPE_MANUFACTURER;
  advData.companyId[0]        = (uint8_t) APP_BLE_ADV_COMPANY_ID_SILICON_LABS;
  advData.companyId[1]        = (uint8_t) APP_BLE_ADV_COMPANY_ID_SILICON_LABS >> 8;
  advData.firmwareId[0]       = (uint8_t) APP_BLE_ADV_FIRMWARE_ID;
  advData.firmwareId[1]       = (uint8_t) APP_BLE_ADV_FIRMWARE_ID >> 8;
  advData.localNameLength     = sizeof(APP_BLE_ADV_DEVICE_NAME_DEFAULT);
  advData.localNameType       = APP_BLE_ADV_TYPE_LOCAL_NAME;
  appBleAdvSetDevName(APP_BLE_ADV_DEVICE_NAME_DEFAULT);

  /* iBeacon payload init */
  iBeaconData.flagsLength         = APP_BLE_ADV_ADV_FLAGS_LENGTH;
  iBeaconData.flagsType           = APP_BLE_ADV_ADV_FLAGS_TYPE;
  iBeaconData.flags               = APP_BLE_ADV_ADV_FLAGS_LE_GENERAL_DISCOVERABLE | APP_BLE_ADV_ADV_FLAGS_BR_EDR_NOT_SUPPORTED;
  iBeaconData.mandatoryDataLength = APP_ADV_BLE_IBEACON_MAN_SPEC_DATA_LEN;
  iBeaconData.mandatoryDataType   = APP_BLE_ADV_MANDATORY_DATA_TYPE_MANUFACTURER;
  iBeaconData.companyId[0]        = (uint8_t) APP_ADV_BLE_IBEACON_PREAMBLE;
  iBeaconData.companyId[1]        = (uint8_t) APP_ADV_BLE_IBEACON_PREAMBLE >> 8;
  iBeaconData.beaconType[0]       = (uint8_t) APP_ADV_BLE_IBEACON_TYPE;
  iBeaconData.beaconType[1]       = (uint8_t) (APP_ADV_BLE_IBEACON_TYPE >> 8);
  memcpy(iBeaconData.uuid, iBeaconUUID, sizeof(iBeaconUUID));
  iBeaconData.major[0]            = (uint8_t) APP_ADV_BLE_IBEACON_MAJOR;
  iBeaconData.major[1]            = (uint8_t) APP_ADV_BLE_IBEACON_MAJOR >> 8;
  iBeaconData.minor[0]            = (uint8_t) APP_ADV_BLE_IBEACON_MINOR;
  iBeaconData.minor[1]            = (uint8_t) APP_ADV_BLE_IBEACON_MINOR >> 8;
  iBeaconData.rssi                = APP_ADV_BLE_IBEACON_RSSI;

  dataConfigured = true;

  return;
}

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
void advInit(void)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  struct gecko_msg_le_gap_stop_advertising_rsp_t* glegsarsp = NULL;
  /* Stop advertisement*/
  glegsarsp = gecko_cmd_le_gap_stop_advertising(0);
  APP_ASSERT_DBG(!glegsarsp->result, glegsarsp->result);

  /* Initialize advertisement */
  appBleAdvInit();

  /* we need to wait here some time with advertisement stopped until changes are registered in
   * stack and advertisement can be started again with a different user defined advertising message */
  ghsstrsp = gecko_cmd_hardware_set_soft_timer
             (
    TIMER_MS_2_TIMERTICK(APP_ADV_BLE_DATA_ALTERNATE_TIME_MS),
    ADV_ALTERNATE_TIMER, false
             );
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);

  advertisingStartedCallback(); /* call callback */
}

void appBleAdvInit(void)
{
  struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t* glegbsadrps = NULL;
  appBleDataConfig();
  glegbsadrps = gecko_cmd_le_gap_bt5_set_adv_data(0, 0, sizeof(advData), (uint8_t*)(&advData));
  APP_ASSERT_DBG(!glegbsadrps->result, glegbsadrps->result);
  beaconData = false;
}

void appBleAdvSetDevName(char deviceName[])
{
  strncpy((char *) advData.localName, deviceName, APP_BLE_ADV_DEVICE_NAME_MAX_LENGTH);
  if ( strlen(deviceName) <=  APP_BLE_ADV_DEVICE_NAME_MAX_LENGTH ) {
    advData.localNameLength = strlen(deviceName) + 1;
  } else {
    advData.localNameLength = APP_BLE_ADV_DEVICE_NAME_MAX_LENGTH + 1;
  }

  return;
}

void appBleAdvSetId(uint32_t uniqueId)
{
  iBeaconData.minor[1] = UINT32_TO_BYTE0(uniqueId);
  iBeaconData.minor[0] = UINT32_TO_BYTE1(uniqueId);
  iBeaconData.major[1] = UINT32_TO_BYTE2(uniqueId);

  return;
}

void appBleAdvStart(void)
{
  advActive = true;
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  struct gecko_msg_le_gap_start_advertising_rsp_t* glegsarsp = NULL;
  glegsarsp = gecko_cmd_le_gap_start_advertising(0, le_gap_user_data, le_gap_connectable_scannable);
  APP_ASSERT_DBG(!glegsarsp->result, glegsarsp->result);
  /* Start timer to change advertisement data */
  ghsstrsp = gecko_cmd_hardware_set_soft_timer
             (
    TIMER_MS_2_TIMERTICK(APP_ADV_BLE_DATA_ALTERNATE_TIME_MS),
    ADV_ALTERNATE_TIMER, false
             );
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);

  return;
}

void appBleAdvStop(void)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  struct gecko_msg_le_gap_stop_advertising_rsp_t* glegsarsp = NULL;
  advActive = false;
  ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, ADV_ALTERNATE_TIMER, false);
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  glegsarsp = gecko_cmd_le_gap_stop_advertising(0);
  APP_ASSERT_DBG(!glegsarsp->result, glegsarsp->result);
  return;
}

void appBleAdvEventHandler()
{
  struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t* glegbsadrps = NULL;
  // Swap advertising message, either iBeacon or regular advertisement data
  if (beaconData) {
    glegbsadrps = gecko_cmd_le_gap_bt5_set_adv_data(0, 0, sizeof(advData), (uint8_t*)(&advData));
    APP_ASSERT_DBG(!glegbsadrps->result, glegbsadrps->result);
    beaconData = false;
  } else {
    glegbsadrps = gecko_cmd_le_gap_bt5_set_adv_data(0, 0, sizeof(iBeaconData), (uint8_t*)(&iBeaconData));
    APP_ASSERT_DBG(!glegbsadrps->result, glegbsadrps->result);
    beaconData = true;
  }
}

bool appBleAdvIsActive(void)
{
  return advActive;
}
