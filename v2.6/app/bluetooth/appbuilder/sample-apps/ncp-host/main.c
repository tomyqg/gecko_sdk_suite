/***************************************************************************//**
 * @file
 * @brief Silicon Labs Embedded NCP Host Example
 * This example shows how to connect to an NCP device via UART. It will reset
 * the connected NCP device and make it advertise.
 * Another WSTK is needed with another radio board running the NCP empty example
 * on it. The WSTK boards' TX, RX and ground pins should be connected in the EXP
 * header pins, respectively.
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

/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Libraries containing default Gecko configuration values */
#include "em_usart.h"
#include "em_emu.h"
#include "em_cmu.h"

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/* BG stack headers */
#include "gecko_bglib.h"

BGLIB_DEFINE();

static void appHandleEvents(struct gecko_cmd_packet *evt);

/* These functions are used by BGLIB via function pointers of the types described below.*/

/***********************************************************************************************//**
 *  \brief  Blocking read data from serial port. The function will block until the desired amount
 *          has been read or an error occurs.
 *  \note  In order to use this function the serial port has to be configured blocking.
 *  \param[in]  dataLength The amount of bytes to read.
 *  \param[out]  data Buffer used for storing the data.
 *  \return  The amount of bytes read or -1 on failure.
 **************************************************************************************************/
int32_t uartRx(uint32_t dataLength, uint8_t* data)
{
  int32_t length = dataLength;
  while (length) {
    *data = USART_Rx(BSP_EXP_USART);
    length -= 1;
    data += 1;
  }
  return dataLength;
}

/***********************************************************************************************//**
 *  \brief  Write data to serial port. The function will block until
 *          the desired amount has been written or an error occurs.
 *  \param[in]  dataLength The amount of bytes to write.
 *  \param[in]  data Buffer used for storing the data.
 *  \return  The amount of bytes written or -1 on failure.
 **************************************************************************************************/
void uartTx(uint32_t dataLength, uint8_t* data)
{
  uint32_t length = dataLength;
  while (length) {
    USART_Tx(BSP_EXP_USART, *data);
    length -= 1;
    data += 1;
  }
}

/***********************************************************************************************//**
 *  \brief  Return the number of bytes in the input buffer.
 *  \return  The number of bytes in the input buffer or -1 on failure.
 **************************************************************************************************/
int32_t uartRxPeek(void)
{
  if (BSP_EXP_USART->STATUS & USART_STATUS_RXDATAV) {
    return 1;
  } else {
    return 0;
  }
}

// App booted flag
static bool appBooted = false;

void usartInit(void)
{
  USART_InitAsync_TypeDef initasync = USART_INITASYNC_DEFAULT;

  initasync.enable               = usartDisable;
  initasync.baudrate             = 115200;
  initasync.databits             = usartDatabits8;
  initasync.parity               = usartNoParity;
  initasync.stopbits             = usartStopbits1;
  initasync.oversampling         = usartOVS16;
  #if defined(USART_INPUT_RXPRS) && defined(USART_CTRL_MVDIS)
  initasync.mvdis                = 0;
  initasync.prsRxEnable          = 0;
  initasync.prsRxCh              = 0;
  #endif

  USART_InitAsync(BSP_EXP_USART, &initasync);
  USART_PrsTriggerInit_TypeDef initprs = USART_INITPRSTRIGGER_DEFAULT;

  initprs.rxTriggerEnable        = 0;
  initprs.txTriggerEnable        = 0;
  initprs.prsTriggerChannel      = usartPrsTriggerCh0;

  USART_InitPrsTrigger(BSP_EXP_USART, &initprs);
#if defined(USART_ROUTEPEN_RXPEN)
  /* Disable CLK pin */
  BSP_EXP_USART->ROUTELOC0  = (BSP_EXP_USART->ROUTELOC0 & (~_USART_ROUTELOC0_CLKLOC_MASK)) | USART_ROUTELOC0_CLKLOC_LOC0;
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN & (~USART_ROUTEPEN_CLKPEN);
  /* Disable CS pin */
  BSP_EXP_USART->ROUTELOC0  = (BSP_EXP_USART->ROUTELOC0 & (~_USART_ROUTELOC0_CSLOC_MASK)) | USART_ROUTELOC0_CSLOC_LOC0;
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN & (~USART_ROUTEPEN_CSPEN);
  /* Disable CTS pin */
  BSP_EXP_USART->ROUTELOC1  = (BSP_EXP_USART->ROUTELOC1 & (~_USART_ROUTELOC1_CTSLOC_MASK)) | (((uint32_t)BSP_EXP_USART_CTS_LOC) << _USART_ROUTELOC1_CTSLOC_SHIFT);
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN & (~USART_ROUTEPEN_CTSPEN);
  /* Disable RTS pin */
  BSP_EXP_USART->ROUTELOC1  = (BSP_EXP_USART->ROUTELOC1 & (~_USART_ROUTELOC1_RTSLOC_MASK)) | (((uint32_t)BSP_EXP_USART_RTS_LOC) << _USART_ROUTELOC1_RTSLOC_SHIFT);
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN & (~USART_ROUTEPEN_RTSPEN);
  /* Set up RX pin */
  BSP_EXP_USART->ROUTELOC0  = (BSP_EXP_USART->ROUTELOC0 & (~_USART_ROUTELOC0_RXLOC_MASK)) | (((uint32_t)BSP_EXP_USART_RX_LOC) << _USART_ROUTELOC0_RXLOC_SHIFT);
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN | USART_ROUTEPEN_RXPEN;
  /* Set up TX pin */
  BSP_EXP_USART->ROUTELOC0  = (BSP_EXP_USART->ROUTELOC0 & (~_USART_ROUTELOC0_TXLOC_MASK)) | (((uint32_t)BSP_EXP_USART_TX_LOC) << _USART_ROUTELOC0_TXLOC_SHIFT);
  BSP_EXP_USART->ROUTEPEN    = BSP_EXP_USART->ROUTEPEN | USART_ROUTEPEN_TXPEN;
#elif defined(GPIO_USART_ROUTEEN_RXPEN)
  GPIO->USARTROUTE[USART_NUM(BSP_EXP_USART)].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN
                                                       | GPIO_USART_ROUTEEN_RXPEN;
  GPIO->USARTROUTE[USART_NUM(BSP_EXP_USART)].TXROUTE =
    (BSP_EXP_USART_TX_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
    | (BSP_EXP_USART_TX_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[USART_NUM(BSP_EXP_USART)].RXROUTE =
    (BSP_EXP_USART_RX_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
    | (BSP_EXP_USART_RX_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT);
#endif

  /* Disable CTS */
  BSP_EXP_USART->CTRLX   = BSP_EXP_USART->CTRLX & (~USART_CTRLX_CTSEN);
  /* Set CTS active low */
  BSP_EXP_USART->CTRLX   = BSP_EXP_USART->CTRLX & (~USART_CTRLX_CTSINV);
  /* Set RTS active low */
  BSP_EXP_USART->CTRLX   = BSP_EXP_USART->CTRLX & (~USART_CTRLX_RTSINV);
  /* Set CS active low */
  BSP_EXP_USART->CTRL    = BSP_EXP_USART->CTRL & (~USART_CTRL_CSINV);
  /* Set TX active high */
  BSP_EXP_USART->CTRL    = BSP_EXP_USART->CTRL & (~USART_CTRL_TXINV);
  /* Set RX active high */
  BSP_EXP_USART->CTRL    = BSP_EXP_USART->CTRL & (~USART_CTRL_RXINV);

  /* Enable USART if opted by user */
  USART_Enable(BSP_EXP_USART, usartEnable);

  /* GPIO configurations */
  GPIO_PinModeSet(BSP_EXP_USART_TX_PORT, BSP_EXP_USART_TX_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BSP_EXP_USART_RX_PORT, BSP_EXP_USART_RX_PIN, gpioModeInput, 0);
}

/**
 * @brief  Main function
 */
int main(void)
{
  struct gecko_cmd_packet* evt;

  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();
  // Initializing the USART
  usartInit();

  /* Initialize BGLIB with our output function for sending messages. */
  BGLIB_INITIALIZE_NONBLOCK(uartTx, uartRx, uartRxPeek);

  /* Reset NCP to ensure it gets into a defined state.
   * Once the chip successfully boots, gecko_evt_system_boot_id event should be received. */
  gecko_cmd_system_reset(0);

  while (1) {
    /* Check for stack event. */
    evt = gecko_peek_event();
    /* Run application and event handler. */
    appHandleEvents(evt);
  }
}

/***************************************************************************************************
 * Static Function Definitions
 **************************************************************************************************/

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
static void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
    return;
  }

  /* Handle events */
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:

      appBooted = true;

      /* Set advertising parameters. 100ms advertisement interval. All channels used.
       * The first parameter is the handle.
       * The next two parameters are minimum and maximum advertising interval, both in
       * units of 0.625ms.
       * The last two parameters mean no advertising limit in time and
       * no limit in number of events, respectively.*/
      gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);

      /* Start general advertising and enable connections, giving a handle followed by
       * discoverability and connectability modes.*/
      gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);

      break;

    case gecko_evt_le_connection_closed_id:

      /* Restart general advertising and re-enable connections after disconnection. */
      gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);

      break;

    default:
      break;
  }
}

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */
