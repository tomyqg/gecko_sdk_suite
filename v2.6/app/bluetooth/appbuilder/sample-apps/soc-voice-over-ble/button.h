/***************************************************************************//**
 * @file
 * @brief Button definitions
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
#ifndef BUTTON_H_
#define BUTTON_H_

/***************************************************************************//**
 * @addtogroup Button
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup Button_External_Messages Button External Messages
 * @{
 * @brief Button external message macro definitions
 ******************************************************************************/
#define EXTSIG_BUTTON_SW1_PRESSED  (1 << 0)     /**< External signal - SW1 button released */
#define EXTSIG_BUTTON_SW1_RELEASED (1 << 1)     /**< External signal - SW2 button released */

/** @} {end defgroup Button_External_Messages} */

/***************************************************************************//**
 * @defgroup Button_Functions Button Functions
 * @{
 * @brief Button functions
 ******************************************************************************/
void BUTTON_Init();

/** @} {end defgroup Button_Functions}*/

/** @} {end addtogroup Button} */

#endif /* BUTTON_H_ */
