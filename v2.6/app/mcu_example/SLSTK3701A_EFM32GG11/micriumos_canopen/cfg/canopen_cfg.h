/***************************************************************************//**
 * @file
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 * The software is governed by the sections of the MSLA applicable to Micrium
 * Software.
 *
 ******************************************************************************/

/*
*********************************************************************************************************
*
*                                        CANOPEN CONFIGURATION
*
*                                      CONFIGURATION TEMPLATE FILE
*
* File : canopen_cfg.h
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef _CANOPEN_CFG_H_
#define _CANOPEN_CFG_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <common/include/lib_def.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN OBJECT DIRECTORY CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_OBJ_PARAM_EN         DEF_ENABLED
#define CANOPEN_OBJ_STRING_EN        DEF_ENABLED
#define CANOPEN_OBJ_DOMAIN_EN        DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN SDO SERVER CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_SDO_MAX_SERVER_QTY   2
#define CANOPEN_SDO_DYN_ID_EN        DEF_ENABLED
#define CANOPEN_SDO_SEG_EN           DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN EMERGENCY CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_EMCY_MAX_ERR_QTY     6
#define CANOPEN_EMCY_REG_CLASS_EN    DEF_DISABLED
#define CANOPEN_EMCY_EMCY_MAN_EN     DEF_DISABLED
#define CANOPEN_EMCY_HIST_EN         DEF_ENABLED
#define CANOPEN_EMCY_HIST_MAN_EN     DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN TIMER MANAGEMENT CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_TMR_MAX_QTY          5


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN PDO TRANSFER CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_RPDO_MAX_QTY         1
#define CANOPEN_RPDO_MAX_MAP_QTY     2
#define CANOPEN_RPDO_DYN_COM_EN      DEF_ENABLED
#define CANOPEN_RPDO_DYN_MAP_EN      DEF_ENABLED

#define CANOPEN_TPDO_MAX_QTY         1
#define CANOPEN_TPDO_MAX_MAP_QTY     2
#define CANOPEN_TPDO_DYN_COM_EN      DEF_ENABLED
#define CANOPEN_TPDO_DYN_MAP_EN      DEF_ENABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN SYNC CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_SYNC_EN              DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      CANOPEN DEBUG CONFIGURATION
*********************************************************************************************************
*********************************************************************************************************
*/

#define CANOPEN_DBG_CTR_ERR_EN       DEF_DISABLED


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _CANOPEN_CFG_H_ */
