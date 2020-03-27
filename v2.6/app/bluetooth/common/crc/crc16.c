/***************************************************************************//**
 * @file
 * @brief CRC16 functionality (CRC16-CCITT)
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
#include "crc16.h"

uint16_t crc16(const uint8_t newByte, uint16_t prevResult)
{
  prevResult = (prevResult >> 8) | (prevResult << 8);
  prevResult ^= newByte;
  prevResult ^= (prevResult & 0xff) >> 4;
  prevResult ^= (prevResult << 8) << 4;

  //[[ What I wanted is the following function:
  // prevResult ^= ((prevResult & 0xff) << 4) << 1;
  // Unfortunately the compiler does this in 46 cycles.  The next line of code
  // does the same thing, but the compiler uses only 10 cycles to implement it.
  //]]
  prevResult ^= ((uint8_t) ( (uint8_t) ( (uint8_t) (prevResult & 0xff) ) << 5))
                | ((uint16_t) ( (uint8_t) ( (uint8_t) (prevResult & 0xff)) >> 3) << 8);

  return prevResult;
}

uint16_t crc16Stream(const uint8_t* buffer, size_t length, uint16_t prevResult)
{
  size_t position = 0;
  for (; position < length; position++) {
    prevResult = crc16(buffer[position], prevResult);
  }

  return prevResult;
}
