/***************************************************************************//**
 * @file
 * @brief Range Test static configuration source.
 * This file contains the different application configurations for each
 * separate radio cards.
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

#include <stdint.h>
#include <stdbool.h>

#include "rangetestconfig.h"

// 'name' member must not contain the following characters as they are used as
// separators in the characteristic: ',' and ':'
RangeTestPhys rangeTestPhys[5] = {
  { .phy = PHY_BLE_1mbps,
    .name = "BLE 1Mbps",
    .isSupported = true,
    .protocol = PROT_BLE },
  { .phy = PHY_BLE_2mbps,
    .name = "BLE 2Mbps",
    .isSupported = true,
    .protocol = PROT_BLE },
  { .phy = PHY_BLE_Coded_125kbps,
    .name = "BLE 125kbps",
    .isSupported = true,
    .protocol = PROT_BLE },
  { .phy = PHY_BLE_Coded_500kbps,
    .name = "BLE 500kbps",
    .isSupported = true,
    .protocol = PROT_BLE },
  { .phy = PHY_IEEE_802154,
    .name = "IEEE 802.15.4",
    .isSupported = true,
    .protocol = PROT_IEEE802154 }
};
