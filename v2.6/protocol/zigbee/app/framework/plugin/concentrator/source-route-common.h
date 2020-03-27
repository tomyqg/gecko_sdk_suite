/***************************************************************************//**
 * @file
 * @brief Common code used for managing source routes on both node-based
 * and host-based gateways. See source-route.c for node-based gateways and
 * source-route-host.c for host-based gateways.
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

#ifndef SILABS_SOURCE_ROUTE_COMMON_H
#define SILABS_SOURCE_ROUTE_COMMON_H

extern uint8_t sourceRouteTableSize;
extern SourceRouteTableEntry *sourceRouteTable;

// A special index. For destinations that are neighbors of the gateway,
// closerIndex is set to 0xFF. For the oldest entry, olderIndex is set to
// 0xFF.
#define NULL_INDEX 0xFF

uint8_t sourceRouteFindIndex(EmberNodeId id);
uint8_t sourceRouteAddEntry(EmberNodeId id, uint8_t furtherIndex);
EmberStatus sourceRouteDeleteEntry(EmberNodeId id);
void sourceRouteInit(void);
uint8_t sourceRouteGetCount(void);
void sourceRouteClearTable(void);
uint8_t sourceRouteAddEntryWithCloserNextHop(EmberNodeId newId,
                                             EmberNodeId closerNodeId);
uint8_t emberGetSourceRouteTableFilledSize(void);
uint8_t emberGetSourceRouteTableTotalSize(void);
uint8_t emberGetSourceRouteOverhead(EmberNodeId destination);

#endif // SILABS_SOURCE_ROUTE_COMMON_H
