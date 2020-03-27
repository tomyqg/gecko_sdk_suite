/***************************************************************************//**
 * @file
 * @brief CLI for the EM4 plugin.
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

#include "app/framework/include/af.h"
#include "em4.h"

#ifndef EMBER_AF_GENERATE_CLI
  #error The em4 plugin is not compatible with the legacy CLI.
#endif

void emberAfPluginEm4StatusCommand(void)
{
  if (!emberAfOKToGoToEM4Callback()) {
    emberAfCorePrintln("Inactive");
    emberAfCorePrintln("In order to activate EM4 mode you must implement emberAfOKToGoToEM4Callback() to return True");
  } else {
    emberAfCorePrintln("Active");
  }
}
