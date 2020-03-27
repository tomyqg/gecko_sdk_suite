/***************************************************************************//**
 * @file
 * @brief Application User Interface
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

#ifndef APP_UI_H
#define APP_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup app_ui Application User Interface
 * \brief User interface specific application file.
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Thunderboard
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app_ui
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Public Macros and Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Data Types
 **************************************************************************************************/

/***************************************************************************************************
 * Public Function Declarations
 **************************************************************************************************/

/**********************************************************************************************//**
 * @brief
 *   Function to initialize user interface LEDs. Should be run once at startup.
 *  return None
 *************************************************************************************************/
void appUiInit(void);

/**********************************************************************************************//**
 * @brief
 *   Function to handle blinking of LEDs. Should be called by a triggered
 *  event.
 *  return None
 *************************************************************************************************/
void appUiLedBlinkEvtHandler(void);

/**********************************************************************************************//**
 * @brief
 *   Function to start blinking blue and green LEDs alternating to indicate
 *  that a HW error has been detected.
 *  return None
 *************************************************************************************************/
void appUiError(void);

/**********************************************************************************************//**
 * @brief
 *   Function to trigger a blink of green LED when a connection is opened.
 *  return None
 *************************************************************************************************/
void appUiConnectionOpened(void);

/**********************************************************************************************//**
 * @brief
 *   Function to stop blinking blue LED when advertisement stops.
 *  return None
 *************************************************************************************************/
void appUiAdvStopped(void);

/**********************************************************************************************//**
 * @brief
 *   Function to start blinking blue LED when advertisement starts.
 *  return None
 *************************************************************************************************/
void appUiAdvStarted(void);

/**********************************************************************************************//**
 * @brief
 *   Function to turn off LEDs before going to sleep.
 *  return None
 *************************************************************************************************/
void appUiSleep(void);

/** @} (end addtogroup app_ui) */
/** @} (end addtogroup Thunderboard) */

#ifdef __cplusplus
};
#endif

#endif /* APP_UI_H */
