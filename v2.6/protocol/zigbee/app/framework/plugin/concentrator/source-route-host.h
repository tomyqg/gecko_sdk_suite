/***************************************************************************//**
 * @file
 * @brief Declarations for managing source routes on a host-based gateway.
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

/** Search for a source route to the given destination. If one is found, return
 * true and copy the relay list to the given location. If no route is found,
 * return false and don't modify the given location.
 */
bool emberFindSourceRoute(EmberNodeId destination,
                          uint8_t *relayCount,
                          uint16_t *relayList);
