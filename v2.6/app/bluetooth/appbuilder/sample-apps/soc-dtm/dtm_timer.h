/***************************************************************************//**
 * @file
 * @brief DTM timer definition
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

#if defined(CRYOTIMER_PRESENT)

#include "em_cryotimer.h"

#define DTM_TIMER_GET_TICKS  CRYOTIMER_CounterGet

#elif defined(BURTC_PRESENT)

#include "em_burtc.h"

#define DTM_TIMER_GET_TICKS  BURTC_CounterGet

#endif
