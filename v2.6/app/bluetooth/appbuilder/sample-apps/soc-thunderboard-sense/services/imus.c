/***************************************************************************//**
 * @file
 * @brief imus.c
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
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gatt_db.h"
#include "native_gecko.h"
#include "infrastructure.h"

/* plugin headers */
#include "app_timer.h"
#include "connection.h"

#include "thunderboard/imu/imu.h"
#include "thunderboard/icm20648.h"

/* Own header*/
#include "imus.h"

// Measurement periodes in ms.
#define ACCELERATION_MEASUREMENT_PERIOD             200
#define ORIENTATION_MEASUREMENT_PERIOD              200

// Number of axis for acceleration and orientation
#define ACC_AXIS                               3
#define ORI_AXIS                               3

// Acceleration and orientation axis payload length in bytes
#define ACC_AXIS_PAYLOAD_LENGTH                2
#define ORI_AXIS_PAYLOAD_LENGTH                2

// Intertial payload length in bytes
#define ACCELERATION_PAYLOAD_LENGTH  (ACC_AXIS * ACC_AXIS_PAYLOAD_LENGTH)
#define ORIENTATION_PAYLOAD_LENGTH   (ORI_AXIS * ORI_AXIS_PAYLOAD_LENGTH)

#define CP_OPCODE_CALIBRATE             0x01
#define CP_OPCODE_ORIRESET              0x02
#define CP_OPCODE_RESPONSE              0x10
#define CP_OPCODE_CALRESET              0x64

#define CP_RESP_SUCCESS                 0x01
#define CP_RESP_ERROR                   0x02

// Error codes
// Client Characteristic Configuration descriptor improperly configured
#define ERR_CCCD_CONF                   0x81

#define MAX_ACC  1000
#define MIN_ACC -1000

#define MAX_ORI       18000
#define ORI_INCREMENT MAX_ORI / 50

static bool cpIndication = false;
static bool accelerationNotification = false;
static bool orientationNotification  = false;
static bool calibrationInProgress = false;

static void imuSendCalibrateDone  (void);
static void handleDeviceInitDeinit(void);

void imuInit(void)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ACC_TIMER, false);
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ORI_TIMER, false);
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);

  accelerationNotification = false;
  orientationNotification  = false;

  return;
}

void imuConnectionOpened(void)
{
  return;
}

void imuServiceStop(void)
{
  imuConnectionClosed();
  return;
}

void imuServiceStart(void)
{
  return;
}

bool imuIsAccelerationNotification(void)
{
  return accelerationNotification;
}

bool imuIsOrientationNotification(void)
{
  return orientationNotification;
}

void imuConnectionClosed(void)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ACC_TIMER, false);
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ORI_TIMER, false);
  APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  accelerationNotification = false;
  orientationNotification  = false;

  return;
}

void imuAccelerationStatusChange(uint8_t connection,
                                 uint16_t clientConfig)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  printf("IMU: Acceleration Status Change: %d:%04x\r\n", connection, clientConfig);

  accelerationNotification = (clientConfig > 0);
  handleDeviceInitDeinit();
  if ( accelerationNotification ) {
    ghsstrsp = gecko_cmd_hardware_set_soft_timer
               (
      TIMER_MS_2_TIMERTICK(ACCELERATION_MEASUREMENT_PERIOD),
      IMU_SERVICE_ACC_TIMER, false
               );
    APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  } else {
    ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ACC_TIMER, false);
    APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  }

  return;
}

void imuOrientationStatusChange(uint8_t connection,
                                uint16_t clientConfig)
{
  struct gecko_msg_hardware_set_soft_timer_rsp_t* ghsstrsp = NULL;
  printf("IMU: Orientation Status Change: %d:%04x\r\n", connection, clientConfig);

  orientationNotification = (clientConfig > 0);
  handleDeviceInitDeinit();
  if ( orientationNotification ) {
    ghsstrsp = gecko_cmd_hardware_set_soft_timer
               (
      TIMER_MS_2_TIMERTICK(ORIENTATION_MEASUREMENT_PERIOD),
      IMU_SERVICE_ORI_TIMER, false
               );
    APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  } else {
    ghsstrsp = gecko_cmd_hardware_set_soft_timer(TIMER_STOP, IMU_SERVICE_ORI_TIMER, false);
    APP_ASSERT_DBG(!ghsstrsp->result, ghsstrsp->result);
  }

  return;
}

void imuAccelerationTimerEvtHandler(void)
{
  struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t* gsscnrsp = NULL;
  if ( calibrationInProgress ) {
    return;
  }

  uint8_t buffer[ACCELERATION_PAYLOAD_LENGTH];
  uint8_t *p;
  int16_t avec[3];

  p = buffer;

  IMU_accelerationGet(avec);
  UINT16_TO_BITSTREAM(p, (uint16_t)avec[0]);
  UINT16_TO_BITSTREAM(p, (uint16_t)avec[1]);
  UINT16_TO_BITSTREAM(p, (uint16_t)avec[2]);

  /*printf("IMU: ACC (%d): %04d,%04d,%04d\r\n", accelerationNotification, avec[0], avec[1], avec[2] );*/
  /*printf("A");*/

  if ( !accelerationNotification ) {
    return;
  }

  gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(
    conGetConnectionId(),
    gattdb_imu_acceleration,
    ACCELERATION_PAYLOAD_LENGTH,
    buffer);
  APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);

  return;
}

void imuOrientationTimerEvtHandler(void)
{
  struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t* gsscnrsp = NULL;
  if ( calibrationInProgress ) {
    return;
  }

  /* int16_t oriX,oriY,oriZ; */
  uint8_t buffer[ORIENTATION_PAYLOAD_LENGTH];
  uint8_t *p;
  int16_t ovec[3];

  p = buffer;

  IMU_orientationGet(ovec);
  UINT16_TO_BITSTREAM(p, (uint16_t)ovec[0]);
  UINT16_TO_BITSTREAM(p, (uint16_t)ovec[1]);
  UINT16_TO_BITSTREAM(p, (uint16_t)ovec[2]);

  /*printf("IMU: ORI (%d): %04d,%04d,%04d\r\n", orientationNotification, ovec[0], ovec[1], ovec[2] );*/
  /*printf("O");*/

  if (!orientationNotification) {
    return;
  }

  gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(
    conGetConnectionId(),
    gattdb_imu_orientation,
    ORIENTATION_PAYLOAD_LENGTH,
    buffer);
  APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);

  return;
}

void imuControlPointStatusChange(uint8_t connection, uint16_t clientConfig)
{
  printf("IMU: CP Status Change: %d:%04x\r\n", connection, clientConfig);

  /* Enable / disable indications */
  cpIndication = (clientConfig > 0);

  return;
}

static void imuSendCalibrateDone(void)
{
  uint8_t respBuf[3];
  uint8_t *respBufp;
  struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t* gsscnrsp = NULL;

  if ( cpIndication ) {
    respBufp = respBuf;
    UINT8_TO_BITSTREAM(respBufp, CP_OPCODE_RESPONSE);
    UINT8_TO_BITSTREAM(respBufp, CP_OPCODE_CALIBRATE);
    UINT8_TO_BITSTREAM(respBufp, CP_RESP_SUCCESS);
    gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(conGetConnectionId(),
                                                                      gattdb_imu_control_point,
                                                                      3,
                                                                      respBuf);
    APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);
  }

  return;
}

void imuDeviceCalibrate(void)
{
  calibrationInProgress = true;

  IMU_gyroCalibrate();

  calibrationInProgress = false;

  imuSendCalibrateDone();

  return;
}

void imuControlPointWrite(uint8array *writeValue)
{
  uint8_t respBuf[3];
  uint8_t *respBufp;
  struct gecko_msg_gatt_server_send_user_write_response_rsp_t* gssuwrrsp = NULL;
  struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t* gsscnrsp = NULL;

  printf("IMU: CP write; %d : %02x:%02x:%02x:%02x\r\n",
         writeValue->len,
         writeValue->data[0],
         writeValue->data[1],
         writeValue->data[2],
         writeValue->data[3]
         );

  respBufp = respBuf;
  if (cpIndication) {
    gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(conGetConnectionId(),
                                                               gattdb_imu_control_point,
                                                               0);
    APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);

    UINT8_TO_BITSTREAM(respBufp, CP_OPCODE_RESPONSE);
    UINT8_TO_BITSTREAM(respBufp, writeValue->data[0]);

    switch (writeValue->data[0]) {
      case CP_OPCODE_CALIBRATE:
        imuDeviceCalibrate();
        break;

      case CP_OPCODE_ORIRESET:
        /* imuDeviceOrientationReset(); */
        UINT8_TO_BITSTREAM(respBufp, CP_RESP_SUCCESS);
        printf("ORIRESET!\r\n");

        gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(conGetConnectionId(),
                                                                          gattdb_imu_control_point,
                                                                          3,
                                                                          respBuf);
        APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);
        break;

      case CP_OPCODE_CALRESET:
        /* imuDeviceCalibrateReset(); */
        UINT8_TO_BITSTREAM(respBufp, CP_RESP_SUCCESS);

        printf("CALRESET!\r\n");
        gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(conGetConnectionId(),
                                                                          gattdb_imu_control_point,
                                                                          3,
                                                                          respBuf);
        APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);
        break;

      default:
        UINT8_TO_BITSTREAM(respBufp, CP_RESP_ERROR);
        gsscnrsp = gecko_cmd_gatt_server_send_characteristic_notification(conGetConnectionId(),
                                                                          gattdb_imu_control_point,
                                                                          3,
                                                                          respBuf);
        APP_ASSERT_DBG(!gsscnrsp->result, gsscnrsp->result);
        break;
    }
  } else {
    gssuwrrsp = gecko_cmd_gatt_server_send_user_write_response(conGetConnectionId(),
                                                               gattdb_imu_control_point,
                                                               ERR_CCCD_CONF);
    APP_ASSERT_DBG(!gssuwrrsp->result, gssuwrrsp->result);
  }

  return;
}

static void handleDeviceInitDeinit(void)
{
  if ( orientationNotification || accelerationNotification ) {
    if ( IMU_getState() == IMU_STATE_DISABLED ) {
      IMU_init();
      IMU_config(225.0f);
    }
  } else {
    if ( IMU_getState() == IMU_STATE_READY ) {
      IMU_deInit();
    }
  }
}
