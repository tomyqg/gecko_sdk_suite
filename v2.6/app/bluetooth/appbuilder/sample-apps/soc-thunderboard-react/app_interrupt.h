/***************************************************************************//**
 * @file
 * @brief Application Interrupt
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

#ifndef APP_INTERRUPT_H
#define APP_INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup app_interrupt Application Interrupt Specific
 * \brief Interrupt specific application file.
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Thunderboard
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app_interrupt
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Defines
 **************************************************************************************************/
/** Application external signal enumeration. */
typedef enum {
  EXTSIGNAL_BUTTON0 =  1 << 0,
    EXTSIGNAL_BUTTON1 =  1 << 1,
    EXTSIGNAL_ACCGYRO =  1 << 2,
    EXTSIGNAL_HALL    =  1 << 3,
    EXTSIGNAL_ALUV    =  1 << 4
} appExtSignal_t;

/***************************************************************************************************
 * Data Types
 **************************************************************************************************/

/***************************************************************************************************
 * Global Variables
 **************************************************************************************************/

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void appInterruptInit(void);

/** @} (end addtogroup app_interrupt) */
/** @} (end addtogroup Thunderboard) */

#ifdef __cplusplus
};
#endif

#endif /* APP_INTERRUPT_H */
