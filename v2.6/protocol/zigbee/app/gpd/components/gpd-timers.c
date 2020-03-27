/***************************************************************************//**
 * @file
 * @brief Routines for timers used by the GPD.
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
#include "gpd-components-common.h"

static volatile bool leTimerRunning = false;

static const LETIMER_Init_TypeDef letimerInit = {
  .enable = false, /* Start counting when init completed*/
  .debugRun = false, /* Counter shall not keep running during debug halt. */
  .comp0Top = true, /* Load COMP0 register into CNT when counter underflows. COMP is used as TOP */
  .bufTop = false, /* Don't load COMP1 into COMP0 when REP0 reaches 0. */
  .out0Pol = 0, /* Idle value for output 0. */
  .out1Pol = 0, /* Idle value for output 1. */
  .ufoa0 = letimerUFOANone, /* PwM output on output 0 */
  .ufoa1 = letimerUFOANone, /* No output on output 1*/
  .repMode = letimerRepeatOneshot /* Count while REP != 0 */
};

bool emberGpdLeTimerRunning(void)
{
  return leTimerRunning;
}

void emberGpdLeTimerInit(void)
{
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFRCO);
  CMU_ClockEnable(cmuClock_CORELE, true); /* Enable CORELE clock */
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  LETIMER_CompareSet(LETIMER0, 0, 0xFFFF);
  NVIC_EnableIRQ(LETIMER0_IRQn);
  LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);

  LETIMER_Init(LETIMER0, &letimerInit);
}

void LETIMER0_IRQHandler(void)
{
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
  leTimerRunning = false;
}

void emberGpdLoadLeTimer(uint32_t timeInUs)
{
  uint32_t matchValue = (timeInUs * 32768) / 1000000;
  LETIMER_Enable(LETIMER0, false);
  LETIMER_RepeatSet(LETIMER0, 0, 1);
  LETIMER_CompareSet(LETIMER0, 0, matchValue);
  LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
  LETIMER_Enable(LETIMER0, true);
  leTimerRunning = true;
}
