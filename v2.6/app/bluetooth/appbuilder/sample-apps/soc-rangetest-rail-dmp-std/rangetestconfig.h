/***************************************************************************//**
 * @file
 * @brief Prototype definitions for Range Test application and radio
 * configuration structs.
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

#ifndef RANGETESTSTATICCONFIG_H_
#define RANGETESTSTATICCONFIG_H_

typedef enum {
  PHY_BLE_1mbps         = 0,
  PHY_BLE_2mbps         = 1,
  PHY_BLE_Coded_125kbps = 2,
  PHY_BLE_Coded_500kbps = 3,
  PHY_IEEE_802154       = 4,
  PHY_NO_OF_ELEMENTS
} RangeTestPhy;

typedef enum {
  PROT_BLE              = 0,
  PROT_IEEE802154       = 1,
  PROT_NO_OF_ELEMENTS
} RangeTestProtocol;

// RAIL buffer sizes
#define RAIL_TX_BUFFER_SIZE         (128u)
#define RAIL_RX_BUFFER_SIZE         (256u)

#define PHY_FREQUENCY             2450
#define PHY_DEFAULT               PHY_IEEE_802154

typedef struct {
  RangeTestPhy phy;
  char* name;
  bool isSupported;
  RangeTestProtocol protocol;
} RangeTestPhys;

// Time spacing between two packets in 10ms units
#define RANGETEST_TX_PERIOD       10

extern RangeTestPhys rangeTestPhys[5];

#endif /* RANGETESTSTATICCONFIG_H_ */
