/***************************************************************************//**
 * @file
 * @brief main.c
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

/**
 * This an client of Voice over Bluetooth Low Energy (VoBLE) application
 * that demonstrates Bluetooth connectivity using BGLIB C function definitions and handling audio streaming for.
 * The example enables Bluetooth advertisements, configures following VoBLE parameters:
 * 1. ADC resolution,
 * 2. Sample Rate,
 * 3. Enable notification for Audio Data Characteristic
 * 4. Enable/Disable filtering
 * 5. Enable/Disable encoding
 * 6. Enable/Disable audio data streaming status
 * After initialization and establishing connection application waits for audio data transmission
 * and finally stores received data into the file or sends it to stdout.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to a command with a command response indicating
 * it has processed the request and then later sending an event indicating
 * the requested operation has been completed. */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "infrastructure.h"

/* BG stack headers */
#include "gecko_bglib.h"

/* hardware specific headers */
#include "uart.h"

/* application specific files */
#include "config.h"
#include "app.h"
#include "parse.h"

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

BGLIB_DEFINE();

/** The default serial port to use for BGAPI communication. */
#if ((_WIN32 == 1) || (__CYGWIN__ == 1))
static char* default_uart_port = DEFAULT_WIN_OS_UART_PORT;
#elif __APPLE__ == 1
static char* default_uart_port = DEFAULT_APPLE_OS_UART_PORT;
#elif __linux == 1
static char* default_uart_port = DEFAULT_LINUX_OS_UART_PORT;
#else
static char* default_uart_port = "";
#endif

/** The default serial port to use for BGAPI communication. */
static char def_uart_port[STR_UART_PORT_SIZE];

/** Usage string */
#define USAGE "Usage: %s [serial port] <baud rate> \n\n"

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/

static int appSerialPortInit(int argc, char* argv[], int32_t timeout);
static void on_message_send(uint32_t msg_len, uint8_t* msg_data);

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  The main program.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning application parameters.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
int main(int argc, char* argv[])
{
  struct gecko_cmd_packet* evt;

  /* Initialize BGLIB with our output function for sending messages. */
  BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

  /* Additional UART port initialization required due to BGAPI communication.*/
  memcpy(def_uart_port, default_uart_port, strlen(default_uart_port));
  CONF_get()->uart_port = def_uart_port;

  /* Parsing list of arguments */
  PAR_parse(argc, argv);

  /* Initialise serial communication as non-blocking. */
  if (appSerialPortInit(argc, argv, 100) < 0) {
    DEBUG_ERROR("Non-blocking serial port init failure\n");
    exit(EXIT_FAILURE);
  }

  // Flush std output
  fflush(stdout);

  DEBUG_INFO("Starting up...\n");
  DEBUG_INFO("Resetting NCP target...\n");

  /* Reset NCP to ensure it gets into a defined state.
   * Once the chip successfully boots, gecko_evt_system_boot_id event should be received. */
  gecko_cmd_system_reset(0);

  while (1) {
    /* Check for stack event. */
    evt = gecko_peek_event();
    /* Run application and event handler. */
    appHandleEvents(evt);
  }

  return -1;
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Function called when a message needs to be written to the serial port.
 *  \param[in] msg_len Length of the message.
 *  \param[in] msg_data Message data, including the header.
 **************************************************************************************************/
static void on_message_send(uint32_t msg_len, uint8_t* msg_data)
{
  /** Variable for storing function return values. */
  int32_t ret;

  ret = uartTx(msg_len, msg_data);
  if (ret < 0) {
    printf("Failed to write to serial port %s, ret: %d, errno: %d\n", CONF_get()->uart_port, ret, errno);
    exit(EXIT_FAILURE);
  }
}

/***********************************************************************************************//**
 *  \brief  Serial Port initialisation routine.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning Serial Port data.
 *  \return  0 on success, -1 on failure.
 **************************************************************************************************/
static int appSerialPortInit(int argc, char* argv[], int32_t timeout)
{
  static char tmpbaud[50];

  /**
   * Handle the command-line arguments.
   */
  switch (argc) {
    case 3:
      CONF_get()->baud_rate = atoi(argv[2]);
    /** Falls through on purpose. */
    case 2:
      CONF_get()->uart_port = argv[1];
    /** Falls through on purpose. */
    default:
      break;
  }
  if (!CONF_get()->uart_port) {/*no uart port given, ask from user*/
    printf("Uart port to use (default %s):", default_uart_port);
    if (fgets(tmpbaud, sizeof(tmpbaud), stdin)) {
      size_t len = strlen(tmpbaud);
      /* fgets is used, so remove the new line character from the end */
      if (tmpbaud[len - 1] == '\n') {
        len--;
        tmpbaud[len] = '\0';
      }
      if (tmpbaud[len - 1] == '\r') {
        len--;
        tmpbaud[len] = '\0';
      }

      if (len > 0) {
        CONF_get()->uart_port = tmpbaud;
      } else {
        printf("Default port is used\n");
        CONF_get()->uart_port = default_uart_port;
      }
    }
  }
  if (!CONF_get()->uart_port || !CONF_get()->baud_rate) {
    printf(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Initialise the serial port with RTS/CTS enabled. */
  return uartOpen((int8_t*)CONF_get()->uart_port, CONF_get()->baud_rate, 0, timeout);
}
