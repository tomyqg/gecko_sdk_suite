/***************************************************************************//**
 * @file
 * @brief Infrastructure to integrate corss-stack codes to BLE SDK
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

#ifndef __STACK_BRIDGE__
#define __STACK_BRIDGE__

#define MEMCOPY(dest, src, len)       memcpy(dest, src, len)
#define MEMSET(dest, val, len)        memset(dest, val, len)
#define MEMCOMPARE(data1, data2, len) memcmp(data1, data2, len)

#define HIGH_BYTE(n)  UINT16_TO_BYTE1(n);
#define LOW_BYTE(n)   UINT16_TO_BYTE0(n);

#define BYTE_0(n)     UINT32_TO_BYTE0(n)
#define BYTE_1(n)     UINT32_TO_BYTE1(n)
#define BYTE_2(n)     UINT32_TO_BYTE2(n)
#define BYTE_3(n)     UINT32_TO_BYTE3(n)

#endif // __STACK_BRIDGE__
