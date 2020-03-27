/***************************************************************************//**
 * @file
 * @brief CR2032 Capacity Calculation
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

#include "cr2032.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Type Definitions
 **************************************************************************************************/

typedef struct {
  uint16_t      voltage;
  uint8_t       capacity;
} voltageCapacityPair_t;

/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/

// Voltage - Capacity pair table.
// Algorithm assumes the values are arranged in a descending order.
// The values in the table are an average of those found in the CR2032 datasheets from
// Energizer, Maxell and Panasonic.
static voltageCapacityPair_t vcPairs[] =
{ { 3000, 100 }, { 2900, 80 }, { 2800, 60 }, { 2700, 40 }, { 2600, 30 },
  { 2500, 20 }, { 2400, 10 }, { 2000, 0 } };

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
uint8_t cr2032_CalculateLevel(uint16_t voltage)
{
  uint32_t res = 0;
  uint8_t i;

  // Iterate through voltage/capacity table until correct interval is found.
  // Then interpolate capacity within that interval based on a linear approximation
  // between the capacity at the low and high end of the interval.
  for (i = 0; i < (sizeof(vcPairs) / sizeof(voltageCapacityPair_t)); i++) {
    if (voltage > vcPairs[i].voltage) {
      if (i == 0) {
        // Higher than maximum voltage in table.
        return vcPairs[0].capacity;
      } else {
        // Calculate the capacity by interpolation.
        res = (voltage - vcPairs[i].voltage)
              * (vcPairs[i - 1].capacity - vcPairs[i].capacity)
              / (vcPairs[i - 1].voltage - vcPairs[i].voltage);
        res += vcPairs[i].capacity;
        return (uint8_t)res;
      }
    }
  }
  // Below the minimum voltage in the table.
  return vcPairs[sizeof(vcPairs) / sizeof(voltageCapacityPair_t) - 1].capacity;
}
