/***************************************************************************//**
 * @file
 * @brief Immediate Alert Service
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
/* BG stack headers */
#include "bg_types.h"
#include "native_gecko.h"

/* application specific headers */
#include "app_ui.h"

/* Own header */
#include "ia.h"

/***********************************************************************************************//**
 * @addtogroup Services
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup ia
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/
/* Text definitions*/
#define IA_HIGH_ALERT_TEXT          "\nAlert level:\n\nHIGH\n"
#define IA_LOW_ALERT_TEXT           "\nAlert level:\n\nLOW\n"
#define IA_NO_ALERT_TEXT            "\nAlert level:\n\nNo Alert\n"

/** Immediate Alert Level.
 *  Client sent a No Immediate Alert message. */
#define ALERT_NO                    0
/** Immediate Alert Level.
 *  Client sent a Mild Immediate Alert Level message. */
#define ALERT_MILD                  1
/** Immediate Alert Level.
 *  Client sent a High Immediate Alert Level message. */
#define ALERT_HIGH                  2

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

/***************************************************************************************************
 * Local Variables
 **************************************************************************************************/

/***************************************************************************************************
 * Function Definitions
 **************************************************************************************************/
void iaImmediateAlertWrite(uint8array *writeValue)
{
  switch (writeValue->data[0]) {
    default:
    case ALERT_NO:
      /* No Alert received */
      appUiWriteString(IA_NO_ALERT_TEXT);
      appUiLedOff();
      break;

    case ALERT_MILD:
      /* Low Alert received */
      appUiWriteString(IA_LOW_ALERT_TEXT);
      appUiLedLowAlert();
      break;

    case ALERT_HIGH:
      /* High Alert received */
      appUiWriteString(IA_HIGH_ALERT_TEXT);
      appUiLedHighAlert();
      break;
  }
}

/** @} (end addtogroup ia) */
/** @} (end addtogroup Services) */
