/***************************************************************************//**
 * @file
 * @brief Definitions for the ZLL Commissioning Server plugin.
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

// *******************************************************************
// * zll-commissioning-server.h
// *
// *
// Copyright 2010-2018 Silicon Laboratories, Inc.
// *******************************************************************

/** @brief No touchlink for non-factory new device.
 *
 * This function will cause the NFN device to refuse network start/join
 * requests in case it receives them and will not allow touchlinking. This can be useful
 * to restrict touchlink stealing.
 */
void emberAfZllNoTouchlinkForNFN(void);

/** @brief Disables touchlink processing.
 *
 * This function will cause the device to refuse network start/join
 * requests if it receives them and will not allow touchlinking.
 * Note that this will have the effect of overriding the
 * emberAfZllNoTouchlinkForNFN function.
 */
void emberAfZllDisable(void);

/** @brief Enables touchlink processing.
 *
 * This function will cause the device to accept network start/join
 * requests if it receives them and will not allow touchlinking.
 * Note that this will have the effect of overriding the
 * emberAfZllNoTouchlinkForNFN function.
 */
void emberAfZllEnable(void);

// For legacy code
#define emberAfPluginZllCommissioningGroupIdentifierCountCallback \
  emberAfPluginZllCommissioningServerGroupIdentifierCountCallback
#define emberAfPluginZllCommissioningGroupIdentifierCallback \
  emberAfPluginZllCommissioningServerGroupIdentifierCallback
#define emberAfPluginZllCommissioningEndpointInformationCountCallback \
  emberAfPluginZllCommissioningServerEndpointInformationCountCallback
#define emberAfPluginZllCommissioningEndpointInformationCallback \
  emberAfPluginZllCommissioningServerEndpointInformationCallback
#define emberAfPluginZllCommissioningIdentifyCallback \
  emberAfPluginZllCommissioningServerIdentifyCallback
