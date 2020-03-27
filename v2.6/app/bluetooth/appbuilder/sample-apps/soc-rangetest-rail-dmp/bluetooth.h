/***************************************************************************//**
 * @file
 * @brief bluetooth.h
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

#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#include "gatt_db.h"

#define APP_CFG_TASK_BLUETOOTH_LL_PRIO    3u
#define APP_CFG_TASK_BLUETOOTH_STACK_PRIO 4u

#define MAX_CONNECTIONS 1

typedef enum {
  unknown     = 0,
  boot_to_dfu = 1,
  deactivated = 2
} ConnClosedReason;

typedef struct {
  uint16_t id;
  uint16_t value;
} charInd;

#define CHAR_IND_LIST_SIZE 16
typedef struct {
  charInd items[CHAR_IND_LIST_SIZE];
  uint8_t head;
  uint8_t tail;
  uint8_t size;
} charIndList;

void bluetoothAppTask(void *p_arg);
void bluetoothActivate(void);
void bluetoothDeactivate(void);
void bluetoothAdvertiseRealtimeData(uint8_t rssi, uint16_t cnt, uint16_t rcvd);
bool bluetoothEnqueueIndication(charInd* bci);
void bluetoothPktsSentIndications(bool enable);

#endif /* BLUETOOTH_H_ */
