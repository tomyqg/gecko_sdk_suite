/***************************************************************************//**
 * @file
 * @brief Application main entry and initialisations.
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
#include "gpd-components-common.h"
#include "gpd-callbacks.h"

// Main entry to application
int main(void)
{
  // Initialise the chip
  CHIP_Init();

  // Initialise NV
  emberGpdNvInit();

  // Initialise the system clocks and other HAL components
  halInit();

  // Initialise timer for rxOffset timing during rxAfterTx
  emberGpdLeTimerInit();

  // Initialise the BSP
  BSP_Init(BSP_INIT_BCC);

  SLEEP_Init(NULL, NULL);
  SLEEP_SleepBlockEnd(sleepEM3);

  // Initialise Radio
  emberGpdRadioInit();

  //Initialise the Gpd
  EmberGpd_t * gpd = emberGpdInit();

  // Call user to implement rest of the thing
  emberGpdAfPluginMainCallback(gpd);
}
