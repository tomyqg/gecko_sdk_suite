/***************************************************************************//**
 * @file
 * @brief Concentrator support for the NCP.
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

#ifndef SILABS_APP_EM260_CONCENTRATOR_H
#define SILABS_APP_EM260_CONCENTRATOR_H

#include "app/util/ezsp/ezsp-frame-utilities.h"

extern EmberEventControl emberAfPluginConcentratorSupportLibraryEventControl;
void emberAfPluginConcentratorSupportLibraryEventHandler(void);

void emConcentratorSupportRouteErrorHandler(EmberStatus status, EmberNodeId nodeId);

void emConcentratorSupportDeliveryErrorHandler(EmberOutgoingMessageType type,
                                               EmberStatus status);

EmberStatus emberAfPluginEzspZigbeeProSetConcentratorCommandCallback(bool on,
                                                                     uint16_t concentratorType,
                                                                     uint16_t minTime,
                                                                     uint16_t maxTime,
                                                                     uint8_t routeErrorThreshold,
                                                                     uint8_t deliveryFailureThreshold,
                                                                     uint8_t maxHops);

#define ADDRESS_DISCOVERY_DELAY_QS 2
#endif // SILABS_APP_EM260_CONCENTRATOR_H
