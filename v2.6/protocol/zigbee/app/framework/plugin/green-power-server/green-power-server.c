/***************************************************************************//**
 * @file
 * @brief Routines for the Green Power Server plugin.
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

#include "enums.h"
#include "app/framework/include/af.h"
#include "app/framework/util/af-main.h"
#include "app/framework/util/common.h"
#include "app/framework/plugin/green-power-server/green-power-server.h"
#include "app/framework/plugin/green-power-common/green-power-common.h"
#include "stack/include/gp-types.h"
#include "stack/gp/gp-sink-table.h"
#include "stack/include/packet-buffer.h"

#define GP_NON_MANUFACTURER_ZCL_HEADER_LENGTH           (3)
#define isClusterInManufactureSpeceficRange(clusterId) ((0xFC00 <= clusterId) && (clusterId <= 0xFFFF))
#define isAttributeInManufactureSpecificRange(attributeId) ((0x5000 <= attributeId) && (attributeId <= 0xFFFF))

typedef struct {
  uint8_t gpdCommand;
  ZigbeeCluster cluster;
  uint8_t endpoints[MAX_ENDPOINT_COUNT];
  uint8_t numberOfEndpoints;
}SupportedGpdCommandClusterEndpointMap;

EmberEventControl emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl;
void emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventHandler(void);
EmberEventControl emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl;
void emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventHandler(void);
EmberEventControl emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventControl;
void emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventHandler(void);

const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_SIMPLE_SENSOR_SWITCH[]          = { 0x000F };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH[]           = { 0x0400 };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_OCCUPANCY_SENSOR_SWITCH[]       = { 0x0406 };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_TEMPERATURE_SENSOR_SWITCH[]     = { 0x0402 };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_PRESSURE_SENSOR_SWITCH[]        = { 0x0403 };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_FLOW_SENSOR_SWITCH[]            = { 0x0404 };
const uint16_t deviceIdCluster_EMBER_GP_DEVICE_ID_GPD_INDOOR_ENVIRONMENT_SENSOR[]     = { 0x0402, 0x0405, 0x0400, 0x040D };

#define DEVICEID_CLUSTER_LOOKUP(num)  { num, (sizeof(deviceIdCluster_##num) / sizeof(uint16_t)), deviceIdCluster_##num }
#define DEVICE_ID_MAP_CLUSTER_TABLE_SIZE (sizeof(gpdDeviceClusterMap) / sizeof(GpDeviceIdAndClusterMap))
const GpDeviceIdAndClusterMap gpdDeviceClusterMap[] = {
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_SIMPLE_SENSOR_SWITCH),             //4
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH),              //11
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_OCCUPANCY_SENSOR_SWITCH),          //12
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_TEMPERATURE_SENSOR_SWITCH),        //30
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_PRESSURE_SENSOR_SWITCH),           //31
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_FLOW_SENSOR_SWITCH),               //32
  DEVICEID_CLUSTER_LOOKUP(EMBER_GP_DEVICE_ID_GPD_INDOOR_ENVIRONMENT_SENSOR),        //33
};

// all define to map CmdID with DeviceID
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_SIMPLE_GENERIC_ONE_STATE_SWITCH[]   = { 2, 0x60, 0x61 };                                                                          //0
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_SIMPLE_GENERIC_TWO_STATE_SWITCH[]   = { 4, 0x62, 0x63, 0x64, 0x65 };                                                              //1
#ifdef EMBER_TEST
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_ON_OFF_SWITCH[]                     = { 8, 0x00, 0x20, 0x21, 0x22, 0x13, 0x1B, 0x40, 0x30 };                                     //2
#else
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_ON_OFF_SWITCH[]                     = { 3, 0x20, 0x21, 0x22 };                                                                   //2
#endif
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LEVEL_CONTROL_SWITCH[]              = { 9, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };                               //3
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_ADVANCED_GENERIC_ONE_STATE_SWITCH[] = { 3, 0x60, 0x61, 0x66 };                                                                   //5
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_ADVANCED_GENERIC_TWO_STATE_SWITCH[] = { 7, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68 };                                           //6
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_GENERIC_SWITCH[]                    = { 2, 0x69, 0x6A };                                                                         //7
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_COLOR_DIMMER_SWITCH[]               = { 12, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B };                     //10
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH[]               = { 2, 0xAF, 0xA6 }; // 0xAF for any command from 0xA0 to 0xA3 to compact TT       //11,12,30,31,32,33
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_DOOR_LOCK_CONTROLLER_SWITCH[]       = { 2, 0x50, 0x51 };                                                                         //20
const uint8_t deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_SCENCES[]                           = { 16, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F }; //unspecified

#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_TEMPERATURE_SENSOR_SWITCH       deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH
#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_PRESSURE_SENSOR_SWITCH          deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH
#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_FLOW_SENSOR_SWITCH              deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH
#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_INDOOR_ENVIRONMENT_SENSOR       deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH
#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_OCCUPANCY_SENSOR_SWITCH         deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH
#define deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_SIMPLE_SENSOR_SWITCH            deviceIdCmds_EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH

#define DEVICEID_CMDS_LOOKUP(num)  { num, deviceIdCmds_##num }
#define DEVICE_ID_MAP_TABLE_SIZE (sizeof(gpdDeviceCmdMap) / sizeof(GpDeviceIdAndCommandMap))
const GpDeviceIdAndCommandMap gpdDeviceCmdMap[] = {
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_SIMPLE_GENERIC_ONE_STATE_SWITCH),  //0
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_SIMPLE_GENERIC_TWO_STATE_SWITCH),  //1
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_ON_OFF_SWITCH),                    //2
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_LEVEL_CONTROL_SWITCH),             //3
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_SIMPLE_SENSOR_SWITCH),             //4
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_ADVANCED_GENERIC_ONE_STATE_SWITCH),//5
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_ADVANCED_GENERIC_TWO_STATE_SWITCH),//6
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_GENERIC_SWITCH),                   //7
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_COLOR_DIMMER_SWITCH),              //10
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_LIGHT_SENSOR_SWITCH),              //11
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_OCCUPANCY_SENSOR_SWITCH),          //12
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_DOOR_LOCK_CONTROLLER_SWITCH),      //20
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_TEMPERATURE_SENSOR_SWITCH),        //30
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_PRESSURE_SENSOR_SWITCH),           //31
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_FLOW_SENSOR_SWITCH),               //32
  DEVICEID_CMDS_LOOKUP(EMBER_GP_DEVICE_ID_GPD_INDOOR_ENVIRONMENT_SENSOR),        //33
};

EmberAfGreenPowerServerGpdSubTranslationTableEntry const* emGpDefaultTablePtr = NULL;
EmberAfGreenPowerServerDefautGenericSwTranslation const*  emGpDefaultGenericSwitchTablePtr = NULL;

#ifndef EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USER_HAS_DEFAULT_TRANSLATION_TABLE
// Following GPDF to Zcl Command only applies for following ApplicationID
// EMBER_GP_APPLICATION_SOURCE_ID(0b000) and EMBER_GP_APPLICATION_IEEE_ADDRESS (0b010)
// NOTE: all mapped ZCL commands have frame control of ZCL_FRAME_CONTROL_CLIENT_TO_SERVER.
const EmberAfGreenPowerServerGpdSubTranslationTableEntry emberGpDefaultTranslationTable[] =
{
  { true, EMBER_ZCL_GP_GPDF_IDENTIFY, 0xFF, HA_PROFILE_ID, ZCL_IDENTIFY_CLUSTER_ID, 1, ZCL_IDENTIFY_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x02, 0x00, 0x3C } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE0, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x00 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE1, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x01 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE2, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x02 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE3, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x03 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE4, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x04 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE5, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x05 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE6, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x06 } },
  { true, EMBER_ZCL_GP_GPDF_RECALL_SCENE7, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_RECALL_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x07 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE0, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x00 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE1, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x01 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE2, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x02 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE3, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x03 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE4, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x04 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE5, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x05 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE6, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x06 } },
  { true, EMBER_ZCL_GP_GPDF_STORE_SCENE7, 0xFF, HA_PROFILE_ID, ZCL_SCENES_CLUSTER_ID, 1, ZCL_STORE_SCENE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x03, 0xFF, 0xFF, 0x07 } },
  { true, EMBER_ZCL_GP_GPDF_OFF, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_ON, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_TOGGLE, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_UP, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_DOWN, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_STEP_UP, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STEP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_STEP_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_STEP_DOWN, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STEP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_STEP_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_LEVEL_CONTROL_STOP, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_UP_WITH_ON_OFF, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_DOWN_WITH_ON_OFF, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_STEP_UP_WITH_ON_OFF, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STEP_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_STEP_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_STEP_DOWN_WITH_ON_OFF, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STEP_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_STEP_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_HUE_STOP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_HUE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x02, EMBER_ZCL_HUE_MOVE_MODE_STOP, 0xFF } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_HUE_UP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_HUE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_HUE_MOVE_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_HUE_DOWN, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_HUE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_HUE_MOVE_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_STEP_HUE_UP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_HUE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_HUE_STEP_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_STEP_HUE_DOWN, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_HUE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_HUE_STEP_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_SATURATION_STOP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED, { 0x01, EMBER_ZCL_SATURATION_MOVE_MODE_STOP } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_SATURATION_UP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_SATURATION_MOVE_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_SATURATION_DOWN, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_SATURATION_MOVE_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_STEP_SATURATION_UP, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_SATURATION_STEP_MODE_UP } },
  { true, EMBER_ZCL_GP_GPDF_STEP_SATURATION_DOWN, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_SATURATION_STEP_MODE_DOWN } },
  { true, EMBER_ZCL_GP_GPDF_MOVE_COLOR, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_SATURATION_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_STEP_COLOR, 0xFF, HA_PROFILE_ID, ZCL_COLOR_CONTROL_CLUSTER_ID, 1, ZCL_STEP_COLOR_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_LOCK_DOOR, 0xFF, HA_PROFILE_ID, ZCL_DOOR_LOCK_CLUSTER_ID, 1, ZCL_LOCK_DOOR_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_UNLOCK_DOOR, 0xFF, HA_PROFILE_ID, ZCL_DOOR_LOCK_CLUSTER_ID, 1, ZCL_UNLOCK_DOOR_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },

  // User configurable as the command does not have a ZCL mapping
  { true, EMBER_ZCL_GP_GPDF_PRESS1_OF1, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                           // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_RELEASE1_OF1, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                        // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_SHORT_PRESS1_OF1, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                 // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_PRESS1_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                           // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_RELEASE1_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                        // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_PRESS2_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                           // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_RELEASE2_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                        // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_SHORT_PRESS1_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                 // GroupId/SceneId = 0
  { true, EMBER_ZCL_GP_GPDF_SHORT_PRESS2_OF2, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },                 // GroupId/SceneId = 0

  // Gereral ZCL command mapping - for which the cluster should be present in
  // gpd commissioning session to validate if it can be supported by sink
  { true, EMBER_ZCL_GP_GPDF_UNLOCK_DOOR, 0xFF, HA_PROFILE_ID, ZCL_DOOR_LOCK_CLUSTER_ID, 1, ZCL_UNLOCK_DOOR_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } },
  { true, EMBER_ZCL_GP_GPDF_REQUEST_ATTRIBUTE, 0xFF, HA_PROFILE_ID, 0xFFFF, 1, ZCL_READ_ATTRIBUTES_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0xFF } },
  { true, EMBER_ZCL_GP_GPDF_READ_ATTR_RESPONSE, 0xFF, HA_PROFILE_ID, 0xFFFF, 0, ZCL_READ_ATTRIBUTES_RESPONSE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0xFF } },
  { true, EMBER_ZCL_GP_GPDF_ZCL_TUNNELING_WITH_PAYLOAD, 0xFF, HA_PROFILE_ID, 0xFFFF, 0, 0xFF, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0xFF } },
  { true, EMBER_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD, 0xFF, HA_PROFILE_ID, 0xFFFF, 0, ZCL_REPORT_ATTRIBUTES_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0xFF } },
  // for compact attribut report, if ZbEndpoint=0xFC then ZbClu + ZbCmd + ZbPayload have No Meaning
  { true, EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING, 0xFF, HA_PROFILE_ID, 0xFFFF, 0, ZCL_REPORT_ATTRIBUTES_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0xFE } },
};

const EmberAfGreenPowerServerGpdSubTranslationTableEntry * getSystemOrUserDefinedDefaultTable(void)
{
  return emberGpDefaultTranslationTable;
}
#define SIZE_OF_DEFAULT_SUB_TRANSLATION_TABLE (sizeof(emberGpDefaultTranslationTable) / sizeof(EmberAfGreenPowerServerGpdSubTranslationTableEntry))
#endif

#ifndef EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USER_HAS_DEFAULT_GENERIC_SWITCH_TRANSLATION_TABLE
EmberAfGreenPowerServerDefautGenericSwTranslation emberGpSwitchTranslationTable[] = {
  // default table for switch configuration :
  // switchType, nbOfIdentifiedContacts, nbOfTTEntriesNeeded, indicativeBitmask, gpdCommand, endpoint, zigbeeProfile, zigbeeCluster, zigbeeCommandId, payloadSrc, zclPayloadDefault
  // 1bp --> Toggle
  // b3, b0, b1, b0 TOGGLE
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 1, 1, 0x0F }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 2bp --> on off   b2, b0 OFF /  b3, b1 ON
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 2, 2, 0x0A }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 2, 2, 0x05 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 3bp --> dim+/stop dim-/stop toggle   b0 UP /b1 DOWN /b3 TOGGLE
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 3, 3, 0x08 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_TOGGLE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 3, 3, 0x02 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_DOWN } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 3, 3, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_UP } } },
  // b0, b1 stop
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 3, 1, 0x03 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 4bp --> on/off dim+/stop dim-/stop
  // b0 UP /b1 DOWN/ b2 OFF/b3 ON
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 4, 4, 0x08 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 4, 4, 0x04 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 4, 4, 0x02 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_DOWN } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 4, 4, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_UP } } },
  // b0, b1 STOP
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 4, 1, 0x03 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 5bp to 8bp --> pass to application
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 5, 1, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 5, 1, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 6, 1, 0x20 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 6, 1, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 8, 1, 0x80 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 8, 1, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 7, 1, 0x40 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_BUTTON_SWITCH_TYPE, 7, 1, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 1 rocker --> on/off
  //  b2, b0 OFF /  b3, b1 ON
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 2, 0x0A }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 2, 0x05 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 2 rocker --> on/off dim+/stop dim-/stop
  // b0 UP /b1 DOWN/ b2 OFF/b3 ON
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 4, 0x08 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 4, 0x04 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 4, 0x02 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_DOWN } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 4, 0x01 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_MOVE_WITH_ON_OFF_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED | EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD, { 0x01, EMBER_ZCL_MOVE_MODE_UP } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 2, 1, 0x03 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFF, HA_PROFILE_ID, ZCL_LEVEL_CONTROL_CLUSTER_ID, 1, ZCL_STOP_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  // 3 to 4 rocker --> pass to application
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 3, 1, 0x20 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 3, 1, 0x20 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },

  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 4, 1, 0x80 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
  { { EMBER_ZCL_GP_ROCKER_SWITCH_TYPE, 4, 1, 0x80 }, { true, EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE, 0xFC, HA_PROFILE_ID, ZCL_ON_OFF_CLUSTER_ID, 1, ZCL_ON_COMMAND_ID, EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA, { 0 } } },
};

const EmberAfGreenPowerServerDefautGenericSwTranslation * getSystemOrUserDefinedDefaultGenericSwitchTable(void)
{
  return emberGpSwitchTranslationTable;
}
#define SIZE_OF_SWITCH_TRANSLATION_TABLE (sizeof(emberGpSwitchTranslationTable) / sizeof(EmberAfGreenPowerServerDefautGenericSwTranslation))
#endif
static void pairingDoneThusSetCustomizedTranslationTable(EmberGpAddress * gpdAddr,
                                                         uint8_t gpdCommandId,
                                                         uint8_t endpoint);
static uint8_t emGpIsToBePairedGpdMatchSinkFunctionality(EmberGpApplicationInfo applicationInfo,
                                                         SupportedGpdCommandClusterEndpointMap *gpdCommandClusterEpMap);
static uint8_t findMatchingGenericTranslationTableEntry(uint8_t entryType,
                                                        uint8_t incomingReqType,
                                                        uint8_t offset,
                                                        bool infoBlockPresent,
                                                        uint8_t gpdCommandId,
                                                        uint16_t zigbeeProfile,
                                                        uint16_t zigbeeCluster,
                                                        uint8_t  zigbeeCommandId,
                                                        uint8_t payloadLength,
                                                        uint8_t* payload,
                                                        uint8_t* outIndex);
static uint8_t emGpTransTableFindMatchingTranslationTableEntry(uint8_t levelOfScan,
                                                               bool infoBlockPresent,
                                                               EmberGpAddress  * gpAddr,
                                                               uint8_t gpdCommandId,
                                                               uint8_t zbEndpoint,
                                                               uint8_t * gpdCmdPayload,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock,
                                                               uint8_t *outIndex,
                                                               uint8_t startIndex);
static uint8_t emGpTransTableDeleteTranslationTableEntryByIndex(uint8_t index,
                                                                bool infoBlockPresent,
                                                                uint8_t gpdCommandId,
                                                                uint16_t zigbeeProfile,
                                                                uint16_t zigbeeCluster,
                                                                uint8_t  zigbeeCommandId,
                                                                uint8_t payloadLength,
                                                                uint8_t* payload,
                                                                uint8_t additionalInfoLength,
                                                                EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock);
static uint8_t emGpTransTableRemoveTranslationTableEntryUpdateCommand(uint8_t index,
                                                                      bool infoBlockPresent,
                                                                      EmberGpAddress * gpdAddr,
                                                                      uint8_t gpdCommandId,
                                                                      uint8_t zbEndpoint,
                                                                      uint16_t zigbeeProfile,
                                                                      uint16_t zigbeeCluster,
                                                                      uint8_t  zigbeeCommandId,
                                                                      uint8_t payloadLength,
                                                                      uint8_t* payload,
                                                                      uint8_t payloadSrc,
                                                                      uint8_t additionalInfoLength,
                                                                      EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock);
static uint8_t emGpTransTableAddPairedDeviceToTranslationTable(uint8_t incomingReqType,
                                                               bool infoBlockPresent,
                                                               EmberGpAddress  * gpdAddr,
                                                               uint8_t gpdCommandId,
                                                               uint8_t zbEndpoint,
                                                               uint16_t zigbeeProfile,
                                                               uint16_t zigbeeCluster,
                                                               uint8_t  zigbeeCommandId,
                                                               uint8_t payloadLength,
                                                               uint8_t* payload,
                                                               uint8_t payloadSrc,
                                                               uint8_t additionalInfoLength,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock,
                                                               uint8_t* outNewTTEntryIndex);
static uint8_t emGpTransTableDeletePairedDevicefromTranslationTableEntry(EmberGpAddress * gpdAddr);
static uint8_t commissionedEndPoints[MAX_ENDPOINT_COUNT] = { 0 };
static uint8_t noOfCommissionedEndpoints = 0;
static EmberAfGreenPowerServerCommissioningState commissioningState = { 0 };
static GpCommDataSaved gpdCommDataSaved = { 0 };
static EmGpCommandTranslationTable emGpTranslationTable = { 0 };
static EmberAfGreenPowerServerGpdSubTranslationTableEntry customizedTranslationTable[EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE] =  { 0 };
static EmberGpTranslationTableAdditionalInfoBlockField emGpAdditionalInfoTable = { 0 };
// Writes the TCInvolved bit - this should only be called on first joining of the node.
static void writeInvolveTCBit(void)
{
  // Proceed with processing the involveTC
  uint8_t gpsSecurityLevelAttribut = 0;
  uint8_t type;
  EmberAfStatus secLevelStatus = emberAfReadAttribute(GP_ENDPOINT,
                                                      ZCL_GREEN_POWER_CLUSTER_ID,
                                                      ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                                      (CLUSTER_MASK_SERVER),
                                                      (uint8_t*)&gpsSecurityLevelAttribut,
                                                      sizeof(uint8_t),
                                                      &type);
  if (secLevelStatus != EMBER_ZCL_STATUS_SUCCESS) {
    return; //  No security Level attribute ? Don't proceed
  }
  emberAfGreenPowerClusterPrintln("");
  emberAfGreenPowerClusterPrint("GPS in ");
  // Find the security state of the node to examine the network security.
  EmberCurrentSecurityState securityState;
  // Distributed network - return with out any change to InvolveTc bit
  if (emberGetCurrentSecurityState(&securityState) == EMBER_SUCCESS
      && (securityState.bitmask & EMBER_DISTRIBUTED_TRUST_CENTER_MODE)) {
    emberAfGreenPowerClusterPrint("Distributed Network");
    // reset the InvolveTC bit
    gpsSecurityLevelAttribut &= ~(GREEN_POWER_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_FIELD_INVOLVE_TC);
  } else {
    // If centralised - checkif default TC link key is used
    const EmberKeyData linkKey = { GP_DEFAULT_LINK_KEY };
    EmberKeyStruct keyStruct;
    EmberStatus keyReadStatus = emberGetKey(EMBER_TRUST_CENTER_LINK_KEY, &keyStruct);
    if (keyReadStatus == EMBER_SUCCESS
        && MEMCOMPARE(keyStruct.key.contents, linkKey.contents, EMBER_ENCRYPTION_KEY_SIZE)) {
      emberAfGreenPowerClusterPrint("Centralised Network : Non Default TC Key used - Set InvoveTc bit");
      // Set the InvolveTC bit
      gpsSecurityLevelAttribut |= GREEN_POWER_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_FIELD_INVOLVE_TC;
    }
  }
  emberAfGreenPowerClusterPrintln("");
  secLevelStatus = emberAfWriteAttribute(GP_ENDPOINT,
                                         ZCL_GREEN_POWER_CLUSTER_ID,
                                         ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                         (CLUSTER_MASK_SERVER),
                                         (uint8_t*)&gpsSecurityLevelAttribut,
                                         type);
  emberAfGreenPowerClusterPrintln("Security Level writen = %d, Status = %d",
                                  gpsSecurityLevelAttribut,
                                  secLevelStatus);
}
// Update the GPS Node state in non volatile token
static void clearGpsStateInToken(void)
{
  #if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  GpsNetworkState gpsNodeState = GREEN_POWER_SERVER_GPS_NODE_STATE_NOT_IN_NETWORK;
  halCommonSetToken(TOKEN_GPS_NETWORK_STATE, &gpsNodeState);
  #endif
}
// Checks if the node is just joined checking from the tokens
static bool updateInvolveTCNeeded(void)
{
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  GpsNetworkState gpsNodeState;
  halCommonGetToken(&gpsNodeState, TOKEN_GPS_NETWORK_STATE);
  if (gpsNodeState == GREEN_POWER_SERVER_GPS_NODE_STATE_IN_NETWORK) {
    return false;
  } else {
    gpsNodeState = GREEN_POWER_SERVER_GPS_NODE_STATE_IN_NETWORK;
    halCommonSetToken(TOKEN_GPS_NETWORK_STATE, &gpsNodeState);
  }
  return true;
#else
  return false;
#endif
}
// Process the update based on statck state
static void updateInvolveTC(EmberStatus status)
{
  // Pre process network status to decide if a InvolveTC is needed
  if (status == EMBER_NETWORK_DOWN
      && emberStackIsPerformingRejoin() == FALSE) {
    clearGpsStateInToken();
    return;
  } else if (status == EMBER_NETWORK_UP) {
    if (!updateInvolveTCNeeded()) {
      return;
    }
    writeInvolveTCBit();
  }
}
// Get the default table entry
const EmberAfGreenPowerServerGpdSubTranslationTableEntry* emGpGetDefaultTable(void)
{
  return emGpDefaultTablePtr;
}

const EmberAfGreenPowerServerDefautGenericSwTranslation* emGpGetDefaultGenericSwicthTable(void)
{
  return emGpDefaultGenericSwitchTablePtr;
}

// Get the entire translation table
EmGpCommandTranslationTable* emGpTransTableGetTranslationTable(void)
{
  return &emGpTranslationTable;
}

// Clears the entire translation table
void emGpTransTableClearTranslationTable(void)
{
  MEMSET(&emGpTranslationTable, 0x00, sizeof(EmGpCommandTranslationTable));
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; i++) {
    emGpTranslationTable.TableEntry[i].offset = 0xFF;
    emGpTranslationTable.TableEntry[i].additionalInfoOffset = 0xFF;
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
    halCommonSetIndexedToken(TOKEN_TRANSLATION_TABLE, i, &emGpTranslationTable.TableEntry[i]);
#endif
  }
  emGpTranslationTable.totalNoOfEntries = 0;
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  halCommonSetToken(TOKEN_TRANSLATION_TABLE_TOTAL_ENTRIES, &emGpTranslationTable.totalNoOfEntries);
#endif
}
// Set the translation table entry at index
void emGpSetTranslationTableEntry(uint8_t index)
{
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  if (index < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
    halCommonSetToken(TOKEN_TRANSLATION_TABLE_TOTAL_ENTRIES, &emGpTranslationTable.totalNoOfEntries);
    halCommonSetIndexedToken(TOKEN_TRANSLATION_TABLE, index, &emGpTranslationTable.TableEntry[index]);
  }
#endif
}
// Get the entire customized table
EmberAfGreenPowerServerGpdSubTranslationTableEntry* emGpGetCustomizedTable(void)
{
  if (EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE) {
    return &customizedTranslationTable[0];
  } else {
    return NULL;
  }
}
// Clears the entire customized table
void emGpClearCustomizedTable(void)
{
  MEMSET(customizedTranslationTable,
         0x00,
         sizeof(EmberAfGreenPowerServerGpdSubTranslationTableEntry)
         * EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE);
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE; i++) {
    halCommonSetIndexedToken(TOKEN_CUSTOMIZED_TABLE, i, &customizedTranslationTable[i]);
  }
#endif
}
// Set the custmized table entry at index
void emGpSetCustomizedTableEntry(uint8_t index)
{
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  if (index < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE) {
    halCommonSetIndexedToken(TOKEN_CUSTOMIZED_TABLE, index, &customizedTranslationTable[index]);
  }
#endif
}
// Get the entire  additional info block table
EmberGpTranslationTableAdditionalInfoBlockField * emGpGetAdditionalInfoTable(void)
{
  return &emGpAdditionalInfoTable;
}
// Clears the entire additional info block table
void embGpClearAdditionalInfoBlockTable(void)
{
  MEMSET(&emGpAdditionalInfoTable, 0x00, sizeof(EmberGpTranslationTableAdditionalInfoBlockField));
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE; i++) {
    halCommonSetIndexedToken(TOKEN_ADDITIONALINFO_TABLE, i, &(emGpAdditionalInfoTable.additionalInfoBlock[i]));
    halCommonSetIndexedToken(TOKEN_ADDITIONALINFO_TABLE_VALID_ENTRIES, i, &(emGpAdditionalInfoTable.validEntry[i]));
  }
  halCommonSetToken(TOKEN_ADDITIONALINFO_TABLE_TOTAL_ENTRIES, &(emGpAdditionalInfoTable.totlaNoOfEntries));
#endif
}
// Set the additional info block entry at index
void emGpSetAdditionalInfoBlockTableEntry(uint8_t index)
{
#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  if (index < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE) {
    halCommonSetIndexedToken(TOKEN_ADDITIONALINFO_TABLE, index, &(emGpAdditionalInfoTable.additionalInfoBlock[index]));
    halCommonSetIndexedToken(TOKEN_ADDITIONALINFO_TABLE_VALID_ENTRIES, index, &(emGpAdditionalInfoTable.validEntry[index]));
    halCommonSetToken(TOKEN_ADDITIONALINFO_TABLE_TOTAL_ENTRIES, &(emGpAdditionalInfoTable.totlaNoOfEntries));
  }
#endif
}

void emberAfPluginTranslationTableInitCallback(void)
{
  // Get user/system defined default table
  emGpDefaultTablePtr = getSystemOrUserDefinedDefaultTable();
  emGpDefaultGenericSwitchTablePtr = getSystemOrUserDefinedDefaultGenericSwitchTable();

#if defined(EMBER_AF_PLUGIN_GREEN_POWER_SERVER_USE_TOKENS) && !defined(EZSP_HOST)
  // On device initialization, Read Translation Table / Customized Table
  // and the Aditional Info Block Table from the persistent memory.

  //emGpGetCustomizedTable();
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE; i++) {
    halCommonGetIndexedToken(&customizedTranslationTable[i], TOKEN_CUSTOMIZED_TABLE, i);
  }

  //emGpTransTableGetTranslationTable();
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; i++) {
    halCommonGetIndexedToken(&emGpTranslationTable.TableEntry[i], TOKEN_TRANSLATION_TABLE, i);
  }
  halCommonGetToken(&emGpTranslationTable.totalNoOfEntries, TOKEN_TRANSLATION_TABLE_TOTAL_ENTRIES);

  //emGpGetAdditionalInfoTable();
  for (int i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE; i++) {
    halCommonGetIndexedToken(&emGpAdditionalInfoTable.additionalInfoBlock[i], TOKEN_ADDITIONALINFO_TABLE, i);
    halCommonGetIndexedToken(&emGpAdditionalInfoTable.validEntry[i], TOKEN_ADDITIONALINFO_TABLE_VALID_ENTRIES, i);
  }
  halCommonGetToken(&emGpAdditionalInfoTable.totlaNoOfEntries, TOKEN_ADDITIONALINFO_TABLE_TOTAL_ENTRIES);
#endif
}

// Send device announcement
static void sendDeviceAnncement(uint16_t nodeId)
{
  EmberApsFrame apsFrameDevAnnce;
  apsFrameDevAnnce.sourceEndpoint = EMBER_ZDO_ENDPOINT;
  apsFrameDevAnnce.destinationEndpoint = EMBER_ZDO_ENDPOINT;
  apsFrameDevAnnce.clusterId = END_DEVICE_ANNOUNCE;
  apsFrameDevAnnce.profileId = EMBER_ZDO_PROFILE_ID;
  apsFrameDevAnnce.options = EMBER_APS_OPTION_SOURCE_EUI64;
  apsFrameDevAnnce.options |= EMBER_APS_OPTION_USE_ALIAS_SEQUENCE_NUMBER;
  apsFrameDevAnnce.groupId = 0;
  uint8_t messageContents[GP_DEVICE_ANNOUNCE_SIZE];
  uint8_t apsSequence = 0;
  // Form the APS message for Bcast
  messageContents[0] = apsSequence; //Sequence
  messageContents[1] = (uint8_t)nodeId; //NodeId
  messageContents[2] = (uint8_t)(nodeId >> 8); //NodeId
  MEMSET(&messageContents[3], 0xFF, 8); //IEEE Address
  messageContents[11] = 0; // Capability
  uint8_t length = GP_DEVICE_ANNOUNCE_SIZE;
#ifndef EZSP_HOST
  EmberMessageBuffer message = emberFillLinkedBuffers(messageContents, length);
  if (message == EMBER_NULL_MESSAGE_BUFFER) {
    return;
  }
  emberProxyBroadcast(nodeId, //EmberNodeId source,
                      0xFFFD, //EmberNodeId destination,
                      0, //uint8_t nwkSequence,
                      &apsFrameDevAnnce, //EmberApsFrame *apsFrame,
                      0xFF,     // use maximum radius
                      message);
  emReleaseMessageBuffer(message);
#else
  ezspProxyBroadcast(nodeId,//EmberNodeId source,
                     0xFFFD,//EmberNodeId destination,
                     0,//uint8_t nwkSequence,
                     &apsFrameDevAnnce,//EmberApsFrame *apsFrame,
                     0xFF,//uint8_t radius,
                     0xFF,// Tag Id
                     length,//uint8_t messageLength,
                     messageContents,//uint8_t *messageContents,
                     &apsSequence);
#endif
}

// Internal functions used to maintain the group table within the context
// of the binding table.
//
// In the binding:
// The first two bytes of the identifier is set to the groupId
// The local endpoint is set to the endpoint that is mapped to this group
static uint8_t findGroupInBindingTable(uint8_t endpoint, uint16_t groupId)
{
  for (uint8_t i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
    EmberBindingTableEntry binding;
    if (emberGetBinding(i, &binding) == EMBER_SUCCESS
        && binding.type == EMBER_MULTICAST_BINDING
        && binding.identifier[0] == LOW_BYTE(groupId)
        && binding.identifier[1] == HIGH_BYTE(groupId)
        && binding.local == endpoint) {
      return i;
    }
  }
  return 0xFF;
}

static EmberAfStatus addToApsGroup(uint8_t endpoint, uint16_t groupId)
{
  if (0xFF != findGroupInBindingTable(endpoint, groupId)) {
    // search returned an valid index, so duplicate exists
    return EMBER_ZCL_STATUS_DUPLICATE_EXISTS;
  }
  // No duplicate entry, try adding
  // Look for an empty binding slot.
  for (uint8_t i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
    EmberBindingTableEntry binding;
    if (emberGetBinding(i, &binding) == EMBER_SUCCESS
        && binding.type == EMBER_UNUSED_BINDING) {
      binding.type = EMBER_MULTICAST_BINDING;
      binding.identifier[0] = LOW_BYTE(groupId);
      binding.identifier[1] = HIGH_BYTE(groupId);
      binding.local = endpoint;

      EmberStatus status = emberSetBinding(i, &binding);
      if (status == EMBER_SUCCESS) {
        return EMBER_ZCL_STATUS_SUCCESS;
      } else {
        emberAfCorePrintln("ERR: Failed to create binding (0x%x)", status);
        return EMBER_ZCL_STATUS_FAILURE;
      }
    }
  }
  emberAfCorePrintln("ERR: Binding table is full");
  return EMBER_ZCL_STATUS_INSUFFICIENT_SPACE;
}

static EmberAfStatus removeFromApsGroup(uint8_t endpoint, uint16_t groupId)
{
  uint8_t index = findGroupInBindingTable(endpoint, groupId);
  if (index == 0xFF) {
    return EMBER_ZCL_STATUS_NOT_FOUND;
  }
  // search returned a valid index, so delete that
  EmberStatus status = emberDeleteBinding(index);
  if (status == EMBER_SUCCESS) {
    return EMBER_ZCL_STATUS_SUCCESS;
  }
  return EMBER_ZCL_STATUS_FAILURE;
}
// Following group of function uses public sink table apis and derive the required
// functionality
static  void gpGpSinkTableSetSecurityFrameCounter(uint8_t index,
                                                  EmberGpSecurityFrameCounter securityFrameCounter)
{
  EmberGpSinkTableEntry entry = { 0 };
  EmberStatus status = emberGpSinkTableGetEntry(index, &entry);
  if (status == EMBER_SUCCESS) {
    entry.gpdSecurityFrameCounter = securityFrameCounter;
    emberGpSinkTableSetEntry(index, &entry);
  }
}

static uint8_t gpGpSinkTableGetDeviceId(uint8_t index)
{
  EmberGpSinkTableEntry entry = { 0 };
  EmberStatus status = emberGpSinkTableGetEntry(index, &entry);
  if (status == EMBER_SUCCESS) {
    return entry.deviceId;
  }
  return 0; // Error
}

// End of gp sink table information access functions.

// Finds and returns the Gp Controlable application endpoint in the APS group
static uint16_t findAppEndpointGroupId(uint8_t endpoint)
{
  for (uint8_t i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
    EmberBindingTableEntry binding;
    if (emberGetBinding(i, &binding) == EMBER_SUCCESS
        && binding.type == EMBER_MULTICAST_BINDING
        && binding.local == endpoint) {
      uint16_t groupId = (binding.identifier[1] << 8) | binding.identifier[0];
      return groupId;
    }
  }
  return 0;
}
static bool isValidAppEndpoint(uint8_t endpoint)
{
  if (endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS
      || endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS) {
    return true;
  }
  for (uint8_t i = 0; i < FIXED_ENDPOINT_COUNT; i++) {
    if (emAfEndpoints[i].endpoint == endpoint) {
      return true;
    }
  }
  return false;
}

static bool isCommissioningAppEndpoint(uint8_t endpoint)
{
  // Gp Controlable end points are all valid enabled endpoints
  if (endpoint >= GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT
      && endpoint <= GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT
      && (endpoint == commissioningState.endpoint
          || commissioningState.endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS      // All Endpoints - Sink derives
          || commissioningState.endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS  // Sink Derives the end points
          || commissioningState.endpoint == GREEN_POWER_SERVER_RESERVED_ENDPOINTS      // No Endpoint specified
          || commissioningState.endpoint == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS)) { // No endpoint specified
    return true;
  }
  emberAfGreenPowerClusterPrintln("Endpoint %d Not in Commissioning Endpoint", endpoint);
  return false;
}

static uint8_t getGpCommissioningEndpoint(uint8_t * endpoints)
{
  uint8_t numberOfAppEndpoints = 0;
  if (commissioningState.endpoint == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS         // All Endpoints - Sink derives
      || commissioningState.endpoint == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS  // Sink Derives the end points
      || commissioningState.endpoint == GREEN_POWER_SERVER_RESERVED_ENDPOINTS      // No Endpoint specified
      || commissioningState.endpoint == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS) {  // No endpoint specified
    for (uint8_t i = 0; i < FIXED_ENDPOINT_COUNT; i++) {
      // Check only valid application endpoint ranges does not include GP EP
      if (emAfEndpoints[i].endpoint >= GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT
          && emAfEndpoints[i].endpoint <= GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT) {
        endpoints[numberOfAppEndpoints++] = emAfEndpoints[i].endpoint;
      }
    }
  } else {
    // else just one endpoint in commissioning
    endpoints[0] = commissioningState.endpoint;
    numberOfAppEndpoints = 1;
  }
  return numberOfAppEndpoints;
}

static uint8_t getAllSinkEndpoints(uint8_t * endpoints)
{
  return getGpCommissioningEndpoint(endpoints);
}

static void getGroupListBasedonAppEp(uint8_t * list)
{
  uint8_t * count = list;
  uint8_t * grouplist = list + 1;
  *count = 0;
  for (uint8_t i = 0; i < FIXED_ENDPOINT_COUNT; i++) {
    if (isCommissioningAppEndpoint(emAfEndpoints[i].endpoint)) {
      uint16_t groupId = findAppEndpointGroupId(emAfEndpoints[i].endpoint);
      if (0 != groupId) {
        (*count)++;
        *grouplist = (uint8_t)groupId;
        grouplist++;
        *grouplist = (uint8_t)(groupId >> 8);
        grouplist++;
        grouplist++; // alias bytes
        grouplist++; // allias
      }
    }
  }
}

static void resetOfMultisensorDataSaved(bool completeReset)
{
  // the reset could be complete or partial - back up the switch information
  EmberGpSwitchInformation tempSwitchInformationStruct = gpdCommDataSaved.switchInformationStruct;
  if (completeReset) {
    // reset all data belonging to this Gpd

    MEMSET(&gpdCommDataSaved, 0, sizeof(GpCommDataSaved));
    // if the switch timer was active, just copy it back
    if (emberEventControlGetActive(emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl)) {
      gpdCommDataSaved.switchInformationStruct = tempSwitchInformationStruct;
    }
    return;
  }
  // minimal reset (partial part):
  MEMSET(gpdCommDataSaved.reportsStorage, 0, SIZE_OF_REPORT_STORAGE);
  gpdCommDataSaved.lastIndex = 0;
  gpdCommDataSaved.numberOfReports = 0;
  gpdCommDataSaved.totalNbOfReport = 0;
}

static bool sinkFunctionalitySupported(uint32_t mask)
{
  uint32_t gpsFuntionnalityAttribut = 0;
  uint32_t gpsActiveFunctionnalityAttribut = 0;
  EmberAfAttributeType type;
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GPS_FUNCTIONALITY_ATTRIBUTE_ID,
                       (CLUSTER_MASK_SERVER),
                       (uint8_t*)&gpsFuntionnalityAttribut,
                       sizeof(uint32_t),
                       &type);
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GPS_ACTIVE_FUNCTIONALITY_ATTRIBUTE_ID,
                       (CLUSTER_MASK_SERVER),
                       (uint8_t*)&gpsActiveFunctionnalityAttribut,
                       sizeof(uint32_t),
                       &type);
  if (mask & (gpsFuntionnalityAttribut & gpsActiveFunctionnalityAttribut)) {
    return true;
  }
  return false;
}

static void fillExternalBufferGpdCommandA0A1Payload(uint8_t gpdCommandId,
                                                    uint8_t * gpdCommandPayload,
                                                    uint8_t zigbeeCommandId,
                                                    bool direction)
{
  // total payload length is given in gpdCommandPayload[0]
  uint8_t * payload = &gpdCommandPayload[1];
  uint8_t frameControl = (ZCL_GLOBAL_COMMAND                                      \
                          | (direction ? ZCL_FRAME_CONTROL_CLIENT_TO_SERVER       \
                             : ZCL_FRAME_CONTROL_SERVER_TO_CLIENT)                \
                          | ((gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG) \
                             ? ZCL_MANUFACTURER_SPECIFIC_MASK : 0));

  uint16_t manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  if (frameControl & ZCL_MANUFACTURER_SPECIFIC_MASK) {
    manufacturerCode = ((*(payload + 1)) << 8) + (*payload);
    payload += 2;
  }
  EmberAfClusterId clusterId = ((*(payload + 1)) << 8) + (*payload);
  payload += 2;
  uint8_t length = (payload - (&(gpdCommandPayload[1])));
  emberAfFillExternalManufacturerSpecificBuffer(frameControl,
                                                clusterId,
                                                manufacturerCode,
                                                zigbeeCommandId,
                                                "");
  // Remaining payload to copy = gpdCommandPayload[0] - what ever populated
  emberAfAppendToExternalBuffer(payload, (gpdCommandPayload[0] - length));
}

static EmberStatus handleA2A3MultiClusterReportForwarding(uint8_t gpdCommandId,
                                                          uint8_t * gpdCommandPayload,
                                                          uint8_t zigbeeCommandId,
                                                          bool direction,
                                                          uint8_t zbEndpoint)
{
  // To handle the Multi Cluster Attribute reporting, as there is no equivalent ZCL command
  // it needed to be broken down to individual packets asA0/A1 and sent.
  // There are two approaches , break down and send in a loop here OR store it and
  // send it with help of a timer.
  uint8_t * payloadFinger = &gpdCommandPayload[1];
  uint8_t clusterReport[30];
  uint8_t startIndex = 1;
  EmberStatus status;

  if (gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG) {
    MEMCOPY(&clusterReport[startIndex], &gpdCommandPayload[1], 2); // ManufactureId copied
    payloadFinger += 2; // source pointer incremented
    startIndex += 2;     // Destination pointer incremented
  }
  // Copy cluster Report feilds for each cluster in incoming payload
  do {
    uint8_t destIndex = startIndex;
    MEMCOPY(&clusterReport[destIndex], payloadFinger, 4); // ClusterId + Attribute Id copied
    payloadFinger += 4; // source pointer incremented for copied cluster and attribute Id
    destIndex += 4;     // Dest pointer Incremented for copied cluster and attribute Id
    clusterReport[destIndex] = *payloadFinger; //AttributeDataType copied
    uint8_t dataSize = emberAfGetDataSize(*payloadFinger); // get the dataSize with the dataType
    payloadFinger += 1; // source pointer incremented for dataType
    destIndex += 1;     // Dest pointer Incremented for dataType
    MEMCOPY(&clusterReport[destIndex], payloadFinger, dataSize); // Attribuetdata copied
    payloadFinger += dataSize; // source pointer incremented for copied data
    destIndex += dataSize;     // Dest pointer incremented for copied data
    clusterReport[0] = (destIndex - 1);  // finally copy the length in index 0;

    fillExternalBufferGpdCommandA0A1Payload((gpdCommandId - 2), // A0/A1 is exact replica of A2/A3 repectively
                                            clusterReport,
                                            zigbeeCommandId,
                                            direction);
    emberAfSetCommandEndpoints(zbEndpoint,
                               zbEndpoint);
    status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, emberAfGetNodeId());
    if (status != EMBER_SUCCESS) {
      return status;
    }
  } while ((payloadFinger - (&gpdCommandPayload[1])) < gpdCommandPayload[0]);
  return status;
}

static void fillExternalBufferGpdCommandA6Payload(uint8_t * gpdCommandPayload)
{
  // A6 - GPD ZCL Tunneling gpdCommandPayload format :
  //  -------------------------------------------------------------------------------------------------------------
  // | Total Length(1) | Option(1) | Manf Id (0/2) | ClusterId(2) | ZCommand(1) | Length(1) | Payload(0/Variable) |
  //  -------------------------------------------------------------------------------------------------------------
  // total payload length is given in gpdCommandPayload[0]
  uint8_t frameControl = gpdCommandPayload[1];
  uint8_t * payload = &gpdCommandPayload[2];

  uint16_t manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  if (frameControl & ZCL_MANUFACTURER_SPECIFIC_MASK) {
    manufacturerCode = ((*(payload + 1)) << 8) + (*payload);
    payload += 2;
  }
  EmberAfClusterId clusterId = ((*(payload + 1)) << 8) + (*payload);
  payload += 2;
  uint8_t zigbeeCommand = *payload;
  payload += 1;
  emberAfFillExternalManufacturerSpecificBuffer(frameControl,
                                                clusterId,
                                                manufacturerCode,
                                                zigbeeCommand,
                                                "");
  uint8_t length = *payload;
  payload += 1;
  emberAfAppendToExternalBuffer(payload, length);
}

static void fillExternalBufferGpdCommandScenesPayload(EmberGpAddress *gpdAddr,
                                                      uint8_t gpdCommandId,
                                                      EmberAfGreenPowerServerGpdSubTranslationTableEntry const * genericTranslationTable,
                                                      uint8_t endpoint)
{
  //If the sink implements the Translation Table, it derives the groupId as alias
  uint16_t groupId = ((uint16_t)genericTranslationTable->zclPayloadDefault[2] << 8) + genericTranslationTable->zclPayloadDefault[1];
  if (groupId == 0xFFFF) {
    groupId = emGpdAlias(gpdAddr);
  }
  // General command filling.
  emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND             \
                             | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER),   \
                            genericTranslationTable->zigbeeCluster,   \
                            genericTranslationTable->zigbeeCommandId, \
                            "");
  emberAfAppendToExternalBuffer((uint8_t *)(&groupId), sizeof(groupId));
  emberAfAppendToExternalBuffer((uint8_t *)(&genericTranslationTable->zclPayloadDefault[3]), sizeof(uint8_t));

  if (gpdCommandId & EMBER_ZCL_GP_GPDF_STORE_SCENE0) {
    // Store Scenes commands from 0x18 to 0x1F
    // Add the endpoint to the group in the store
    EmberAfStatus status = addToApsGroup(endpoint, groupId);
    emberAfCorePrintln("Store Scene : Added endpoint %d to Group %d Status = %d", endpoint, groupId, status);
  } else {
    // Recall Scenes commands from 0x10 to 0x17 - the payload is already ready
  }
}
static uint8_t findTTEntriesByStartIndex(EmberGpAddress *addr,
                                         uint8_t gpdCommandId,
                                         uint8_t * gpdCommandPayload,
                                         uint8_t * outIndex,
                                         uint8_t startIndex)
{
  uint8_t status = GP_TRANSLATION_TABLE_STATUS_FAILED;
  if (gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
      || gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE
      || gpdCommandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
    uint8_t * payload = NULL;
    // In incomming GPDF has following interpretation for the command payload
    // gpdCommandPayload [0] = length (has special meaning 0xFF)
    // gpdCommandPayload [1] = data start
    if (gpdCommandPayload != NULL
        && gpdCommandPayload[0]
        && gpdCommandPayload[0] != GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_UNSPECIFIED_LENGTH
        && gpdCommandPayload[0] != GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_TT_DERIVED_LENGTH) {
      payload = &gpdCommandPayload[1];
    }
    // this is special case as there may be multiple configuration for the same
    // switch and command - hence the payload which is the currentStaus of contact
    // need to match as well.
    status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                              | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID
                                                              | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_PAYLOAD),
                                                             true,
                                                             addr,
                                                             gpdCommandId,
                                                             0,
                                                             payload,
                                                             NULL,
                                                             outIndex,
                                                             startIndex);
  } else {
    status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                              | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID),
                                                             false,
                                                             addr,
                                                             gpdCommandId,
                                                             0,
                                                             NULL,
                                                             NULL,
                                                             outIndex,
                                                             startIndex);
    if (status != GP_TRANSLATION_TABLE_STATUS_SUCCESS
        || *outIndex == GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX) {
      if (gpdCommandId == EMBER_ZCL_GP_GPDF_ATTRIBUTE_REPORTING
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MULTI_CLUSTER_RPTG
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG) {
        status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                                  | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID),
                                                                 false,
                                                                 addr,
                                                                 EMBER_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD,
                                                                 0,
                                                                 NULL,
                                                                 NULL,
                                                                 outIndex,
                                                                 startIndex);
      }
    }
  }
  return status;
}
// The following function ensures that the command and payload combination is valid. Green Power Spec has the commands specified in following
// category : 1. Holds the command payload that comes from GPD and 2. The command without a payload.
// These two categories of commands are arranged in order of their command ids in the spec.
// Ref "Table 54 – Payloadless GPDF commands sent by GPD" and "Table 55 – GPDF commands with payload sent by GPD"
static bool isGpdCommandPayloadValid(uint8_t gpdCommandId,
                                     uint8_t * gpdCommandPayload)
{
  if (gpdCommandPayload == NULL
      && (gpdCommandId == EMBER_ZCL_GP_GPDF_ZCL_TUNNELING_WITH_PAYLOAD
          || gpdCommandId == EMBER_ZCL_GP_GPDF_ATTRIBUTE_REPORTING
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MULTI_CLUSTER_RPTG
          || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG
          || gpdCommandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING)) {
    return false;
  }
  return true;
}
static EmberStatus filloutCommandAndForward(EmberGpAddress *addr,
                                            uint8_t gpdCommandId,
                                            uint8_t * gpdCommandPayload,
                                            EmGpCommandTranslationTableEntry * tableEntry)
{
  if (tableEntry->entry != NO_ENTRY) {
    EmberAfGreenPowerServerGpdSubTranslationTableEntry const * genericTranslationTable = NULL;
    if (tableEntry->entry == DEFAULT_TABLE_ENTRY) {
      EmberAfGreenPowerServerGpdSubTranslationTableEntry const* defaultTable = emGpGetDefaultTable();
      genericTranslationTable = &(defaultTable[tableEntry->offset]);
      if (defaultTable == NULL) {
        return EMBER_ERR_FATAL;
      }
    } else if (tableEntry->entry == CUSTOMIZED_TABLE_ENTRY) {
      EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTable = emGpGetCustomizedTable();
      if (customizedTable == NULL) {
        return EMBER_ERR_FATAL;
      }
      genericTranslationTable = &(customizedTable[tableEntry->offset]);
    }
    EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = NULL;
    if (tableEntry->infoBlockPresent) {
      EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
      addInfo = &(additionalInfoTable->additionalInfoBlock[tableEntry->additionalInfoOffset]);
      if (addInfo == NULL) {
        return EMBER_ERR_FATAL;
      }
    }
    // Pass the frame to Application
    if (genericTranslationTable->endpoint == EMBER_AF_GP_TRANSLATION_TABLE_ZB_ENDPOINT_PASS_FRAME_TO_APLLICATION) {
      emberAfGreenPowerClusterPassFrameWithoutTranslationCallback(addr,
                                                                  gpdCommandId,
                                                                  gpdCommandPayload);
      return EMBER_SUCCESS;
    }
    // Do Payload validation of all the commands that must carry a payload else it is an error.
    if (!isGpdCommandPayloadValid(gpdCommandId, gpdCommandPayload)) {
      return EMBER_ERR_FATAL;
    }
    // Start to fill and send the commands
    if (gpdCommandId == EMBER_ZCL_GP_GPDF_ZCL_TUNNELING_WITH_PAYLOAD) {
      fillExternalBufferGpdCommandA6Payload(gpdCommandPayload);
    } else if (gpdCommandId == EMBER_ZCL_GP_GPDF_ATTRIBUTE_REPORTING
               || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG) {
      fillExternalBufferGpdCommandA0A1Payload(gpdCommandId,
                                              gpdCommandPayload,
                                              genericTranslationTable->zigbeeCommandId,
                                              (bool)genericTranslationTable->serverClient);
    } else if (gpdCommandId == EMBER_ZCL_GP_GPDF_MULTI_CLUSTER_RPTG
               || gpdCommandId == EMBER_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG) {
      // This will fill up and send out multiple UCAST to the dest app endpoint.
      return handleA2A3MultiClusterReportForwarding(gpdCommandId,
                                                    gpdCommandPayload,
                                                    genericTranslationTable->zigbeeCommandId,
                                                    (bool)genericTranslationTable->serverClient,
                                                    tableEntry->zbEndpoint);
    } else if (gpdCommandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
      if (addInfo == NULL) {
        return EMBER_ERR_FATAL;
      }
      uint8_t dataSize = emberAfGetDataSize(addInfo->optionData.compactAttr.attributeDataType);
      emberAfFillExternalBuffer((ZCL_GLOBAL_COMMAND                                                              \
                                 | ((addInfo->optionData.compactAttr.attributeOptions & 0x01)                    \
                                    ? ZCL_FRAME_CONTROL_SERVER_TO_CLIENT : ZCL_FRAME_CONTROL_CLIENT_TO_SERVER)), \
                                addInfo->optionData.compactAttr.clusterID,                                       \
                                genericTranslationTable->zigbeeCommandId,                                        \
                                "");
      emberAfAppendToExternalBuffer((uint8_t *)(&addInfo->optionData.compactAttr.attributeID),
                                    2);
      emberAfAppendToExternalBuffer((uint8_t *)(&addInfo->optionData.compactAttr.attributeDataType),
                                    1);
      emberAfAppendToExternalBuffer((uint8_t *)(gpdCommandPayload       \
                                                + CAR_DATA_POINT_OFFSET \
                                                + addInfo->optionData.compactAttr.attrOffsetWithinReport), dataSize);
    } else if (gpdCommandId >= EMBER_ZCL_GP_GPDF_RECALL_SCENE0
               && gpdCommandId <= EMBER_ZCL_GP_GPDF_STORE_SCENE7) {
      // All scenes commands 0x10 to 0x1F is prepared and handled here
      fillExternalBufferGpdCommandScenesPayload(addr,
                                                gpdCommandId,
                                                genericTranslationTable,
                                                tableEntry->zbEndpoint);
    } else {
      // General command filling.
      emberAfFillExternalBuffer((ZCL_CLUSTER_SPECIFIC_COMMAND             \
                                 | ZCL_FRAME_CONTROL_CLIENT_TO_SERVER),   \
                                genericTranslationTable->zigbeeCluster,   \
                                genericTranslationTable->zigbeeCommandId, \
                                "");

      // First copy the pre-configured source
      if (genericTranslationTable->payloadSrc & EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED) {
        emberAfAppendToExternalBuffer(&genericTranslationTable->zclPayloadDefault[1],
                                      genericTranslationTable->zclPayloadDefault[0]);
      }
      // Over write the payload from gpd command source - as this is the priority
      if ((genericTranslationTable->payloadSrc & EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD)
          && gpdCommandPayload != NULL) {
        emberAfAppendToExternalBuffer(&gpdCommandPayload[1],
                                      gpdCommandPayload[0]);
      }
    }
    emberAfSetCommandEndpoints(tableEntry->zbEndpoint,
                               tableEntry->zbEndpoint);
    return emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, emberAfGetNodeId());
  }
  return EMBER_ERR_FATAL;
}
static void forwardGpdCommandBasedOnTranslationTable(EmberGpAddress *addr,
                                                     uint8_t gpdCommandId,
                                                     uint8_t * gpdCommandPayload)
{
  // Get translation table head
  EmGpCommandTranslationTable * translationTable = emGpTransTableGetTranslationTable();
  if (translationTable == NULL) {
    // Error or No TT suported
    return;
  }
  // Loop through the TT entries and forward the commands to the endpoints that are commissionined
  uint8_t startIndex = 0;
  while (true) {
    uint8_t outIndex = GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX;
    uint8_t status = GP_TRANSLATION_TABLE_STATUS_FAILED;
    status = findTTEntriesByStartIndex(addr, gpdCommandId, gpdCommandPayload, &outIndex, startIndex);
    if (status != GP_TRANSLATION_TABLE_STATUS_SUCCESS
        || outIndex == GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX) {
      // Error or end of TT
      return;
    }
    filloutCommandAndForward(addr,
                             gpdCommandId,
                             gpdCommandPayload,
                             &translationTable->TableEntry[outIndex]);
    startIndex = outIndex + 1;
  }
}

// Green Power Cluster GP forward CommandID after received a GP Notification
// we go here on Notification reception (not on direct GPD GPDF reception)
static void emberAfGreenPowerClusterForwardGpdfCommand(EmberGpAddress *addr,
                                                       uint8_t gpdCommandId,
                                                       uint8_t * gpdCommandPayload)
{
  // Call user first to give a chance to handle the notification.
  if (emberAfGreenPowerClusterGpNotificationForwardCallback(addr,
                                                            gpdCommandId,
                                                            gpdCommandPayload)) {
    return;
  }
  // The attribute is supported to handle translation ?
  if (!sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_TRANSLATION_TABLE)) {
    return;
  }
  forwardGpdCommandBasedOnTranslationTable(addr,
                                           gpdCommandId,
                                           gpdCommandPayload);
}

// decommission a gpd
static void decommissionGpd(uint8_t secLvl,
                            uint8_t keyType,
                            EmberGpAddress * gpdAddr,
                            bool setRemoveGpdflag)
{
  uint8_t sinkEntryIndex = emberGpSinkTableLookup(gpdAddr);
  if (sinkEntryIndex != 0xFF) {
    // to delete custom Translation table entry for this GPD
    emGpTransTableDeletePairedDevicefromTranslationTableEntry(gpdAddr);
    EmberGpSinkType gpsCommunicationMode = EMBER_GP_SINK_TYPE_D_GROUPCAST;
    EmberAfAttributeType type;
    emberAfReadAttribute(GP_ENDPOINT,
                         ZCL_GREEN_POWER_CLUSTER_ID,
                         ZCL_GP_SERVER_GPS_COMMUNICATION_MODE_ATTRIBUTE_ID,
                         (CLUSTER_MASK_SERVER),
                         (uint8_t*)&gpsCommunicationMode,
                         sizeof(uint8_t),
                         &type);
    uint16_t dGroupId = 0xFFFF;
    uint32_t pairingOptions = (setRemoveGpdflag ? EMBER_AF_GP_PAIRING_OPTION_REMOVE_GPD : 0)
                              | (gpdAddr->applicationId & EMBER_AF_GP_NOTIFICATION_OPTION_APPLICATION_ID)
                              | (gpsCommunicationMode << EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                              | (secLvl << EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                              | (keyType << EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET);

    if (gpsCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST) {
      dGroupId = emGpdAlias(gpdAddr);
    }
    EmberEUI64 ourEUI;
    emberAfGetEui64(ourEUI);
    emberAfFillCommandGreenPowerClusterGpPairingSmart(pairingOptions,
                                                      gpdAddr->id.sourceId,
                                                      gpdAddr->id.gpdIeeeAddress,
                                                      gpdAddr->endpoint,
                                                      ourEUI,
                                                      emberGetNodeId(),
                                                      dGroupId,
                                                      0xFF,
                                                      0xFFFFFFFF,
                                                      NULL,
                                                      0xFFFF,
                                                      0xFF);
    EmberApsFrame *apsFrame;
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;
    apsFrame->destinationEndpoint = GP_ENDPOINT;
    uint8_t retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
    emberAfGreenPowerClusterPrintln("Gp Pairing for Decommissing send returned %d", retval);

    // move send gp pairing config from elsewhere to here in case of Cgroup=communication mode
    // this is before removingthe sink entry in order to have a valid DeviceID to indicate into
    // the gp pairing config frame
    uint8_t deviceId = gpGpSinkTableGetDeviceId(sinkEntryIndex);
    if (sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
        || sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION)) {
      emberAfFillCommandGreenPowerClusterGpPairingConfigurationSmart(EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_GPD,
                                                                     0,
                                                                     gpdAddr->id.sourceId,
                                                                     gpdAddr->id.gpdIeeeAddress,
                                                                     gpdAddr->endpoint,
                                                                     deviceId,
                                                                     0,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0xFE,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0,
                                                                     NULL,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     NULL);
      EmberApsFrame * apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;
      apsFrame->destinationEndpoint = GP_ENDPOINT;
      uint8_t retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
      emberAfGreenPowerClusterPrintln("pairing send returned %d", retval);
    }

    removeFromApsGroup(GP_ENDPOINT, dGroupId);
    emberGpSinkTableRemoveEntry(sinkEntryIndex);
  }
}

static void handleChannelRequest(uint16_t options,
                                 uint16_t gppShortAddress,
                                 bool rxAfterTx,
                                 uint8_t* gpdCommandPayload)
{
  if (!(commissioningState.inCommissioningMode)) {
    emberAfGreenPowerClusterPrintln("DROP : Channel Request: Sink not in commissioning!");
    return;
  }
  if (!rxAfterTx) {
    // DROP as the rxAfterTx is not set of this notification
    emberAfGreenPowerClusterPrintln("DROP : Channel Request NO - rxAfterTx");
    return;
  }
  //Basic sink just runs with the first one, doesn't select the best
  uint8_t responseOption = options & EMBER_AF_GP_NOTIFICATION_OPTION_APPLICATION_ID; // Application Id
  uint8_t nextChannel = (gpdCommandPayload[1]
                         & EMBER_AF_GP_GPD_CHANNEL_REQUEST_CHANNEL_TOGGLING_BEHAVIOR_RX_CHANNEL_NEXT_ATTEMPT);
  uint8_t channelConfigPayload;
  channelConfigPayload = (emberAfGetRadioChannel() - 11) & EMBER_AF_GP_GPD_CHANNEL_CONFIGURATION_CHANNEL_OPERATIONAL_CHANNEL;
  channelConfigPayload |= (1 << EMBER_AF_GP_GPD_CHANNEL_CONFIGURATION_CHANNEL_BASIC_OFFSET); // BASIC
  emberAfFillCommandGreenPowerClusterGpResponseSmart(responseOption,
                                                     gppShortAddress,
                                                     nextChannel,
                                                     0,
                                                     NULL,
                                                     0,
                                                     EMBER_ZCL_GP_GPDF_CHANNEL_CONFIGURATION,
                                                     sizeof(uint8_t),
                                                     &channelConfigPayload);
  EmberApsFrame *apsFrame;
  apsFrame = emberAfGetCommandApsFrame();
  apsFrame->sourceEndpoint = GP_ENDPOINT;
  apsFrame->destinationEndpoint = GP_ENDPOINT;
  uint8_t UNUSED retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
}

//Green Power Cluster Gp Notification
bool emberAfGreenPowerClusterGpNotificationCallback(uint16_t options,
                                                    uint32_t gpdSrcId,
                                                    uint8_t* gpdIeee,
                                                    uint8_t  gpdEndpoint,
                                                    uint32_t gpdSecurityFrameCounter,
                                                    uint8_t  gpdCommandId,
                                                    uint8_t* gpdCommandPayload,
                                                    uint16_t gppShortAddress,
                                                    uint8_t  gppDistance)
{
  uint8_t gppLink = 0xFF;
  uint8_t rssi = 0xFF;
  uint8_t linkQuality = 0xFF;
  uint8_t allZeroesIeeeAddress[17] = { 0 };

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = allZeroesIeeeAddress;
  }

  emberAfGreenPowerClusterPrintln("command %d", gpdCommandId);
  //if (gpdCommandPayload != NULL) { // Ensure gpdCommandPayload is not NULL to print the payload
  //  emberAfGreenPowerClusterPrint("payload: ");
  //  emberAfGreenPowerClusterPrintBuffer(&gpdCommandPayload[1],
  //                                      gpdCommandPayload[0],
  //                                      true);
  //  emberAfGreenPowerClusterPrintln("");
  //}

  if ((options & EMBER_AF_GP_NOTIFICATION_OPTION_RX_AFTER_TX)) {
    //gppDistance = gppDistance; // gppDistance unchanged
  } else {
    gppLink = gppDistance;
    rssi = gppLink & EMBER_AF_GP_GPP_GPD_LINK_RSSI;
    // since gp 1.0b-14-
    // "The RSSI sub-field of the GPP-GPD link field encodes the RSSI from the range <+8 ; -109> [dBm],
    // with 2dBm granularity.", then
    // 0 represent -109dbm
    // 1 represent -107dbm
    // 2 represent -105dbm
    // ...
    // 54 represent -1dbm
    // 55 represent  0dbm // "110 is add to capped RSSI value, to obtain a non-negative value" gp1.0b 14-
    // 56 represent  2dbm
    // 57 represent  4dbm
    // 58 represent  6dbm
    // 59 represent  8dbm
    // 60 is value not used
    if (rssi < 55) {
      rssi = -109 + 2 * rssi;
    } else {
      rssi = (rssi - 55) * 2;
    }
    linkQuality = ((gppLink & EMBER_AF_GP_GPP_GPD_LINK_LINK_QUALITY)
                   >> EMBER_AF_GP_GPP_GPD_LINK_LINK_QUALITY_OFFSET);
  }

  EmberGpAddress gpdAddr;
  if (!emGpMakeAddr(&gpdAddr, (options & EMBER_AF_GP_NOTIFICATION_OPTION_APPLICATION_ID), gpdSrcId, gpdIeee, gpdEndpoint)) {
    return false;
  }
  // Find the sink entry and update the security frame counter from gpd
  uint8_t sinkIndex = emberGpSinkTableLookup(&gpdAddr);
  if (sinkIndex != 0xFF) {
    EmberGpSinkTableEntry entry = { 0 };
    EmberStatus status = emberGpSinkTableGetEntry(sinkIndex, &entry);
    // GPD Security validation, if fails drop!
    if (status == EMBER_SUCCESS) {
      uint8_t receivedSecLevel = ((options & EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL)
                                  >> EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);
      uint8_t receivedKeyType = ((options & EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE)
                                 >> EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET);
      uint8_t sinkSecLevel = (entry.securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
      uint8_t sinkKeyType = ((entry.securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                             >> EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
      if (sinkSecLevel > 0) {
        if (sinkSecLevel > receivedSecLevel
            || sinkKeyType != receivedKeyType
            || entry.gpdSecurityFrameCounter > gpdSecurityFrameCounter) {
          // DROP
          emberAfGreenPowerClusterPrintln("Gp Notif : DROP - SecLevel, Key type or framecounter mismatch");
          return true;
        }
      }
    }
    gpGpSinkTableSetSecurityFrameCounter(sinkIndex, gpdSecurityFrameCounter);
  }
  if (gpdCommandId != EMBER_ZCL_GP_GPDF_DECOMMISSIONING
      && gpdCommandId != EMBER_ZCL_GP_GPDF_CHANNEL_REQUEST) {
    emberAfGreenPowerClusterForwardGpdfCommand(&gpdAddr,
                                               gpdCommandId,
                                               gpdCommandPayload);
  } else if (gpdCommandId == EMBER_ZCL_GP_GPDF_DECOMMISSIONING) {
    uint8_t secLvl = ((options & EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL)
                      >> EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);
    uint8_t keyType = ((options & EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE)
                       >> EMBER_AF_GP_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET);
    decommissionGpd(secLvl, keyType, &gpdAddr, true);
  } else if (gpdCommandId == EMBER_ZCL_GP_GPDF_CHANNEL_REQUEST
             && gpdCommandPayload != NULL) { // Channel Request always has one byte payload.
    handleChannelRequest(options,
                         gppShortAddress,
                         ((options & EMBER_AF_GP_NOTIFICATION_OPTION_RX_AFTER_TX) ? true : false),
                         gpdCommandPayload);
  }
  return true;
}

// Allow the sink to decided if it match with the GP frame it received
static bool endpointAndClusterIdValidation(uint8_t endpoint,
                                           bool server,
                                           EmberAfClusterId clusterId)
{
  if (isCommissioningAppEndpoint(endpoint)
      && ((server && emberAfContainsServer(endpoint, clusterId))
          || (!server && emberAfContainsClient(endpoint, clusterId)))) {
    return true;
  }
  return false;
}

static void handleBidirectionalCommissioningGpdf(EmberGpAddress * gpdAddr,
                                                 uint8_t securityLevel,
                                                 uint8_t gpdfExtendedOptions,
                                                 uint8_t gpdfOptions,
                                                 const EmberKeyData * linkKey,
                                                 EmberKeyData * key,
                                                 uint32_t outgoingFrameCounter,
                                                 uint16_t gppShortAddress)
{
  EmberAfAttributeType type;
  uint8_t gpsSecurityKeyTypeAtrribute = 0;
  EmberKeyData gpsKeyAttribute = { { 0 } };
  EmberKeyData sendKey = { { 0 } };
  uint8_t sendSecurityKeyType = 0;
  bool sendKeyEncryption = false;
  bool sendKeyinReply = false;
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_TYPE_ATTRIBUTE_ID,
                       CLUSTER_MASK_SERVER,
                       (uint8_t *)&gpsSecurityKeyTypeAtrribute,
                       sizeof(uint8_t),
                       &type);
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                       CLUSTER_MASK_SERVER,
                       gpsKeyAttribute.contents,
                       EMBER_ENCRYPTION_KEY_SIZE,
                       &type);
  uint8_t incomingGpdKeyType = ((gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE)
                                >> EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
  bool incomingGpdKeyEncryption = ((gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION)
                                   ? true : false);
  if (gpsSecurityKeyTypeAtrribute == 0
      && (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)) {
    // Use the proposed key, that has come in the commissioning gfdf and saved in
    // gpdCommDataSaved
    sendKeyinReply = false;
    sendSecurityKeyType = incomingGpdKeyType;
  }
  if (incomingGpdKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_INDIVIDIGUAL_GPD_KEY
      && (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)
      && gpsSecurityKeyTypeAtrribute != 0) {
    sendSecurityKeyType = gpsSecurityKeyTypeAtrribute;
    MEMCOPY(sendKey.contents, gpsKeyAttribute.contents, EMBER_ENCRYPTION_KEY_SIZE);
    sendKeyEncryption = incomingGpdKeyEncryption;
    sendKeyinReply = true;
    // Set up same key and key type for the Gp Pairing
    gpdCommDataSaved.securityKeyType = sendSecurityKeyType;
    MEMCOPY(gpdCommDataSaved.key.contents, sendKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
  }
  // If the GPD proposes a OOB key, and the key request is `false`, it means the commissioning
  // is expected to use that OOB only. No need to send the key back.
  if (!(gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)) {
    sendSecurityKeyType = incomingGpdKeyType;
    sendKeyinReply = false;
  }
  if (incomingGpdKeyType == (gpsSecurityKeyTypeAtrribute & GPS_ATTRIBUTE_KEY_TYPE_MASK)
      && 0 == MEMCOMPARE(key->contents,
                         gpsKeyAttribute.contents,
                         EMBER_ENCRYPTION_KEY_SIZE)
      && (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)
      && gpsSecurityKeyTypeAtrribute != 0) {
    sendSecurityKeyType = gpsSecurityKeyTypeAtrribute;
    sendKeyinReply = false;
    sendKeyEncryption = false;
  }

  uint8_t commReplyOptions = 0;
  emberAfGreenPowerClusterPrintln("Added securityLevel %d", securityLevel);
  commReplyOptions |= (securityLevel << EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_SECURITY_LEVEL_OFFSET);
  if (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_PAN_ID_REQUEST) {
    commReplyOptions |= EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_PAN_ID_PRESENT;
    emberAfGreenPowerClusterPrintln("Added PanId");
  }
  emberAfGreenPowerClusterPrintln("Added Key Type = %d", sendSecurityKeyType);
  commReplyOptions |= (sendSecurityKeyType
                       << EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_KEY_TYPE_OFFSET);

  if (sendKeyinReply) {
    emberAfGreenPowerClusterPrintln("Added Key Present");
    commReplyOptions |= EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPD_SECURITY_KEY_PRESENT;
    if (sendKeyEncryption) { // GPDF GPKeyEncryption Subfeild
      emberAfGreenPowerClusterPrintln("Added Key Encrypted");
      commReplyOptions |= EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPDKEY_ENCRYPTION;
    }
  }
  // Prepare the payload
  uint8_t commReplyPayload[COMM_REPLY_PAYLOAD_SIZE] = { 0 };
  commReplyPayload[0] = commReplyOptions;
  uint8_t index = 1;
  if (commReplyOptions & EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_PAN_ID_PRESENT) {
    emberStoreLowHighInt16u(commReplyPayload + index, emberAfGetPanId());
    index += 2;
  }

  if (commReplyOptions & EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPD_SECURITY_KEY_PRESENT) {
    if (commReplyOptions & EMBER_AF_GP_GPD_COMMISSIONING_REPLY_OPTIONS_GPDKEY_ENCRYPTION) {
      uint8_t mic[4] = { 0 };
      uint32_t fc = outgoingFrameCounter + 1;
      emberAfGreenPowerClusterPrintln("Key :");
      for (int i = 0; i < EMBER_ENCRYPTION_KEY_SIZE; i++) {
        emberAfGreenPowerClusterPrint("%x", sendKey.contents[i]);
      }
      emberAfGreenPowerClusterPrintln("");
      // Encrypt the Commissioning Reply (outgoing) Key
      emGpKeyTcLkDerivation(gpdAddr,
                            fc,
                            mic,
                            linkKey,
                            &sendKey,
                            false);
      MEMCOPY(commReplyPayload + index, sendKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
      index += EMBER_ENCRYPTION_KEY_SIZE;
      MEMCOPY(commReplyPayload + index, mic, 4);
      index += 4;
      MEMCOPY(commReplyPayload + index, &fc, 4);
      index += 4;
    } else {
      MEMCOPY(commReplyPayload + index, sendKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
      index += EMBER_ENCRYPTION_KEY_SIZE;
    }
  }
  emberAfGreenPowerClusterPrintln("GpResponse : Sending Commissioning Reply");
  emberAfFillCommandGreenPowerClusterGpResponseSmart(gpdAddr->applicationId,
                                                     gppShortAddress,
                                                     (emberAfGetRadioChannel() - 11),
                                                     gpdAddr->id.sourceId,
                                                     gpdAddr->id.gpdIeeeAddress,
                                                     gpdAddr->endpoint,
                                                     EMBER_ZCL_GP_GPDF_COMMISSIONING_REPLY,
                                                     index,
                                                     commReplyPayload);
  EmberApsFrame *apsFrame;
  apsFrame = emberAfGetCommandApsFrame();
  apsFrame->sourceEndpoint = GP_ENDPOINT;
  apsFrame->destinationEndpoint = GP_ENDPOINT;
  EmberStatus UNUSED retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  emberAfGreenPowerClusterPrintln("GpResponse send returned %d", retval);
}

static bool isSinkTableUpdateNeeded(uint8_t sinkEntryIndex,
                                    uint32_t newSinkTableOptions,
                                    uint8_t newSinkTableSecurityOptions)
{
  EmberGpSinkTableEntry sinkEntry = { 0 };
  if (emberGpSinkTableGetEntry(sinkEntryIndex, &sinkEntry) == EMBER_SUCCESS) {
    if (sinkEntry.options == newSinkTableOptions
        && ((sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SECURITY_USE)
            || (sinkEntry.securityOptions))
        && sinkEntry.securityOptions == newSinkTableSecurityOptions) {
      if ((gpdCommDataSaved.securityKeyType != EMBER_ZCL_GP_SECURITY_KEY_TYPE_INDIVIDIGUAL_GPD_KEY
           && gpdCommDataSaved.securityKeyType != EMBER_ZCL_GP_SECURITY_KEY_TYPE_NETWORK_DERIVED_GROUP_KEY)) {
        // GPD use shared key, check existing key present in gpSharedSecurityKey attribute
        EmberKeyData gpSharedSecurityKey;
        EmberAfAttributeType type;
        EmberAfStatus status = emberAfReadAttribute(GP_ENDPOINT,
                                                    ZCL_GREEN_POWER_CLUSTER_ID,
                                                    ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                                                    (CLUSTER_MASK_SERVER),
                                                    gpSharedSecurityKey.contents,
                                                    EMBER_ENCRYPTION_KEY_SIZE,
                                                    &type);
        if (status == EMBER_ZCL_STATUS_SUCCESS
            && 0 == MEMCOMPARE(gpSharedSecurityKey.contents,
                               gpdCommDataSaved.key.contents,
                               EMBER_ENCRYPTION_KEY_SIZE)) {
          return false;
        }
      } else if (0 == MEMCOMPARE(sinkEntry.gpdKey.contents,
                                 gpdCommDataSaved.key.contents,
                                 EMBER_ENCRYPTION_KEY_SIZE)) {
        return false;
      }
    }
  }
  return true;
}

static void writeKeysToSharedAttribute(void)
{
  // write into "gpSharedSecurityKey" if the SecurityKeyType has value :
  // - 0b010 (GPD group key), 0b001 (NWK key) or
  // - 0b011 (NK derived key),
  // the GPDkey parameter MAY be omitted and the key MAY be stored in the
  // gpSharedSecurityKey parameter instead.
  // If SecurityLevel has value other than 0b00 and the SecurityKeyType has
  // value 0b111 (derived individual GPD key),
  // the sink table GPDkey parameter MAY be omitted and the key MAY calculated on the fly,
  // based on the value stored in the gpSharedSecurityKey parameter (=groupKey)
  if ((gpdCommDataSaved.securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_ZIGBEE_NETWORK_KEY)
      || (gpdCommDataSaved.securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_GPD_GROUP_KEY)
      || (gpdCommDataSaved.securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_NETWORK_DERIVED_GROUP_KEY)) {
    // write gpSharedSecurityKey attribute
    // key is present in sink table if keyType is OOB or Derived-NK,
    // else it is possible to use "gpsSharedSecurityKey" attribut to save space in the sink table
    EmberAfStatus status = emberAfWriteAttribute(GP_ENDPOINT,
                                                 ZCL_GREEN_POWER_CLUSTER_ID,
                                                 ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                                                 CLUSTER_MASK_SERVER,
                                                 gpdCommDataSaved.key.contents,
                                                 ZCL_SECURITY_KEY_ATTRIBUTE_TYPE);
    // verify if this optional "gpsSharedSecurityKey" attribute is supported by this product
    if (status == EMBER_ZCL_STATUS_SUCCESS) {
      // optional "gpsSharedSecurityKey" attribute is supported
      // continue by saving keyType in "gpsSharedSecurityKeyType"
      status = emberAfWriteAttribute(GP_ENDPOINT,
                                     ZCL_GREEN_POWER_CLUSTER_ID,
                                     ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_TYPE_ATTRIBUTE_ID,
                                     CLUSTER_MASK_SERVER,
                                     (uint8_t *)&gpdCommDataSaved.securityKeyType,
                                     ZCL_BITMAP8_ATTRIBUTE_TYPE);
      // if the status return is not OK, delete previous key saved and store key only in sink table
      if (status != EMBER_ZCL_STATUS_SUCCESS) {
        uint8_t contents[EMBER_ENCRYPTION_KEY_SIZE];
        MEMSET(contents, 0xFF, EMBER_ENCRYPTION_KEY_SIZE);
        emberAfWriteAttribute(GP_ENDPOINT,
                              ZCL_GREEN_POWER_CLUSTER_ID,
                              ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                              CLUSTER_MASK_SERVER,
                              contents,
                              ZCL_SECURITY_KEY_ATTRIBUTE_TYPE);
      }
    }
  }
}

static void sendGpPairingConfigBasedOnSinkCommunicationMode(EmberGpAddress gpdAddr)
{
  EmberGpSinkType sinkCommunicationMode = EMBER_GP_SINK_TYPE_D_GROUPCAST;
  EmberAfStatus status;
  EmberAfAttributeType type;
  status = emberAfReadAttribute(GP_ENDPOINT,
                                ZCL_GREEN_POWER_CLUSTER_ID,
                                ZCL_GP_SERVER_GPS_COMMUNICATION_MODE_ATTRIBUTE_ID,
                                (CLUSTER_MASK_SERVER),
                                (uint8_t*)&sinkCommunicationMode,
                                sizeof(uint8_t),
                                &type);
  emberAfGreenPowerClusterPrintln("Communication mode to send GpPairingConfig %d", sinkCommunicationMode);
  if (sinkCommunicationMode != EMBER_GP_SINK_TYPE_GROUPCAST) {
    return;
  }
  // Send the Gp Configuration if the sink supports
  // - sink table-based forwarding
  if (sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
      || sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION) ) {
    // Send GP Pairing Config with
    // EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE
    // | EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES
    // | (communication mode = GroupCast Forwarding (2))
    uint16_t pairigConfigOptions = GP_PAIRING_CONFIGURATION_FIXED_FLAG;
    pairigConfigOptions |= ((gpdCommDataSaved.applicationInfo.applInfoBitmap)
                            ? EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_INFORMATION_PRESENT : 0);
    uint8_t groupList[] = { 1, 1, 0, 0, 0, 1, 0, 0, 0 };
    getGroupListBasedonAppEp(groupList);
    uint8_t securityOptions = gpdCommDataSaved.securityLevel
                              | (gpdCommDataSaved.securityKeyType << EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);

    emberAfFillCommandGreenPowerClusterGpPairingConfigurationSmart(EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_EXTEND_SINK_TABLE_ENTRY,
                                                                   pairigConfigOptions,
                                                                   gpdAddr.id.sourceId,
                                                                   gpdAddr.id.gpdIeeeAddress,
                                                                   gpdAddr.endpoint,
                                                                   gpdCommDataSaved.applicationInfo.deviceId,
                                                                   ((4 * groupList[0]) + 1),
                                                                   groupList,
                                                                   0,
                                                                   0,
                                                                   securityOptions,
                                                                   gpdCommDataSaved.outgoingFrameCounter,
                                                                   gpdCommDataSaved.key.contents,
                                                                   0xFE,
                                                                   NULL,
                                                                   gpdCommDataSaved.applicationInfo.applInfoBitmap,
                                                                   0,
                                                                   0,
                                                                   0,
                                                                   NULL,
                                                                   0,
                                                                   NULL,
                                                                   NULL,
                                                                   gpdCommDataSaved.switchInformationStruct.switchInfoLength,
                                                                   (gpdCommDataSaved.switchInformationStruct.nbOfContacts  \
                                                                    + (gpdCommDataSaved.switchInformationStruct.switchType \
                                                                       << EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET)),
                                                                   gpdCommDataSaved.switchInformationStruct.currentContact,
                                                                   0,
                                                                   0,
                                                                   NULL);
    EmberApsFrame *apsFrame;
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;
    apsFrame->destinationEndpoint = GP_ENDPOINT;

    uint8_t retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
    emberAfGreenPowerClusterPrintln("Gp Pairing Config Extend Sink send returned %d", retval);
    if (gpdCommDataSaved.applicationInfo.applInfoBitmap
        & EMBER_AF_GP_APPLICATION_INFORMATION_APPLICATION_DESCRIPTION_PRESENT) {
      emberAfFillCommandGreenPowerClusterGpPairingConfigurationSmart(EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_APPLICATION_DESCRIPTION,
                                                                     0,
                                                                     gpdAddr.id.sourceId,
                                                                     gpdAddr.id.gpdIeeeAddress,
                                                                     gpdAddr.endpoint,
                                                                     gpdCommDataSaved.applicationInfo.deviceId,
                                                                     0,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0xFE,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0,
                                                                     NULL,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     0,
                                                                     gpdCommDataSaved.totalNbOfReport,
                                                                     gpdCommDataSaved.numberOfReports,
                                                                     gpdCommDataSaved.reportsStorage);
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;
      apsFrame->destinationEndpoint = GP_ENDPOINT;
      retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
      emberAfGreenPowerClusterPrintln("Gp Pairing Config App Description send returned %d", retval);
    }
  }
}

static void handleSinkEntryAndPairing(EmberGpAddress gpdAddr,
                                      uint8_t gpdfOptions,
                                      uint8_t gpdfExtendedOptions,
                                      uint32_t outgoingFrameCounter,
                                      uint8_t securityLevel,
                                      uint8_t securityKeyType,
                                      EmberGpApplicationInfo appInfoStruct)
{
  // if RxAfterTx clear (and also not waiting for Application description command which follows)
  // in case GPD is doing unidirectional commissioning
  // -------------------------------------------------
  bool gpdFixed = false;
  bool gpdMacCapabilities = false;

  if (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_MAC_SEQ_NUM_CAP) {
    gpdMacCapabilities = true;
  }
  if (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_FIXED_LOCATION) {
    gpdFixed = true;
  }
  // communication mode choices:
  // - EMBER_GP_SINK_TYPE_FULL_UNICAST,
  // - EMBER_GP_SINK_TYPE_D_GROUPCAST,
  // - EMBER_GP_SINK_TYPE_GROUPCAST,
  // - EMBER_GP_SINK_TYPE_LW_UNICAST,
  EmberGpSinkType sinkCommunicationMode = EMBER_GP_SINK_TYPE_D_GROUPCAST;
  if (gpdCommDataSaved.communicationMode == EMBER_GP_SINK_TYPE_UNUSED) {
    EmberAfStatus status;
    EmberAfAttributeType type;
    status = emberAfReadAttribute(GP_ENDPOINT,
                                  ZCL_GREEN_POWER_CLUSTER_ID,
                                  ZCL_GP_SERVER_GPS_COMMUNICATION_MODE_ATTRIBUTE_ID,
                                  (CLUSTER_MASK_SERVER),
                                  (uint8_t*)&sinkCommunicationMode,
                                  sizeof(uint8_t),
                                  &type);
  } else {
    sinkCommunicationMode = gpdCommDataSaved.communicationMode;
  }
  // by default full unicast and derived groupcast communication mode
  // will use derived alias instead assigned alias,
  bool sinkTableAssignedAliasNeeded = false;
  if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_FULL_UNICAST
      || sinkCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST) {
    sinkTableAssignedAliasNeeded = false;
  }
  // So only a tool writing sink table or an application setting to
  // true "useGivenAssignedAlias"
  if (gpdCommDataSaved.useGivenAssignedAlias) {
    sinkTableAssignedAliasNeeded = true;
  }
  uint32_t newSinkTableOptions = (gpdAddr.applicationId & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_APPLICATION_ID)
                                 | (sinkCommunicationMode << EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET)
                                 | (gpdMacCapabilities << EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES_OFFSET)
                                 | (((gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP) >> EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP_OFFSET) << EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_RX_ON_CAPABILITY_OFFSET)
                                 | (gpdFixed << EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_FIXED_LOCATION_OFFSET)
                                 | (sinkTableAssignedAliasNeeded << EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS_OFFSET)
                                 | (((gpdfOptions & 0x80) && gpdfExtendedOptions) ? EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SECURITY_USE : 0);
  // SecurityUse bit always set to 0b1, so securityOptions field always present
  // also to indicate level 0 or key NONE, cause it will be extremly rare that a GPD is SecLvl=0,
  uint8_t newSinkTableSecurityOptions = gpdCommDataSaved.securityLevel
                                        | (gpdCommDataSaved.securityKeyType << EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
  uint8_t sinkEntryIndex = emberGpSinkTableLookup(&gpdAddr);
  // if entry already exists and not needed to be updated then return
  if (sinkEntryIndex != 0xFF
      && !isSinkTableUpdateNeeded(sinkEntryIndex,
                                  newSinkTableOptions,
                                  newSinkTableSecurityOptions)) {
    return;
  }
  // if it is a new entry, create one
  if (sinkEntryIndex == 0xFF) {
    sinkEntryIndex = emberGpSinkTableFindOrAllocateEntry(&gpdAddr);
  }
  // must have a valid sinkEntryIndex by now, else indicates issue
  // with sink table such as sink table full.
  if (sinkEntryIndex != 0xFF) {
    // Start updating sink table
    EmberGpSinkTableEntry sinkEntry;
    emberGpSinkTableGetEntry(sinkEntryIndex, &sinkEntry);
    sinkEntry.options = newSinkTableOptions;
    sinkEntry.deviceId = gpdCommDataSaved.applicationInfo.deviceId;
    sinkEntry.assignedAlias = emGpdAlias(&gpdAddr);
    if (gpdCommDataSaved.useGivenAssignedAlias) {
      sinkEntry.assignedAlias = gpdCommDataSaved.givenAlias;
    }
    if (sinkEntry.groupcastRadius == 0xff
        || sinkEntry.groupcastRadius < gpdCommDataSaved.groupcastRadius) {
      sinkEntry.groupcastRadius = gpdCommDataSaved.groupcastRadius;
    }
    // Add the Group
    uint16_t groupId = 0xFFFF;
    if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST) {
      groupId = emGpdAlias(&gpdAddr);
      EmberAfStatus status = addToApsGroup(GP_ENDPOINT, groupId);
      emberAfCorePrintln("Added to Group %d Status = %d", groupId, status);
    } else if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_GROUPCAST) {
      uint8_t groupList[] = { 1, 1, 0, 0, 0, 1, 0, 0, 0 };
      getGroupListBasedonAppEp(groupList);
      for (uint8_t index = 0; index < groupList[0]; index++) {
        sinkEntry.sinkList[index].type = EMBER_GP_SINK_TYPE_GROUPCAST;
        sinkEntry.sinkList[index].target.groupcast.groupID = (groupList[2] << 8) + groupList[1];
        sinkEntry.sinkList[index].target.groupcast.alias = (groupList[4] << 8) + groupList[3];
      }
      groupId = sinkEntry.sinkList[0].target.groupcast.groupID;
      EmberAfStatus UNUSED status = addToApsGroup(GP_ENDPOINT, groupId);
    }
    sinkEntry.securityOptions = newSinkTableSecurityOptions;
    // carefull, take the gpd outgoing FC (not the framecounter of the commissioning frame "gpdSecurityFrameCounter")
    sinkEntry.gpdSecurityFrameCounter = outgoingFrameCounter;
    MEMCOPY(sinkEntry.gpdKey.contents, gpdCommDataSaved.key.contents, EMBER_ENCRYPTION_KEY_SIZE);
    emberGpSinkTableSetEntry(sinkEntryIndex, &sinkEntry);
    // End of Sink Table Update

    // Write the keys into the shared attributes if supported
    writeKeysToSharedAttribute();
    // Start to prepare to send a GP Pairing Command
    EmberEUI64 ourEUI;
    emberAfGetEui64(ourEUI);
    uint32_t pairingOptions = 0;
    pairingOptions = EMBER_AF_GP_PAIRING_OPTION_ADD_SINK
                     | (gpdAddr.applicationId & EMBER_AF_GP_PAIRING_OPTION_APPLICATION_ID)
                     | (sinkCommunicationMode << EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                     | (gpdFixed << EMBER_AF_GP_PAIRING_OPTION_GPD_FIXED_OFFSET)
                     | (gpdMacCapabilities << EMBER_AF_GP_PAIRING_OPTION_GPD_MAC_SEQUENCE_NUMBER_CAPABILITIES_OFFSET)
                     | (gpdCommDataSaved.securityLevel << EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                     | (gpdCommDataSaved.securityKeyType << EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET)
                     | ((gpdMacCapabilities || gpdCommDataSaved.securityLevel) ? EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_FRAME_COUNTER_PRESENT : 0)
                     | ((gpdCommDataSaved.securityLevel && gpdCommDataSaved.securityKeyType) ? EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_KEY_PRESENT : 0)
                     | (sinkTableAssignedAliasNeeded << EMBER_AF_GP_PAIRING_OPTION_ASSIGNED_ALIAS_PRESENT_OFFSET)
                     | ((sinkCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST
                         || sinkCommunicationMode == EMBER_GP_SINK_TYPE_GROUPCAST)
                        ? EMBER_AF_GP_PAIRING_OPTION_GROUPCAST_RADIUS_PRESENT : 0);

    emberAfFillCommandGreenPowerClusterGpPairingSmart(pairingOptions,
                                                      gpdAddr.id.sourceId,
                                                      gpdAddr.id.gpdIeeeAddress,
                                                      gpdAddr.endpoint,
                                                      ourEUI,
                                                      emberGetNodeId(),
                                                      groupId,
                                                      appInfoStruct.deviceId,
                                                      outgoingFrameCounter,//gpdCommDataSaved.outgoingFrameCounter,
                                                      gpdCommDataSaved.key.contents,
                                                      sinkEntry.assignedAlias,
                                                      sinkEntry.groupcastRadius);
    EmberApsFrame *apsFrame;
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;
    apsFrame->destinationEndpoint = GP_ENDPOINT;
    uint8_t retval;
    if (gpdCommDataSaved.doNotSendGpPairing) {
      #ifdef EMBER_AF_PLUGIN_GREEN_POWER_CLIENT
      // if doNotSendGpPairing is set, just loop it back to self to the proxy on the same node.
      // that will add the proxy table to do diect gpdf forwarding.
      retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, emberAfGetNodeId());
      emberAfGreenPowerClusterPrintln("Gp Pairing UCAST to self returned %d", retval);
      #endif
    } else {
      retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
      emberAfGreenPowerClusterPrintln("Gp Pairing BCAST send returned %d", retval);
    }
    // Clear the following flag unconditionally as that only gets set in one path of Gp Pairing Configuration Command
    gpdCommDataSaved.doNotSendGpPairing = false;

    // Send GpPairing Configuration Based on Sink Communication mode
    sendGpPairingConfigBasedOnSinkCommunicationMode(gpdAddr);
    // If a pairing was added, the sink SHALL send a Device_annce command
    // for the alias (with the exception of lightweight unicast communication mode).
    if (sinkCommunicationMode != EMBER_GP_SINK_TYPE_LW_UNICAST) {
      sendDeviceAnncement(sinkEntry.assignedAlias);
    }
  } else {
    // Entry could not be created - no action
  }
}
// this function is used to test if sink can support needed translation
// and also to add the entries to TT. The reason is that for a bidirectional
// commissioning the commissioning reply goes out just by testing it but adds
// the TT after the sink is supported
static bool handleGpdMatchingAndAddingTTEntry(EmberGpAddress gpdAddr,
                                              bool addTTEntry)
{
  // Relation between GP deviceID and Translation table content creation
  // for example : GPD_DeviceID 0x02 (on/off)
  // means TT entries ZbProfile ZHA, ZbCluster 0x0006, ZbCmd ON+OFF+TOGGLE,
  // for Gps endpoint which match this cluster On/Off 0x0006
  uint8_t gpdMatch;
  SupportedGpdCommandClusterEndpointMap gpdCommandClusterEpMap[50] = { 0 };
  gpdMatch = emGpIsToBePairedGpdMatchSinkFunctionality(gpdCommDataSaved.applicationInfo,
                                                       gpdCommandClusterEpMap);
  if (!addTTEntry) {
    // just test but do not add.
    return (gpdMatch ? true : false);
  }
  // In case the sink supports the gpd parameters, create an TT entry
  // for each command
  EmGpCommandTranslationTable * gptranslationtable = emGpTransTableGetTranslationTable();
  uint8_t totalTableEntriesBefore = gptranslationtable->totalNoOfEntries;
  for (uint8_t cmdIndex = 0; cmdIndex < gpdMatch; cmdIndex++) {
    for (uint8_t i = 0; i < gpdCommandClusterEpMap[cmdIndex].numberOfEndpoints; i++) {
      emberAfGreenPowerClusterPrintln("gpdCommandClusterEpMap[%d].gpdCommand = %d gpdCommandClusterEpMap[%d].endpoints[%d] = %d",
                                      cmdIndex,
                                      gpdCommandClusterEpMap[cmdIndex].gpdCommand,
                                      cmdIndex,
                                      i,
                                      gpdCommandClusterEpMap[cmdIndex].endpoints[i]);
      pairingDoneThusSetCustomizedTranslationTable(&gpdAddr,
                                                   gpdCommandClusterEpMap[cmdIndex].gpdCommand,
                                                   gpdCommandClusterEpMap[cmdIndex].endpoints[i]);
    }
  }
  uint8_t totalTableEntriesAfter = gptranslationtable->totalNoOfEntries;
  gpdMatch = totalTableEntriesAfter - totalTableEntriesBefore; //Get Actual entries added to the table
  return (gpdMatch ? true : false);
}

static void handleClosingCommissioningSessionOnFirstPairing(void)
{
  uint8_t gpsCommissioningExitMode;
  EmberAfStatus statusExitMode;
  EmberAfAttributeType type;
  statusExitMode = emberAfReadAttribute(GP_ENDPOINT,
                                        ZCL_GREEN_POWER_CLUSTER_ID,
                                        ZCL_GP_SERVER_GPS_COMMISSIONING_EXIT_MODE_ATTRIBUTE_ID,
                                        (CLUSTER_MASK_SERVER),
                                        (uint8_t*)&gpsCommissioningExitMode,
                                        sizeof(uint8_t),
                                        &type);
  if (statusExitMode == EMBER_ZCL_STATUS_SUCCESS
      && (gpsCommissioningExitMode & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)) {
    // if commissioning mode is ON and received frame set it to OFF, exit comissioning mode
    commissioningState.inCommissioningMode = false;

    // commissioning session ended here,clean up stored information
    resetOfMultisensorDataSaved(true);

    if (commissioningState.proxiesInvolved) {
      uint8_t retval;
      uint8_t proxyOptions = 0;
      emberAfFillCommandGreenPowerClusterGpProxyCommissioningModeSmart(proxyOptions,
                                                                       0,
                                                                       0);
      EmberApsFrame *apsFrame;
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;
      apsFrame->destinationEndpoint = GP_ENDPOINT;
      retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
      emberAfGreenPowerClusterPrintln("SinkCommissioningModeCallback(Exit) send returned %d", retval);
      // add a call for user application to let it know that the commissioning session ends
      // for application perspective this should be as same as a commissioning windows tiemout expiration
      // then the following user callback is used:
      emberAfGreenPowerServerCommissioningTimeoutCallback(COMMISSIONING_TIMEOUT_TYPE_COMMISSIONING_WINDOW_TIMEOUT,
                                                          noOfCommissionedEndpoints,
                                                          commissionedEndPoints);
    }
  }
}

static void finalisePairing(EmberGpAddress gpdAddr,
                            uint8_t gpdfOptions,
                            uint8_t gpdfExtendedOptions,
                            uint32_t outgoingFrameCounter,
                            uint8_t securityLevel,
                            uint8_t securityKeyType,
                            EmberGpApplicationInfo appInfoStruct)
{
  // Check if the Sink can support the functionality of GPD
  if (handleGpdMatchingAndAddingTTEntry(gpdAddr, false)) {
    // If bidirectional flag is set, queue a Commissioning Reply
    // with Gp Response.
    if (gpdCommDataSaved.reportCollectonState == GP_SINK_COMM_STATE_SEND_COMM_REPLY) {
      emberAfGreenPowerClusterPrintln("Sink Supports GPD - Calling GP Response");
      const EmberKeyData linkKey = { GP_DEFAULT_LINK_KEY };
      // Finalize the sink entry upon reception of success
      handleBidirectionalCommissioningGpdf(&gpdAddr,
                                           securityLevel,
                                           gpdfExtendedOptions,
                                           gpdfOptions,
                                           &linkKey,
                                           &gpdCommDataSaved.key,
                                           outgoingFrameCounter,
                                           gpdCommDataSaved.gppShortAddress);
      gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_WAIT_FOR_SUCCESS;
    } else if (gpdCommDataSaved.reportCollectonState == GP_SINK_COMM_STATE_FINALISE_PAIRING) {
      emberAfGreenPowerClusterPrintln("Calling Adding TT and Sink ");
      handleGpdMatchingAndAddingTTEntry(gpdAddr, true);
      handleSinkEntryAndPairing(gpdAddr,
                                gpdfOptions,
                                gpdfExtendedOptions,
                                outgoingFrameCounter,
                                securityLevel,
                                securityKeyType,
                                appInfoStruct);
      handleClosingCommissioningSessionOnFirstPairing();
      gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_PAIRING_DONE;
      // Call the user to inform the pairing is finalised.
      emberAfGreenPowerServerPairingCompleteCallback(noOfCommissionedEndpoints,
                                                     commissionedEndPoints);
    }
  } else {
    emberAfGreenPowerClusterPrintln("Finalize Pairing : Could not Add TT");
  }
}

static bool receivedKey(EmberGpAddress gpdAddr)
{
  if (emberAfGreenPowerCommonGpAddrCompare(&gpdCommDataSaved.addr, &gpdAddr)
      && (gpdCommDataSaved.gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT)) {
    // last saved gpd is the current Gpd with received key
    return true;
  }
  return false;
}
static bool gpdAddrZero(EmberGpAddress * gpdAddr)
{
  EmberGpAddress gpdAddrZero = { 0 };
  gpdAddrZero.applicationId = gpdAddr->applicationId;
  gpdAddrZero.endpoint = gpdAddr->endpoint;
  return emberAfGreenPowerCommonGpAddrCompare(&gpdAddrZero, gpdAddr);
}
static bool gpCommissioningNotificationCommissioningGpdf(uint16_t commNotificationOptions,
                                                         EmberGpAddress gpdAddr,
                                                         uint32_t gpdSecurityFrameCounter,
                                                         uint8_t gpdCommandId,
                                                         uint8_t* gpdCommandPayload,
                                                         uint16_t gppShortAddress,
                                                         uint8_t rssi,
                                                         uint8_t linkQuality,
                                                         uint8_t gppDistance,
                                                         uint32_t commissioningNotificationMic)
{
  emberAfGreenPowerClusterPrintln("Process GP CN");
  if (!(commissioningState.inCommissioningMode)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : Sink not in commissioning!");
    return true;
  }
  // Drop for the gpd addr 0
  if (gpdAddrZero(&gpdAddr)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : GPD Address is 0!");
    return true;
  }
  EmberGpApplicationInfo appInfoStruct = { 0 };
  uint8_t index = 1; // gpdCommandPayload [0] is length, hence set to 1
  appInfoStruct.deviceId = gpdCommandPayload[index++];
  uint8_t gpdfOptions = gpdCommandPayload[index++];
  // parse option and ext option from gpdCommandPayload of the commissioning command
  uint8_t gpdfExtendedOptions = 0;
  if (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_EXTENDED_OPTIONS_FIELD) {
    gpdfExtendedOptions = gpdCommandPayload[index++];
  }
  // Parse security key type and level
  uint8_t gpsSecurityLevelAttribut = 0;
  EmberAfStatus status;
  EmberAfAttributeType type;
  status = emberAfReadAttribute(GP_ENDPOINT,
                                ZCL_GREEN_POWER_CLUSTER_ID,
                                ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                (CLUSTER_MASK_SERVER),
                                (uint8_t*)&gpsSecurityLevelAttribut,
                                sizeof(uint8_t),
                                &type);
  // Reject the req if InvolveTC is set in the attribute
  if (gpsSecurityLevelAttribut & 0x08) { // Involve TC is set in the Sink
    emberAfGreenPowerClusterPrintln("DROP - GP CN : Involve TC bit is set in gpsSecurityLevelAttribute!");
    return true;
  }

  uint8_t securityLevel = (gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_SECURITY_LEVEL_CAPABILITIES);

  if (securityLevel == EMBER_GP_SECURITY_LEVEL_RESERVED
      || (status == EMBER_ZCL_STATUS_SUCCESS
          && (((gpsSecurityLevelAttribut & 0x03) > securityLevel) // GPD Security Level is lower than Sink supports
              || ((gpsSecurityLevelAttribut & 0x04) // Sink requires the GPD to support key encryption
                  && !(gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION))))) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : Sec Level < gpsSecurityLevelAttribute!");
    return true;
  }
  uint8_t securityKeyType = ((gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE)
                             >> EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
  // If security Level is none 0 and Gpd is in commissioning process currently
  // It has following combinitions for key negotiation
  // Unidirectional (rxAfterTx = 0) : shall send a key in one of the commissioning frames (current or last one)
  //                                  if it does not have then drop it.
  // Bidirectional  (rxAfterTx = 1) : may send a key and/or request one
  if (securityLevel != 0
      && !receivedKey(gpdAddr)  // has it received key from GPD in past, for first commissioning frame - this will be false
      && !(gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT)) { // No Key in this frame
    emberAfGreenPowerClusterPrintln("Sec Level > 0, No Key Received Yet");
    if (!(commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)) {
      emberAfGreenPowerClusterPrintln("DROP - GP CN : No Key Present, Unidirectional RxAfterTx = 0");
      return true;
    } else {
      // RxAfterTx is set but no request key, then Drop
      if (!(gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_GP_SECURITY_KEY_REQUEST)) {
        emberAfGreenPowerClusterPrintln("DROP - GP CN : No Key Request, Bidirectional RxAfterTx = 1");
        return true;
      }
    }
  }
  EmberKeyData key = { 0 };
  if (gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT) {
    MEMMOVE(key.contents, (gpdCommandPayload + index), EMBER_ENCRYPTION_KEY_SIZE);
    index += EMBER_ENCRYPTION_KEY_SIZE;
    // If the incoming key is encrypted, check its authenticity
    if (gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION) {
      EmberKeyData linkKey = { GP_DEFAULT_LINK_KEY };
      uint8_t mic[4] = { 0 };
      // Read in reverse for comparison with generated calculated mic array
      uint32_t keyMic = emberFetchLowHighInt32u(gpdCommandPayload + index);
      index += 4;
      // Decrypt and derive the incomming Key
      emGpKeyTcLkDerivation(&gpdAddr,
                            0,
                            mic,
                            &linkKey,
                            &key,
                            true);
      emberAfGreenPowerClusterPrintln("rx MIC %4x", keyMic);
      emberAfGreenPowerClusterPrint("Calculated MIC :");
      emberAfGreenPowerClusterPrintBuffer(mic, 4, 0);
      emberAfGreenPowerClusterPrint("\nKey : ");
      emberAfGreenPowerClusterPrintBuffer(key.contents, 16, 0);
      emberAfGreenPowerClusterPrintln("");
      if (0 != MEMCOMPARE(mic, (uint8_t*)(&keyMic), 4)) {
        emberAfGreenPowerClusterPrintln("DROP - GP CN : MIC mismatch");
        return true;
      }
    }
  }
  // Initialize framecounter with gpdSecurityFrameCounter,
  // then if gpdf has security info pick it from gpd command payload.
  uint32_t outgoingFrameCounter = gpdSecurityFrameCounter;
  if (gpdfExtendedOptions & EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_OUTGOING_COUNTER_PRESENT) {
    outgoingFrameCounter = emberFetchLowHighInt32u(gpdCommandPayload + index);
    index += 4;
  }

  if (gpdfOptions & EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_APPLICATION_INFORMATION_PRESENT) {
    appInfoStruct.applInfoBitmap = gpdCommandPayload[index++];
  }

  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_MANUFACTURE_ID_PRESENT) {
    appInfoStruct.manufacturerId = emberFetchLowHighInt16u(gpdCommandPayload + index);
    index += 2;
  }
  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_MODEL_ID_PRESENT) {
    appInfoStruct.modelId = emberFetchLowHighInt16u(gpdCommandPayload + index);
    index += 2;
  }
  // Parse the Commands and flag the generic command
  bool genericCmdPresent = false;
  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_COMMANDS_PRESENT) {
    appInfoStruct.numberOfGpdCommands = gpdCommandPayload[index++];
    for (int i = 0; i < appInfoStruct.numberOfGpdCommands; i++) {
      appInfoStruct.gpdCommands[i] = gpdCommandPayload[index];
      if ((appInfoStruct.gpdCommands[i] == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS)
          || (appInfoStruct.gpdCommands[i] == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE)) {
        genericCmdPresent = true;
      }
      index++;
    }
  }
  // Parse the cluster list
  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) {
    uint8_t numClusters = gpdCommandPayload[index++];
    appInfoStruct.numberOfGpdServerCluster = numClusters & 0x0F;
    for (int i = 0; i < appInfoStruct.numberOfGpdServerCluster; i++) {
      appInfoStruct.serverClusters[i] = emberFetchLowHighInt16u(gpdCommandPayload + index);
      index += 2;
    }
    appInfoStruct.numberOfGpdClientCluster = numClusters >> 4;
    for (int i = 0; i < appInfoStruct.numberOfGpdClientCluster; i++) {
      appInfoStruct.clientClusters[i] = emberFetchLowHighInt16u(gpdCommandPayload + index);
      index += 2;
    }
  }

  EmberGpSwitchInformation switchInformationStruct = { 0 };
  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT) {
    switchInformationStruct.switchInfoLength = gpdCommandPayload[index++];
    switchInformationStruct.nbOfContacts = (gpdCommandPayload[index]
                                            & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_NB_OF_CONTACT);
    switchInformationStruct.switchType = ((gpdCommandPayload[index++]
                                           & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE)
                                          >> EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET);
    switchInformationStruct.currentContact = gpdCommandPayload[index++];
  }

  if (index > gpdCommandPayload[0] + 1) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : Short payload");
    //we ran off the end of the payload
    return true;
  }
  if (emberAfGreenPowerCommonGpAddrCompare(&gpdCommDataSaved.addr, &gpdAddr) == false) {
    resetOfMultisensorDataSaved(true);
    emberAfGreenPowerClusterPrintln("GP CN : Saving the GPD");
    MEMCOPY(&(gpdCommDataSaved.addr), &gpdAddr, sizeof(EmberGpAddress));
    gpdCommDataSaved.securityKeyType = securityKeyType;
    gpdCommDataSaved.securityLevel = securityLevel;
    MEMCOPY(gpdCommDataSaved.key.contents, key.contents, EMBER_ENCRYPTION_KEY_SIZE);
    gpdCommDataSaved.gpdfOptions = gpdfOptions;
    gpdCommDataSaved.gpdfExtendedOptions = gpdfExtendedOptions;
    MEMCOPY(&(gpdCommDataSaved.switchInformationStruct), &switchInformationStruct, sizeof(EmberGpSwitchInformation));
    MEMCOPY(&(gpdCommDataSaved.applicationInfo), &appInfoStruct, sizeof(EmberGpApplicationInfo));
  }
  // Update the frame counter in all scenarios
  gpdCommDataSaved.outgoingFrameCounter = outgoingFrameCounter;
  // If there is a sink already then update the security frame counter
  uint8_t sinkIndex = emberGpSinkTableLookup(&gpdAddr);
  if (sinkIndex != 0xFF) {
    EmberGpSinkTableEntry entry = { 0 };
    EmberStatus status = emberGpSinkTableGetEntry(sinkIndex, &entry);
    if (status == EMBER_SUCCESS) {
      emberAfGreenPowerClusterPrintln("GP CN : Sink exists, update frame counter");
      gpGpSinkTableSetSecurityFrameCounter(sinkIndex, outgoingFrameCounter);
    }
  }
  // Use the sinkDefault communication mode as the Gpdf has no option to send one
  gpdCommDataSaved.communicationMode = EMBER_GP_SINK_TYPE_UNUSED;

  if ((appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT)
      && (appInfoStruct.deviceId == EMBER_GP_DEVICE_ID_GPD_GENERIC_SWITCH
          || genericCmdPresent == true)) {
    // on each reception of a Generic Switch commissioning command start a 60s
    // timeout to collect the PRESS and RELEASE order frames to extend
    // GenericSwitch Translation Table created for this GPD
    emberAfGreenPowerClusterPrintln("GP CN : Start Switch Commissioning Timeout");
    uint32_t delay = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_GENERIC_SWITCH_COMMISSIONING_TIMEOUT_IN_SEC;
    emberEventControlSetDelayMS(emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl,
                                delay * MILLISECOND_TICKS_PER_SECOND);
  }

  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS
      && commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : rxAfterTx and Application Description follows both are set");
    return true;
  }

  if (gpdCommDataSaved.reportCollectonState == GP_SINK_COMM_STATE_COLLECT_REPORTS
      && commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN : rxAfterTx while report collection in progress");
    return true;
  }

  if (appInfoStruct.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS) {
    if (!emberEventControlGetActive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl)) {
      emberAfGreenPowerClusterPrintln("GP CN : Reseting Partial data - Only Report Data");
      resetOfMultisensorDataSaved(false);
    }
    emberAfGreenPowerClusterPrintln("GP CN : Start MS/CAR Commissioning Timeout and waiting for ADCF");
    uint32_t delay = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
    emberEventControlSetDelayMS(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl,
                                delay * MILLISECOND_TICKS_PER_SECOND);
    // All set to collect the report descriptors
    gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
    return true;
  }
  // Check if bidirectional commissioning, flag the state to finalize the pairing.
  if (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    // Send Commissioning reply and finalize the sink entry upon reception of success
    gpdCommDataSaved.gppShortAddress = gppShortAddress;
    gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_SEND_COMM_REPLY;
  } else {
    // No reports to collect AND no commReply to be sent, then finalise pairing
    gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
  }
  // Based on the states the following function either sends a commReply or finalises pairing
  finalisePairing(gpdAddr,
                  gpdfOptions,
                  gpdfExtendedOptions,
                  outgoingFrameCounter,
                  securityLevel,
                  securityKeyType,
                  appInfoStruct);
  return true;
}

static uint8_t reportLength(uint8_t * inReport)
{
  uint8_t length = 0; // 0 - position of reportId
  length += 1;  // 1 - position of reportOption
  if (inReport[length]
      & EMBER_AF_GP_GPD_APPLICATION_DESCRIPTION_COMMAND_REPORT_OPTIONS_TIMEOUT_PERIOD_PRESENT) {
    length += 3; // 2,3 - position of timeout and 4 position of length
  } else {
    length += 1; // 2 position of length excluding itself
  }
  length += inReport[length]; // sum up length of report descriptor remaining length
  length += 1; // add the length byte itself
  return length;
}

static uint8_t * findReportId(uint8_t reportId,
                              uint8_t numberOfReports,
                              uint8_t * reports)
{
  uint8_t * temp = reports;
  uint8_t length = 0; // 0 - position of reportId
  for (uint8_t noOfReport = 0; noOfReport < numberOfReports; noOfReport++) {
    // calculate the length of each report in the payload
    if (*temp == reportId) {
      return temp;
    }
    length = reportLength(temp);
    temp = temp + length;
  }
  return NULL;
}

static bool saveReportDescriptor(uint8_t totalNumberOfReports,
                                 uint8_t numberOfReports,
                                 uint8_t * reports)
{
  if ((totalNumberOfReports == 0)
      || ((gpdCommDataSaved.numberOfReports == totalNumberOfReports)
          && (gpdCommDataSaved.numberOfReports == gpdCommDataSaved.totalNbOfReport))) {
    return true; // the report was already collected
  }
  uint8_t * temp = reports;
  uint8_t length = 0; // 0 - position of reportId
  for (uint8_t noOfReport = 0; noOfReport < numberOfReports; noOfReport++) {
    length = reportLength(temp);
    // Negative test - drop the packet if the record is of bad length - 4.4.1.12
    // Minimum length for 1 report that has 1 option record in that 1 attribute record =
    // reportId(1) + option(1) + length(1) of option records
    // Option Record = option (1) + clusterid (2)
    // Attribute record = attrId (2) + datatype (1) + att option(1)
    if (length < GREEN_POWER_SERVER_MIN_REPORT_LENGTH ) {
      // Error , so return without saving
      return false;
    }
    // The report is not already collected
    if (NULL ==  findReportId(*temp,
                              gpdCommDataSaved.numberOfReports,
                              gpdCommDataSaved.reportsStorage)) {
      // now save the entire payload, appending to last received reports if any
      MEMCOPY(&gpdCommDataSaved.reportsStorage[gpdCommDataSaved.lastIndex],
              temp,
              length);
      gpdCommDataSaved.numberOfReports++;
      gpdCommDataSaved.lastIndex += length;
      gpdCommDataSaved.totalNbOfReport = totalNumberOfReports;
    } else {
      emberAfGreenPowerClusterPrintln("Duplicate ReportId %d - Not Saved", *temp);
    }
    temp = temp + length;
  }
  return true;
}

uint8_t * findDataPointDescriptorInReport(uint8_t dataPointDescriptorId,
                                          uint8_t * report)
{
  uint8_t remainingLength;
  uint8_t * finger;
  if (report[1]) {
    remainingLength = report[4]; // with timeout
    finger = &report[5];
  } else {
    remainingLength = report[2]; // without timeout
    finger = &report[3];
  }
  uint8_t tempDatapointDescrId = 0;
  for (int index = 0; index < remainingLength - 1; index++) {
    // travel through each data point descriptor from here until the input is
    // found
    if (tempDatapointDescrId == dataPointDescriptorId) {
      return &finger[index];
    }
    uint8_t datapointOptions = finger[index];
    index += 3 + ((datapointOptions & 0x10) ? 2 : 0); //for ManufactureId
    // traverse the attribute records in each data point
    for (int i = 0; i < ((datapointOptions & 0x07) + 1); i++) {
      index += 3;
      uint8_t attributeOptions = finger[index];
      index += ((attributeOptions & 0x0f) + 1);
    }
    tempDatapointDescrId++;
  }
  return NULL;
}

// This function, validates and changes the option to fit the check if success
static bool manufactureIdValidation(uint8_t * attributeOptions,
                                    uint16_t manufacturerID,
                                    uint16_t clusterId,
                                    uint16_t attributeId)
{
  // Check if the cluster or attribute is in range of Manufacture Specific range
  if (((*attributeOptions) & 0x02)
      && (isClusterInManufactureSpeceficRange(clusterId)
          || isAttributeInManufactureSpecificRange(attributeId))) {
    // Check if the manufactureId is supported in sink for the attribute/cluster
    if (manufacturerID != EMBER_AF_MANUFACTURER_CODE) {
      // manufacturId present but does not match the manufactureCode in sink
      // TODO : call emberAfLocateAttributeMetadata to locate if any such
      // attribute of a cluster is present in sink or not.
      return false;
    } else {
      return true;
    }
  }
  // Neither the cluster not attribute is manufacture specific, hence ensure the
  // manufacture specific flag in option;
  *attributeOptions &= ~(0x02);
  return true;
}

static void addReportstoTranslationTable(EmberGpAddress * gpdAddr,
                                         uint8_t gpdCommandId,
                                         uint8_t endpoint)
{
  // none entry match, create TT entries for New GPD
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField additionalInfo = { 0 };
  uint8_t additionalInfoReportLen = 0;
  uint8_t additionalInfoRecordLen = 0;
  uint8_t additionalInfoAttrLen = 0;
  uint8_t noOfattributes = 0;
  uint8_t length = 0;
  uint8_t reportLength = 0;
  uint8_t recordRemainingLength = 0;
  uint8_t attributeRemainingLength = 0;
  uint8_t outIndex = 0xFF;

  uint8_t index = 0;
  uint8_t * report = NULL;
  // Populate all the reports as option records of the additional block
  for (index = 0; index < gpdCommDataSaved.numberOfReports; index++) {
    report = findReportId(index,
                          gpdCommDataSaved.numberOfReports,
                          gpdCommDataSaved.reportsStorage);
    if (report != NULL) {
      length = 0;
      reportLength = 0;
      additionalInfoReportLen = 0;

      additionalInfo.optionData.compactAttr.reportIdentifier = report[length]; //reportId
      length += 1; //reportId
      additionalInfoReportLen++;
      if (report[length] & EMBER_AF_GP_GPD_APPLICATION_DESCRIPTION_COMMAND_REPORT_OPTIONS_TIMEOUT_PERIOD_PRESENT) {
        length += 3; // option and 2 byte timeout
      } else {
        length += 1; // just the option byte
      }
      recordRemainingLength = report[length];
      length += 1; // length
      additionalInfoReportLen++;

      reportLength = length; // length
      do {
        emberAfPrintBuffer(EMBER_AF_PRINT_CORE, &report[length - 4], (recordRemainingLength + 4), true);
        emberAfGreenPowerClusterPrintln("");
        additionalInfoRecordLen = 0;
        uint8_t tempAttributeOption = ((report[length] & 0x1F) >> 3); // attribute option
        noOfattributes = ( (report[length] & 0x07) + 1);
        length += 1; //data point options
        additionalInfoRecordLen++;
        additionalInfo.optionData.compactAttr.clusterID = report[length] + ((uint16_t)report[length + 1] << 8);
        length += 2; // clusterid
        additionalInfoRecordLen += 2;
        if (tempAttributeOption & 0x02) { // ManufactureId present
          additionalInfo.optionData.compactAttr.manufacturerID = report[length] + ((uint16_t)report[length + 1] << 8);
          length += 2;
        }
        additionalInfoRecordLen = (additionalInfoReportLen + additionalInfoRecordLen);
        for (uint8_t attrRecordIndex = 0; attrRecordIndex < noOfattributes; attrRecordIndex++) {
          additionalInfo.optionData.compactAttr.attributeOptions = tempAttributeOption;
          additionalInfoAttrLen = 0;
          additionalInfo.optionData.compactAttr.attributeID = report[length] + ((uint16_t)report[length + 1] << 8);
          length += 2;
          additionalInfoAttrLen += 2;
          additionalInfo.optionData.compactAttr.attributeDataType = report[length];
          length += 1;
          additionalInfoAttrLen++;
          uint8_t reportDescAttributeOption = report[length];
          length += 1;
          attributeRemainingLength = (reportDescAttributeOption & 0x0F) + 1;
          if (reportDescAttributeOption & 0x10) {
            if ((attributeRemainingLength) && (recordRemainingLength > (length - reportLength))) {
              additionalInfo.optionData.compactAttr.attrOffsetWithinReport = report[length];
              length += 1;
              attributeRemainingLength--;
            }
          } else {
            additionalInfo.optionData.compactAttr.attrOffsetWithinReport = 0x00;
          }
          additionalInfoAttrLen++;
          if (reportDescAttributeOption & 0x20) {
            if ((attributeRemainingLength) && (recordRemainingLength > (length - reportLength))) {
              length += attributeRemainingLength;
            }
          }
          outIndex = 0xFF;
          if (reportDescAttributeOption & 0x10) { // Reported bit set
            if (endpointAndClusterIdValidation(endpoint,
                                               !(additionalInfo.optionData.compactAttr.attributeOptions & 0x01), // Mask for server/Client
                                               additionalInfo.optionData.compactAttr.clusterID)
                && manufactureIdValidation(&(additionalInfo.optionData.compactAttr.attributeOptions),
                                           additionalInfo.optionData.compactAttr.manufacturerID,
                                           additionalInfo.optionData.compactAttr.clusterID,
                                           additionalInfo.optionData.compactAttr.attributeID)) {
              if ((additionalInfo.optionData.compactAttr.attributeOptions) & 0x02) {
                additionalInfoRecordLen += 2;
              }
              additionalInfo.optionSelector = (additionalInfoRecordLen + additionalInfoAttrLen - additionalInfoReportLen);//(endRecord - startRecord - 1);
              emberAfGreenPowerClusterPrintln(" AdditionalInfo.optionSelector : %d additionalInfoReportdLen = %d additionalInfoRecordLen = %d additionalInfoAttrLen",
                                              additionalInfo.optionSelector,
                                              additionalInfoReportLen,
                                              additionalInfoRecordLen,
                                              additionalInfoAttrLen);
              additionalInfo.totalLengthOfAddInfoBlock = (additionalInfoRecordLen + additionalInfoAttrLen);

              uint8_t status = 0xFF;
              uint8_t outIndex = 0xFF;
              status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                                        | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID
                                                                        | GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT
                                                                        | GP_TRANSLATION_TABLE_SCAN_LEVEL_ADDITIONAL_INFO_BLOCK),//uint8_t levelOfScan,
                                                                       true, //bool infoBlockPresent,
                                                                       gpdAddr,
                                                                       EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING,
                                                                       endpoint,//uint8_t zbEndpoint,
                                                                       NULL,//uint8_t * GpdCmdPayload,
                                                                       &additionalInfo,
                                                                       &outIndex,
                                                                       0);
              if (status != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
                status = emGpTransTableAddPairedDeviceToTranslationTable(ADD_PAIRED_DEVICE,
                                                                         true,
                                                                         gpdAddr,
                                                                         EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING,
                                                                         endpoint,
                                                                         0,
                                                                         additionalInfo.optionData.compactAttr.clusterID,
                                                                         0,
                                                                         0,
                                                                         NULL,
                                                                         0,
                                                                         additionalInfo.totalLengthOfAddInfoBlock,
                                                                         &additionalInfo,
                                                                         &outIndex);
                emberAfGreenPowerClusterPrintln("Status %d attrRecord index = %d ", status, attrRecordIndex);
              }
            }
          }
        }
        emberAfGreenPowerClusterPrintln("record index = %d ", index);
      } while (recordRemainingLength > (length - reportLength));
    }
  }
}

static bool gpCommissioningNotificationApplicationDescriptionGpdf(uint16_t commNotificationOptions,
                                                                  EmberGpAddress gpdAddr,
                                                                  uint32_t gpdSecurityFrameCounter,
                                                                  uint8_t gpdCommandId,
                                                                  uint8_t* gpdCommandPayload,
                                                                  uint16_t gppShortAddress,
                                                                  uint8_t rssi,
                                                                  uint8_t linkQuality,
                                                                  uint8_t gppDistance,
                                                                  uint32_t commissioningNotificationMic)
{
  static bool lastReportIdHasRxAfterTx = false;
  emberAfGreenPowerClusterPrintln("GP CN : Process ADCF");
  // Process only if the application description is from the same gpd that sent the E0
  // commissioning cmd previously and in the report collection state.
  if (!emberAfGreenPowerCommonGpAddrCompare(&gpdCommDataSaved.addr, &gpdAddr)) {
    emberAfGreenPowerClusterPrintln("ADCF DROP : Not from Commissioning gpd");
    return true;
  }
  if (gpdCommDataSaved.reportCollectonState != GP_SINK_COMM_STATE_COLLECT_REPORTS
      && !lastReportIdHasRxAfterTx) {
    emberAfGreenPowerClusterPrintln("ADCF DROP : Not in report collection state (only allow bidirectional frames)");
    return true;
  }
  // Process this command if the MS timer is still running that was started in a commissioning gpdf
  // else reject command or the node has collected the reports just allow the bidirectional frames to pull a
  // commissioning reply out
  if (!emberEventControlGetActive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl)
      && !lastReportIdHasRxAfterTx) {
    emberAfGreenPowerClusterPrintln("ADCF DROP : MS timer has expired");
    return true;
  }
  // The first Application description frame received,start the timeout
  if (gpdCommDataSaved.totalNbOfReport == 0) {
    lastReportIdHasRxAfterTx = false;
    // start a timeout to collect the ReportDescriptors thanks to the application description frames
    uint32_t delay =  EMBER_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
    emberEventControlSetDelayMS(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl,
                                delay * MILLISECOND_TICKS_PER_SECOND);
  }
  // Collect the report descriptors in RAM with short payload error check
  if (!saveReportDescriptor(gpdCommandPayload[1],    // Total Number of Reports
                            gpdCommandPayload[2],    // Number of reports in this frame
                            &gpdCommandPayload[3])) { // Start of report data
    // Error in Application Description payload
    emberAfGreenPowerClusterPrintln("ADCF DROP : Error in Report Desc.");
    return true;
  }
  // just remeber if the lastfarme during the report collection has indicated a rx capability when out of order
  if ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX)
      && findReportId((gpdCommandPayload[1] - 1),
                      gpdCommandPayload[2],
                      &gpdCommandPayload[3]) != NULL) {
    lastReportIdHasRxAfterTx = true;
  }
  // Check if all the reports received else keep collecting
  if (gpdCommDataSaved.numberOfReports != gpdCommDataSaved.totalNbOfReport) {
    emberAfGreenPowerClusterPrintln("ADCF : Wait for % d more reports",
                                    (gpdCommDataSaved.totalNbOfReport - gpdCommDataSaved.totalNbOfReport));
    return true;
  }
  emberAfGreenPowerClusterPrintln("ADCF : Collected All the reports -- looking for last reportId");
  // Find the last reportId (= total reports - 1) in this frame it it was decided that it was a bidirectional
  // with rx capability
  if (lastReportIdHasRxAfterTx
      && findReportId((gpdCommandPayload[1] - 1),
                      gpdCommandPayload[2],
                      &gpdCommandPayload[3]) == NULL) {
    return true;
  }
  emberAfGreenPowerClusterPrintln("ADCF : Last reportId found in this frame");
  emberEventControlSetInactive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl);
  // If this frame with last reportId has the rxAfterTx set send a commissioning reply and the pairing to be finalized
  // in Success
  if (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) {
    // Send Commissioning reply and finalize the sink entry upon reception of success
    gpdCommDataSaved.gppShortAddress = gppShortAddress;
    gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_SEND_COMM_REPLY;
  } else {
    gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
  }

  finalisePairing(gpdAddr,
                  gpdCommDataSaved.gpdfOptions,
                  gpdCommDataSaved.gpdfExtendedOptions,
                  gpdCommDataSaved.outgoingFrameCounter,
                  gpdCommDataSaved.securityLevel,
                  gpdCommDataSaved.securityKeyType,
                  gpdCommDataSaved.applicationInfo);
  return true;
}

static bool gpCommissioningNotificationSuccessGpdf(uint16_t commNotificationOptions,
                                                   EmberGpAddress gpdAddr,
                                                   uint32_t gpdSecurityFrameCounter,
                                                   uint8_t gpdCommandId,
                                                   uint8_t* gpdCommandPayload,
                                                   uint16_t gppShortAddress,
                                                   uint8_t rssi,
                                                   uint8_t linkQuality,
                                                   uint8_t gppDistance,
                                                   uint32_t commissioningNotificationMic)
{
  // Ensure Application Description Follows flag is not set  follows
  emberAfGreenPowerClusterPrintln("GP CN : Process Success");
  if (!(commissioningState.inCommissioningMode)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success : Sink not in commissioning!");
    return true;
  }
  // Drop for the gpd addr 0
  if (gpdAddrZero(&gpdAddr)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success: GPD Address is 0!");
    return true;
  }

  // Drop for the different gpdf than the one in commissioning
  if (!emGpAddressMatch(&gpdCommDataSaved.addr, &gpdAddr)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success: GPD Address different than the one in commissioning!");
    return true;
  }

  uint8_t gpsSecurityLevelAttribut = 0;
  EmberAfStatus status;
  EmberAfAttributeType type;
  status = emberAfReadAttribute(GP_ENDPOINT,
                                ZCL_GREEN_POWER_CLUSTER_ID,
                                ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                (CLUSTER_MASK_SERVER),
                                (uint8_t*)&gpsSecurityLevelAttribut,
                                sizeof(uint8_t),
                                &type);
  uint8_t securityLevel = ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                           >> EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);

  if (securityLevel == EMBER_GP_SECURITY_LEVEL_RESERVED
      || (status == EMBER_ZCL_STATUS_SUCCESS
          && (gpsSecurityLevelAttribut & 0x03) > securityLevel)
      || (securityLevel != gpdCommDataSaved.securityLevel)) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success: Sec Level mismatch");
    return true;
  }
  // security level has passed so check the FC
  if (gpdCommDataSaved.outgoingFrameCounter > gpdSecurityFrameCounter) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success: Frame Counter Too Low");
    return true;
  }
  // FC is fine, check the MIC
  gpdCommDataSaved.outgoingFrameCounter = gpdSecurityFrameCounter;
  emberAfGreenPowerClusterPrintln("\nGP Success Sec Level %d", securityLevel);
  if (securityLevel >= EMBER_GP_SECURITY_LEVEL_FC_MIC
      && (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_PROCESSING_FAILED)) {
    emberAfPluginGreenPowerServerGpdSecurityFailureCallback(gpdAddr);
    uint8_t mic[4] = { 0 };
    bool securityProcessing = emGpCalculateIncomingCommandMic(&gpdAddr,
                                                              ((gpdCommDataSaved.securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_INDIVIDIGUAL_GPD_KEY) \
                                                               ? EMBER_AF_GREEN_POWER_GP_INDIVIDUAL_KEY : EMBER_AF_GREEN_POWER_GP_SHARED_KEY),
                                                              securityLevel,
                                                              gpdSecurityFrameCounter,
                                                              gpdCommandId,
                                                              gpdCommandPayload,
                                                              ((securityLevel > EMBER_GP_SECURITY_LEVEL_FC_MIC) ? true : false),
                                                              mic,
                                                              &gpdCommDataSaved.key);
    emberAfGreenPowerClusterPrintln("\nReceived Mic = %4x", commissioningNotificationMic);
    emberAfGreenPowerClusterPrint("\nGP Success Sec Level %d App Id %d Returned %x \n Gp Success Calculated Mic : ",
                                  securityLevel,
                                  gpdAddr.applicationId,
                                  securityProcessing);
    for (int i = 0; i < 4; i++) {
      emberAfGreenPowerClusterPrint("%x ", mic[i]);
    }
    if (securityProcessing
        && (mic[3] != ((commissioningNotificationMic >> 24) & 0xFF)
            || mic[2] != ((commissioningNotificationMic >> 16) & 0xFF)
            || mic[1] != ((commissioningNotificationMic >> 8) & 0xFF)
            || mic[0] != ((commissioningNotificationMic >> 0) & 0xFF))) {
      emberAfGreenPowerClusterPrintln("DROP - GP CN Success: - MIC Mismatch");
      return true;
    }
  }
  if (gpdCommDataSaved.reportCollectonState != GP_SINK_COMM_STATE_WAIT_FOR_SUCCESS) {
    emberAfGreenPowerClusterPrintln("DROP - GP CN Success: - Not waiting for Success");
  }
  // Find the sink entry and update the security frame counter from gpd
  uint8_t sinkIndex = emberGpSinkTableLookup(&gpdAddr);
  if (sinkIndex != 0xFF) {
    EmberGpSinkTableEntry entry = { 0 };
    EmberStatus status = emberGpSinkTableGetEntry(sinkIndex, &entry);
    if (status == EMBER_SUCCESS
        && (entry.gpdSecurityFrameCounter < gpdSecurityFrameCounter)) {
      emberAfGreenPowerClusterPrintln("GP CN : Process Success - Update Frame Counter");
      gpGpSinkTableSetSecurityFrameCounter(sinkIndex, gpdSecurityFrameCounter);
    }
  }

  emberAfGreenPowerClusterPrint("GP CN : Process Success - Finalise Pairing");
  gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
  finalisePairing(gpdAddr,
                  gpdCommDataSaved.gpdfOptions,
                  gpdCommDataSaved.gpdfExtendedOptions,
                  gpdCommDataSaved.outgoingFrameCounter,
                  gpdCommDataSaved.securityLevel,
                  gpdCommDataSaved.securityKeyType,
                  gpdCommDataSaved.applicationInfo);
  return true;
}

static bool gpCommissioningNotificationDecommissioningGpdf(uint16_t commNotificationOptions,
                                                           EmberGpAddress gpdAddr,
                                                           uint32_t gpdSecurityFrameCounter,
                                                           uint8_t gpdCommandId,
                                                           uint8_t* gpdCommandPayload,
                                                           uint16_t gppShortAddress,
                                                           uint8_t rssi,
                                                           uint8_t linkQuality,
                                                           uint8_t gppDistance,
                                                           uint32_t commissioningNotificationMic)
{
  uint8_t secLvl = ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                    >> EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET);
  uint8_t keyType = ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_KEY_TYPE)
                     >> EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_KEY_TYPE_OFFSET);
  decommissionGpd(secLvl, keyType, &gpdAddr, true);
  return true;
}

//Green Power Cluster Gp Commissioning Notification
bool emberAfGreenPowerClusterGpCommissioningNotificationCallback(uint16_t commNotificationOptions,
                                                                 uint32_t gpdSrcId,
                                                                 uint8_t* gpdIeee,
                                                                 uint8_t gpdEndpoint,
                                                                 uint32_t gpdSecurityFrameCounter,
                                                                 uint8_t gpdCommandId,
                                                                 uint8_t* gpdCommandPayload,
                                                                 uint16_t gppShortAddress,
                                                                 uint8_t gppLink,
                                                                 uint32_t commissioningNotificationMic)
{
  uint8_t rssi;
  uint8_t linkQuality;
  uint8_t gppDistance;
  uint8_t allZeroesIeeeAddress[17] = { 0 };

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = allZeroesIeeeAddress;
  }

  if (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX ) {
    gppDistance = gppLink;
  } else {
    rssi = gppLink & EMBER_AF_GP_GPP_GPD_LINK_RSSI;
    // since gp 1.0b-14-
    // "The RSSI sub-field of the GPP-GPD link field encodes the RSSI from the range <+8 ; -109> [dBm],
    // with 2dBm granularity.", then
    // 0 represent -109dbm
    // 1 represent -107dbm
    // 2 represent -105dbm
    // ...
    // 54 represent -1dbm
    // 55 represent  0dbm // "110 is add to capped RSSI value, to obtain a non-negative value" gp1.0b 14-
    // 56 represent  2dbm
    // 57 represent  4dbm
    // 58 represent  6dbm
    // 59 represent  8dbm
    // 60 is value not used
    if ( rssi < 55 ) {
      rssi = -109 + 2 * rssi;
    } else {
      rssi = (rssi - 55) * 2;
    }
    linkQuality = (gppLink & EMBER_AF_GP_GPP_GPD_LINK_LINK_QUALITY)
                  >> EMBER_AF_GP_GPP_GPD_LINK_LINK_QUALITY_OFFSET;
  }
  uint8_t tempCommand = gpdCommandId;
  EmberGpAddress gpdAddr;
  if (!emGpMakeAddr(&gpdAddr, (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_APPLICATION_ID), gpdSrcId, gpdIeee, gpdEndpoint)) {
    return false;
  }
  // If the security Level is 3 and security Processing failed is set,
  // the payload is encrypted, so decrypt the commandId so that it can be passed
  // to the following cases.
  uint8_t secLevel = (commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL)
                     >> EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_LEVEL_OFFSET;
  if ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_SECURITY_PROCESSING_FAILED)
      && EMBER_GP_SECURITY_LEVEL_FC_MIC_ENCRYPTED == secLevel) {
    uint8_t paload[GP_MAX_COMMISSIONING_PAYLOAD_SIZE] = { 0 };
    paload[0] = gpdCommandId;
    uint8_t length = 1;
    if (gpdCommandPayload) {
      if (gpdCommandPayload[0] != 0xFF) {
        MEMCOPY((paload + 1), gpdCommandPayload + 1, gpdCommandPayload[0]);
        length += gpdCommandPayload[0];
      }
    }
    emGpCalculateIncomingCommandDecrypt(&gpdAddr,
                                        gpdSecurityFrameCounter,
                                        length,
                                        paload,
                                        &gpdCommDataSaved.key);
    // Pick the command Id after decryption for further processing.
    tempCommand = paload[0];
  }

  // split commissioning notification callback function in following underFunction:
  switch (tempCommand) {
    case EMBER_ZCL_GP_GPDF_COMMISSIONING:
      gpCommissioningNotificationCommissioningGpdf(commNotificationOptions,
                                                   gpdAddr,
                                                   gpdSecurityFrameCounter,
                                                   gpdCommandId,
                                                   gpdCommandPayload,
                                                   gppShortAddress,
                                                   rssi,
                                                   linkQuality,
                                                   gppDistance,
                                                   commissioningNotificationMic);
      break;
    case EMBER_ZCL_GP_GPDF_APPLICATION_DESCRIPTION:
      gpCommissioningNotificationApplicationDescriptionGpdf(commNotificationOptions, // look at this
                                                            gpdAddr,
                                                            gpdSecurityFrameCounter,
                                                            gpdCommandId,
                                                            gpdCommandPayload,
                                                            gppShortAddress,
                                                            rssi,
                                                            linkQuality,
                                                            gppDistance,
                                                            commissioningNotificationMic);
      break;
    case EMBER_ZCL_GP_GPDF_DECOMMISSIONING:
      gpCommissioningNotificationDecommissioningGpdf(commNotificationOptions, // look at this
                                                     gpdAddr,
                                                     gpdSecurityFrameCounter,
                                                     gpdCommandId,
                                                     gpdCommandPayload,
                                                     gppShortAddress,
                                                     rssi,
                                                     linkQuality,
                                                     gppDistance,
                                                     commissioningNotificationMic);
      break;
    case EMBER_ZCL_GP_GPDF_SUCCESS:
      gpCommissioningNotificationSuccessGpdf(commNotificationOptions,
                                             gpdAddr,
                                             gpdSecurityFrameCounter,
                                             gpdCommandId,
                                             gpdCommandPayload,
                                             gppShortAddress,
                                             rssi,
                                             linkQuality,
                                             gppDistance,
                                             commissioningNotificationMic);
      break;
    case EMBER_ZCL_GP_GPDF_CHANNEL_REQUEST:
      handleChannelRequest(commNotificationOptions,
                           gppShortAddress,
                           ((commNotificationOptions & EMBER_AF_GP_COMMISSIONING_NOTIFICATION_OPTION_RX_AFTER_TX) ? true : false),
                           gpdCommandPayload);
      break;
    default:
      // nothing to do
      break;
  }
  return false;
}
// Gets switch configuration from configured switches in switch translation table
static const EmberAfGreenPowerServerDefautGenericSwTranslation * getSwConfig(uint8_t gpdCommandId,
                                                                             uint8_t switchType,
                                                                             uint8_t noOfPairedBits,
                                                                             uint8_t currentContactStatus)
{
  const EmberAfGreenPowerServerDefautGenericSwTranslation* genericSwitchTable = emGpGetDefaultGenericSwicthTable();
  if (genericSwitchTable == NULL) {
    return NULL;
  }
  for (int i = 0; i < SIZE_OF_SWITCH_TRANSLATION_TABLE; i++) {
    if (switchType == genericSwitchTable[i].SwitchType.switchType
        && (gpdCommandId == genericSwitchTable[i].genericSwitchDefaultTableEntry.gpdCommand)
        && noOfPairedBits == genericSwitchTable[i].SwitchType.nbOfIdentifiedContacts) {
      if (currentContactStatus & genericSwitchTable[i].SwitchType.indicativeBitmask) {
        return &(genericSwitchTable[i]);
      }
    }
  }
  return NULL;
}
// Prepares an additional info block from the switch configuration and received
// contact status
static void swInfoToAdditionalInfo(EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * additionalInfo,
                                   uint8_t currentContact)
{
  additionalInfo->totalLengthOfAddInfoBlock = 3;
  additionalInfo->optionSelector = 1;
  additionalInfo->optionData.genericSwitch.contactBitmask = currentContact;
  additionalInfo->optionData.genericSwitch.contactStatus = currentContact;
}

void ignoreAnyBitSetHigherThanNoOfContacts(uint8_t noOfContacts, uint8_t *currentContact)
{
  uint8_t BitMask = 0;
  for (uint8_t iContact = 0; iContact < noOfContacts; iContact++) {
    BitMask |=  (0x01 << iContact);
  }
  (*currentContact) &= BitMask;
}

uint8_t calculatePairedBits(uint8_t noOfContacts, uint8_t *savedContact)
{
  uint8_t noOfPairedBits = 0;
  ignoreAnyBitSetHigherThanNoOfContacts(noOfContacts,
                                        savedContact);

  for (uint8_t iContact = 0; iContact < noOfContacts; iContact++) {
    if ((*savedContact) & (0x01 << iContact)) {
      noOfPairedBits++;
    }
  }
  return noOfPairedBits;
}

static void countSwitchEntries(EmberGpAddress * gpdAddr,
                               uint8_t *savedContact)
{
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = NULL;
  for ( uint8_t tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
    if (emGptranslationtable->TableEntry[tableIndex].entry == NO_ENTRY) {
      continue;
    }
    if (emberAfGreenPowerCommonGpAddrCompare(&emGptranslationtable->TableEntry[tableIndex].gpAddr, gpdAddr)) {
      if (emGptranslationtable->TableEntry[tableIndex].additionalInfoOffset != 0xFF) {
        addInfo = &(additionalInfoTable->additionalInfoBlock[emGptranslationtable->TableEntry[tableIndex].additionalInfoOffset]);
        *savedContact |= addInfo->optionData.genericSwitch.contactStatus;
      }
    }
  }
}
static void pairingDoneThusAddSwitchToTranslationTable(EmberGpAddress * gpdAddr,
                                                       uint8_t gpdCommandId,
                                                       uint8_t endpoint)
{
  uint8_t savedContact = 0;
  uint8_t bitShift = 0;
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField additionalInfo = { 0 };
  uint8_t alignSwitchType = gpdCommDataSaved.switchInformationStruct.switchType;
  // look at the switch type and supported no of contacts to find how many
  //commands and what are the commands need to be added in TT.
  const EmberAfGreenPowerServerDefautGenericSwTranslation *swConfig;
  // Find the first entry of the configuration, that will tell how many entries
  // to follow.
  //
  if (gpdCommDataSaved.switchInformationStruct.switchType == EMBER_ZCL_GP_UNKNOWN_SWITCH_TYPE) {
    alignSwitchType = EMBER_ZCL_GP_BUTTON_SWITCH_TYPE;
  } else {
    alignSwitchType = gpdCommDataSaved.switchInformationStruct.switchType;
    /// The follwong is to ensure the rocker get both th entries in a pair even if one of the bit is set -
    if (gpdCommDataSaved.switchInformationStruct.switchType == EMBER_ZCL_GP_ROCKER_SWITCH_TYPE) {
      // Realign the the current contacts a pair of bits
      // Any of the bit pairs (b7,b6),(b5,b4),(b3,b2) or (b1,b0) set both the bits of that pair
      for (uint8_t i = 0; i < gpdCommDataSaved.switchInformationStruct.nbOfContacts; i += 2) {
        if (gpdCommDataSaved.switchInformationStruct.currentContact & (0x03 << i)) {
          gpdCommDataSaved.switchInformationStruct.currentContact |= (0x03 << i);
        }
      }
    }
  }

  countSwitchEntries(gpdAddr, &savedContact);
  if (savedContact == (savedContact | gpdCommDataSaved.switchInformationStruct.currentContact)) {
    return; //Ignore the contact if it is already commissioned
  }
  if (savedContact) {   // check if the switch already commissioned!
    for (bitShift = 0; bitShift < gpdCommDataSaved.switchInformationStruct.nbOfContacts; bitShift++) {
      uint8_t contactStatus = savedContact & (0x01 << bitShift);
      if (contactStatus) {
        EmberGpTranslationTableAdditionalInfoBlockOptionRecordField additionalInfo = { 0 };
        swInfoToAdditionalInfo(&additionalInfo, contactStatus);
        uint8_t outIndex = 0xFF;
        uint8_t status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                                          | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID
                                                                          | GP_TRANSLATION_TABLE_SCAN_LEVEL_ADDITIONAL_INFO_BLOCK),//uint8_t levelOfScan,
                                                                         true,//bool infoBlockPresent,
                                                                         gpdAddr,
                                                                         gpdCommandId,
                                                                         0,//uint8_t zbEndpoint,
                                                                         0,
                                                                         &additionalInfo,
                                                                         &outIndex,
                                                                         0);
        if (status == GP_TRANSLATION_TABLE_STATUS_SUCCESS
            && outIndex != 0xFF) {
          emGpTransTableDeleteTranslationTableEntryByIndex(outIndex,
                                                           true,
                                                           gpdCommandId,
                                                           0,
                                                           0,
                                                           0,
                                                           0x00,
                                                           NULL,
                                                           0x00,
                                                           NULL);
        }
      }
    }
  }
  savedContact |= gpdCommDataSaved.switchInformationStruct.currentContact;
  uint8_t pairedBits = calculatePairedBits(gpdCommDataSaved.switchInformationStruct.nbOfContacts, &savedContact);
  //addpaired
  for (bitShift = 0; bitShift < gpdCommDataSaved.switchInformationStruct.nbOfContacts; bitShift++) {
    uint8_t contactStatus = (savedContact & (0x01 << bitShift));
    swConfig = getSwConfig(gpdCommandId, alignSwitchType, pairedBits, contactStatus);
    if (NULL != swConfig) {
      MEMSET(&additionalInfo, 0x00, sizeof(EmberGpTranslationTableAdditionalInfoBlockOptionRecordField));
      swInfoToAdditionalInfo(&additionalInfo, contactStatus);
      uint8_t outIndex = 0xFF;
      uint8_t status;
      const EmberAfGreenPowerServerGpdSubTranslationTableEntry * entry;
      entry = &(swConfig->genericSwitchDefaultTableEntry);
      status = emGpTransTableAddPairedDeviceToTranslationTable(TRANSLATION_TABLE_UPDATE,
                                                               true,
                                                               gpdAddr,
                                                               entry->gpdCommand,
                                                               endpoint,
                                                               entry->zigbeeProfile,
                                                               entry->zigbeeCluster,
                                                               entry->zigbeeCommandId,
                                                               entry->zclPayloadDefault[0],
                                                               (uint8_t *)&(entry->zclPayloadDefault[1]),
                                                               entry->payloadSrc,
                                                               additionalInfo.totalLengthOfAddInfoBlock,
                                                               &additionalInfo,
                                                               &outIndex);
    }
  }
}

static void pairingDoneThusAddMultiSensorToTranslationTable(EmberGpAddress * gpdAddr,
                                                            uint8_t gpdCommandId,
                                                            uint8_t endpoint)
{
  // algo: check in translation table if wanted entry exists
  // if not:
  // - create it or
  // - add to it the optionRecord which miss
  addReportstoTranslationTable(gpdAddr, gpdCommandId, endpoint);
}

//After the pairing session, manage the customized translation table
static void pairingDoneThusSetCustomizedTranslationTable(EmberGpAddress * gpdAddr,
                                                         uint8_t gpdCommandId,
                                                         uint8_t endpoint)
{
  // If switch command and recievd within the commissioning time
  if ((gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
       || gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE)
      && emberEventControlGetActive(emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl)) {
    pairingDoneThusAddSwitchToTranslationTable(gpdAddr,
                                               gpdCommandId,
                                               endpoint);
    return;
  }
  // Multisensor or CAR
  if (emGpAddressMatch(&gpdCommDataSaved.addr, gpdAddr)
      && gpdCommandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
    pairingDoneThusAddMultiSensorToTranslationTable(gpdAddr,
                                                    gpdCommandId,
                                                    endpoint);
    return;
  }
  uint8_t outIndex = GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX;
  uint8_t status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                                    | GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID
                                                                    | GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT),
                                                                   false,
                                                                   gpdAddr,
                                                                   gpdCommandId,
                                                                   endpoint,
                                                                   NULL,
                                                                   NULL,
                                                                   &outIndex,
                                                                   0);
  if (outIndex == GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX
      && status != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
    uint8_t newTTEntryIndex = 0xFF;
    emGpTransTableAddPairedDeviceToTranslationTable(ADD_PAIRED_DEVICE,
                                                    false,
                                                    gpdAddr,
                                                    gpdCommandId,
                                                    endpoint,
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    NULL,
                                                    0,
                                                    0,
                                                    NULL,
                                                    &newTTEntryIndex);
  }
}

// Commissioning Mode callback
bool emberAfGreenPowerClusterGpSinkCommissioningModeCallback(uint8_t options,
                                                             uint16_t gpmAddrForSecurity,
                                                             uint16_t gpmAddrForPairing,
                                                             uint8_t sinkEndpoint)
{
  // Test 4..4.6 - not a sink ep or bcast ep - drop
  if (!isValidAppEndpoint(sinkEndpoint)) {
    emberAfGreenPowerClusterPrintln("DROP - Comm Mode Callback: Sink EP not supported");
    return true;
  }
  if ((options & EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_SECURITY)
      || (options & EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_PAIRING)) {
    return true;
  }
  EmberAfAttributeType type;
  uint8_t gpsSecurityLevelAttribut = 0;
  EmberAfStatus secLevelStatus = emberAfReadAttribute(GP_ENDPOINT,
                                                      ZCL_GREEN_POWER_CLUSTER_ID,
                                                      ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                                      (CLUSTER_MASK_SERVER),
                                                      (uint8_t*)&gpsSecurityLevelAttribut,
                                                      sizeof(uint8_t),
                                                      &type);
  // Reject the req if InvolveTC is et in the attribute
  if (secLevelStatus == EMBER_ZCL_STATUS_SUCCESS
      && (gpsSecurityLevelAttribut & 0x08)) {
    return true;
  }
  uint16_t commissioningWindow = 0;
  uint8_t proxyOptions = 0;
  if (options & EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_ACTION) {
    commissioningState.inCommissioningMode = true;
    commissioningState.endpoint = sinkEndpoint; // The commissioning endpoint can be 0xFF as well
    //enter commissioning mode
    if (options
        & (EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_SECURITY
           | EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_GPM_IN_PAIRING)) {
      //these SHALL be 0 for now
      //TODO also check involve-TC
      goto kickout;
    }
    if (options & EMBER_AF_GP_SINK_COMMISSIONING_MODE_OPTIONS_INVOLVE_PROXIES) {
      commissioningState.proxiesInvolved = true;
    } else {
      commissioningState.proxiesInvolved = false;
    }

    // default 180s of GP specification
    commissioningWindow = EMBER_AF_ZCL_CLUSTER_GP_GPS_COMMISSIONING_WINDOWS_DEFAULT_TIME_S;

    uint8_t gpsCommissioningExitMode;
    uint16_t gpsCommissioningWindows;
    EmberAfStatus statusExitMode = emberAfReadAttribute(GP_ENDPOINT,
                                                        ZCL_GREEN_POWER_CLUSTER_ID,
                                                        ZCL_GP_SERVER_GPS_COMMISSIONING_EXIT_MODE_ATTRIBUTE_ID,
                                                        CLUSTER_MASK_SERVER,
                                                        (uint8_t*)&gpsCommissioningExitMode,
                                                        sizeof(uint8_t),
                                                        &type);
    EmberAfStatus statusCommWindow = emberAfReadAttribute(GP_ENDPOINT,
                                                          ZCL_GREEN_POWER_CLUSTER_ID,
                                                          ZCL_GP_SERVER_GPS_COMMISSIONING_WINDOW_ATTRIBUTE_ID,
                                                          CLUSTER_MASK_SERVER,
                                                          (uint8_t*)&gpsCommissioningWindows,
                                                          sizeof(uint16_t),
                                                          &type);
    proxyOptions = EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION;

    // set commissioningWindow based on "gpsCommissioningWindows" attribute if different from
    // commissioningWindow will be part of the frame if it is different from default 180s
    // in any case there is a timeout running into the GPPs to exit,
    // the default one (180s) not include or the one include in the ProxyCommMode(Enter) frame
    if (statusCommWindow == EMBER_ZCL_STATUS_SUCCESS
        && (gpsCommissioningExitMode
            & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION)
        && gpsCommissioningWindows != commissioningWindow) {
      commissioningWindow = gpsCommissioningWindows;
      proxyOptions |= EMBER_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION;
    }
    emberEventControlSetDelayMS(emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventControl,
                                commissioningWindow * MILLISECOND_TICKS_PER_SECOND);
    // only one of the 2 next flag shall be set at the same time
    if (statusExitMode == EMBER_ZCL_STATUS_SUCCESS
        && (gpsCommissioningExitMode
            & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)
        && !(gpsCommissioningExitMode
             & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT)) {
      proxyOptions |= EMBER_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS;
    } else if ((statusExitMode == EMBER_ZCL_STATUS_SUCCESS)
               && (gpsCommissioningExitMode
                   & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT)
               && !(gpsCommissioningExitMode
                    & EMBER_AF_GP_SINK_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS)) {
      proxyOptions |= EMBER_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT;
    } else {
      // in case both are cleared and both are set nothing more to do
    }
  } else if (commissioningState.inCommissioningMode) {
    // if commissioning mode is ON and received frame set it to OFF, exit comissioning mode
    commissioningState.inCommissioningMode = false;
    // deactive the timer
    emberEventControlSetInactive(emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventControl);
    if (commissioningState.proxiesInvolved) {
      //involve proxies
      proxyOptions = 0;
    }
  }
  // On commissionning entry or exist, reset the number of report config received
  resetOfMultisensorDataSaved(true);

  emberAfFillCommandGreenPowerClusterGpProxyCommissioningModeSmart(proxyOptions,
                                                                   commissioningWindow,
                                                                   0);
  EmberApsFrame *apsFrame;
  apsFrame = emberAfGetCommandApsFrame();
  apsFrame->sourceEndpoint = GP_ENDPOINT;
  apsFrame->destinationEndpoint = GP_ENDPOINT;
  uint8_t retval;
  if (commissioningState.proxiesInvolved) {
    retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
  } else {
    #ifdef EMBER_AF_PLUGIN_GREEN_POWER_CLIENT
    // Put the proxy instance on this node commissioning mode so that it can accept a pairing from itself.
    // This is to ensure the node will be able to handle gpdf commands after pairig.
    retval = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, emberAfGetNodeId());
    #endif
  }
  emberAfGreenPowerClusterPrintln("SinkCommissioningModeCallbacksend returned %d", retval);
  kickout: return true;
}

//function return true if the param_in communication mode is supported by the sink
static bool emGpCheckCommunicationModeSupport(uint8_t communicationModeToCheck)
{
  uint32_t gpsFuntionnalityAttribut = 0;
  uint32_t gpsActiveFunctionnalityAttribut = 0;
  EmberAfAttributeType type;
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GPS_FUNCTIONALITY_ATTRIBUTE_ID,
                       CLUSTER_MASK_SERVER,
                       (uint8_t*)&gpsFuntionnalityAttribut,
                       sizeof(uint32_t),
                       &type);
  emberAfReadAttribute(GP_ENDPOINT,
                       ZCL_GREEN_POWER_CLUSTER_ID,
                       ZCL_GP_SERVER_GPS_ACTIVE_FUNCTIONALITY_ATTRIBUTE_ID,
                       (CLUSTER_MASK_SERVER),
                       (uint8_t*)&gpsActiveFunctionnalityAttribut,
                       sizeof(uint32_t),
                       &type);
  uint32_t currentFunctionnalities = gpsFuntionnalityAttribut & gpsActiveFunctionnalityAttribut;
  if ((communicationModeToCheck == EMBER_GP_SINK_TYPE_FULL_UNICAST
       && (currentFunctionnalities & EMBER_AF_GP_GPS_FUNCTIONALITY_FULL_UNICAST_COMMUNICATION))
      || (communicationModeToCheck == EMBER_GP_SINK_TYPE_D_GROUPCAST
          && (currentFunctionnalities & EMBER_AF_GP_GPS_FUNCTIONALITY_DERIVED_GROUPCAST_COMMUNICATION))
      || (communicationModeToCheck == EMBER_GP_SINK_TYPE_GROUPCAST
          && (currentFunctionnalities & EMBER_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION))
      || (communicationModeToCheck == EMBER_GP_SINK_TYPE_LW_UNICAST
          && (currentFunctionnalities & EMBER_AF_GP_GPS_FUNCTIONALITY_LIGHTWEIGHT_UNICAST_COMMUNICATION))) {
    return true;
  }
  return false;
}

static void handleGpPairingConfigNoAction(EmberGpAddress * gpdAddr,
                                          bool sendGpPairing)
{
  uint8_t sinkEntryIndex = emberGpSinkTableLookup(gpdAddr);
  if (sinkEntryIndex != 0xFF
      && sendGpPairing) {
    // entry present and Gp Pairning need to be sent
    // Send out Gp Pairing using Sink params
    // Start updating sink table
    EmberGpSinkTableEntry sinkEntry;
    emberGpSinkTableGetEntry(sinkEntryIndex, &sinkEntry);
    uint8_t appId = (sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_APPLICATION_ID);
    uint8_t sinkCommunicationMode = (sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE)
                                    >> EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET;
    bool gpdFixed = (sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_FIXED_LOCATION)
                    ? true : false;
    bool gpdMacCapabilities = (sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES)
                              ? true : false;
    bool sinkTableAssignedAliasNeeded = (sinkEntry.options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS)
                                        ? true : false;
    uint8_t securityLevel = (sinkEntry.securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
    uint8_t securityKeyType = ((sinkEntry.securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                               >> EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET);
    EmberEUI64 ourEUI;
    emberAfGetEui64(ourEUI);
    uint32_t pairingOptions = 0;
    pairingOptions = EMBER_AF_GP_PAIRING_OPTION_ADD_SINK
                     | (appId & EMBER_AF_GP_PAIRING_OPTION_APPLICATION_ID)
                     | (sinkCommunicationMode << EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                     | (gpdFixed << EMBER_AF_GP_PAIRING_OPTION_GPD_FIXED_OFFSET)
                     | (gpdMacCapabilities << EMBER_AF_GP_PAIRING_OPTION_GPD_MAC_SEQUENCE_NUMBER_CAPABILITIES_OFFSET)
                     | (securityLevel << EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                     | (securityKeyType << EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET)
                     | ((gpdMacCapabilities || securityLevel) ? EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_FRAME_COUNTER_PRESENT : 0)
                     | ((securityLevel && securityKeyType) ? EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_KEY_PRESENT : 0)
                     | (sinkTableAssignedAliasNeeded << EMBER_AF_GP_PAIRING_OPTION_ASSIGNED_ALIAS_PRESENT_OFFSET)
                     | ((sinkCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST
                         || sinkCommunicationMode == EMBER_GP_SINK_TYPE_GROUPCAST)
                        ? EMBER_AF_GP_PAIRING_OPTION_GROUPCAST_RADIUS_PRESENT : 0);
    uint16_t groupId = 0xFFFF;
    if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST) {
      groupId = emGpdAlias(gpdAddr);
    } else if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_GROUPCAST) {
      groupId = sinkEntry.sinkList[0].target.groupcast.groupID;
    }
    emberAfFillCommandGreenPowerClusterGpPairingSmart(pairingOptions,
                                                      gpdAddr->id.sourceId,
                                                      gpdAddr->id.gpdIeeeAddress,
                                                      gpdAddr->endpoint,
                                                      ourEUI,
                                                      emberGetNodeId(),
                                                      groupId,
                                                      sinkEntry.deviceId,
                                                      sinkEntry.gpdSecurityFrameCounter,
                                                      sinkEntry.gpdKey.contents,
                                                      sinkEntry.assignedAlias,
                                                      sinkEntry.groupcastRadius);
    EmberApsFrame *apsFrame;
    apsFrame = emberAfGetCommandApsFrame();
    apsFrame->sourceEndpoint = GP_ENDPOINT;
    apsFrame->destinationEndpoint = GP_ENDPOINT;
    uint8_t retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
    emberAfGreenPowerClusterPrintln("pairing send returned %d", retval);
  }
}
// The the applicationInformation passed as 0xFF if the field is not
// present in the pay load, so set it back to 0 as it is a mask and should
// be treated appropriately
#define RESET_BOUND(input) (input == 0xFF) ?  0 : input

bool emberAfGreenPowerClusterGpPairingConfigurationCallback(uint8_t actions,
                                                            uint16_t options,
                                                            uint32_t gpdSrcId,
                                                            uint8_t* gpdIeee,
                                                            uint8_t gpdEndpoint,
                                                            uint8_t deviceId,
                                                            uint8_t groupListCount,
                                                            uint8_t* groupList,
                                                            uint16_t gpdAssignedAlias,
                                                            uint8_t groupcastRadius,
                                                            uint8_t securityOptions,
                                                            uint32_t gpdSecurityFrameCounter,
                                                            uint8_t* gpdSecurityKey,
                                                            uint8_t numberOfPairedEndpoints,
                                                            uint8_t* pairedEndpoints,
                                                            uint8_t applicationInformation,
                                                            uint16_t manufacturerId,
                                                            uint16_t modeId,
                                                            uint8_t numberOfGpdCommands,
                                                            uint8_t* gpdCommandIdList,
                                                            uint8_t clusterIdListCount,
                                                            uint8_t* clusterListServer,
                                                            uint8_t* clusterListClient,
                                                            uint8_t switchInformationLength,
                                                            uint8_t genericSwitchConfiguration,
                                                            uint8_t currentContactStatus,
                                                            uint8_t totalNumberOfReports,
                                                            uint8_t numberOfReports,
                                                            uint8_t* reportDescriptor)
{
  EmberAfClusterCommand *cmd =  emberAfCurrentCommand();
  if (cmd->source == emberGetNodeId()) {
    // Drop Loopback packets else it will try to process.
    return true;
  }
  applicationInformation = RESET_BOUND(applicationInformation);
  numberOfGpdCommands = RESET_BOUND(numberOfGpdCommands);
  clusterIdListCount = RESET_BOUND(clusterIdListCount); // 0xFF may be a good case as well
  switchInformationLength = RESET_BOUND(switchInformationLength);
  totalNumberOfReports = RESET_BOUND(totalNumberOfReports);
  numberOfReports = RESET_BOUND(numberOfReports);
  securityOptions = RESET_BOUND(securityOptions);
  uint8_t allZeroesIeeeAddress[17] = { 0 };

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = allZeroesIeeeAddress;
  }

  // Save all the relevant information to the gpdData for further processing
  uint8_t gpdAppId = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_ID);
  EmberGpAddress gpdAddr;
  if (!emGpMakeAddr(&gpdAddr, gpdAppId, gpdSrcId, gpdIeee, gpdEndpoint)) {
    return false;
  }
  // Drop for the gpd addr 0
  if (gpdAddrZero(&gpdAddr)) {
    emberAfGreenPowerClusterPrintln("DROP - GP Pairing Config : GPD Address is 0!");
    return true;
  }
  if ((options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE)
      && ((securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL) == EMBER_GP_SECURITY_LEVEL_RESERVED)) {
    emberAfGreenPowerClusterPrintln("DROP - GP Pairing Config : Security Level reserved !");
    return true;
  }
  uint8_t gpPairingConfigCommunicationMode = (options
                                              & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE)
                                             >> EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_COMMUNICATION_MODE_OFFSET;
  bool seqNumberCapability = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES)
                             ? true : false;
  bool rxOnCapability = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_RX_ON_CAPABILITY)
                        ? true : false;
  bool fixedLocation = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_FIXED_LOCATION)
                       ? true : false;
  bool assignedAllias = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_ASSIGNED_ALIAS)
                        ? true : false;
  bool securityUse = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SECURITY_USE)
                     ? true : false;
  bool appInformationPresent = (options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_APPLICATION_INFORMATION_PRESENT)
                               ? true : false;
  uint8_t gpConfigAtion = (actions & EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_ACTION);
  if (assignedAllias) {
    gpdCommDataSaved.useGivenAssignedAlias = true;
    gpdCommDataSaved.givenAlias = gpdAssignedAlias;
  }
  if (appInformationPresent) { // application info present
    gpdCommDataSaved.applicationInfo.applInfoBitmap = applicationInformation;
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_MANUFACTURE_ID_PRESENT) { // ManufactureId Present
      gpdCommDataSaved.applicationInfo.manufacturerId = manufacturerId;
    }
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_MODEL_ID_PRESENT) { // ModelId Present
      gpdCommDataSaved.applicationInfo.modelId = modeId;
    }
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_COMMANDS_PRESENT) {   // gpd Commands present
      gpdCommDataSaved.applicationInfo.numberOfGpdCommands = numberOfGpdCommands;
      MEMCOPY(gpdCommDataSaved.applicationInfo.gpdCommands,
              gpdCommandIdList,
              numberOfGpdCommands);
    }
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) { // Cluster List present
      gpdCommDataSaved.applicationInfo.numberOfGpdClientCluster = clusterIdListCount >> 4;
      MEMCOPY(gpdCommDataSaved.applicationInfo.clientClusters,
              clusterListClient,
              (gpdCommDataSaved.applicationInfo.numberOfGpdClientCluster * 2));
      gpdCommDataSaved.applicationInfo.numberOfGpdServerCluster = clusterIdListCount & 0x0f;
      MEMCOPY(gpdCommDataSaved.applicationInfo.serverClusters,
              clusterListServer,
              (gpdCommDataSaved.applicationInfo.numberOfGpdServerCluster * 2));
    }
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_PRESENT) {
      gpdCommDataSaved.switchInformationStruct.switchInfoLength = switchInformationLength;
      gpdCommDataSaved.switchInformationStruct.nbOfContacts = (genericSwitchConfiguration
                                                               & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_NB_OF_CONTACT);
      gpdCommDataSaved.switchInformationStruct.switchType = (genericSwitchConfiguration
                                                             & EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE)
                                                            >> EMBER_AF_GP_APPLICATION_INFORMATION_SWITCH_INFORMATION_CONFIGURATION_SWITCH_TYPE_OFFSET;
      gpdCommDataSaved.switchInformationStruct.currentContact = currentContactStatus;

      //then set the delay to repeat and activate it in that much delay
      uint32_t delay = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_GENERIC_SWITCH_COMMISSIONING_TIMEOUT_IN_SEC;
      emberEventControlSetDelayMS(emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl,
                                  delay * MILLISECOND_TICKS_PER_SECOND);
    }
    // Following is conditional compiled as it causes failure for sugbsequent commissioning without waiting to expire the previous MS time.
    #ifdef EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ENABLE_DIRECT_ADDITION_OF_REPORTS
    if (applicationInformation & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS) {
      // Start the timer for the first time.
      if (gpdCommDataSaved.reportCollectonState == GP_SINK_COMM_STATE_IDLE
          && (gpConfigAtion == EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REPLACE_SINK_TABLE_ENTRY
              || gpConfigAtion == EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_EXTEND_SINK_TABLE_ENTRY)) {
        gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
        uint32_t delay = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
        emberEventControlSetDelayMS(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl,
                                    delay * MILLISECOND_TICKS_PER_SECOND);
      }
    }
    #endif
  }
  // Always present in the GpPairingConfig payload
  gpdCommDataSaved.applicationInfo.deviceId = deviceId;
  gpdCommDataSaved.applicationInfo.numberOfPairedEndpoints = numberOfPairedEndpoints;
  if (numberOfPairedEndpoints < 0xFD) {
    MEMCOPY(gpdCommDataSaved.applicationInfo.pairedEndpoints,
            pairedEndpoints,
            numberOfPairedEndpoints);
  }
  uint8_t gpdfExtendedOptions = 0;
  if (securityUse) {
    gpdCommDataSaved.securityLevel = (securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL);
    gpdCommDataSaved.securityKeyType = (securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                                       >> EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET;
    gpdCommDataSaved.outgoingFrameCounter = gpdSecurityFrameCounter;
    gpdfExtendedOptions = gpdCommDataSaved.securityLevel
                          | (gpdCommDataSaved.securityKeyType << EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_KEY_TYPE_OFFSET);
    if (gpdSecurityKey) {
      gpdfExtendedOptions |= EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_PRESENT;
      gpdfExtendedOptions |= ((gpdCommDataSaved.securityLevel == 3)
                              ? EMBER_AF_GP_GPD_COMMISSIONING_EXTENDED_OPTIONS_GPD_KEY_ENCRYPTION : 0);
      MEMCOPY(gpdCommDataSaved.key.contents, gpdSecurityKey, EMBER_ENCRYPTION_KEY_SIZE);
    }
  }
  gpdCommDataSaved.gpdfExtendedOptions |= gpdfExtendedOptions;
  uint8_t gpdfOptions = 0;
  gpdfOptions = (seqNumberCapability ? EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_MAC_SEQ_NUM_CAP : 0)
                | (rxOnCapability ? EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_RX_ON_CAP : 0)
                | (appInformationPresent ? EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_APPLICATION_INFORMATION_PRESENT : 0)
                | (fixedLocation ? EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_FIXED_LOCATION : 0)
                | (securityUse ? EMBER_AF_GP_GPD_COMMISSIONING_OPTIONS_EXTENDED_OPTIONS_FIELD : 0);
  gpdCommDataSaved.gpdfOptions |= gpdfOptions;
  gpdCommDataSaved.groupcastRadius = groupcastRadius;
  uint8_t gpPairingConfigSecurityLevel = securityOptions
                                         & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL;
  uint8_t gpPairingConfigSecurityKeyType = (securityOptions
                                            & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                                           >> EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET;
  // update sink Table entries for all GPD with that applicationID (SrcID or IEEE)
  bool gpdWildcardIdUpdateAllGpd = false;
  uint8_t sinkEntryIndex = 0xFF;
  bool replaceOrExtendSink = false;
  uint8_t gpsSecurityLevelAttribut = 0;
  EmberAfStatus status;
  EmberAfAttributeType type;
  status = emberAfReadAttribute(GP_ENDPOINT,
                                ZCL_GREEN_POWER_CLUSTER_ID,
                                ZCL_GP_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_ID,
                                (CLUSTER_MASK_SERVER),
                                (uint8_t*)&gpsSecurityLevelAttribut,
                                sizeof(uint8_t),
                                &type);

  gpsSecurityLevelAttribut = gpsSecurityLevelAttribut & 0x03; // mask the encryption part
  if ((groupListCount == 0xFF || groupList == NULL)
      || (sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_SINK_TABLE_BASED_GROUPCAST_FORWARDING)
          || sinkFunctionalitySupported(EMBER_AF_GP_GPS_FUNCTIONALITY_PRE_COMMISSIONED_GROUPCAST_COMMUNICATION))) {
    if (cmd->type != EMBER_INCOMING_BROADCAST
        && gpPairingConfigSecurityLevel == EMBER_GP_SECURITY_LEVEL_RESERVED) {
      emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
      return true;
    }
    if ((gpdAddr.applicationId == EMBER_GP_APPLICATION_SOURCE_ID
         && gpdAddr.id.sourceId == GP_ADDR_SRC_ID_WILDCARD)
        && ((gpdAddr.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS)
            && (emberAfMemoryByteCompare(gpdAddr.id.gpdIeeeAddress, EUI64_SIZE, 0xFF)))) {
      gpdWildcardIdUpdateAllGpd = true;
    } else {
      gpdWildcardIdUpdateAllGpd = false;
    }
    switch (gpConfigAtion) {
      // Action 0b000
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_NO_ACTION:
        handleGpPairingConfigNoAction(&gpdAddr,
                                      ((actions & EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING)
                                       ? true : false));
        return true;
      // Action = 0b010
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REPLACE_SINK_TABLE_ENTRY:
        if (gpPairingConfigSecurityLevel != EMBER_GP_SECURITY_LEVEL_RESERVED
            && (gpPairingConfigSecurityLevel >= gpsSecurityLevelAttribut
                && emGpCheckCommunicationModeSupport(gpPairingConfigCommunicationMode))) {
          sinkEntryIndex = emberGpSinkTableLookup(&gpdAddr);
          if (sinkEntryIndex != 0xFF) {
            emGpTransTableDeletePairedDevicefromTranslationTableEntry(&gpdAddr);
            emberGpSinkTableRemoveEntry(sinkEntryIndex);
          }
        }
      // Fall through  - Intentional
      // Action = 0b001
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_EXTEND_SINK_TABLE_ENTRY:
        if (gpPairingConfigSecurityLevel != EMBER_GP_SECURITY_LEVEL_RESERVED
            && (gpPairingConfigSecurityLevel >= gpsSecurityLevelAttribut
                && emGpCheckCommunicationModeSupport(gpPairingConfigCommunicationMode))) {
          replaceOrExtendSink = true;
        }
        break;

      // Action = 0b011
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_A_PAIRING:
        sinkEntryIndex = emberGpSinkTableLookup(&gpdAddr);
        if (sinkEntryIndex != 0xFF) {
          // remove this particular pairing for this GPD ID in the translation Table
          // if 1 report identifier is missing, delete the sensor entry for this GPD
          emGpTransTableDeletePairedDevicefromTranslationTableEntry(&gpdAddr);
        } else if (gpdWildcardIdUpdateAllGpd == true) {
          // TODO: apply action to all GPD with this particular applicationID (SrcId or IEEE)
          // ...
        }
        {
          uint8_t outIndex = GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX;
          uint8_t status = emGpTransTableFindMatchingTranslationTableEntry(GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID,
                                                                           false,
                                                                           &gpdAddr,
                                                                           0,
                                                                           0,
                                                                           NULL,
                                                                           NULL,
                                                                           &outIndex,
                                                                           0);
          if (status != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
            decommissionGpd(0, 0, &gpdAddr, false);
            // Avoid GpPairing to be sent in this function.
            // because the decommissioning had sent one out.
            actions &= (!(EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING));
          }
        }
        break;
      // Action = 0b100
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_GPD:
        decommissionGpd(0, 0, &gpdAddr, true);
        break;
      // Action = 0b101
      case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_APPLICATION_DESCRIPTION:
      {
        // if total number of report is 0 then it is the first gp pairing
        if (gpdCommDataSaved.totalNbOfReport == 0) {
          // first sensor to follow, reset structure for the report descriptors
          resetOfMultisensorDataSaved(false);
          MEMCOPY(&gpdCommDataSaved.addr, &gpdAddr, sizeof(EmberGpAddress));
          gpdCommDataSaved.totalNbOfReport = totalNumberOfReports;
          gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
        }
        // if the timer is running then process else just drop
        if (!emberEventControlGetActive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl)) {
          emberAfGreenPowerClusterPrintln("Gp PairingConfig App Description - DROP : MS timer has expired");
          return true;
        }
        if (gpdCommDataSaved.reportCollectonState != GP_SINK_COMM_STATE_COLLECT_REPORTS) {
          // not in right state
          return true;
        }
        if (!saveReportDescriptor(totalNumberOfReports,
                                  numberOfReports,
                                  reportDescriptor)) {
          // error in the application desc payload
          return true;
        }
        if (gpdCommDataSaved.numberOfReports != gpdCommDataSaved.totalNbOfReport) {
          // still to collect more reports
          return true;
        }
        // Report collection is over - finalise the pairing
        gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
        finalisePairing(gpdAddr,
                        gpdCommDataSaved.gpdfOptions,
                        gpdCommDataSaved.gpdfExtendedOptions,
                        gpdCommDataSaved.outgoingFrameCounter,
                        gpdCommDataSaved.securityLevel,
                        gpdCommDataSaved.securityKeyType,
                        gpdCommDataSaved.applicationInfo);
        return true;
      }
      break;   // Actions = Application Description
      default:
        break;
    }
    if (replaceOrExtendSink) {
      emberAfGreenPowerClusterPrintln("Replace or Extend Sink");
      gpdCommDataSaved.communicationMode = gpPairingConfigCommunicationMode;
      // Set/reset the doNotSendGpPairing flag to be used when finaliing paring , Note : the negation.
      gpdCommDataSaved.doNotSendGpPairing = ((actions & EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING)
                                             ? false : true);
      if ((gpdCommDataSaved.applicationInfo.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_GPD_APPLICATION_DESCRIPTION_COMMAND_FOLLOWS)) {
        if (!emberEventControlGetActive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl)) {
          emberAfGreenPowerClusterPrintln("Gp PairingConfig : Reseting Partial data - Only Report Data");
          resetOfMultisensorDataSaved(false);
        }
        emberAfGreenPowerClusterPrintln("Gp PairingConfig : Start MS/CAR Commissioning Timeout and waiting for ADCF");
        uint32_t delay = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_MULTI_SENSOR_COMMISSIONING_TIMEOUT_IN_SEC;
        emberEventControlSetDelayMS(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl,
                                    delay * MILLISECOND_TICKS_PER_SECOND);
        // All set to collect the report descriptors
        gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_COLLECT_REPORTS;
        return true;
      }
      // Replace or Extend Sink with new application information, then create TT entries as well - if the application description is not following
      // Makes proxy on this node enter commissioning so that it can creat a proxy entry when Gp Pairing is heard
      if (!(commissioningState.inCommissioningMode)) {
        // The sink is not in commissioning mode for the CT based commissioning.
        // The endpoints are supplied by the CT, hence the sink need to match all the endpoints on it.
        commissioningState.endpoint = GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS;
      #ifdef EMBER_AF_PLUGIN_GREEN_POWER_CLIENT
        emberAfGreenPowerClusterGpProxyCommissioningModeCallback((EMBER_AF_GP_PROXY_COMMISSIONING_MODE_OPTION_ACTION // Enter Commissioning
                                                                  | EMBER_AF_GP_PROXY_COMMISSIONING_MODE_EXIT_MODE_ON_FIRST_PAIRING_SUCCESS), // Exit on First pairing
                                                                 0,   // No Commissioning window
                                                                 0);  // No channel present
      #endif
      }
      gpdCommDataSaved.reportCollectonState = GP_SINK_COMM_STATE_FINALISE_PAIRING;
      finalisePairing(gpdAddr,
                      gpdfOptions,
                      gpdfExtendedOptions,
                      gpdSecurityFrameCounter,
                      gpdCommDataSaved.securityLevel,
                      gpdCommDataSaved.securityKeyType,
                      gpdCommDataSaved.applicationInfo);
      return true;
    }

    // The Send GP Pairing sub-field, if set to 0b1 indicates that the receiving
    // sink is requested to send GP Pairing command upon completing the handling
    // of GP Pairing Configuration
    if (actions & EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_SEND_GP_PAIRING) {
      uint32_t pairingOptions = 0x00;
      switch (actions & EMBER_AF_GP_PAIRING_CONFIGURATION_ACTIONS_ACTION) {
        case EMBER_ZCL_GP_PAIRING_CONFIGURATION_ACTION_REMOVE_A_PAIRING:
          pairingOptions = gpdAppId
                           | (gpPairingConfigCommunicationMode << EMBER_AF_GP_PAIRING_OPTION_COMMUNICATION_MODE_OFFSET)
                           | (((options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES) >> EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_SEQUENCE_NUMBER_CAPABILITIES_OFFSET) << EMBER_AF_GP_PAIRING_OPTION_GPD_MAC_SEQUENCE_NUMBER_CAPABILITIES_OFFSET)
                           | (((options & EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_FIXED_LOCATION) >> EMBER_AF_GP_PAIRING_CONFIGURATION_OPTION_FIXED_LOCATION_OFFSET) << EMBER_AF_GP_PAIRING_OPTION_GPD_FIXED_OFFSET)
                           | (gpPairingConfigSecurityLevel << EMBER_AF_GP_PAIRING_OPTION_SECURITY_LEVEL_OFFSET)
                           | (gpPairingConfigSecurityKeyType << EMBER_AF_GP_PAIRING_OPTION_SECURITY_KEY_TYPE_OFFSET)
                           | EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_FRAME_COUNTER_PRESENT
                           | EMBER_AF_GP_PAIRING_OPTION_GPD_SECURITY_KEY_PRESENT;
          break;
        default:
          // all other action replace,extend sink, remove GPD etc will send the
          // GP Pairing in theri respective routines.
          return true;
      }
      uint16_t dGroupId = 0xFFFF;
      if (gpPairingConfigCommunicationMode == EMBER_GP_SINK_TYPE_D_GROUPCAST) {
        // for DGroupID = derived groupID, call this function to get the groupId
        dGroupId = emGpdAlias(&gpdAddr);
        EmberAfStatus UNUSED status = addToApsGroup(GP_ENDPOINT, dGroupId);
      }
      EmberEUI64 ourEUI;
      emberAfGetEui64(ourEUI);
      emberAfFillCommandGreenPowerClusterGpPairingSmart(pairingOptions,
                                                        gpdSrcId,
                                                        gpdIeee,
                                                        gpdEndpoint,
                                                        ourEUI,
                                                        emberGetNodeId(),
                                                        dGroupId,
                                                        gpdCommDataSaved.applicationInfo.deviceId,
                                                        gpdCommDataSaved.outgoingFrameCounter,
                                                        gpdCommDataSaved.key.contents,
                                                        (assignedAllias ? gpdAssignedAlias : emGpdAlias(&gpdAddr)),
                                                        groupcastRadius);
      EmberApsFrame *apsFrame;
      apsFrame = emberAfGetCommandApsFrame();
      apsFrame->sourceEndpoint = GP_ENDPOINT;
      apsFrame->destinationEndpoint = GP_ENDPOINT;
      UNUSED EmberStatus retval;
      retval = emberAfSendCommandBroadcast(EMBER_RX_ON_WHEN_IDLE_BROADCAST_ADDRESS);
      emberAfGreenPowerClusterPrintln("pairing BCAST send returned %d", retval);
    }
  }
  return true;
}

// Simply builds a list of all the gpd commands supported by the standard deviceId
// consumed by formGpdCommandListFromIncommingCommReq
static uint8_t getCommandListFromDeviceIdLookup(uint8_t gpdDeviceId,
                                                uint8_t * gpdCommandList)
{
  for (uint8_t index = 0; index < DEVICE_ID_MAP_TABLE_SIZE; index++) {
    if (gpdDeviceCmdMap[index].deviceId == gpdDeviceId) {
      MEMCOPY(gpdCommandList,
              &gpdDeviceCmdMap[index].cmd[1], // commands supported
              gpdDeviceCmdMap[index].cmd[0]); // length
      return gpdDeviceCmdMap[index].cmd[0];
    }
  }
  return 0;
}

// Simply builds a list of all the gpd cluster supported by the standard deviceId
// consumed by formGpdClusterListFromIncommingCommReq
static uint8_t getClusterListFromDeviceIdLookup(uint8_t gpdDeviceId,
                                                ZigbeeCluster * gpdClusterList)
{
  for (uint8_t mapTableIndex = 0; mapTableIndex < DEVICE_ID_MAP_CLUSTER_TABLE_SIZE; mapTableIndex++) {
    if (gpdDeviceClusterMap[mapTableIndex].deviceId == gpdDeviceId) {
      for (uint8_t clusterIndex = 0; clusterIndex < gpdDeviceClusterMap[mapTableIndex].numberOfClusters; clusterIndex++) {
        // cluster supported
        gpdClusterList[clusterIndex].clusterId = gpdDeviceClusterMap[mapTableIndex].cluster[clusterIndex];
        // GPD announce this cluster as server,
        // this cluster is recorded as client side to evaluate it presence on the sink
        gpdClusterList[clusterIndex].serverClient = 0; // reverse for the match
      }
      return gpdDeviceClusterMap[mapTableIndex].numberOfClusters;
    }
  }
  return 0;
}

// Builds a list of all the gpd commands from incoming commissioning req
static uint8_t formGpdCommandListFromIncommingCommReq(uint8_t gpdDeviceId,
                                                      uint8_t * inGpdCommandList,
                                                      uint8_t inCommandListLength,
                                                      uint8_t * gpdCommandList)
{
  uint8_t gpdCommandListLength = 0;
  // if there is already a command list present in incoming payload copy that
  // Realign A0-A3 as AF if TT compacting is enabled and filter out duplicate AF
  bool foundAlready = false;
  for (int i = 0; i < inCommandListLength; i++) {
    if (inGpdCommandList[i] == EMBER_ZCL_GP_GPDF_ATTRIBUTE_REPORTING
        || inGpdCommandList[i] == EMBER_ZCL_GP_GPDF_MULTI_CLUSTER_RPTG
        || inGpdCommandList[i] == EMBER_ZCL_GP_GPDF_MFR_SP_ATTR_RPTG
        || inGpdCommandList[i] == EMBER_ZCL_GP_GPDF_MFR_SP_MULTI_CLUSTER_RPTG) {
      if (!foundAlready) {
        foundAlready = true;
        gpdCommandList[gpdCommandListLength++] = EMBER_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD;
      }
    } else {
      gpdCommandList[gpdCommandListLength++] = inGpdCommandList[i];
    }
  }
  //Now extend the command list for the device id, take out the duplicate if any
  if (gpdDeviceId != 0xFE) {
    // look up the device Id derive the commands if there is no commands supplied
    if (gpdCommandListLength == 0) {
      gpdCommandListLength += getCommandListFromDeviceIdLookup(gpdDeviceId,
                                                               gpdCommandList);
    }
  }
  return gpdCommandListLength;
}

// resolves and relates endpoint - cluster from the given the number of paired endpoints
// cluster list (present in application info and appl description)
static uint8_t buildSinkSupportedClusterEndPointListForGpdCommands(uint8_t numberOfPairedEndpoints,
                                                                   uint8_t * pairedEndpoints,
                                                                   uint8_t numberOfClusters,
                                                                   ZigbeeCluster * clusterList,
                                                                   SupportedGpdCommandClusterEndpointMap * gpdCommandClusterEpMap)
{
  //for (int i = 0; i < numberOfClusters; i++) {
  //  emberAfGreenPowerClusterPrint("clusterList[%d] = %x ", i, clusterList[i]);
  //}
  //emberAfGreenPowerClusterPrintln("");
  uint8_t tempEndpointsOnSink[MAX_ENDPOINT_COUNT] = { 0 };
  uint8_t tempSupportedEndpointCount = 0;
  if (numberOfPairedEndpoints == GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS
      || numberOfPairedEndpoints == GREEN_POWER_SERVER_RESERVED_ENDPOINTS) {
    // The target endpoint for validation is the Current commissioning Ep
    tempSupportedEndpointCount = getGpCommissioningEndpoint(tempEndpointsOnSink);
  } else if (numberOfPairedEndpoints == GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS
             || numberOfPairedEndpoints == GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS) {
    tempSupportedEndpointCount = getAllSinkEndpoints(tempEndpointsOnSink);
  } else {
    for (int i = 0; i < MAX_ENDPOINT_COUNT; i++) {
      for (int j = 0; j < numberOfPairedEndpoints; j++) {
        emberAfGreenPowerClusterPrintln("Checking PairedEp[%d]=%x with SinkEp = %d ",
                                        j, pairedEndpoints[j], emAfEndpoints[i].endpoint);
        if (pairedEndpoints[j] == emAfEndpoints[i].endpoint) {
          tempEndpointsOnSink[tempSupportedEndpointCount] = emAfEndpoints[i].endpoint;
          tempSupportedEndpointCount++;
          // Found, so break inner loop
          // - this is ensure the temp endpoint list is sorted without duplicate
          break;
        }
      }
    }
  }
  // Here, the endpoints are filtered, those, only are supported out of input paired
  // endpoint list - run through all the clusters for each end point to filter
  // (endpoint, cluster) pair that is supported.
  uint8_t count = 0;
  for (int i = 0; i < tempSupportedEndpointCount; i++) {
    for (int j = 0; j < numberOfClusters; j++) {
      if (0xFFFF != clusterList[j].clusterId) {
        if (endpointAndClusterIdValidation(tempEndpointsOnSink[i],
                                           clusterList[j].serverClient,
                                           clusterList[j].clusterId)) {
          gpdCommandClusterEpMap->endpoints[count] = tempEndpointsOnSink[i];
          count++;
          break;
        }
      }
    }
  }
  noOfCommissionedEndpoints = 0;
  gpdCommandClusterEpMap->numberOfEndpoints = 0;
  if (count <= MAX_ENDPOINT_COUNT) {
    gpdCommandClusterEpMap->numberOfEndpoints = count;
    noOfCommissionedEndpoints = count;
    MEMCOPY(commissionedEndPoints, gpdCommandClusterEpMap->endpoints, count);
  }
  return count;
}

static uint8_t appendClusterSupportForGenericSwitch(ZigbeeCluster * clusterPtr,
                                                    EmberGpSwitchInformation * switchInformationStruct)
{
  clusterPtr[0].clusterId = ZCL_ON_OFF_CLUSTER_ID;
  clusterPtr[0].serverClient = 1;
  return 1;
}

static bool validateSwitchCommand(uint8_t * validatedCommands,
                                  uint8_t commandIndex,
                                  uint8_t commandId,
                                  EmberGpApplicationInfo * applicationInfo,
                                  SupportedGpdCommandClusterEndpointMap * gpdCommandClusterEpMap)
{
  uint8_t noOfClusters = 0;
  ZigbeeCluster cluster[30];
  // handle switch validation
  // from user get a set of clusters that generic switch wants to use
  if (commandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
      || commandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE) {
    gpdCommandClusterEpMap[commandIndex].gpdCommand = commandId;
    ZigbeeCluster * clusterPtr = &cluster[noOfClusters];
    noOfClusters += appendClusterSupportForGenericSwitch(clusterPtr,
                                                         &gpdCommDataSaved.switchInformationStruct);
    uint8_t endpointCount = 0;
    endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo->numberOfPairedEndpoints,
                                                                        applicationInfo->pairedEndpoints,
                                                                        noOfClusters,
                                                                        cluster,
                                                                        &gpdCommandClusterEpMap[commandIndex]);
    if (endpointCount) {
      *validatedCommands += 1;
    }
    return true;
  }
  return false;
}

static uint8_t appendClustersFromApplicationDescription(ZigbeeCluster * clusterPtr,
                                                        bool reverseDirection)
{
  uint8_t count = 0;
  for (int index = 0; index < gpdCommDataSaved.numberOfReports; index++) {
    uint8_t * report = findReportId(index,
                                    gpdCommDataSaved.numberOfReports,
                                    gpdCommDataSaved.reportsStorage);
    if (report) {
      for (int j = 0;; j++) {
        uint8_t * datapointOption = findDataPointDescriptorInReport(j,
                                                                    report);
        if (datapointOption) {
          clusterPtr[count].serverClient = (datapointOption[0] & 0x08) >> 3;
          if (reverseDirection) {
            clusterPtr[count].serverClient = !(clusterPtr[count].serverClient);
          }
          clusterPtr[count].clusterId = datapointOption[1] + (((uint16_t)datapointOption[2]) << 8);
          count++;
        } else {
          break; // break the inner for loop if datapointOption is null
        }
      }
    }
  }
  return count;
}

static bool validateCARCommand(uint8_t * validatedCommands,
                               uint8_t commandIndex,
                               uint8_t commandId,
                               EmberGpApplicationInfo * applicationInfo,
                               SupportedGpdCommandClusterEndpointMap * gpdCommandClusterEpMap)
{
  uint8_t noOfClusters = 0;
  ZigbeeCluster cluster[30];
  // Handle Compact Attribute Reporting command validation, check the clusterts present in the
  // stored report descriptor and validate those cluster to see if sink has it on the eintended
  // endpoint on sink.
  if (commandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
    gpdCommandClusterEpMap[commandIndex].gpdCommand = commandId;
    ZigbeeCluster * clusterPtr = &cluster[noOfClusters];
    noOfClusters += appendClustersFromApplicationDescription(clusterPtr,
                                                             true);
    uint8_t endpointCount = 0;
    endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo->numberOfPairedEndpoints,
                                                                        applicationInfo->pairedEndpoints,
                                                                        noOfClusters,
                                                                        cluster,
                                                                        &gpdCommandClusterEpMap[commandIndex]);
    if (endpointCount) {
      *validatedCommands += 1;
    }
    return true;
  }
  return false;
}

// final list of mapping of gpdCommand-Cluster-EndpointList that is supported
static uint8_t validateGpdCommandSupportOnSink(uint8_t * gpdCommandList,
                                               uint8_t gpdCommandListLength,
                                               EmberGpApplicationInfo applicationInfo,
                                               SupportedGpdCommandClusterEndpointMap * gpdCommandClusterEpMap)
{
  uint8_t validatedCommands = 0;
  for (int commandIndex = 0; commandIndex < gpdCommandListLength; commandIndex++) {
    // First see if it is switch command - validate it else do rest of
    // the command validation
    if (validateSwitchCommand(&validatedCommands,
                              commandIndex,
                              gpdCommandList[commandIndex],
                              &applicationInfo,
                              gpdCommandClusterEpMap)) {
    } else if (validateCARCommand(&validatedCommands,
                                  commandIndex,
                                  gpdCommandList[commandIndex],
                                  &applicationInfo,
                                  gpdCommandClusterEpMap)) {
    } else {
      // default sub translation table has the (gpdCommnd<->ClusterId) mapping,
      // use the look up to decide when the mapped clusterId = 0xFFFF, then it
      // need to use the cluster list from the incoming message payload.
      // pick up clusters to validate against the supplied pair of end points
      // and available endpoints in sink;
      gpdCommandClusterEpMap[commandIndex].gpdCommand = gpdCommandList[commandIndex];
      // Prepare a cluster list that will be input for next level
      uint8_t noOfClusters = 0;
      ZigbeeCluster cluster[30] = { 0 };
      if ((applicationInfo.numberOfGpdClientCluster != 0)
          || (applicationInfo.numberOfGpdServerCluster != 0)) {
        // Now a generic cluster - read, write, reporting etc
        // many clusters are in for the command
        // prepare a list of all the clusters in the incoming message
        // pick from appInfo
        for (int i = 0; i < applicationInfo.numberOfGpdClientCluster; i++) {
          cluster[noOfClusters].clusterId = applicationInfo.clientClusters[i];
          cluster[noOfClusters].serverClient = 1; // reverse for the match
          noOfClusters++;
        }
        for (int i = 0; i < applicationInfo.numberOfGpdServerCluster; i++) {
          cluster[noOfClusters].clusterId = applicationInfo.serverClusters[i];
          cluster[noOfClusters].serverClient = 0; // reverse for the match
          noOfClusters++;
        }
      } else {
        uint8_t outIndex = 0xFF;
        uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
        const EmberAfGreenPowerServerGpdSubTranslationTableEntry* defaultTable = emGpGetDefaultTable();
        assert(defaultTable != NULL);
        if (defaultTable != NULL) {
          ret = findMatchingGenericTranslationTableEntry(DEFAULT_TABLE_ENTRY,
                                                         ADD_PAIRED_DEVICE,
                                                         0xFF,
                                                         0,
                                                         gpdCommandList[commandIndex],
                                                         0,
                                                         0,
                                                         0,
                                                         0,
                                                         NULL,
                                                         &outIndex);
        }
        if (ret != GP_TRANSLATION_TABLE_STATUS_SUCCESS && outIndex == 0xFF) {
          continue;
        }
        // Apend the cluster found in the TT (default or customised) if available
        if (0xFFFF != defaultTable[outIndex].zigbeeCluster) {
          cluster[noOfClusters].clusterId = defaultTable[outIndex].zigbeeCluster;
          cluster[noOfClusters].serverClient = defaultTable[outIndex].serverClient;
          noOfClusters += 1;
        } else {
          // Now, get the list of clusters from the device Id if the cluster associated with this
          // command in the translation table is reserved.
          noOfClusters += getClusterListFromDeviceIdLookup(applicationInfo.deviceId,
                                                           &cluster[noOfClusters]);
        }
      }
      uint8_t endpointCount = 0;
      endpointCount = buildSinkSupportedClusterEndPointListForGpdCommands(applicationInfo.numberOfPairedEndpoints,
                                                                          applicationInfo.pairedEndpoints,
                                                                          noOfClusters,
                                                                          cluster,
                                                                          &gpdCommandClusterEpMap[commandIndex]);
      if (endpointCount) {
        validatedCommands += 1;
      }
      //for loop
    }
  }
  return validatedCommands;
}

// Checks if GP can support some or all of the commands and clusters
static uint8_t emGpIsToBePairedGpdMatchSinkFunctionality(EmberGpApplicationInfo applicationInfo,
                                                         SupportedGpdCommandClusterEndpointMap * gpdCommandClusterEpMap)
{
  // Call user to get a decission
  if (!emberAfPluginGreenPowerServerGpdCommissioningCallback(&applicationInfo)) {
    return 0;
  }
  // to hold the temporary list of gpd commands derived from deviceId and added
  // from the gpdCommandList from app info
  uint8_t gpdCommandList[50] = { 0 };
  // Build a list of all the commands - present and derived from GPD ID look up
  uint8_t gpdCommandListLength = formGpdCommandListFromIncommingCommReq(applicationInfo.deviceId,
                                                                        applicationInfo.gpdCommands,
                                                                        applicationInfo.numberOfGpdCommands,
                                                                        gpdCommandList);
  // Simulate the commands as per the spec from other information in absence of
  // command list and device Id
  if (gpdCommandListLength == 0
      && 0xFE == applicationInfo.deviceId) {
    uint8_t commandIndex = 0;
    if (applicationInfo.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_APPLICATION_DESCRIPTION_PRESENT) {
      // this command is for a CAR
      gpdCommandList[commandIndex++] = EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING;
    }
    if (applicationInfo.applInfoBitmap & EMBER_AF_GP_APPLICATION_INFORMATION_CLUSTER_LIST_PRESENT) {
      // Add the AF to compact the Translation table A0/A1
      gpdCommandList[commandIndex++] = EMBER_ZCL_GP_GPDF_ANY_GPD_SENSOR_CMD;
      // Add the Tunneling as an implicit to adding reporting A0/A1
      gpdCommandList[commandIndex++] = EMBER_ZCL_GP_GPDF_ZCL_TUNNELING_WITH_PAYLOAD;
    }
    gpdCommandListLength = commandIndex;
  }

  uint8_t validatedCommands = 0;
  validatedCommands = validateGpdCommandSupportOnSink(gpdCommandList,
                                                      gpdCommandListLength,
                                                      applicationInfo,
                                                      gpdCommandClusterEpMap);
  return validatedCommands;
}

// Switch commissioning timeout handler
void emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventHandler(void)
{
  // stop the delay
  emberEventControlSetInactive(emberAfPluginGreenPowerServerGenericSwitchCommissioningTimeoutEventControl);
  emberAfGreenPowerServerCommissioningTimeoutCallback(COMMISSIONING_TIMEOUT_TYPE_GENERIC_SWITCH,
                                                      noOfCommissionedEndpoints,
                                                      commissionedEndPoints);
  resetOfMultisensorDataSaved(true);
}

void emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventHandler(void)
{
  emberEventControlSetInactive(emberAfPluginGreenPowerServerCommissioningWindowTimeoutEventControl);
  emberAfGreenPowerServerCommissioningTimeoutCallback(COMMISSIONING_TIMEOUT_TYPE_COMMISSIONING_WINDOW_TIMEOUT,
                                                      noOfCommissionedEndpoints,
                                                      commissionedEndPoints);
}
// MS commissioning timeout handler
void emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventHandler(void)
{
  // stop the delay
  emberEventControlSetInactive(emberAfPluginGreenPowerServerMultiSensorCommissioningTimeoutEventControl);
  emberAfGreenPowerServerCommissioningTimeoutCallback(COMMISSIONING_TIMEOUT_TYPE_MULTI_SENSOR,
                                                      noOfCommissionedEndpoints,
                                                      commissionedEndPoints);
  // reset RAM data saved concerning multisensor
  resetOfMultisensorDataSaved(true);
  // what else to do ?
  // emberAfGreenPowerClusterExitCommissioningMode();
}

bool emberAfGreenPowerClusterGpPairingSearchCallback(uint16_t options,
                                                     uint32_t gpdSrcId,
                                                     uint8_t* gpdIeee,
                                                     uint8_t endpoint)
{
  return true;
}

// This function Copies the addiitional information block structure to an array to be
// used for the Translation Table request/update request reponses.
uint16_t emGpCopyAdditionalInfoBlockArrayToStructure(uint8_t * additionalInfoBlockIn,
                                                     EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * additionalInfoBlockOut,
                                                     uint8_t gpdCommandId)
{
  uint8_t charCount = 0;
  uint8_t *additionalInfoBlockInPtr = additionalInfoBlockIn;
  uint8_t recordIndex = 0;
  uint8_t optionSelector = 0;
  uint8_t attributeoptions = 0;
  uint8_t totalLengthOfAddInfoBlockCnt = 0;
  emberAfGreenPowerClusterPrintln("GP SERVER - STORE ADDITIONAL INFOROMATION BLOCK into a structure");

  // copy GPD Additional information block length
  additionalInfoBlockOut->totalLengthOfAddInfoBlock = emberAfGetInt8u(additionalInfoBlockInPtr, 0, 1);
  additionalInfoBlockInPtr += sizeof(uint8_t);
  totalLengthOfAddInfoBlockCnt = additionalInfoBlockOut->totalLengthOfAddInfoBlock;

  if ((additionalInfoBlockOut->totalLengthOfAddInfoBlock) != 0x00) {
    optionSelector = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlock);
    additionalInfoBlockInPtr += sizeof(uint8_t);
    totalLengthOfAddInfoBlockCnt -= 1;

    if ( ((optionSelector & 0x0F) != 0) && (totalLengthOfAddInfoBlockCnt > 0)) {
      // copy  option selector field
      additionalInfoBlockOut->optionSelector = optionSelector;

      if (gpdCommandId  == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING ) {
        additionalInfoBlockOut->optionData.compactAttr.reportIdentifier = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;

        additionalInfoBlockOut->optionData.compactAttr.attrOffsetWithinReport = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;

        additionalInfoBlockOut->optionData.compactAttr.clusterID = emberAfGetInt16u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint16_t);
        totalLengthOfAddInfoBlockCnt -= 2;

        additionalInfoBlockOut->optionData.compactAttr.attributeID = emberAfGetInt16u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint16_t);
        totalLengthOfAddInfoBlockCnt -= 2;

        additionalInfoBlockOut->optionData.compactAttr.attributeDataType = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;

        attributeoptions = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;
        //additionalInfoBlockOut->optionRecord[recordIndex].optionData.compactAttr.manufacturerIdPresent  = ( (attributeoptions >> 1) & 0x01);
        //additionalInfoBlockOut->optionRecord[recordIndex].optionData.compactAttr.clientServer = (attributeoptions & 0x01);
        additionalInfoBlockOut->optionData.compactAttr.attributeOptions  = attributeoptions;
        if ( additionalInfoBlockOut->optionData.compactAttr.attributeOptions & 0x02 ) {
          additionalInfoBlockOut->optionData.compactAttr.manufacturerID = emberAfGetInt16u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
          additionalInfoBlockInPtr += sizeof(uint16_t);
          totalLengthOfAddInfoBlockCnt -= 1;
        }
        recordIndex++;
      } else {
        additionalInfoBlockOut->optionData.genericSwitch.contactStatus = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;

        additionalInfoBlockOut->optionData.genericSwitch.contactBitmask = emberAfGetInt8u(additionalInfoBlockInPtr, 0, totalLengthOfAddInfoBlockCnt);
        additionalInfoBlockInPtr += sizeof(uint8_t);
        totalLengthOfAddInfoBlockCnt -= 1;
      }
    }
  }
  charCount = (uint16_t)(additionalInfoBlockInPtr - additionalInfoBlockIn);
  if (additionalInfoBlockOut->totalLengthOfAddInfoBlock != (charCount - 1)) {
    emberAfGreenPowerClusterPrintln("[%s:%d] Error in Addiotional Information Block", __FUNCTION__, __LINE__);
  }
  return charCount;
}

static uint16_t copyTranslationTableEntryToBuffer(uint8_t entryIndex,
                                                  EmberAfGreenPowerServerGpdSubTranslationTableEntry * translationTableEntry,
                                                  uint8_t * tempDatabuffer,
                                                  uint8_t tempDatabufferLen)
{
  uint8_t charCount = 0;
  uint8_t *bufPtr = tempDatabuffer;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  emberAfGreenPowerClusterPrintln("GP SERVER - STORE TRANSLATION TABLE ENTRY into a buffer");
  // no more ID field

  // copy 32bit or 64bit address field
  if (emGptranslationtable->TableEntry[entryIndex].gpAddr.applicationId == EMBER_GP_APPLICATION_SOURCE_ID) {
    emberAfCopyInt32u(bufPtr, 0, emGptranslationtable->TableEntry[entryIndex].gpAddr.id.sourceId);
    bufPtr += sizeof(uint32_t);
  } else if (emGptranslationtable->TableEntry[entryIndex].gpAddr.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    MEMMOVE(bufPtr, emGptranslationtable->TableEntry[entryIndex].gpAddr.id.gpdIeeeAddress, EUI64_SIZE);
    bufPtr += EUI64_SIZE;

    // copy ieee endpoint field
    emberAfCopyInt8u(bufPtr, 0, emGptranslationtable->TableEntry[entryIndex].gpAddr.endpoint);
    bufPtr += sizeof(uint8_t);
  }

  // copy GPD command ID field
  emberAfCopyInt8u(bufPtr, 0, (uint8_t)(translationTableEntry->gpdCommand));
  bufPtr += sizeof(uint8_t);

  // copy zbEndpoint field
  emberAfCopyInt8u(bufPtr, 0, (uint8_t)(translationTableEntry->endpoint));
  bufPtr += sizeof(uint8_t);

  // copy profile field
  emberAfCopyInt16u(bufPtr, 0, (uint16_t)(translationTableEntry->zigbeeProfile));
  bufPtr += sizeof(uint16_t);

  // copy cluster field
  emberAfCopyInt16u(bufPtr, 0, (uint16_t)(translationTableEntry->zigbeeCluster));
  bufPtr += sizeof(uint16_t);

  // copy Zigbee Command ID field
  emberAfCopyInt8u(bufPtr, 0, (uint8_t)(translationTableEntry->zigbeeCommandId));
  bufPtr += sizeof(uint8_t);

  uint8_t zclPayloadLen = translationTableEntry->zclPayloadDefault[0];
  // copy Zigbee payload length ID field
  emberAfCopyInt8u(bufPtr, 0, (uint8_t)(zclPayloadLen));
  bufPtr += sizeof(uint8_t);

  // copy Zigbee payload - first byte is a length and rest is a payload
  if (zclPayloadLen > 0 && zclPayloadLen < 0xFE) {
    MEMMOVE(bufPtr, &(translationTableEntry->zclPayloadDefault[1]), zclPayloadLen);
    bufPtr += zclPayloadLen;
  }

  if (emGptranslationtable->TableEntry[entryIndex].infoBlockPresent == TRUE) {
    EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
    uint8_t additionalInfoOffset = emGptranslationtable->TableEntry[entryIndex].additionalInfoOffset;
    EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = &(additionalInfoTable->additionalInfoBlock[additionalInfoOffset]);
    charCount = emCopyAdditionalInfoBlockStructureToArray(translationTableEntry->gpdCommand,
                                                          addInfo,
                                                          bufPtr);
    if (charCount) {
      bufPtr += charCount;
    }
  }
  charCount = (uint16_t)(bufPtr - tempDatabuffer);
  return charCount;
}

bool emberAfGreenPowerClusterGpTranslationTableRequestCallback(uint8_t startIndex)
{
  uint8_t retval;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  uint8_t entryIndex = 0;
  uint8_t options = 0;

  emberAfGreenPowerClusterPrintln("Got translation table request with index %x emGptranslationtable->totalNoOfEntries = %d",
                                  startIndex,
                                  emGptranslationtable->totalNoOfEntries);
  // only respond to unicast messages.
  if (emberAfCurrentCommand()->type != EMBER_INCOMING_UNICAST) {
    emberAfGreenPowerClusterPrintln("Not unicast");
    goto kickout;
  }

  // the device SHALL check if it implements a Translation Table.
  if (EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE == 0) {
    emberAfGreenPowerClusterPrintln("Unsup cluster command");
    goto kickout;
  }

  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    return false;
  }
  if ((emGptranslationtable->totalNoOfEntries == 0)
      || (startIndex >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE)) {
    // "index" is already 0xFF if search by ID
    // or already set to the value from "SinkTableRequest" triggered frame in case it is search by INDEX
    emberAfFillCommandGreenPowerClusterGpTranslationTableResponse(EMBER_ZCL_GP_TRANSLATION_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                  0x00, //options
                                                                  emGptranslationtable->totalNoOfEntries, //totalNoOfEntries
                                                                  0x00, //startIndex
                                                                  0x00, //entryCount
                                                                  NULL,
                                                                  0);
    emberAfSendResponse();
    goto kickout;
  } else {
    for (entryIndex = startIndex; entryIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; entryIndex++) {
      if (!emGptranslationtable->TableEntry[entryIndex].entry) {
        continue;
      } else {
        break;
      }
    }
    if (entryIndex >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
      emberAfFillCommandGreenPowerClusterGpTranslationTableResponse(EMBER_ZCL_GP_TRANSLATION_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                                    0x00, //options
                                                                    emGptranslationtable->totalNoOfEntries, //totalNoOfEntries
                                                                    0x00, //startIndex
                                                                    0x00, //entryCount
                                                                    NULL,
                                                                    0);
      emberAfSendResponse();
      goto kickout;
    }
    uint16_t entriesCount = 0;
    uint8_t  tempDatabuffer[EMBER_AF_RESPONSE_BUFFER_LEN];
    uint16_t tempDataLength = 0;
    uint8_t entryCountOffset = (appResponseLength + 7);
    EmberAfGreenPowerServerGpdSubTranslationTableEntry TranslationTableEntry = { 0 };

    for (entryIndex = startIndex; entryIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; entryIndex++) {
      if (!emGptranslationtable->TableEntry[entryIndex].entry) {
        continue;
      }
      emberAfGreenPowerClusterPrintln("Send the response with translation table entries -- entryIndex = %d", entryIndex);
      retval  = emGpTransTableGetTranslationTableEntry(entryIndex, &TranslationTableEntry);
      if (retval == GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
        if (!entriesCount) {
          //Fill the options with first translation entry found after startIndex
          options = (emGptranslationtable->TableEntry[entryIndex].infoBlockPresent << 0x03 | emGptranslationtable->TableEntry[entryIndex].gpAddr.applicationId);
        }
        if ( options == ((emGptranslationtable->TableEntry[entryIndex].infoBlockPresent << 0x03)
                         | (emGptranslationtable->TableEntry[entryIndex].gpAddr.applicationId))) {
          // Copy to a temp buffer and add if there is space
          tempDataLength = copyTranslationTableEntryToBuffer(entryIndex, &TranslationTableEntry, tempDatabuffer, EMBER_AF_RESPONSE_BUFFER_LEN);
          // If space add to buffer
          if ((sizeof(appResponseData) - (entryCountOffset + 1)) >= (appResponseLength + tempDataLength)) {
            if (!entriesCount) {
              emberAfFillCommandGreenPowerClusterGpTranslationTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                            options, //options
                                                                            emGptranslationtable->totalNoOfEntries, //totalNoOfEntries
                                                                            startIndex, //startIndex
                                                                            0x00, //entryCount
                                                                            NULL,
                                                                            0);
            }
            MEMMOVE(&appResponseData[appResponseLength], tempDatabuffer, tempDataLength);
            appResponseLength +=  tempDataLength;
            entriesCount++;
          } else {
            break;
          }
        }
      }
    }
    if (entriesCount == 0) {
      emberAfGreenPowerClusterPrintln("INSUFFICIENT SPACE"); //send Insufficient space message if not even one entry could fit in the response
      emberAfFillCommandGreenPowerClusterGpTranslationTableResponse(EMBER_ZCL_STATUS_INSUFFICIENT_SPACE,
                                                                    0x00, //options
                                                                    emGptranslationtable->totalNoOfEntries, //totalNoOfEntries
                                                                    0x00, //startIndex
                                                                    0x00, //entryCount
                                                                    NULL,
                                                                    0);
      emberAfSendResponse();
      goto kickout;
    } else {
      //Insert the number of entries actually included @ entryCountOffset
      appResponseData[entryCountOffset] = entriesCount;
      EmberStatus status = emberAfSendResponse();
      if (status == EMBER_MESSAGE_TOO_LONG) {
        emberAfFillCommandGreenPowerClusterGpTranslationTableResponse(EMBER_ZCL_STATUS_INSUFFICIENT_SPACE,
                                                                      0x00, //options
                                                                      emGptranslationtable->totalNoOfEntries, //totalNoOfEntries
                                                                      0x00, //startIndex
                                                                      0x00, //entryCount
                                                                      NULL,
                                                                      0);
        emberAfSendResponse();
      }
    }
  }           //end of else
  kickout: return true;
}

bool emberAfGreenPowerClusterGpTranslationTableUpdateCallback(uint16_t options,
                                                              uint32_t gpdSrcId,
                                                              uint8_t* gpdIeee,
                                                              uint8_t gpdEndpoint,
                                                              uint8_t* translations)
{
  uint8_t retval = 0;
  uint16_t payloadOffset = 0;
  uint8_t gpApplicationId = (options & 0x0007);
  EmberGpAddress gpdAddr;
  uint8_t allZeroesIeeeAddress[17] = { 0 };

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = allZeroesIeeeAddress;
  }

  if (!emGpMakeAddr(&gpdAddr, gpApplicationId, gpdSrcId, gpdIeee, gpdEndpoint)) {
    return false;
  }
  uint8_t action = ((options >> 3) & 0x0003); //3..4bits
  uint8_t noOfTranslations = ((options >> 5) & 0x0003); //5..7 bits
  bool additionalInfoBlockPresent = ((options >> 8) & 0x0001); //8 bits
  uint8_t index = 0;
  uint8_t gpdCommandId = 0;
  uint8_t zbEndpoint = 0;
  uint16_t zigbeeProfile;
  uint16_t zigbeeCluster = 0;
  uint8_t  zigbeeCommandId = 0;
  uint8_t payloadLength = 0;
  uint8_t payload[EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN] = { 0 };
  uint8_t payloadSrc = 0;
  uint8_t additionalInfoLength = 0;
  uint8_t* additionalInfoBlockIn = NULL;
  uint8_t* translationsEntryPtr = translations;
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField additionalInfoBlockOut;
  for (uint8_t i = 0; i < noOfTranslations; i++) {
    index = (uint8_t)emberAfGetInt8u(translationsEntryPtr, payloadOffset, payloadOffset + 1); //1byte
    payloadOffset += 1u;
    gpdCommandId = (uint8_t)emberAfGetInt8u(translationsEntryPtr, payloadOffset, (payloadOffset + 1)); //1byte
    payloadOffset += 1u;
    zbEndpoint = (uint8_t)emberAfGetInt8u(translationsEntryPtr, payloadOffset, (payloadOffset + 1)); //1byte
    payloadOffset += 1u;
    zigbeeProfile = (uint16_t)emberAfGetInt16u(translationsEntryPtr, payloadOffset, (payloadOffset + 2)); //2byte
    payloadOffset += 2u;
    zigbeeCluster = (uint16_t)emberAfGetInt16u(translationsEntryPtr, payloadOffset, (payloadOffset + 2)); //2byte
    payloadOffset += 2u;
    zigbeeCommandId = (uint8_t)emberAfGetInt8u(translationsEntryPtr, payloadOffset, (payloadOffset + 1)); //1byte
    payloadOffset += 1u;
    payloadLength = (uint8_t)emberAfGetInt8u(translationsEntryPtr, payloadOffset, (payloadOffset + 1)); //1byte
    payloadOffset += 1u;
    if (payloadLength > 0) {
      //Do not copy the payload when payloadLength is 0xFF 0r 0xFE
      if ( payloadLength <= EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN) {
        MEMCOPY(payload, &translationsEntryPtr[payloadOffset], payloadLength);
        payloadOffset += payloadLength;
      }
    }
    if (additionalInfoBlockPresent) {
      additionalInfoBlockIn = &translationsEntryPtr[payloadOffset];
      retval = emGpCopyAdditionalInfoBlockArrayToStructure(additionalInfoBlockIn, &additionalInfoBlockOut, gpdCommandId);
      additionalInfoLength = retval;
      payloadOffset += retval;
    }

    index = 0xFF; //In the current version of the specification, the Index field SHALL always be set to 0xff
                  //upon transmis-sion and SHALL always be ignored on reception
    if ( action == 0x00) {
      retval = emGpTransTableAddTranslationTableEntryUpdateCommand(index,
                                                                   additionalInfoBlockPresent,
                                                                   &gpdAddr,
                                                                   gpdCommandId,
                                                                   zbEndpoint,
                                                                   zigbeeProfile,
                                                                   zigbeeCluster,
                                                                   zigbeeCommandId,
                                                                   payloadLength,
                                                                   payload,
                                                                   payloadSrc,
                                                                   additionalInfoLength,
                                                                   &additionalInfoBlockOut);
      if (retval != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
        if ( retval == GP_TRANSLATION_TABLE_STATUS_ENTRY_NOT_EMPTY ) {
          emberAfGreenPowerClusterPrintln("Entry @Index [%d] is not empty", index);
        } else if ( retval == GP_TRANSLATION_TABLE_STATUS_PARAM_DOES_NOT_MATCH) {
          emberAfGreenPowerClusterPrintln("Parameter does not match @Index [%d]", index);
          emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE); //send failure notification immediately
        }
      }
    } else if (action == 0x01) {
      retval = emGpTransTableReplaceTranslationTableEntryUpdateCommand(index,
                                                                       additionalInfoBlockPresent,
                                                                       &gpdAddr,
                                                                       gpdCommandId,
                                                                       zbEndpoint,
                                                                       zigbeeProfile,
                                                                       zigbeeCluster,
                                                                       zigbeeCommandId,
                                                                       payloadLength,
                                                                       payload,
                                                                       payloadSrc,
                                                                       additionalInfoLength,
                                                                       &additionalInfoBlockOut);
      if (retval != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
        if ( retval == GP_TRANSLATION_TABLE_STATUS_ENTRY_EMPTY) {
          emberAfGreenPowerClusterPrintln("Entry @Index [%d] is empty", index);
        } else if (retval == GP_TRANSLATION_TABLE_STATUS_PARAM_DOES_NOT_MATCH) {
          emberAfGreenPowerClusterPrintln("Parameter does not match @Index [%d]", index);
          emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
        }
      }
    } else if (action == 0x02) {
      retval = emGpTransTableRemoveTranslationTableEntryUpdateCommand(index,
                                                                      additionalInfoBlockPresent,
                                                                      &gpdAddr,
                                                                      gpdCommandId,
                                                                      zbEndpoint,
                                                                      zigbeeProfile,
                                                                      zigbeeCluster,
                                                                      zigbeeCommandId,
                                                                      payloadLength,
                                                                      payload,
                                                                      payloadSrc,
                                                                      additionalInfoLength,
                                                                      &additionalInfoBlockOut);
      if (retval != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
        if (retval == GP_TRANSLATION_TABLE_STATUS_ENTRY_EMPTY) {
          emberAfGreenPowerClusterPrintln("Entry @Index [%d] is empty", index);
        } else if (retval == GP_TRANSLATION_TABLE_STATUS_PARAM_DOES_NOT_MATCH) {
          emberAfGreenPowerClusterPrintln("Parameter does not match @Index [%d]", index);
          emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_FAILURE);
        }
      }
    } else {
      goto kickout;
    }
    translationsEntryPtr += payloadOffset; //If successful move the pointer to the next translation
    payloadOffset = 0;
  }
  kickout: return true;
}

static uint16_t emberAfGreenPowerServerStoreSinkTableEntry(EmberGpSinkTableEntry* entry,
                                                           uint8_t* buffer)
{
  uint8_t *finger = buffer;
  uint8_t securityLevel = entry->securityOptions & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_LEVEL;
  uint8_t securityKeyType = (entry->securityOptions
                             & EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE)
                            >> EMBER_AF_GP_SINK_TABLE_ENTRY_SECURITY_OPTIONS_SECURITY_KEY_TYPE_OFFSET;

  emberAfGreenPowerClusterPrintln("GP SERVER - STORE SINK TABLE ENTRY into a buffer");
  // copy options field
  emberAfCopyInt16u(finger, 0, (uint16_t)(entry->options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_MASK));
  finger += sizeof(uint16_t);
  // copy 32bit or 64bit address field
  if (entry->gpd.applicationId == EMBER_GP_APPLICATION_SOURCE_ID) {
    emberAfCopyInt32u(finger, 0, entry->gpd.id.sourceId);
    finger += sizeof(uint32_t);
  } else if (entry->gpd.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    MEMMOVE(finger, entry->gpd.id.gpdIeeeAddress, EUI64_SIZE);
    finger += EUI64_SIZE;
  }
  // copy ieee endpoint field
  if (entry->gpd.applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS) {
    emberAfCopyInt8u(finger, 0, entry->gpd.endpoint);
    finger += sizeof(uint8_t);
  }
  // DeviceID field
  emberAfGreenPowerClusterPrintln("GPD deviceId %x", entry->deviceId);
  emberAfCopyInt8u(finger, 0, entry->deviceId);
  finger += sizeof(uint8_t);

  // copy Group List field if present
  EmberGpSinkType sinkCommunicationMode = (EmberGpSinkType)((entry->options
                                                             & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE)
                                                            >> EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_COMMUNICATION_MODE_OFFSET);
  if (sinkCommunicationMode == EMBER_GP_SINK_TYPE_GROUPCAST) {
    // let's count
    uint8_t index = 0;
    uint8_t *entryCount = finger;
    emberAfCopyInt8u(finger, 0, 0x00);
    finger += sizeof(uint8_t);
    for (index = 0; index < GP_SINK_LIST_ENTRIES; index++) {
      // table is available
      EmberGpSinkListEntry * sinkEntry = &entry->sinkList[index];
      if (sinkEntry->type == EMBER_GP_SINK_TYPE_GROUPCAST) {
        emberAfCopyInt16u(finger, 0, sinkEntry->target.groupcast.groupID);
        finger += sizeof(uint16_t);
        emberAfCopyInt16u(finger, 0, sinkEntry->target.groupcast.alias);
        finger += sizeof(uint16_t);
        (*entryCount)++;
      } else {
        continue;
      }
    }
  }
  // copy GPD assigned alias field
  if (entry->options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_ASSIGNED_ALIAS) {
    emberAfGreenPowerClusterPrintln("assigned alias %2x", entry->assignedAlias);
    emberAfCopyInt16u(finger, 0, entry->assignedAlias);
    finger += sizeof(uint16_t);
  }
  // copy Groupcast radius field
  emberAfCopyInt8u(finger, 0, entry->groupcastRadius);
  finger += sizeof(uint8_t);

  // copy Security Options field
  if (entry->options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SECURITY_USE) {
    emberAfGreenPowerClusterPrintln("security options %1x", entry->securityOptions);
    emberAfCopyInt8u(finger, 0, entry->securityOptions);
    finger += sizeof(uint8_t);

    // copy GPD Sec Frame Counter (if lvl>0 or if SeqNumCapability=0b1)
    // Lvl>0 is check cause it is allow to set SecurityUse=0b1 and tell SecLvl=0
    if ( (securityLevel > EMBER_GP_SECURITY_LEVEL_NONE)
         || (entry->options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES) ) {
      emberAfGreenPowerClusterPrintln("security frame counter %4x", entry->gpdSecurityFrameCounter);
      emberAfCopyInt32u(finger, 0, entry->gpdSecurityFrameCounter);
      finger += sizeof(uint32_t);

      // If SecurityLevel is 0b00 or if the SecurityKeyType has value:
      // - 0b001 (NWK key),
      // - 0b010 (GPD group key)
      // - 0b111 (derived individual GPD key),
      // the GPDkey parameter MAY be omitted and the key MAY be stored in the
      // gpSharedSecurityKey parameter instead.
      // If SecurityLevel has value other than 0b00 and the SecurityKeyType has
      // value 0b111 (derived individual GPD key), the sink table GPDkey parameter MAY be
      // omitted and the key MAY calculated on the fly, based on the value stored
      // in the gpSharedSecurityKey parameter (=groupKey).
      if ( !(securityLevel == EMBER_GP_SECURITY_LEVEL_NONE) ) {
        if ( !(securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_ZIGBEE_NETWORK_KEY)
             && !(securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_GPD_GROUP_KEY)
             && !(securityKeyType == EMBER_ZCL_GP_SECURITY_KEY_TYPE_DERIVED_INDIVIDUAL_GPD_KEY) ) {
          // if keyType is OOB and NK, else it is possible to use "gpsSharedSecurityKey" attribut
          // to save space in the sink table
          MEMMOVE(finger, entry->gpdKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
          finger += EMBER_ENCRYPTION_KEY_SIZE;
        } else {
          // key is present in the reponse message but not store in the sink table but in
          // the gpSharedSecurityKey attribut, so read "gpSharedSecurityKey" attribut
          EmberKeyData gpSharedSecurityKey;
          EmberAfAttributeType type;
          EmberAfStatus status = emberAfReadAttribute(GP_ENDPOINT,
                                                      ZCL_GREEN_POWER_CLUSTER_ID,
                                                      ZCL_GP_SERVER_GP_SHARED_SECURITY_KEY_ATTRIBUTE_ID,
                                                      CLUSTER_MASK_SERVER,
                                                      gpSharedSecurityKey.contents,
                                                      EMBER_ENCRYPTION_KEY_SIZE,
                                                      &type);
          if (status == EMBER_ZCL_STATUS_SUCCESS) {
            MEMMOVE(finger, gpSharedSecurityKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
          } else {
            // optional "gpsSharedSecurityKey" attribut is not supported
            // thus key is always into the sink table even if it is shared
            MEMMOVE(finger, entry->gpdKey.contents, EMBER_ENCRYPTION_KEY_SIZE);
          }
          finger += EMBER_ENCRYPTION_KEY_SIZE;
        }
      }
    }
  } else if (entry->options & EMBER_AF_GP_SINK_TABLE_ENTRY_OPTIONS_SEQUENCE_NUM_CAPABILITIES) {
    // gpdSecurityFrameCounter is mandatory if Security use = 0b1 (treated above)
    //      or Sequence number capabilities = 0b1 and Security use = 0b0
    emberAfGreenPowerClusterPrintln("security frame counter %4x", entry->gpdSecurityFrameCounter);
    emberAfCopyInt32u(finger, 0, entry->gpdSecurityFrameCounter);
    finger += sizeof(uint32_t);
  } else {
    // if no security and no seq number capability, nothing more to copy into the response
  }
  return (uint16_t)(finger - buffer);
}

bool emberAfGreenPowerClusterGpSinkTableRequestCallback(uint8_t options,
                                                        uint32_t gpdSrcId,
                                                        uint8_t* gpdIeee,
                                                        uint8_t endpoint,
                                                        uint8_t index)
{
  uint8_t entryIndex = 0;
  uint8_t validEntriesCount = 0;
  uint8_t appId = options & EMBER_AF_GP_SINK_TABLE_REQUEST_OPTIONS_APPLICATION_ID;
  uint8_t requestType = (options & EMBER_AF_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TYPE)
                        >> EMBER_AF_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TYPE_OFFSET;
  EmberGpSinkTableEntry entry;
  uint8_t allZeroesIeeeAddress[17] = { 0 };

  //handle null args for EZSP
  if (gpdIeee  == NULL) {
    gpdIeee = allZeroesIeeeAddress;
  }

  emberAfGreenPowerClusterPrintln("Got sink table request with options %1x and index %1x", options, index);
  // only respond to unicast messages.
  if (emberAfCurrentCommand()->type != EMBER_INCOMING_UNICAST) {
    emberAfGreenPowerClusterPrintln("Not unicast");
    goto kickout;
  }

  // the device SHALL check if it implements a Sink Table.
  // If not, it SHALL generate a ZCL Default Response command,
  // with the Status code field carrying UNSUP_CLUSTER_COMMAND, subject to the rules as specified in sec. 2.4.12 of [3]
  if (EMBER_GP_SINK_TABLE_SIZE == 0) {
    emberAfGreenPowerClusterPrintln("Unsup cluster command");
    emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_UNSUP_CLUSTER_COMMAND);
    return true;
  }

  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X", emberAfCurrentEndpoint());
    return false;
  }

  for (entryIndex = 0; entryIndex < EMBER_GP_SINK_TABLE_SIZE; entryIndex++) {
    if (emberGpSinkTableGetEntry(entryIndex, &entry) == EMBER_SUCCESS) {
      if (entry.status == EMBER_GP_SINK_TABLE_ENTRY_STATUS_ACTIVE) {
        validEntriesCount++;
      }
    }
  }
  // If its Sink Table is empty, and the triggering GP Sink Table Request was received in unicast,
  // then the GP Sink Table Response SHALL be sent with Status NOT_FOUND,
  // Total number of non-empty Sink Table entries carrying 0x00,
  // Start index carrying 0xFF (in case of request by GPD ID) or
  // the Index value from the triggering GP Sink Table Request (in case of request by index),
  // Entries count field set to 0x00, and any Sink Table entry fields absent
  if (validEntriesCount == 0) {
    // "index" is already 0xFF if search by ID
    // or already set to the value from "SinkTableRequest" triggered frame in case it is search by INDEX
    emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                           0x00,
                                                           index,
                                                           0x00,
                                                           NULL,
                                                           0);
    emberAfSendResponse();
    return true;
  } else {
    // Valid Entries are present!
    if (requestType == EMBER_ZCL_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TABLE_ENTRIES_BY_GPD_ID) {
      EmberGpAddress gpdAddr;
      emGpMakeAddr(&gpdAddr, appId, gpdSrcId, gpdIeee, endpoint);
      entryIndex = emberGpSinkTableLookup(&gpdAddr);
      if (entryIndex == 0xFF) {
        // Valid entries present but none for this gpdAddr - Send NOT FOUND sesponse.
        emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                               validEntriesCount,
                                                               entryIndex,
                                                               0x00,
                                                               NULL,
                                                               0);
        emberAfSendResponse();
        goto kickout;
      } else {
        // A valid entry with the ID is present
        if (emberGpSinkTableGetEntry(entryIndex, &entry) == EMBER_SUCCESS) {
          // If the triggering GP Sink Table Request command contained a GPD ID field, the device SHALL check
          // if it has a Sink Table entry for this GPD ID (and Endpoint, if ApplicationID = 0b010). If yes, the device
          // SHALL create a GP Sink Table Response with Status SUCCESS, Total number of non-empty Sink Table
          // entries carrying the total number of non-empty Sink Table entries on this device, Start index set to
          // 0xff, Entries count field set to 0x01, and one Sink Table entry field for the requested GPD ID (and
          // Endpoint, if ApplicationID = 0b010), formatted as specified in sec. A.3.3.2.2.1, present.
          emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                 validEntriesCount,
                                                                 0xff,
                                                                 1,
                                                                 NULL,
                                                                 0);
          appResponseLength += emberAfGreenPowerServerStoreSinkTableEntry(&entry, (appResponseData + appResponseLength));
          emberAfSendResponse();
        } else {
          // Not found status to go out.
        }
        goto kickout;
      }
    } else if (requestType == EMBER_ZCL_GP_SINK_TABLE_REQUEST_OPTIONS_REQUEST_TABLE_ENTRIES_BY_INDEX) {
      if (index >= validEntriesCount) {
        emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_NOT_FOUND,
                                                               validEntriesCount,
                                                               index,
                                                               0x00,
                                                               NULL,
                                                               0);
        emberAfSendResponse();
        return true;
      } else {
        // return the sink table entry content into the reponse payload from indicated
        // index and nexts until these are consistant (adress type, etc) and
        // as long as it feet into one message
        emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                               validEntriesCount,
                                                               index,
                                                               0xff, //validEntriesCount - index??,
                                                               NULL, // ?? is there a way to indicate the pointer and the lenght of the entry
                                                               0);
        validEntriesCount = 0;
        uint16_t entriesCount = 0;
        for (entryIndex = 0; entryIndex < EMBER_GP_SINK_TABLE_SIZE; entryIndex++) {
          if (emberGpSinkTableGetEntry(entryIndex, &entry) != EMBER_SUCCESS) {
            continue;
          }

          uint8_t  tempDatabuffer[EMBER_AF_RESPONSE_BUFFER_LEN];
          uint16_t tempDataLength = 0;

          if (entry.status != EMBER_GP_SINK_TABLE_ENTRY_STATUS_UNUSED
              && entry.status != 0) {
            validEntriesCount++;
            if (validEntriesCount > index) {
              // Copy to a temp buffer and add if there is space
              tempDataLength = emberAfGreenPowerServerStoreSinkTableEntry(&entry, tempDatabuffer);
              // If space add to buffer
              if (sizeof(appResponseData) > (appResponseLength + tempDataLength)) {
                MEMMOVE(&appResponseData[appResponseLength], tempDatabuffer, tempDataLength);
                appResponseLength +=  tempDataLength;
                entriesCount++;
              } else {
                break;
              }
            }
          }
        }
        //Insert the number of entries actually included
        appResponseData[GP_SINK_TABLE_RESPONSE_ENTRIES_OFFSET + GP_NON_MANUFACTURER_ZCL_HEADER_LENGTH] = entriesCount;
        EmberStatus status = emberAfSendResponse();
        if (status == EMBER_MESSAGE_TOO_LONG) {
          emberAfFillCommandGreenPowerClusterGpSinkTableResponse(EMBER_ZCL_GP_SINK_TABLE_RESPONSE_STATUS_SUCCESS,
                                                                 validEntriesCount,
                                                                 index,
                                                                 0x00,
                                                                 NULL,
                                                                 0);
          emberAfSendResponse();
        }
        goto kickout;
      }
    } else {
      // nothing, other value of requestType are reserved
    }
  }
  kickout: return true;
}

bool emberAfGreenPowerClusterGpProxyTableResponseCallback(uint8_t status,
                                                          uint8_t totalNumberofNonEmptyProxyTableEntries,
                                                          uint8_t startIndex,
                                                          uint8_t entriesCount,
                                                          uint8_t* proxyTableEntry)
{
  if (emberAfCurrentEndpoint() != GP_ENDPOINT) {
    emberAfGreenPowerClusterPrintln("Drop frame due to unknown endpoint: %X",
                                    emberAfCurrentEndpoint());
  } else {
    emberAfCorePrintln("status: %d \t totalNumberofNonEmptyProxyTableEntries: \
                       %d \t startIndex :%d \t entriesCount :%d", status,
                       totalNumberofNonEmptyProxyTableEntries, startIndex,
                       entriesCount);
    if (proxyTableEntry != NULL) {
      int i = 0;
      emberAfCorePrint("proxyTableEntry [");
      while (proxyTableEntry[i]) {
        emberAfCorePrint("%02x", (unsigned int) proxyTableEntry[i]);
        i++;
      }
      emberAfCorePrintln("] \n");
    } else {
      emberAfCorePrintln("proxyTableEntry [ NULL ]");
    }
  }
  return false;
}

static uint8_t compareTranslationTableAdditionalInfoBlockEntry(uint8_t gpdCommand,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* srcAddlInfoBlock,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* dstAddlInfoBlock)
{
  if (gpdCommand == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING) {
    if ((srcAddlInfoBlock->optionData.compactAttr.reportIdentifier == dstAddlInfoBlock->optionData.compactAttr.reportIdentifier)
        && (srcAddlInfoBlock->optionData.compactAttr.attrOffsetWithinReport == dstAddlInfoBlock->optionData.compactAttr.attrOffsetWithinReport)
        && (srcAddlInfoBlock->optionData.compactAttr.clusterID == dstAddlInfoBlock->optionData.compactAttr.clusterID)
        && (srcAddlInfoBlock->optionData.compactAttr.attributeID == dstAddlInfoBlock->optionData.compactAttr.attributeID)
        && (srcAddlInfoBlock->optionData.compactAttr.attributeDataType == dstAddlInfoBlock->optionData.compactAttr.attributeDataType)
        && (srcAddlInfoBlock->optionData.compactAttr.attributeOptions == dstAddlInfoBlock->optionData.compactAttr.attributeOptions)
        && (srcAddlInfoBlock->optionData.compactAttr.manufacturerID == dstAddlInfoBlock->optionData.compactAttr.manufacturerID)) {
      return true;
    }
  } else if (gpdCommand == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
             || gpdCommand == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE) {
    if ((srcAddlInfoBlock->optionData.genericSwitch.contactStatus == dstAddlInfoBlock->optionData.genericSwitch.contactStatus)
        && (srcAddlInfoBlock->optionData.genericSwitch.contactBitmask == dstAddlInfoBlock->optionData.genericSwitch.contactBitmask)) {
      return true;
    }
  }
  return false;
}

static uint8_t createTanslationTableAdditionalInfoBlockEntry(uint8_t gpdCommand,
                                                             EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = NULL;
  for (uint8_t optionRecord = 0; optionRecord < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE; optionRecord++) {
    addInfo = &(additionalInfoTable->additionalInfoBlock[optionRecord]);
    if (compareTranslationTableAdditionalInfoBlockEntry(gpdCommand, addInfo, additionalInfoBlock)) {
      additionalInfoTable->validEntry[optionRecord]++;
      emGpSetAdditionalInfoBlockTableEntry(optionRecord);
      return optionRecord;
    }
  }
  for (uint8_t optionRecord = 0; optionRecord < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE; optionRecord++) {
    addInfo = &(additionalInfoTable->additionalInfoBlock[optionRecord]);
    if (additionalInfoTable->validEntry[optionRecord] == 0) {
      MEMCOPY(addInfo, additionalInfoBlock, sizeof(EmberGpTranslationTableAdditionalInfoBlockOptionRecordField));
      additionalInfoTable->validEntry[optionRecord]  += 1;
      additionalInfoTable->totlaNoOfEntries++;
      emGpSetAdditionalInfoBlockTableEntry(optionRecord);
      return optionRecord;
    }
  }
  return 0xFF;
}

static uint8_t deleteTanslationTableAdditionalInfoBlockEntry(uint8_t gpdCommand,
                                                             EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  uint8_t optionRecord = 0;
  EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = NULL;
  for (optionRecord = 0; optionRecord < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE; optionRecord++) {
    addInfo = &(additionalInfoTable->additionalInfoBlock[optionRecord]);
    if (compareTranslationTableAdditionalInfoBlockEntry(gpdCommand, addInfo, additionalInfoBlock)) {
      if (additionalInfoTable->validEntry[optionRecord]) {
        additionalInfoTable->validEntry[optionRecord]--;
        if (additionalInfoTable->validEntry[optionRecord] == 0) {
          MEMSET(addInfo, 0x00, sizeof(EmberGpTranslationTableAdditionalInfoBlockOptionRecordField));
          additionalInfoTable->totlaNoOfEntries--;
        }
        emGpSetAdditionalInfoBlockTableEntry(optionRecord);
        return optionRecord;
      }
    }
  }
  return 0xFF;
}

bool isCustomizedTableEntryReferenced(uint8_t index, uint8_t offset)
{
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  if ((emGptranslationtable != NULL) && (offset != 0xFF)) {
    for (uint8_t i = 0; i < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; i++) {
      if ((emGptranslationtable->TableEntry[i].entry == CUSTOMIZED_TABLE_ENTRY)
          && (emGptranslationtable->TableEntry[i].offset == offset)
          && (index != i)) {
        return true;           //do not delete as refernce is found for this customized table entry
      }
    }
  }
  return false;
}
void deleteCustomizednTableEntry(uint8_t index)
{
  EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTable = emGpGetCustomizedTable();
  if (index < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE) {
    MEMSET(&(customizedTable[index]),
           0x00,
           sizeof(EmberAfGreenPowerServerGpdSubTranslationTableEntry));
    emGpSetCustomizedTableEntry(index);
  }
}

static uint8_t emGpTransTableCreateCustomizedTranslationTableEntry(bool infoBlockPresent,
                                                                   EmberGpAddress * gpdAddr,
                                                                   uint8_t gpdCommandId,
                                                                   uint8_t zbEndpoint,
                                                                   uint16_t zigbeeProfile,
                                                                   uint16_t zigbeeCluster,
                                                                   uint8_t  zigbeeCommandId,
                                                                   uint8_t payloadLength,
                                                                   uint8_t* payload,
                                                                   uint8_t payloadSrc,
                                                                   uint8_t* outIndex)
{
  uint8_t cTableIndex;
  EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTable = emGpGetCustomizedTable();
  EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTableEntry = NULL;
  if (customizedTable == NULL) {
    return GP_TRANSLATION_TABLE_STATUS_FAILED;
  }
  for (cTableIndex = 0; cTableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE; cTableIndex++) {
    customizedTableEntry = &(customizedTable[cTableIndex]);
    if (customizedTableEntry->validEntry == false) {
      break;
    }
  }
  if (cTableIndex == EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE) {
    return GP_TRANSLATION_TABLE_STATUS_FULL;
  }
  customizedTableEntry->gpdCommand = gpdCommandId;
  customizedTableEntry->endpoint = zbEndpoint;
  customizedTableEntry->zigbeeProfile = zigbeeProfile;
  customizedTableEntry->zigbeeCluster = zigbeeCluster;
  customizedTableEntry->zigbeeCommandId = zigbeeCommandId;
  if (payloadSrc) {
    customizedTableEntry->payloadSrc = payloadSrc;
  } else {
    customizedTableEntry->payloadSrc = EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA;
  }
  customizedTableEntry->zclPayloadDefault[0] = payloadLength;
  if (payloadLength > 0 && payloadLength < 0xFE
      &&  payloadLength < EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN) {
    MEMCOPY(&customizedTableEntry->zclPayloadDefault[1], payload, payloadLength);
  }
  customizedTableEntry->validEntry = true;
  *outIndex = cTableIndex;
  emGpSetCustomizedTableEntry(cTableIndex);
  return GP_TRANSLATION_TABLE_STATUS_SUCCESS;
}

static uint8_t findMatchingGenericTranslationTableEntry(uint8_t entryType,
                                                        uint8_t incomingReqType,
                                                        uint8_t offset,
                                                        bool infoBlockPresent,
                                                        uint8_t gpdCommandId,
                                                        uint16_t zigbeeProfile,
                                                        uint16_t zigbeeCluster,
                                                        uint8_t  zigbeeCommandId,
                                                        uint8_t payloadLength,
                                                        uint8_t* payload,
                                                        uint8_t* outIndex)
{
  uint8_t tableSize = 0xFF;
  EmberAfGreenPowerServerGpdSubTranslationTableEntry const* genericTable = NULL;
  EmberAfGreenPowerServerGpdSubTranslationTableEntry const* genericTableEntry = NULL;

  if (entryType == CUSTOMIZED_TABLE_ENTRY) {
    genericTable = emGpGetCustomizedTable();
    tableSize = EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE;
  } else if (entryType == DEFAULT_TABLE_ENTRY) {
    genericTable = emGpGetDefaultTable();
    tableSize = SIZE_OF_DEFAULT_SUB_TRANSLATION_TABLE;
  }

  if (genericTable == NULL || tableSize == 0xFF) {
    return GP_TRANSLATION_TABLE_STATUS_FAILED;
  }
  if (offset == 0xFF) {
    //If the offset is 0xFF then search entire Customized Table to get the matching entry
    for (uint8_t index = 0; (index < tableSize); index++) {
      genericTableEntry = &(genericTable[index]);
      if (genericTableEntry->validEntry != true) {
        continue;
      }
      if ((genericTableEntry->gpdCommand == gpdCommandId)
          && ((incomingReqType == ADD_PAIRED_DEVICE)
              || ((incomingReqType == TRANSLATION_TABLE_UPDATE)
                  && (genericTableEntry->zigbeeProfile == zigbeeProfile)
                  && (genericTableEntry->zigbeeCluster == zigbeeCluster)
                  && (genericTableEntry->zigbeeCommandId == zigbeeCommandId)
                  && (genericTableEntry->zclPayloadDefault[0] == payloadLength)))) {
        *outIndex = index;
        break;
      }
    }           // end of for
  } else {
    genericTableEntry = &(genericTable[offset]);
    //If the offset is not 0xFF then look for matching entry in customized Table[offset]
    if (genericTableEntry->validEntry == true) {
      if ((genericTableEntry->gpdCommand == gpdCommandId)
          && ((incomingReqType == ADD_PAIRED_DEVICE)
              || ((incomingReqType == TRANSLATION_TABLE_UPDATE)
                  && (genericTableEntry->zigbeeProfile == zigbeeProfile)
                  && (genericTableEntry->zigbeeCluster == zigbeeCluster)))) {
        *outIndex = offset;
      }
    }
  }

  if (*outIndex != 0xFF) {
    return GP_TRANSLATION_TABLE_STATUS_SUCCESS;
  }
  return GP_TRANSLATION_TABLE_STATUS_FAILED;
}

static uint8_t emGpTransTableAddTranslationTableEntryByIndex(uint8_t incomingReqType,
                                                             uint8_t index,
                                                             bool infoBlockPresent,
                                                             EmberGpAddress  * gpdAddr,
                                                             uint8_t gpdCommandId,
                                                             uint8_t zbEndpoint,
                                                             uint16_t zigbeeProfile,
                                                             uint16_t zigbeeCluster,
                                                             uint8_t  zigbeeCommandId,
                                                             uint8_t payloadLength,
                                                             uint8_t* payload,
                                                             uint8_t payloadSrc,
                                                             uint8_t additionalInfoLength,
                                                             EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  uint8_t outIndex = 0xFF;
  uint8_t entryType = NO_ENTRY;
  for (entryType = CUSTOMIZED_TABLE_ENTRY; entryType > NO_ENTRY; entryType--) {
    // First search for the matching entry in the Customized table If not found then search in the Default Table
    //
    ret = findMatchingGenericTranslationTableEntry(entryType,
                                                   incomingReqType,
                                                   0xFF,
                                                   infoBlockPresent,
                                                   gpdCommandId,
                                                   zigbeeProfile,
                                                   zigbeeCluster,
                                                   zigbeeCommandId,
                                                   payloadLength,
                                                   payload,
                                                   &outIndex);
    if (ret == GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
      break;
    }
  }
  if ((outIndex == 0xFF) && (entryType == NO_ENTRY)) {
    ret = emGpTransTableCreateCustomizedTranslationTableEntry(infoBlockPresent,
                                                              gpdAddr,
                                                              gpdCommandId,
                                                              zbEndpoint,
                                                              zigbeeProfile,
                                                              zigbeeCluster,
                                                              zigbeeCommandId,
                                                              payloadLength,
                                                              payload,
                                                              payloadSrc,
                                                              &outIndex);

    if (ret != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
      return ret;
    } else {
      entryType = CUSTOMIZED_TABLE_ENTRY;
    }
  }
  if (outIndex != 0xFF) {
    if (emGptranslationtable->TableEntry[index].entry == NO_ENTRY) {
      //Do not Increment total number of entries while replacing Table entry
      emGptranslationtable->totalNoOfEntries++;
      MEMSET(&emGptranslationtable->TableEntry[index], 0x00, sizeof(EmGpCommandTranslationTableEntry));
      emGptranslationtable->TableEntry[index].offset = 0xFF;
      emGptranslationtable->TableEntry[index].additionalInfoOffset = 0xFF;
    }
    if (infoBlockPresent) {
      uint8_t addInfoOffset = emGptranslationtable->TableEntry[index].additionalInfoOffset;
      if ((emGptranslationtable->TableEntry[index].entry != NO_ENTRY) && (addInfoOffset != 0xFF)) {
        EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
        EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = &(additionalInfoTable->additionalInfoBlock[addInfoOffset]);
        uint8_t status = deleteTanslationTableAdditionalInfoBlockEntry(gpdCommandId, addInfo);
        if (status != 0xFF) {
          emGptranslationtable->TableEntry[index].additionalInfoOffset = 0xFF;
        }
      }
      emGptranslationtable->TableEntry[index].additionalInfoOffset = createTanslationTableAdditionalInfoBlockEntry(gpdCommandId, additionalInfoBlock);
      emGpPrintAdditionalInfoBlock(gpdCommandId, emGptranslationtable->TableEntry[index].additionalInfoOffset);
    }
    if ((entryType == CUSTOMIZED_TABLE_ENTRY)
        && (emGptranslationtable->TableEntry[index].offset != 0xFF)
        && (emGptranslationtable->TableEntry[index].offset != outIndex)) {
      if (!isCustomizedTableEntryReferenced(index, emGptranslationtable->TableEntry[index].offset)) {
        deleteCustomizednTableEntry(emGptranslationtable->TableEntry[index].offset);
        emGptranslationtable->TableEntry[index].offset = 0xFF;
      }
    }

    emGptranslationtable->TableEntry[index].infoBlockPresent = infoBlockPresent;
    emGptranslationtable->TableEntry[index].gpApplicationId = gpdAddr->applicationId;
    MEMCOPY(&(emGptranslationtable->TableEntry[index].gpAddr), gpdAddr, sizeof(EmberGpAddress));
    emGptranslationtable->TableEntry[index].gpdCommand = gpdCommandId;
    emGptranslationtable->TableEntry[index].zbEndpoint = zbEndpoint;
    emGptranslationtable->TableEntry[index].offset = outIndex;
    emGptranslationtable->TableEntry[index].entry = entryType;
    emGpSetTranslationTableEntry(index);
    ret = GP_TRANSLATION_TABLE_STATUS_SUCCESS;
  }
  return ret;
}

static uint8_t emGpTransTableDeleteTranslationTableEntryByIndex(uint8_t index,
                                                                bool infoBlockPresent,
                                                                uint8_t gpdCommandId,
                                                                uint16_t zigbeeProfile,
                                                                uint16_t zigbeeCluster,
                                                                uint8_t  zigbeeCommandId,
                                                                uint8_t payloadLength,
                                                                uint8_t* payload,
                                                                uint8_t additionalInfoLength,
                                                                EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  uint8_t status = 0xFF;
  uint8_t outIndex = 0xFF;

  ret = findMatchingGenericTranslationTableEntry(emGptranslationtable->TableEntry[index].entry,
                                                 DELETE_PAIRED_DEVICE,
                                                 emGptranslationtable->TableEntry[index].offset,
                                                 infoBlockPresent,
                                                 gpdCommandId,
                                                 zigbeeProfile,
                                                 zigbeeCluster,
                                                 zigbeeCommandId,
                                                 payloadLength,
                                                 payload,
                                                 &outIndex);
  if (ret == GP_TRANSLATION_TABLE_STATUS_SUCCESS && outIndex != 0xFF) {
    EmberGpTranslationTableAdditionalInfoBlockField * additionalInfoTable = emGpGetAdditionalInfoTable();
    uint8_t addInfoOffset = emGptranslationtable->TableEntry[index].additionalInfoOffset;
    if (addInfoOffset != 0xFF) {
      status = deleteTanslationTableAdditionalInfoBlockEntry(gpdCommandId, &(additionalInfoTable->additionalInfoBlock[addInfoOffset]));
      if (status != 0xFF) {
        emGptranslationtable->TableEntry[index].additionalInfoOffset = 0xFF;
      }
    }
    if ((emGptranslationtable->TableEntry[index].entry == CUSTOMIZED_TABLE_ENTRY) && (emGptranslationtable->TableEntry[index].offset != 0xFF)) {
      if (!isCustomizedTableEntryReferenced(index, emGptranslationtable->TableEntry[index].offset)) {
        deleteCustomizednTableEntry(emGptranslationtable->TableEntry[index].offset);
      }
    }
    MEMSET(&emGptranslationtable->TableEntry[index], 0x00, sizeof(EmGpCommandTranslationTableEntry));
    emGptranslationtable->TableEntry[index].entry = NO_ENTRY;
    emGptranslationtable->TableEntry[index].offset = 0xFF;
    emGptranslationtable->TableEntry[index].additionalInfoOffset = 0xFF;
    emGptranslationtable->totalNoOfEntries--;
    emGpSetTranslationTableEntry(index);
    ret = GP_TRANSLATION_TABLE_STATUS_SUCCESS;
  }
  return ret;
}

static uint8_t emGpTransTableFindMatchingTranslationTableEntry(uint8_t levelOfScan,
                                                               bool infoBlockPresent,
                                                               EmberGpAddress  * gpAddr,
                                                               uint8_t gpdCommandId,
                                                               uint8_t zbEndpoint,
                                                               uint8_t* gpdCmdPayload,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* addInfoBlock,
                                                               uint8_t* outIndex,
                                                               uint8_t startIndex)
{
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  uint8_t scanLevelMatchFound = 0xFF;
  EmberAfGreenPowerServerGpdSubTranslationTableEntry const* genericTranslationTable = NULL;  //this pointer either point to default table or customied table
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  if (!emGptranslationtable->totalNoOfEntries) {
    return GP_TRANSLATION_TABLE_STATUS_EMPTY;
  }
  if (startIndex >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
    return GP_TRANSLATION_TABLE_STATUS_FAILED;
  }
  for ( uint8_t tableIndex = startIndex; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
    scanLevelMatchFound = 0;
    if (emGptranslationtable->TableEntry[tableIndex].entry == NO_ENTRY) {
      continue;
    }
    if (levelOfScan & GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID) {
      if (emberAfGreenPowerCommonGpAddrCompare(&emGptranslationtable->TableEntry[tableIndex].gpAddr, gpAddr)) {
        scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID;
      }
    }
    if (levelOfScan & GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID) {
      if (emGptranslationtable->TableEntry[tableIndex].gpdCommand == gpdCommandId) {
        scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_CMD_ID;
      }
    }
    if (levelOfScan & GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT) {
      if (emGptranslationtable->TableEntry[tableIndex].entry == DEFAULT_TABLE_ENTRY) {
        EmberAfGreenPowerServerGpdSubTranslationTableEntry const* defaultTable = emGpGetDefaultTable();
        if (defaultTable == NULL) {
          return GP_TRANSLATION_TABLE_STATUS_FAILED;
        }
        genericTranslationTable = &(defaultTable[emGptranslationtable->TableEntry[tableIndex].offset]);
      } else if (emGptranslationtable->TableEntry[tableIndex].entry == CUSTOMIZED_TABLE_ENTRY) {
        EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTable = emGpGetCustomizedTable();
        if (customizedTable == NULL) {
          return GP_TRANSLATION_TABLE_STATUS_FAILED;
        }
        genericTranslationTable = &(customizedTable[emGptranslationtable->TableEntry[tableIndex].offset]);
      }
      if (genericTranslationTable == NULL) {
        return GP_TRANSLATION_TABLE_STATUS_FAILED;
      }
      if (genericTranslationTable->endpoint == EMBER_AF_GP_TRANSLATION_TABLE_ZB_ENDPOINT_PASS_FRAME_TO_APLLICATION) {
        scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT;
      }
      if (emGptranslationtable->TableEntry[tableIndex].zbEndpoint == zbEndpoint) {
        scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT;
      }
    }
    if (emGptranslationtable->TableEntry[tableIndex].infoBlockPresent == true) {
      uint8_t additionalInfoOffset = emGptranslationtable->TableEntry[tableIndex].additionalInfoOffset;
      if (additionalInfoOffset == 0xFF) {
        continue;
      }
      EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
      EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = &(additionalInfoTable->additionalInfoBlock[additionalInfoOffset]);

      if (levelOfScan & GP_TRANSLATION_TABLE_SCAN_LEVEL_ADDITIONAL_INFO_BLOCK) {
        if (compareTranslationTableAdditionalInfoBlockEntry(gpdCommandId, addInfo, addInfoBlock)) {
          scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_ADDITIONAL_INFO_BLOCK;
        }
      }
      if (levelOfScan & GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_PAYLOAD) {
        if (gpdCmdPayload == NULL) {
          continue;
        }
        if ((gpdCommandId == EMBER_ZCL_GP_GPDF_COMPACT_ATTRIBUTE_REPORTING)
            && (addInfo->optionData.compactAttr.reportIdentifier  == gpdCmdPayload[0])) {
          // end of choice1 and choice2
          scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_PAYLOAD;
        } else if ((gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_PRESS
                    || gpdCommandId == EMBER_ZCL_GP_GPDF_8BITS_VECTOR_RELEASE)
                   && (addInfo->optionData.genericSwitch.contactStatus == (addInfo->optionData.genericSwitch.contactBitmask & gpdCmdPayload[0]))) {
          scanLevelMatchFound |= GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_PAYLOAD;
        }
      }
    }  //end of InfoBlockPresent
    if (levelOfScan == scanLevelMatchFound) {
      *outIndex = tableIndex;
      ret = GP_TRANSLATION_TABLE_STATUS_SUCCESS;
      break;
    } else {
      continue;
    }
  }
  return ret;
}

static uint8_t emGpTransTableAddPairedDeviceToTranslationTable(uint8_t incomingReqType,
                                                               bool infoBlockPresent,
                                                               EmberGpAddress  * gpdAddr,
                                                               uint8_t gpdCommandId,
                                                               uint8_t zbEndpoint,
                                                               uint16_t zigbeeProfile,
                                                               uint16_t zigbeeCluster,
                                                               uint8_t  zigbeeCommandId,
                                                               uint8_t payloadLength,
                                                               uint8_t* payload,
                                                               uint8_t payloadSrc,
                                                               uint8_t additionalInfoLength,
                                                               EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock,
                                                               uint8_t* outNewTTEntryIndex)
{
  int tableIndex;
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  if ((emGptranslationtable->totalNoOfEntries) >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
    return GP_TRANSLATION_TABLE_STATUS_FULL;
  }

  for (tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
    if (emGptranslationtable->TableEntry[tableIndex].entry == NO_ENTRY ) {
      break;
    }
  }

  if (tableIndex == EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
    return GP_TRANSLATION_TABLE_STATUS_FULL;
  }

  ret = emGpTransTableAddTranslationTableEntryByIndex(incomingReqType,
                                                      tableIndex,
                                                      infoBlockPresent,
                                                      gpdAddr,
                                                      gpdCommandId,
                                                      zbEndpoint,
                                                      zigbeeProfile,
                                                      zigbeeCluster,
                                                      zigbeeCommandId,
                                                      payloadLength,
                                                      payload,
                                                      payloadSrc,
                                                      additionalInfoLength,
                                                      additionalInfoBlock);
  if (ret != GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
    MEMSET(&emGptranslationtable->TableEntry[tableIndex], 0x00, sizeof(EmGpCommandTranslationTableEntry));
    emGptranslationtable->TableEntry[tableIndex].offset = 0xFF;
    emGptranslationtable->TableEntry[tableIndex].additionalInfoOffset = 0xFF;
    emGptranslationtable->TableEntry[tableIndex].entry = NO_ENTRY;
  } else {
    *outNewTTEntryIndex = tableIndex;
  }
  return ret;
}

static uint8_t emGpTransTableDeletePairedDevicefromTranslationTableEntry(EmberGpAddress * gpdAddr)
{
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  if (!emGptranslationtable->totalNoOfEntries) {
    return GP_TRANSLATION_TABLE_STATUS_EMPTY;
  }
  uint8_t entryIndex;
  do {
    entryIndex = GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX;
    uint8_t status = emGpTransTableFindMatchingTranslationTableEntry(GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID,
                                                                     false,
                                                                     gpdAddr,
                                                                     0x00,
                                                                     0x00,
                                                                     NULL,
                                                                     NULL,
                                                                     &entryIndex,
                                                                     0);
    if (status == GP_TRANSLATION_TABLE_STATUS_SUCCESS
        && (entryIndex != 0xFF)) {
      EmberGpTranslationTableAdditionalInfoBlockField *additionalInfoTable = emGpGetAdditionalInfoTable();
      uint8_t addInfoOffset = emGptranslationtable->TableEntry[entryIndex].additionalInfoOffset;
      EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * addInfo = &(additionalInfoTable->additionalInfoBlock[addInfoOffset]);
      if (addInfoOffset != 0xFF) {
        status = deleteTanslationTableAdditionalInfoBlockEntry(emGptranslationtable->TableEntry[entryIndex].gpdCommand,
                                                               addInfo);
        if (status != 0xFF) {
          emGptranslationtable->TableEntry[entryIndex].additionalInfoOffset = 0xFF;
        }
      }
      if ((emGptranslationtable->TableEntry[entryIndex].entry == CUSTOMIZED_TABLE_ENTRY) && (emGptranslationtable->TableEntry[entryIndex].offset != 0xFF)) {
        if (!isCustomizedTableEntryReferenced(entryIndex, emGptranslationtable->TableEntry[entryIndex].offset)) {
          deleteCustomizednTableEntry(emGptranslationtable->TableEntry[entryIndex].offset);
        }
      }
      MEMSET(&emGptranslationtable->TableEntry[entryIndex], 0x00, sizeof(EmGpCommandTranslationTableEntry));
      emGptranslationtable->TableEntry[entryIndex].entry = NO_ENTRY;
      emGptranslationtable->TableEntry[entryIndex].offset = 0xFF;
      emGptranslationtable->TableEntry[entryIndex].additionalInfoOffset = 0xFF;
      emGptranslationtable->totalNoOfEntries--;
      emGpSetTranslationTableEntry(entryIndex);
    }
  } while (entryIndex != 0xFF);
  return GP_TRANSLATION_TABLE_STATUS_SUCCESS;
}

uint8_t emGpTransTableGetTranslationTableEntry(uint8_t entryIndex,
                                               EmberAfGreenPowerServerGpdSubTranslationTableEntry* TranslationTableEntry)
{
  EmGpCommandTranslationTable const * emGptranslationtable = emGpTransTableGetTranslationTable();
  EmberAfGreenPowerServerGpdSubTranslationTableEntry const* genericTranslationTable = NULL;
  if (TranslationTableEntry == NULL || (entryIndex > EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE)) {
    return GP_TRANSLATION_TABLE_STATUS_FAILED;
  }

  if (entryIndex != 0xFF ) {
    if (emGptranslationtable->TableEntry[entryIndex].entry == NO_ENTRY) {
      return GP_TRANSLATION_TABLE_STATUS_FAILED;
    }
    MEMSET(TranslationTableEntry, 0, sizeof(EmberAfGreenPowerServerGpdSubTranslationTableEntry));
    TranslationTableEntry->endpoint = emGptranslationtable->TableEntry[entryIndex].zbEndpoint;
    TranslationTableEntry->gpdCommand = emGptranslationtable->TableEntry[entryIndex].gpdCommand;
    if (emGptranslationtable->TableEntry[entryIndex].entry == DEFAULT_TABLE_ENTRY) {
      EmberAfGreenPowerServerGpdSubTranslationTableEntry const* defaultTable = emGpGetDefaultTable();
      if (defaultTable == NULL) {
        return EMBER_ERR_FATAL;
      }
      genericTranslationTable = &(defaultTable[emGptranslationtable->TableEntry[entryIndex].offset]);
    } else if (emGptranslationtable->TableEntry[entryIndex].entry == CUSTOMIZED_TABLE_ENTRY) {
      EmberAfGreenPowerServerGpdSubTranslationTableEntry* customizedTable = emGpGetCustomizedTable();
      if (customizedTable == NULL) {
        return EMBER_ERR_FATAL;
      }
      genericTranslationTable = &(customizedTable[emGptranslationtable->TableEntry[entryIndex].offset]);
    }
    if (genericTranslationTable != NULL) {
      TranslationTableEntry->validEntry = genericTranslationTable->validEntry;
      TranslationTableEntry->zigbeeProfile = genericTranslationTable->zigbeeProfile;
      TranslationTableEntry->zigbeeCluster = genericTranslationTable->zigbeeCluster;
      TranslationTableEntry->zigbeeCommandId = genericTranslationTable->zigbeeCommandId;
      MEMCOPY(&(TranslationTableEntry->payloadSrc),
              &(genericTranslationTable->payloadSrc),
              sizeof(uint8_t));
      MEMCOPY(&(TranslationTableEntry->zclPayloadDefault),
              &(genericTranslationTable->zclPayloadDefault),
              sizeof(EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN));
      return GP_TRANSLATION_TABLE_STATUS_SUCCESS;
    } else {
      return GP_TRANSLATION_TABLE_STATUS_FAILED;
    }
  }
  return GP_TRANSLATION_TABLE_STATUS_FAILED;
}

uint8_t emGpTransTableAddTranslationTableEntryUpdateCommand(uint8_t index,
                                                            bool infoBlockPresent,
                                                            EmberGpAddress  * gpdAddr,
                                                            uint8_t gpdCommandId,
                                                            uint8_t zbEndpoint,
                                                            uint16_t zigbeeProfile,
                                                            uint16_t zigbeeCluster,
                                                            uint8_t  zigbeeCommandId,
                                                            uint8_t payloadLength,
                                                            uint8_t* payload,
                                                            uint8_t payloadSrc,
                                                            uint8_t additionalInfoLength,
                                                            EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  bool ret = FALSE;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  if (index == 0xFF) {
    //If index is 0xff, the sink SHALL choose any free entry
    uint8_t newTTEntryIndex;
    ret = emGpTransTableAddPairedDeviceToTranslationTable(TRANSLATION_TABLE_UPDATE,
                                                          infoBlockPresent,
                                                          gpdAddr,
                                                          gpdCommandId,
                                                          zbEndpoint,
                                                          zigbeeProfile,
                                                          zigbeeCluster,
                                                          zigbeeCommandId,
                                                          payloadLength,
                                                          payload,
                                                          payloadSrc,
                                                          additionalInfoLength,
                                                          additionalInfoBlock,
                                                          &newTTEntryIndex);
  } else {
    if (index >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
      return GP_TRANSLATION_TABLE_STATUS_FAILED;
    }
    if (emGptranslationtable->TableEntry[index].entry != NO_ENTRY) {
      return GP_TRANSLATION_TABLE_STATUS_ENTRY_NOT_EMPTY;
    } else {
      ret = emGpTransTableAddTranslationTableEntryByIndex(TRANSLATION_TABLE_UPDATE,
                                                          index,
                                                          infoBlockPresent,
                                                          gpdAddr,
                                                          gpdCommandId,
                                                          zbEndpoint,
                                                          zigbeeProfile,
                                                          zigbeeCluster,
                                                          zigbeeCommandId,
                                                          payloadLength,
                                                          payload,
                                                          payloadSrc,
                                                          additionalInfoLength,
                                                          additionalInfoBlock);
    }
  }
  return ret;
}

uint8_t emGpTransTableReplaceTranslationTableEntryUpdateCommand(uint8_t index,
                                                                bool infoBlockPresent,
                                                                EmberGpAddress  * gpdAddr,
                                                                uint8_t gpdCommandId,
                                                                uint8_t zbEndpoint,
                                                                uint16_t zigbeeProfile,
                                                                uint16_t zigbeeCluster,
                                                                uint8_t  zigbeeCommandId,
                                                                uint8_t payloadLength,
                                                                uint8_t* payload,
                                                                uint8_t payloadSrc,
                                                                uint8_t additionalInfoLength,
                                                                EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  bool found = FALSE;
  int tableIndex = 0;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  if (index != 0xFF ) {
    if (index >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
      return GP_TRANSLATION_TABLE_STATUS_FAILED;
    }
    if (emGptranslationtable->TableEntry[index].entry == NO_ENTRY) {
      return GP_TRANSLATION_TABLE_STATUS_ENTRY_EMPTY;
    }
    ret = emGpTransTableAddTranslationTableEntryByIndex(TRANSLATION_TABLE_UPDATE,
                                                        index,
                                                        infoBlockPresent,
                                                        gpdAddr,
                                                        gpdCommandId,
                                                        zbEndpoint,
                                                        zigbeeProfile,
                                                        zigbeeCluster,
                                                        zigbeeCommandId,
                                                        payloadLength,
                                                        payload,
                                                        payloadSrc,
                                                        additionalInfoLength,
                                                        additionalInfoBlock);
  } else {
    /*If index is 0xff, the sink replacing any number of translation entry(s) for the same
       (GPD ID, GPD Endpoint, GPD CommandID, EndPoint, Profile, Cluster) quintuple by the supplied number of entries*/

    for (tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
      if (!emGptranslationtable->TableEntry[tableIndex].entry) {
        continue;
      }
      const EmberAfGreenPowerServerGpdSubTranslationTableEntry * genericTranslationTable = NULL;
      if (emGptranslationtable->TableEntry[tableIndex].entry == DEFAULT_TABLE_ENTRY) {
        genericTranslationTable = &emberGpDefaultTranslationTable[emGptranslationtable->TableEntry[tableIndex].offset];
      } else if (emGptranslationtable->TableEntry[tableIndex].entry == CUSTOMIZED_TABLE_ENTRY) {
        genericTranslationTable = &customizedTranslationTable[emGptranslationtable->TableEntry[tableIndex].offset];
      }

      if ((emberAfGreenPowerCommonGpAddrCompare(&emGptranslationtable->TableEntry[tableIndex].gpAddr, gpdAddr)
           && (uint8_t)emGptranslationtable->TableEntry[tableIndex].gpdCommand == (uint8_t)gpdCommandId)
          && (emGptranslationtable->TableEntry[tableIndex].zbEndpoint == zbEndpoint)
          && (genericTranslationTable->zigbeeProfile == zigbeeProfile)
          && (genericTranslationTable->zigbeeCluster == zigbeeCluster)) {
        ret = emGpTransTableAddTranslationTableEntryByIndex(TRANSLATION_TABLE_UPDATE,
                                                            tableIndex,
                                                            infoBlockPresent,
                                                            gpdAddr,
                                                            gpdCommandId,
                                                            zbEndpoint,
                                                            zigbeeProfile,
                                                            zigbeeCluster,
                                                            zigbeeCommandId,
                                                            payloadLength,
                                                            payload,
                                                            payloadSrc,
                                                            additionalInfoLength,
                                                            additionalInfoBlock);
        if ( ret == GP_TRANSLATION_TABLE_STATUS_SUCCESS) {
          found = TRUE;
        }
      }
    }
    if (!found) { //If Cannot replace, add new entry to the table
      for (tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
        if (emGptranslationtable->TableEntry[tableIndex].entry) {
          continue;
        }
        ret = emGpTransTableAddTranslationTableEntryByIndex(TRANSLATION_TABLE_UPDATE,
                                                            tableIndex,
                                                            infoBlockPresent,
                                                            gpdAddr,
                                                            gpdCommandId,
                                                            zbEndpoint,
                                                            zigbeeProfile,
                                                            zigbeeCluster,
                                                            zigbeeCommandId,
                                                            payloadLength,
                                                            payload,
                                                            payloadSrc,
                                                            additionalInfoLength,
                                                            additionalInfoBlock);
        break;
      }
    }
  }
  return ret;
}

static uint8_t emGpTransTableRemoveTranslationTableEntryUpdateCommand(uint8_t index,
                                                                      bool infoBlockPresent,
                                                                      EmberGpAddress * gpdAddr,
                                                                      uint8_t gpdCommandId,
                                                                      uint8_t zbEndpoint,
                                                                      uint16_t zigbeeProfile,
                                                                      uint16_t zigbeeCluster,
                                                                      uint8_t  zigbeeCommandId,
                                                                      uint8_t payloadLength,
                                                                      uint8_t* payload,
                                                                      uint8_t payloadSrc,
                                                                      uint8_t additionalInfoLength,
                                                                      EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* additionalInfoBlock)
{
  uint8_t ret = GP_TRANSLATION_TABLE_STATUS_FAILED;
  int tableIndex = 0;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();

  if (index != 0xFF ) {
    if (index >= EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
      return GP_TRANSLATION_TABLE_STATUS_FAILED;
    }
    if (emGptranslationtable->TableEntry[index].entry == NO_ENTRY) {
      return GP_TRANSLATION_TABLE_STATUS_ENTRY_EMPTY;
    }
    ret = emGpTransTableDeleteTranslationTableEntryByIndex(index,
                                                           infoBlockPresent,
                                                           gpdCommandId,
                                                           zigbeeProfile,
                                                           zigbeeCluster,
                                                           zigbeeCommandId,
                                                           0x00,
                                                           NULL,
                                                           0x00,
                                                           NULL);
  } else {
    // If index is 0xff, the sink removes any number of translation entry(s) for the same
    // (GPD ID, GPD Endpoint, GPD CommandID, EndPoint, Profile, Cluster) quintuple by the supplied number of entries
    for (tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
      if (emGptranslationtable->TableEntry[tableIndex].entry == NO_ENTRY ) {
        continue;
      }
      const EmberAfGreenPowerServerGpdSubTranslationTableEntry * genericTranslationTable = NULL;
      if (emGptranslationtable->TableEntry[tableIndex].entry == DEFAULT_TABLE_ENTRY) {
        genericTranslationTable = &emberGpDefaultTranslationTable[emGptranslationtable->TableEntry[tableIndex].offset];
      } else if (emGptranslationtable->TableEntry[tableIndex].entry == CUSTOMIZED_TABLE_ENTRY) {
        genericTranslationTable = &customizedTranslationTable[emGptranslationtable->TableEntry[tableIndex].offset];
      }
      if (emberAfGreenPowerCommonGpAddrCompare(&emGptranslationtable->TableEntry[tableIndex].gpAddr, gpdAddr)
          && (emGptranslationtable->TableEntry[tableIndex].gpdCommand == gpdCommandId)
          && (emGptranslationtable->TableEntry[tableIndex].zbEndpoint == zbEndpoint)
          && (genericTranslationTable->zigbeeProfile == zigbeeProfile)
          && (genericTranslationTable->zigbeeCluster == zigbeeCluster)) {
        ret = emGpTransTableDeleteTranslationTableEntryByIndex(tableIndex,
                                                               infoBlockPresent,
                                                               gpdCommandId,
                                                               zigbeeProfile,
                                                               zigbeeCluster,
                                                               zigbeeCommandId,
                                                               0x00,
                                                               NULL,
                                                               0x00,
                                                               NULL);
      }
    }
  }
  return ret;
}

uint8_t emGpTransTableGetFreeEntryIndex(void)
{
  uint8_t tableIndex;
  EmGpCommandTranslationTable * emGptranslationtable = emGpTransTableGetTranslationTable();
  for ( tableIndex = 0; tableIndex < EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE; tableIndex++) {
    if (emGptranslationtable->TableEntry[tableIndex].entry == NO_ENTRY ) {
      break;
    }
  }
  if (tableIndex == EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE) {
    return GP_TRANSLATION_TABLE_STATUS_FAILED;
  } else {
    return tableIndex;
  }
}

void removeGpdEndpointFromTranslationTable(EmberGpAddress *gpdAddr, uint8_t zbEndpoint)
{
  uint8_t outIndex = 0xFF;
  uint8_t startIndex = 0x00;
  EmGpCommandTranslationTable * translationTable = emGpTransTableGetTranslationTable();
  emberAfGreenPowerClusterPrintln("removeGpdEndpointFromTranslationTable zbEndpoint %d ", zbEndpoint);

  // by default : remove one particular GPD from this combo edp
  uint8_t levelOfScanMask = (GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID | GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT);
  if ((gpdAddr == NULL)
      || ((gpdAddr->applicationId == EMBER_GP_APPLICATION_SOURCE_ID
           && gpdAddr->id.sourceId == GP_ADDR_SRC_ID_WILDCARD)
          && ((gpdAddr->applicationId == EMBER_GP_APPLICATION_IEEE_ADDRESS)
              && (emberAfMemoryByteCompare(gpdAddr->id.gpdIeeeAddress, EUI64_SIZE, 0xFF))))) {
    // WildcardId thus remove AllGpd from this combo edp
    levelOfScanMask = GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT;
  }

  do {
    outIndex = 0xFF;
    uint8_t status = emGpTransTableFindMatchingTranslationTableEntry((GP_TRANSLATION_TABLE_SCAN_LEVEL_GPD_ID
                                                                      | GP_TRANSLATION_TABLE_SCAN_LEVEL_ZB_ENDPOINT),//uint8_t levelOfScan,
                                                                     false,//bool infoBlockPresent,
                                                                     gpdAddr,
                                                                     0,
                                                                     zbEndpoint,//uint8_t zbEndpoint,
                                                                     0,
                                                                     NULL,
                                                                     &outIndex,
                                                                     startIndex);
    if (status == GP_TRANSLATION_TABLE_STATUS_SUCCESS
        && outIndex != 0xFF) {
      emGpTransTableDeleteTranslationTableEntryByIndex(outIndex,
                                                       true,
                                                       translationTable->TableEntry[outIndex].gpdCommand,
                                                       0,
                                                       0,
                                                       0,
                                                       0x00,
                                                       NULL,
                                                       0x00,
                                                       NULL);
      startIndex = outIndex + 1;
    }
  } while (outIndex != 0xFF);
}

bool emAfPluginGreenPowerServerRetrieveAttributeAndCraftResponse(uint8_t endpoint,
                                                                 EmberAfClusterId clusterId,
                                                                 EmberAfAttributeId attrId,
                                                                 uint8_t mask,
                                                                 uint16_t manufacturerCode,
                                                                 uint16_t readLength)
{
  uint8_t sinkTableEntryAppResponseData[EMBER_AF_RESPONSE_BUFFER_LEN];
  uint8_t zclStatus = EMBER_ZCL_STATUS_SUCCESS;
  uint16_t stringDataOffsetStart = 0;
  uint16_t stringLength = 0;
  bool    status = false;

  if (endpoint != GP_ENDPOINT
      || clusterId != ZCL_GREEN_POWER_CLUSTER_ID
      || attrId != ZCL_GP_SERVER_SINK_TABLE_ATTRIBUTE_ID
      || mask != CLUSTER_MASK_SERVER
      || manufacturerCode != EMBER_AF_NULL_MANUFACTURER_CODE) {
    // do nothing but return false
  } else if (readLength < 6) {
    emberAfGreenPowerClusterPrintln("ERROR, Buffer length supplied %d is too small", readLength);
    // Can't fit the ZCL header in available length so exit with error
    // do nothing but return true as we are processing the correct type of packet
  } else {
    emberAfPutInt16uInResp(attrId);
    // The sink table attribute is a long string ZCL attribute type, which means
    // it is encoded starting with a 2-byte length. We fill in the real length
    // after we have encoded the whole sink table.
    // Four bytes extra = 2byte length + 1 byte ZCl Status + 1 byte Attribute Type
    uint16_t stringDataOffset =  appResponseLength + 4;
    stringDataOffsetStart = stringDataOffset;
    // Search the sink table and respond with entries
    for (uint8_t i = 0; i < EMBER_GP_SINK_TABLE_SIZE; i++) {
      EmberGpSinkTableEntry entry;
      if (emberGpSinkTableGetEntry(i, &entry) == EMBER_SUCCESS) {
        emberAfGreenPowerClusterPrintln("Craft Response - Encode Sink Table %d", i);
        // Have a valid entry so encode response in temp buffer and add if it fits
        uint16_t sinkTableEntryLength = emberAfGreenPowerServerStoreSinkTableEntry(&entry, sinkTableEntryAppResponseData);
        if ((sinkTableEntryLength + stringDataOffset) > readLength) {
          // String is too big so
          zclStatus = EMBER_ZCL_STATUS_INSUFFICIENT_SPACE;
          emberAfGreenPowerClusterPrintln("Sink Table Attribute read INSUFFICIENT SPACE");
          break;
        } else {
          emberAfGreenPowerClusterPrintln("SAVE ENTRY %d", i);
          MEMMOVE(&appResponseData[stringDataOffset], sinkTableEntryAppResponseData, sinkTableEntryLength);
          stringDataOffset += sinkTableEntryLength;
        }
      }
    }
    // calculate string length
    stringLength = stringDataOffset - stringDataOffsetStart;
    if (zclStatus == EMBER_ZCL_STATUS_SUCCESS) {
      emberAfPutInt8uInResp(zclStatus);
      emberAfPutInt8uInResp(ZCL_LONG_OCTET_STRING_ATTRIBUTE_TYPE);
      emberAfPutInt16uInResp(stringLength);
      emberAfGreenPowerClusterPrintln(" calculated string length = %d", (stringDataOffsetStart - stringDataOffset));
    } else {
      emberAfPutInt8uInResp(zclStatus);
    }
    appResponseLength += stringLength;
    status = true;
  }
  emberAfGreenPowerClusterPrintln(" String length = %d ", stringLength);
  return status;
}

void emberAfPluginGreenPowerServerInitCallback(void)
{
  // Bring up the Sink table
  emberAfGreenPowerClusterPrintln("SinkTable Init..");
  emberGpSinkTableInit();
  emberAfPluginTranslationTableInitCallback();
  // A test to see the security upon reset
  // uncomment the assert to just run the security test
  //emGpTestSecurity();
  //assert(false);
}

void emberAfPluginGreenPowerServerStackStatusCallback(EmberStatus status)
{
  emberAfGreenPowerClusterPrintln("Green Power Server Stack Status Callback status = %x", status);
  if (status == EMBER_NETWORK_DOWN
      && emberStackIsPerformingRejoin() == FALSE) {
    // Clear the additional info , translation table and sink table in order.
    embGpClearAdditionalInfoBlockTable();
    emGpClearCustomizedTable();
    emGpTransTableClearTranslationTable();
    emberGpSinkTableClearAll();
  }
  if (!emberAfGreenPowerServerUpdateInvolveTCCallback(status)) {
    updateInvolveTC(status);
  }
}
