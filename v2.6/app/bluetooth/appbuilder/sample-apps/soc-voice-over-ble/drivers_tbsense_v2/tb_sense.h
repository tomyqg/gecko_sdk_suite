/***************************************************************************//**
 * @file
 * @brief *******************************************************************************
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

#ifndef __TB_SENSE_H_
#define __TB_SENSE_H_

/***************************************************************************//**
 * @addtogroup TB_Sense
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @defgroup TB_Sense Specific Thunderboard Sense 2 API
 * @{
 * @brief Specific Thunderboard Sense 2 functions
 ******************************************************************************/

/** @} {end defgroup TB_Sense} */

/***************************************************************************//**
 * @defgroup TB_Sense_Functions Application function definitions
 * @{
 * @brief Thunderboard Sense 2 public API function definitions
 ******************************************************************************/

void vobleInit(void);
void writeRequestAdcResolutionHandle(struct gecko_cmd_packet* evt);
void processData(int16_t *pSrc);

/** @} {end defgroup TB_Sense_Functions}*/

/** @} {end addtogroup TB_Sense} */

#endif /* __TB_SENSE_H_ */
