/***************************************************************************//**
 * @file
 * @brief Silicon Labs Voice over Bluetooth Low Energy Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows transmit Voice over Bluetooth Low Energy (VoBLE). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
 * Voice transmission starts after SW1 button is pressed and released and stops after SW2 button is pressed and released.
 * Audio data stream could be filtered depending on configuration and encoded to ADPCM format.
 * Finally encoded data are written into "Audio Data" characteristic being a part of "Voice over BLE" Service.
 *
 * VoBLE Service contains following configuration characteristics:
 * 1. ADC Resolution - Set ADC resolution 8 or 12 bits
 * 2. Sample Rate    - Set sample rate 8 or 16 [kHz]
 * 3. Filter Enable  - Enable/Disable filtering
 *
 * There is also example of NCP application (voice_over_bluetooth_low_energy_app)
 * which demonstrate how to connect to Thunderboard Sense v1,
 * set correct configuration and store data into the file.
 *
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

#include <stdbool.h>
#include <sleep.h>
#include "board_features.h"
#include "native_gecko.h"
#include "gatt_db.h"

#ifdef FEATURE_SPI_FLASH
#include "em_usart.h"
#include "mx25flash_spi.h"
#endif /* FEATURE_SPI_FLASH */

#include "pti.h"

#include "thunderboard/util.h"
#include "thunderboard/board.h"

#include "InitDevice.h"
#include <filter.h>
#include "common.h"
#include "button.h"
#include "adpcm.h"
#include "mic.h"
#include "tb_sense.h"

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 4
#endif

uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

/* Gecko configuration parameters (see gecko_configuration.h) */
static const gecko_configuration_t config = {
  .config_flags = 0,
#if defined(FEATURE_LFXO)
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
#else
  .sleep.flags = 0,
#endif // LFXO
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .bluetooth.sleep_clock_accuracy = 100, // ppm
  .gattdb = &bg_gattdb_data,
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
  .pa.config_enable = 1, // Set this to be a valid PA config
  .pa.input = GECKO_RADIO_PA_INPUT_DCDC,
};

/* Flag for indicating DFU Reset must be performed */
uint8_t boot_to_dfu = 0;

/* Microphone variable */
extern biquad_t *filter;
static bool micEnabled = false;
static audio_data_buff_t micAudioBuffer;
static uint8_t ble_connection;

void init(void)
{
  UTIL_init();
  BOARD_init();
  BUTTON_Init();
  vobleInit();
  cb_init(getCircularBuffer(), (MIC_SAMPLE_BUFFER_SIZE * 10), sizeof(uint8_t));
}

/***************************************************************************//**
 * @brief
 *    Sets sample rate in Voice over BLE configuration structure
 *
 * @return
 *    None
 ******************************************************************************/
void writeRequestSampleRateHandle(struct gecko_cmd_packet* evt)
{
  switch ( evt->data.evt_gatt_server_user_write_request.value.data[0] ) {
    case sr_16k:
      getVobleConfig()->sampleRate = sr_16k;
      break;
    case sr_8k:
    default:
      getVobleConfig()->sampleRate = sr_8k;
      break;
  }

  send_write_response(evt, gattdb_sample_rate, bg_err_success);
}

/***************************************************************************//**
 * @brief
 *    Set enable/disable filtering flag in Voice over BLE configuration structure
 *
 * @return
 *    None
 ******************************************************************************/
void writeRequestFilterEnableHandle(struct gecko_cmd_packet* evt)
{
  getVobleConfig()->filter_enabled = (bool)evt->data.evt_gatt_server_user_write_request.value.data[0];
  send_write_response(evt, gattdb_filter_enable, bg_err_success);
}

/***************************************************************************//**
 * @brief
 *    Set enable/disable encoding flag in Voice over BLE configuration structure
 *
 * @return
 *    None
 ******************************************************************************/
void writeRequestEncodingEnableHandle(struct gecko_cmd_packet* evt)
{
  getVobleConfig()->encoding_enabled = (bool)evt->data.evt_gatt_server_user_write_request.value.data[0];
  send_write_response(evt, gattdb_encoding_enable, bg_err_success);
}

/***************************************************************************//**
 * @brief
 *    Set enable/disable transfer status flag in Voice over BLE configuration structure
 *
 * @return
 *    None
 ******************************************************************************/
void writeRequestTransferStatusHandle(struct gecko_cmd_packet* evt)
{
  getVobleConfig()->transfer_status = (bool)evt->data.evt_gatt_server_user_write_request.value.data[0];
  send_write_response(evt, gattdb_transfer_status, bg_err_success);
}

/***************************************************************************//**
 * @brief
 *    Send audio data from circular buffer. Data are sent in packages of MIC_SEND_BUFFER_SIZE size.
 *    If there is less then MIC_SEND_BUFFER_SIZE in circular buffer data will be send after next DMA readout.
 *
 * @return
 *    None
 ******************************************************************************/
void send_audio_data(void)
{
  uint16_t cb_error;
  uint8_t buffer[MIC_SEND_BUFFER_SIZE];

  cb_error = cb_pop_buff(getCircularBuffer(), buffer, MIC_SEND_BUFFER_SIZE);

  if ( cb_error == cb_err_ok ) {
    /* Write data to characteristic */
    gecko_cmd_gatt_server_send_characteristic_notification(ble_connection, gattdb_audio_data, (MIC_SEND_BUFFER_SIZE), buffer);
    gecko_external_signal(EXT_SIGNAL_READY_TO_SEND);
  }
}

/**
 * @brief  Main function
 */
int main(void)
{
#ifdef FEATURE_SPI_FLASH
  /* Put the SPI flash into Deep Power Down mode for those radio boards where it is available */
  MX25_init();
  MX25_DP();
  /* We must disable SPI communication */
  MX25_deinit();
#endif /* FEATURE_SPI_FLASH */

  /* Initialize peripherals */
  enter_DefaultMode_from_RESET();

#if (HAL_PTI_ENABLE == 1) || defined(FEATURE_PTI_SUPPORT)
  /* Configure and enable PTI */
  configEnablePti();
#endif

  /* Initialize stack */
  gecko_init(&config);

  init();

  while (1) {
    /* Event pointer for handling events */
    struct gecko_cmd_packet* evt;

    /* Check for stack event. */
    evt = gecko_wait_event();

    /* Handle events */
    switch (BGLIB_MSG_ID(evt->header)) {
      /* This boot event is generated when the system boots up after reset.
       * Do not call any stack commands before receiving the boot event.
       * Here the system is set to start advertising immediately after boot procedure. */
      case gecko_evt_system_boot_id:

        /* Set advertising parameters. 100ms advertisement interval.
         * The first two parameters are minimum and maximum advertising interval, both in
         * units of (milliseconds * 1.6). */
        gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);
        gecko_cmd_gatt_set_max_mtu(250);

        /* Start general advertising and enable connections. */
        gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        break;

      case gecko_evt_le_connection_closed_id:

        /* Check if need to boot to dfu mode */
        if (boot_to_dfu) {
          /* Enter to DFU OTA mode */
          gecko_cmd_system_reset(2);
        } else {
          /* Restart advertising after client has disconnected */
          gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        }
        break;

      /* This event is triggered after the connection has been opened */
      case gecko_evt_le_connection_opened_id:
        ble_connection = evt->data.evt_le_connection_opened.connection;
        break;

      /* Events related to OTA upgrading
         ----------------------------------------------------------------------------- */

      case gecko_evt_gatt_server_user_write_request_id:
        switch (evt->data.evt_gatt_server_user_write_request.characteristic) {
          /* Check if the user-type OTA Control Characteristic was written.
           * If ota_control was written, boot the device into Device Firmware Upgrade (DFU) mode. */
          case gattdb_ota_control:
            /* Set flag to enter to OTA mode */
            boot_to_dfu = 1;

            /* Send response to Write Request */
            send_write_response(evt, gattdb_ota_control, bg_err_success);

            /* Close connection to enter to DFU OTA mode */
            gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection);
            break;

          /* Check if the user-type ADC Resolution Characteristic was written
             If adc_resolution was written, set correct configuration in voble structure.*/
          case gattdb_adc_resolution:
            writeRequestAdcResolutionHandle(evt);
            break;

          /* Check if the user-type Sample Rate Characteristic was written
             If sample_rate was written, set correct configuration in voble structure.*/
          case gattdb_sample_rate:
            writeRequestSampleRateHandle(evt);
            break;

          /* Check if the user-type Filter Enable Characteristic was written
             If filter_enable was written, set correct configuration in voble structure.*/
          case gattdb_filter_enable:
            writeRequestFilterEnableHandle(evt);
            break;

          /* Check if the user-type Encoding Enable Characteristic was written
             If encoding_enable was written, set correct configuration in voble structure.*/
          case gattdb_encoding_enable:
            writeRequestEncodingEnableHandle(evt);
            break;

          /* Check if the user-type Transfer Status Characteristic was written
             If transfer_status was written, set correct configuration in voble structure.*/
          case gattdb_transfer_status:
            writeRequestTransferStatusHandle(evt);
            break;

          default:
            break;
        }
        break;

      case gecko_evt_system_external_signal_id:
        switch (evt->data.evt_system_external_signal.extsignals) {
          case EXTSIG_BUTTON_SW1_PRESSED:
            if (!micEnabled) {
              /*Microphone initialization and start audio data acquisition*/
              uint32_t status = MIC_init(getVobleConfig()->sampleRate, &micAudioBuffer);
              if ( status ) {
                break;
              }

              /* ADPCM encoder initialization */
              ADPCM_init();

              /* Filter initialization. Filter parameters: */
              if ( getVobleConfig()->filter_enabled) {
                filter_parameters_t fp = DEFAULT_FILTER;
                filter = FIL_Init(&fp);
              }

              /* Block Energy Mode 2 when audio acquisition starts*/
              SLEEP_SleepBlockBegin(sleepEM2);

              /* Audio data acquisition started*/
              micEnabled = true;

              /* Transfer status flag set to 1*/
              uint8_t transfer_start = TRANSFER_START;
              gecko_cmd_gatt_server_send_characteristic_notification(ble_connection, gattdb_transfer_status, sizeof(uint8_t), (uint8_t *)(&transfer_start));
            }
            break;
          case EXTSIG_BUTTON_SW1_RELEASED:
            if (micEnabled) {
              /* Filter deinitialization*/
              free(filter); filter = NULL;

              /*Microphone deinitialization */
              MIC_deInit();

              /* Unblock Energy Mode 2 when audio acquisition stopped*/
              SLEEP_SleepBlockEnd(sleepEM2);

              /* Audio data acquisition stopped*/
              micEnabled = false;

              /* Transfer status flag set to 0*/
              uint8_t transfer_stop = TRANSFER_STOP;
              gecko_cmd_gatt_server_send_characteristic_notification(ble_connection, gattdb_transfer_status, sizeof(uint8_t), (uint8_t *)(&transfer_stop));
            }
            break;

          /* Process data from buffer0 */
          case EXT_SIGNAL_BUFFER0_READY:
            processData(MIC_getSampleBuffer0());
            break;

          /* Process data from buffer1 */
          case EXT_SIGNAL_BUFFER1_READY:
            processData(MIC_getSampleBuffer1());
            break;

          /* Data ready to send in circular buffer */
          case EXT_SIGNAL_READY_TO_SEND:
            send_audio_data();
            break;

          default:
            break;
        }
        break;

      default:
        break;
    }
  }

  return 0;
}

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */
