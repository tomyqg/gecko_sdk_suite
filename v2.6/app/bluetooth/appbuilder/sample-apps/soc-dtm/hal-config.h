/***************************************************************************//**
 * @file
 * @brief hal-config.h
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

#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

#include "board_features.h"
#include "hal-config-board.h"
#include "hal-config-app-common.h"

#ifndef HAL_VCOM_ENABLE
#define HAL_VCOM_ENABLE                   (0)
#endif
#ifndef HAL_I2CSENSOR_ENABLE
#define HAL_I2CSENSOR_ENABLE              (0)
#endif
#ifndef HAL_SPIDISPLAY_ENABLE
#define HAL_SPIDISPLAY_ENABLE             (0)
#endif

#ifdef  FEATURE_EXP_HEADER_USART3

#define BSP_EXP_USART           USART3

#define BSP_EXP_USART_CTS_PIN   BSP_USART3_CTS_PIN
#define BSP_EXP_USART_CTS_PORT  BSP_USART3_CTS_PORT
#define BSP_EXP_USART_CTS_LOC   BSP_USART3_CTS_LOC

#define BSP_EXP_USART_RTS_PIN   BSP_USART3_RTS_PIN
#define BSP_EXP_USART_RTS_PORT  BSP_USART3_RTS_PORT
#define BSP_EXP_USART_RTS_LOC   BSP_USART3_RTS_LOC

#define BSP_EXP_USART_RX_PIN    BSP_USART3_RX_PIN
#define BSP_EXP_USART_RX_PORT   BSP_USART3_RX_PORT
#define BSP_EXP_USART_RX_LOC    BSP_USART3_RX_LOC

#define BSP_EXP_USART_TX_PIN    BSP_USART3_TX_PIN
#define BSP_EXP_USART_TX_PORT   BSP_USART3_TX_PORT
#define BSP_EXP_USART_TX_LOC    BSP_USART3_TX_LOC

#else

#define BSP_EXP_USART           USART0

#define BSP_EXP_USART_CTS_PIN   BSP_USART0_CTS_PIN
#define BSP_EXP_USART_CTS_PORT  BSP_USART0_CTS_PORT
#define BSP_EXP_USART_CTS_LOC   BSP_USART0_CTS_LOC

#define BSP_EXP_USART_RTS_PIN   BSP_USART0_RTS_PIN
#define BSP_EXP_USART_RTS_PORT  BSP_USART0_RTS_PORT
#define BSP_EXP_USART_RTS_LOC   BSP_USART0_RTS_LOC

#define BSP_EXP_USART_RX_PIN    BSP_USART0_RX_PIN
#define BSP_EXP_USART_RX_PORT   BSP_USART0_RX_PORT
#define BSP_EXP_USART_RX_LOC    BSP_USART0_RX_LOC

#define BSP_EXP_USART_TX_PIN    BSP_USART0_TX_PIN
#define BSP_EXP_USART_TX_PORT   BSP_USART0_TX_PORT
#define BSP_EXP_USART_TX_LOC    BSP_USART0_TX_LOC

#endif // FEATURE_EXP_HEADER_USART3

#endif
