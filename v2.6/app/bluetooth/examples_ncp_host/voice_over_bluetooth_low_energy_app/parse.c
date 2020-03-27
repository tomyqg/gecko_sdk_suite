/***************************************************************************//**
 * @file
 * @brief Parse source file
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

#include <string.h>

/* BG stack headers */
#include "gecko_bglib.h"

/* Own header */
#include "parse.h"
#include "config.h"

/***************************************************************************************************
 * Static Function Declarations
 **************************************************************************************************/
static int parse_address(const char *str, bd_addr *addr);
static void usage(void);
static void help(void);
static void printf_configuration(void);

/***************************************************************************************************
 * Public Variables
 **************************************************************************************************/

/***************************************************************************************************
 * Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Initialize adc_sample_rate variable in configuration structure by data from argument list.
 *  \param[in]  sample rate
 **************************************************************************************************/
void init_sample_rate(adc_sample_rate_t sr)
{
  switch (sr) {
    case sr_8k:
      CONF_get()->adc_sample_rate = sr_8k;
      break;
    case sr_16k:
    default:
      CONF_get()->adc_sample_rate = sr_16k;
      break;
  }
}

/***********************************************************************************************//**
 *  \brief  Initialize adc_resolution variable in configuration structure by data from argument list.
 *  \param[in]  adc resolution
 **************************************************************************************************/
void init_adc_resolution(adc_resolution_t res)
{
  switch (res) {
    case adc_res_8b:
      CONF_get()->adc_resolution = adc_res_8b;
      break;
    case adc_res_12b:
    default:
      CONF_get()->adc_resolution = adc_res_12b;
      break;
  }
}

/***********************************************************************************************//**
 *  \brief  Print bluetooth address on stdout.
 *  \param[in]  bluetooth address
 **************************************************************************************************/
static void printf_address(bd_addr *addr)
{
  printf("Remote address:          %02x:%02x:%02x:%02x:%02x:%02x\n", addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1], addr->addr[0]);
}

/***********************************************************************************************//**
 *  \brief  Parse bluetooth address.
 *  \param[in]  data to parse
 *  \param[out] parsed bluetooth address
 *  \return 0 if success, otherwise -1
 **************************************************************************************************/
static int parse_address(const char *str, bd_addr *addr)
{
  int a[6];
  int i;
  i = sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
             &a[5],
             &a[4],
             &a[3],
             &a[2],
             &a[1],
             &a[0]
             );
  if (i != 6) {
    return -1;
  }

  for (i = 0; i < 6; i++) {
    addr->addr[i] = (uint8_t)a[i];
  }

  return 0;
}

/***********************************************************************************************//**
 *  \brief  Print example of usage that application on stdout.
 **************************************************************************************************/
static void usage(void)
{
  printf("Example of usage:\n");
  printf("  voice_over_bluetooth_low_energy_app.exe -v -p COM1 -b 115200 -a 00:0b:57:1a:8c:2d -s 16 -r 12\n");
  printf("  voice_over_bluetooth_low_energy_app.exe -p COM1 -b 115200 -a 00:0b:57:1a:8c:2d \n");
  printf("  voice_over_bluetooth_low_energy_app.exe -a 00:0b:57:1a:8c:2d \n");
  printf("  voice_over_bluetooth_low_energy_app.exe -h \n");
}

/***********************************************************************************************//**
 *  \brief  Print help on stdout.
 **************************************************************************************************/
static void help(void)
{
  printf("Help:\n");
  printf("-p <port>       - COM port\n");
  printf("                  Default COM port is:\n");
  printf("                  Windows OS - %s\n", DEFAULT_WIN_OS_UART_PORT);
  printf("                  Apple OS   - %s\n", DEFAULT_APPLE_OS_UART_PORT);
  printf("                  Linux OS   - %s\n", DEFAULT_LINUX_OS_UART_PORT);
  printf("-b <baud_rate>  - Baud rate.\n");
  printf("                  Default %d b/s.\n", DEFAULT_UART_BAUD_RATE);
  printf("-o <file_name>  - Output file name.\n");
  printf("                  Audio data send to stdout by default.\n");
  printf("-a <bt_address> - Remote device bluetooth address. \n");
  printf("                  No default bluetooth address.\n");
  printf("-s <8/16>       - ADC sampling rate.\n");
  printf("                  8 or 16 kHz sampling rate can be used. Default - 16 kHz.\n");
  printf("-r <8/12>       - ADC resolution.\n");
  printf("                  8 or 12 bits resolution can be used. Default - 12 bits.\n");
  printf("-f <1/0>        - Enable/Disable filtering.\n");
  printf("                  Default filtering disabled. When filtering enabled HPF filter is used.\n");
  printf("-e <1/0>        - Enable/Disable encoding.\n");
  printf("                  Encoding enabled by default.\n");
  printf("-t <1/0>        - Enable/Disable transfer status.\n");
  printf("                  Transfer status enabled by default.\n");
  printf("-h              - Help\n");
  printf("-v              - Verbose\n");
  usage();
  exit(EXIT_SUCCESS);
}

/***********************************************************************************************//**
 *  \brief  Print configuration parameters on stdout.
 **************************************************************************************************/
static void printf_configuration(void)
{
  printf("Parameters:\n");
  printf("  Baud rate:               %d\n", CONF_get()->baud_rate);
  printf("  UART port:               %s\n", CONF_get()->uart_port);
  if ( CONF_get()->output_to_stdout == true) {
    printf("  Audio data send to:      stdout\n");
  } else {
    printf("  Audio data store into:   %s\n", CONF_get()->out_file_name);
  }
  printf("  "); printf_address(&CONF_get()->remote_address);
  printf("  Audio data notification: %s\n", CONF_get()->audio_data_notification ? "Enabled" : "Disabled");
  printf("  ADC sample rate:         %d[kHz]\n", CONF_get()->adc_sample_rate);
  printf("  ADC resolution:          %d-bits\n", CONF_get()->adc_resolution);
  printf("  Filtering:               %s\n", CONF_get()->filter_enabled ? "Enabled" : "Disabled");
  printf("  Encoding:                %s\n", CONF_get()->encoding_enabled ? "Enabled" : "Disabled");
  printf("  Transfer status:         %s\n", CONF_get()->transfer_status ? "Enabled" : "Disabled");
  printf("\n");
  return;
}

/***********************************************************************************************//**
 *  \brief  Add extension to output file depending on application parameters.
 **************************************************************************************************/
static void addExtensionToFile(void)
{
  char *ptr = NULL;

  if ( strcmp(CONF_get()->out_file_name, "-") == 0 ) {
    CONF_get()->output_to_stdout = true;
    return;
  }

  size_t ptrLen = 1 + strlen(CONF_get()->out_file_name) + 4;
  ptr = (char *)malloc(ptrLen);

  if ( ptr == NULL) {
    DEBUG_ERROR("Memory allocation failed. Exiting.\n,");
    exit(1);
  }

  strcpy(ptr, CONF_get()->out_file_name);

  if ( CONF_get()->encoding_enabled ) {
    strcat(ptr, IMA_FILE_EXTENSION);
  } else {
    switch (CONF_get()->adc_resolution) {
      case adc_res_8b:
        strcat(ptr, S8_FILE_EXTENSION);
        break;
      case adc_res_12b:
        strcat(ptr, S16_FILE_EXTENSION);
        break;
      default:
        break;
    }
  }

  if ( CONF_get()->out_file_name != NULL) {
    free(CONF_get()->out_file_name);
    CONF_get()->out_file_name = NULL;
  }

  CONF_get()->out_file_name = malloc(1 + strlen(ptr));
  strcpy(CONF_get()->out_file_name, ptr);

  free(ptr);
}

/***********************************************************************************************//**
 *  \brief  Parse application parameters.
 *  \param[in] argc Argument count.
 *  \param[in] argv Buffer contaning application parameters.
 **************************************************************************************************/
void PAR_parse(int argc, char **argv)
{
  static char uart_port_name[STR_UART_PORT_SIZE];
  bool verbose = false;

  if (argc == 1) {
    help();
  }

  for (uint8_t i = 0; i < argc; i++) {
    if (argv[i][0] == '-') {
      if ( argv[i][1] == 'p') {
        size_t len_arg = strlen(argv[i + 1]);
        size_t len = (len_arg > (STR_UART_PORT_SIZE - 1)) ? STR_UART_PORT_SIZE : len_arg;

        memcpy(uart_port_name, argv[i + 1], len);
        uart_port_name[len + 1] = '\0';
        CONF_get()->uart_port = uart_port_name;
      }

      if ( argv[i][1] == 'o') {
        size_t fLen = strlen(argv[i + 1]) + 1;
        CONF_get()->out_file_name = malloc(fLen);
        memcpy(CONF_get()->out_file_name, argv[i + 1], fLen);
      }

      if ( argv[i][1] == 'a') {
        if (parse_address(argv[i + 1], &CONF_get()->remote_address)) {
          DEBUG_ERROR("Unable to parse address %s", argv[i + 1]);
          exit(EXIT_FAILURE);
        } else {
          CONF_get()->remote_address_set = true;
        }
      }

      if ( argv[i][1] == 'b') {
        CONF_get()->baud_rate = (int)atoi(argv[i + 1]);
      }

      if ( argv[i][1] == 's') {
        init_sample_rate(atoi(argv[i + 1]));
      }

      if ( argv[i][1] == 'r') {
        init_adc_resolution(atoi(argv[i + 1]));
      }

      if ( argv[i][1] == 'f') {
        CONF_get()->filter_enabled = (bool)atoi(argv[i + 1]);
      }

      if ( argv[i][1] == 'e') {
        CONF_get()->encoding_enabled = (bool)atoi(argv[i + 1]);
      }

      if ( argv[i][1] == 't') {
        CONF_get()->transfer_status = (bool)atoi(argv[i + 1]);
      }

      if ( argv[i][1] == 'h') {
        help();
      }

      if ( argv[i][1] == 'v') {
        verbose = true;
      }
    }
  }

  addExtensionToFile();

  if (verbose) {
    printf_configuration();
  }
}
