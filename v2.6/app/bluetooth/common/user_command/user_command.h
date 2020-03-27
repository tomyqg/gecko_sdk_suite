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

#ifndef __USER_COMMAND__
#define __USER_COMMAND__

#include "read_char.h"
#include "command_interpreter.h"
#include "user_command_config.h"

#define RETCODE_USER_COMMAND_ESCAPE     -2
#define RETCODE_USER_COMMAND_NO_CHAR    -1
#define RETCODE_USER_COMMAND_VALID_CHAR 0

void cliInitialize(void);
int8_t cliHandleInput(void);

#endif // __USER_COMMAND__
