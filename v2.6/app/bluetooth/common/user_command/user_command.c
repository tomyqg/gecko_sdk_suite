/***************************************************************************//**
 * @file
 * @brief Provides high-level integration for character read and the
 * command-interpreter
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include COMMON_HEADER
#include "user_command.h"

CommandState_t state;
char ciBuffer[256];

static const CommandEntry_t commands[] = USER_COMMAND_LIST;

void cliInitialize(void)
{
  ciInitState(&state, ciBuffer, sizeof(ciBuffer), (CommandEntry_t *)commands);
}

int8_t cliHandleInput(void)
{
  uint8_t chr = (uint8_t)readChar();

  if (chr == 0x1B || chr == 0x04) {
    // Return RETCODE_USER_COMMAND_ESCAPE if ESC or Ctrl-D
    return RETCODE_USER_COMMAND_ESCAPE;
  } else if (chr != '\0' && chr != 0xFF) {
    if (ciProcessInput(&state, (char*)&chr, 1) > 0) {
      printf("> ");
    }
  }

  return RETCODE_USER_COMMAND_VALID_CHAR;
}
