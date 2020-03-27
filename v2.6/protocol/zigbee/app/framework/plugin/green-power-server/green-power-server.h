/***************************************************************************//**
 * @file
 * @brief APIs and defines for the Green Power Server plugin.
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

#ifndef _SILABS_GREEN_POWER_SERVER_H_
#define _SILABS_GREEN_POWER_SERVER_H_

#ifndef EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE
#define EMBER_AF_PLUGIN_GREEN_POWER_SERVER_CUSTOMIZED_GPD_TRANSLATION_TABLE_SIZE (20)
#endif

#ifndef EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE
#define EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE 30
#endif

#define EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE

#define EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN (7)

// Key Types for MIC Calculations
#define EMBER_AF_GREEN_POWER_GP_SHARED_KEY 0
#define EMBER_AF_GREEN_POWER_GP_INDIVIDUAL_KEY 1
// defines macros for Gpd To Zcl Cmd Mapping Payload Src
#define  EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_PRECONFIGURED 1
#define  EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_NA 2
#define  EMBER_AF_GREEN_POWER_ZCL_PAYLOAD_SRC_GPD_CMD 4

// GPD payload length related macros used in the green power server
#define GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_LENGTH_INDEX 0x00
#define GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_INDEX        0x01

#define GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_UNSPECIFIED_LENGTH 0xFF
#define GREEN_POWER_SERVER_GPD_COMMAND_PAYLOAD_TT_DERIVED_LENGTH  0xFE

#define GP_TRANSLATION_TABLE_STATUS_SUCCESS 0x00
#define GP_TRANSLATION_TABLE_STATUS_FAILED 0xFF
#define GP_TRANSLATION_TABLE_STATUS_FULL   0xFE
#define GP_TRANSLATION_TABLE_STATUS_EMPTY  0xFD
#define GP_TRANSLATION_TABLE_STATUS_ENTRY_EMPTY 0xFC
#define GP_TRANSLATION_TABLE_STATUS_ENTRY_NOT_EMPTY 0xFB
#define GP_TRANSLATION_TABLE_STATUS_PARAM_DOES_NOT_MATCH 0xFA
#define GP_TRANSLATION_TABLE_STATUS_CUSTOMIZED_TABLE_FULL  0xF9

#define GP_TRANSLATION_TABLE_ENTRY_INVALID_INDEX 0xFF

#define GREEN_POWER_SERVER_NO_PAIRED_ENDPOINTS     0x00
#define GREEN_POWER_SERVER_RESERVED_ENDPOINTS      0xFD
#define GREEN_POWER_SERVER_SINK_DERIVES_ENDPOINTS  0xFE
#define GREEN_POWER_SERVER_ALL_SINK_ENDPOINTS      0xFF

#define GREEN_POWER_SERVER_MIN_VALID_APP_ENDPOINT  1
#define GREEN_POWER_SERVER_MAX_VALID_APP_ENDPOINT  240

#define GREEN_POWER_SERVER_GPS_SECURITY_LEVEL_ATTRIBUTE_FIELD_INVOLVE_TC 0x08

#define GP_DEVICE_ANNOUNCE_SIZE 12

#define SIZE_OF_REPORT_STORAGE 82
#define COMM_REPLY_PAYLOAD_SIZE 30
#define GP_MAX_COMMISSIONING_PAYLOAD_SIZE 59
#define GP_SINK_TABLE_RESPONSE_ENTRIES_OFFSET           (3)
#define EMBER_AF_ZCL_CLUSTER_GP_GPS_COMMISSIONING_WINDOWS_DEFAULT_TIME_S (180)
#define GP_ADDR_SRC_ID_WILDCARD (0xFFFFFFFF)
#define GPS_ATTRIBUTE_KEY_TYPE_MASK (0x07)
#define GP_PAIRING_CONFIGURATION_FIXED_FLAG (0x230)

#define GREEN_POWER_SERVER_MIN_REPORT_LENGTH (10)

// payload [0] = length of payload,[1] = cmdID,[2] = report id,[3] = 1st data value
#define FIX_SHIFT_REPORTING_DATA_POSITION_CONVERT_TO_PAYLOAD_INDEX          (3)
// In CAR GPD : payload [0] = reportId, payload[1] = first data point
#define CAR_DATA_POINT_OFFSET 1
#define GP_DEFAULT_LINK_KEY { 0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39 }

typedef uint8_t GpsNetworkState;
enum {
  GREEN_POWER_SERVER_GPS_NODE_STATE_NOT_IN_NETWORK,
  GREEN_POWER_SERVER_GPS_NODE_STATE_IN_NETWORK
};

typedef uint8_t EmberSinkCommissionState;
enum {
  GP_SINK_COMM_STATE_IDLE,
  GP_SINK_COMM_STATE_COLLECT_REPORTS,
  GP_SINK_COMM_STATE_SEND_COMM_REPLY,
  GP_SINK_COMM_STATE_WAIT_FOR_SUCCESS,
  GP_SINK_COMM_STATE_FINALISE_PAIRING,
  GP_SINK_COMM_STATE_PAIRING_DONE,
};

typedef uint8_t GpTableType;
enum {
  NO_ENTRY,
  DEFAULT_TABLE_ENTRY,
  CUSTOMIZED_TABLE_ENTRY,
}; //EmGpTableType

enum {
  ADD_PAIRED_DEVICE = 1,
  DELETE_PAIRED_DEVICE = 1,
  TRANSLATION_TABLE_UPDATE = 2,
}; //IncommingReqType

enum {
  COMMISSIONING_TIMEOUT_TYPE_GENERIC_SWITCH = 0,
  COMMISSIONING_TIMEOUT_TYPE_MULTI_SENSOR = 1,
  COMMISSIONING_TIMEOUT_TYPE_COMMISSIONING_WINDOW_TIMEOUT = 2
}; // The commissioning timeout type.

typedef struct {
  uint8_t applicationId;
  uint8_t additionalInfoBlockPresent;
} GpTranslationTableOptionField;

typedef struct {
  bool inCommissioningMode;
  bool proxiesInvolved;
  uint8_t endpoint;
} EmberAfGreenPowerServerCommissioningState;

typedef struct {
  uint8_t       switchType;
  uint8_t       nbOfIdentifiedContacts;
  uint8_t       nbOfTTEntriesNeeded;
  uint8_t       indicativeBitmask;
}EmGpSwitchTypeData;

typedef struct {
  // used by generic switch and multi-sensor, for example
  //at present 3 entries comapct attrubte reporting, vector press & vector Release
  EmberGpTranslationTableAdditionalInfoBlockOptionRecordField additionalInfoBlock[EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE];
  uint8_t validEntry[EMBER_AF_PLUGIN_GREEN_POWER_SERVER_ADDITIONALINFO_TABLE_SIZE];
  uint8_t totlaNoOfEntries;
}EmberGpTranslationTableAdditionalInfoBlockField;

typedef struct {
  bool            validEntry;
  uint8_t         gpdCommand;
  uint8_t         endpoint;
  uint16_t        zigbeeProfile;
  uint16_t        zigbeeCluster;
  uint8_t         serverClient;
  uint8_t         zigbeeCommandId;
  uint8_t         payloadSrc;

  // This is a Zigbee string.
  // If the Length sub-field of the ZigBee Command payload field is set to 0x00,
  // the Payload sub-field is not present, and the ZigBee command is sent without payload.
  //
  // If the Length sub-field of the ZigBee Command payload field is set to 0xff,
  // the Payload sub-field is not present, and the payload from the triggering
  // GPD command is to be copied verbatim into the ZigBee command.  If the Length
  // sub-field of the ZigBee Command payload field is set to 0xfe, the Payload
  //sub-field is not present, and the pay-load from the triggering GPD command
  //needs to be parsed. For all other values of the Length sub-field,
  //the Payload sub-field is present, has a length as defined in the Length
  //sub-field and specifies the pay-load to be used.
  uint8_t zclPayloadDefault[EMBER_AF_GREEN_POWER_SERVER_TRANSLATION_TABLE_ENTRY_ZCL_PAYLOAD_LEN];
} EmberAfGreenPowerServerGpdSubTranslationTableEntry;

typedef struct {
  bool            infoBlockPresent;
  uint8_t         gpdCommand;
  uint8_t         zbEndpoint;
  uint8_t         offset;
  GpTableType     entry;
  EmberGpAddress  gpAddr;
  EmberGpApplicationId gpApplicationId;
  uint8_t         additionalInfoOffset;
} EmGpCommandTranslationTableEntry;

typedef struct {
  EmGpCommandTranslationTableEntry TableEntry[EMBER_AF_PLUGIN_GREEN_POWER_SERVER_TRANSLATION_TABLE_SIZE];
  uint8_t totalNoOfEntries;
}EmGpCommandTranslationTable;

typedef struct {
  uint8_t       endpoint;
  uint8_t       gpdCommand;
  uint16_t      zigbeeProfile;
} EmberAfGreenPowerServerMultiSensorTranslation;

typedef struct {
  EmGpSwitchTypeData  SwitchType;
  EmberAfGreenPowerServerGpdSubTranslationTableEntry genericSwitchDefaultTableEntry;
} EmberAfGreenPowerServerDefautGenericSwTranslation;

typedef struct {
  uint8_t deviceId;
  const uint8_t * cmd;
}GpDeviceIdAndCommandMap;

typedef struct {
  uint8_t deviceId;
  uint8_t numberOfClusters;
  const uint16_t * cluster;
}GpDeviceIdAndClusterMap;

typedef struct {
  uint16_t clusterId;
  bool serverClient;
}ZigbeeCluster;

// Structure to hold the information from commissioning command when received
// and used for subsequent processing
typedef struct {
  EmberGpAddress                addr;
  // saved from the commissioning frame 0xE0
  uint8_t                       gpdfOptions;
  uint8_t                       gpdfExtendedOptions;
  EmberGpSinkType               communicationMode;
  uint8_t                       groupcastRadius;
  uint8_t                       securityLevel;
  EmberKeyData                  key;
  uint32_t                      outgoingFrameCounter;
  bool                          useGivenAssignedAlias;
  uint16_t                      givenAlias;
  EmberGpApplicationInfo        applicationInfo;
  uint8_t                       securityKeyType;

  // data link to generic switch
  EmberGpSwitchInformation      switchInformationStruct;

  // multi-sensor and compact reporting,
  // data link to AppliDescriptionCmd (0xE4), one report descriptor (at a time)
  // total number of report the GPD sensor
  uint8_t                       totalNbOfReport;
  uint8_t                       numberOfReports;
  uint8_t                       lastIndex;
  uint8_t                       reportsStorage[SIZE_OF_REPORT_STORAGE];
  // state machine
  uint16_t                      gppShortAddress;
  EmberSinkCommissionState      reportCollectonState;
  // Send GP Pairing bit for current commissioning
  bool                          doNotSendGpPairing;
} GpCommDataSaved;

extern const EmberAfGreenPowerServerGpdSubTranslationTableEntry * getSystemOrUserDefinedDefaultTable(void);
extern const EmberAfGreenPowerServerDefautGenericSwTranslation* getSystemOrUserDefinedDefaultGenericSwitchTable(void);
extern EmberStatus ezspProxyBroadcast(EmberNodeId source,
                                      EmberNodeId destination,
                                      uint8_t nwkSequence,
                                      EmberApsFrame *apsFrame,
                                      uint8_t radius,
                                      uint8_t messageTag,
                                      uint8_t messageLength,
                                      uint8_t *messageContents,
                                      uint8_t *apsSequence);
// print to cli the customized translation table entries
void emberAfPluginGreenPowerServerTablePrint(void);
void emberAfPluginGreenPowerServerClearSinkTable(void);
EmGpCommandTranslationTable * emGpTransTableGetTranslationTable(void);
uint8_t emGpTransTableGetFreeEntryIndex(void);
void emGpTransTableClearTranslationTable(void);
uint8_t emGpTransTableGetTranslationTableEntry(uint8_t entryIndex,
                                               EmberAfGreenPowerServerGpdSubTranslationTableEntry *TranslationTableEntry);
uint8_t emGpTransTableReplaceTranslationTableEntryUpdateCommand(uint8_t Index,
                                                                bool infoBlockPresent,
                                                                EmberGpAddress * gpdAddr,
                                                                uint8_t gpdCommandId,
                                                                uint8_t ZbEndpoint,
                                                                uint16_t zigbeeProfile,
                                                                uint16_t zigbeeCluster,
                                                                uint8_t  zigbeeCommandId,
                                                                uint8_t payloadLength,
                                                                uint8_t* payload,
                                                                uint8_t payloadSrc,
                                                                uint8_t additionalInfoLength,
                                                                EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* AdditionalInfoBlock);
uint8_t emGpTransTableAddTranslationTableEntryUpdateCommand(uint8_t Index,
                                                            bool infoBlockPresent,
                                                            EmberGpAddress * gpdAddr,
                                                            uint8_t gpdCommandId,
                                                            uint8_t ZbEndpoint,
                                                            uint16_t zigbeeProfile,
                                                            uint16_t zigbeeCluster,
                                                            uint8_t  zigbeeCommandId,
                                                            uint8_t payloadLength,
                                                            uint8_t* payload,
                                                            uint8_t payloadSrc,
                                                            uint8_t additionalInfoLength,
                                                            EmberGpTranslationTableAdditionalInfoBlockOptionRecordField* AdditionalInfoBlock);
uint16_t emGpCopyAdditionalInfoBlockArrayToStructure(uint8_t * additionalInfoBlockIn,
                                                     EmberGpTranslationTableAdditionalInfoBlockOptionRecordField * additionalInfoBlockOut,
                                                     uint8_t gpdCommandId);
void emberAfPluginGreenPowerServerSinkTablePrint(void);
void emGpPrintAdditionalInfoBlock(uint8_t gpdCommand, uint8_t addInfoOffset);
// security function prototypes
bool emGpKeyTcLkDerivation(EmberGpAddress * gpdAddr,
                           uint32_t gpdSecurityFrameCounter,
                           uint8_t mic[4],
                           const EmberKeyData * linkKey,
                           EmberKeyData * key,
                           bool directionIncomming);
bool emGpCalculateIncomingCommandMic(EmberGpAddress * gpdAddr,
                                     uint8_t keyType,
                                     uint8_t securityLevel,
                                     uint32_t gpdSecurityFrameCounter,
                                     uint8_t gpdCommandId,
                                     uint8_t * gpdCommandPayload,
                                     bool encryptedPayload,
                                     uint8_t mic[4],
                                     EmberKeyData * key);
bool emGpCalculateIncomingCommandDecrypt(EmberGpAddress * gpdAddr,
                                         uint32_t gpdSecurityFrameCounter,
                                         uint8_t payloadLength,
                                         uint8_t * payload,
                                         EmberKeyData * key);
// zigbee device functions
EmberStatus emSendZigDevMessage(EmberNodeId destination,
                                const char * format, ...);
void emZigDevPrepareZdoMessage(EmberApsFrame *apsFrame,
                               uint16_t clusterId,
                               EmberApsOption options,
                               uint8_t sequence);
// gp security test function
void emGpTestSecurity(void);
void emGpTransTableClearTranslationTable(void);
void emGpSetTranslationTableEntry(uint8_t index);
EmberAfGreenPowerServerGpdSubTranslationTableEntry* emGpGetCustomizedTable(void);
void emGpClearCustomizedTable(void);
void emGpSetCustomizedTableEntry(uint8_t index);
EmberGpTranslationTableAdditionalInfoBlockField * emGpGetAdditionalInfoTable(void);
void embGpClearAdditionalInfoBlockTable(void);
void emGpSetAdditionalInfoBlockTableEntry(uint8_t index);
void emberAfGreenPowerServerCommissioningTimeoutCallback(uint8_t commissioningTimeoutType,
                                                         uint8_t numberOfEndpoints,
                                                         uint8_t * endpoint);
void emberAfGreenPowerServerPairingCompleteCallback(uint8_t numberOfEndpoints,
                                                    uint8_t * endpoint);

void removeGpdEndpointFromTranslationTable (EmberGpAddress *gpdAddr, uint8_t zbEndpoint);
#endif //_GREEN_POWER_SERVER_H_
