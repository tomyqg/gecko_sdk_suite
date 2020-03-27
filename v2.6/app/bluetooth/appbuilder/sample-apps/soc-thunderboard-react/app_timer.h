/***************************************************************************//**
 * @file
 * @brief Application timer header file
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

#ifndef APP_TIMER_H
#define APP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/** Timer Frequency used. */
#define TIMER_CLK_FREQ ((uint32)32768)
/** Convert msec to timer ticks. */
#define TIMER_MS_2_TIMERTICK(ms) ((TIMER_CLK_FREQ * ms) / 1000)
/** Stop timer. */
#define TIMER_STOP 0

/** Application timer enumeration. */
typedef enum {
  UI_TIMER                 =  0,
  ADV_TIMEOUT_TIMER        =  1,
  ADV_ALTERNATE_TIMER      =  2,
  UPDATE_CON_PARAM_TIMER   =  3,
  ACCORI_SERVICE_ACC_TIMER =  4,
  ACCORI_SERVICE_ORI_TIMER =  5,
  BATTERY_DEVICE_TIMER     =  6,
  BATT_SERVICE_TIMER       =  7,
  CSC_SERVICE_TIMER        =  8,
} appTimer_t;

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#endif /* APP_TIMER_H */
