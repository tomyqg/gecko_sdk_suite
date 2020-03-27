/***************************************************************************//**
 * @file
 * @brief LEUART/DMA in EM2 example for EFM32GG_DK3750 starter kit
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
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_leuart.h"
#include "em_dma.h"
#include "dmactrl.h"

/** LEUART Rx/Tx Port/Pin Location */
#define LEUART_LOCATION    0
#define LEUART_TXPORT      gpioPortD            /* LEUART transmission port */
#define LEUART_TXPIN       4                    /* LEUART transmission pin */
#define LEUART_RXPORT      gpioPortD            /* LEUART reception port */
#define LEUART_RXPIN       5                    /* LEUART reception pin */

/***************************************************************************//**
 * @brief  Setting up LEUART
 ******************************************************************************/
void setupLeuart(void)
{
  /* Enable peripheral clocks */
  CMU_ClockEnable(cmuClock_HFPER, true);
  /* Configure GPIO pins */
  CMU_ClockEnable(cmuClock_GPIO, true);
  /* To avoid false start, configure output as high */
  GPIO_PinModeSet(LEUART_TXPORT, LEUART_TXPIN, gpioModePushPull, 1);
  GPIO_PinModeSet(LEUART_RXPORT, LEUART_RXPIN, gpioModeInput, 0);

  LEUART_Init_TypeDef init = LEUART_INIT_DEFAULT;

  /* Enable CORE LE clock in order to access LE modules */
  CMU_ClockEnable(cmuClock_CORELE, true);

  /* Select LFXO for LEUARTs (and wait for it to stabilize) */
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
  CMU_ClockEnable(cmuClock_LEUART0, true);

  /* Do not prescale clock */
  CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1);

  /* Configure LEUART */
  init.enable = leuartDisable;

  LEUART_Init(LEUART0, &init);

  /* Enable pins at default location */
  LEUART0->ROUTE = LEUART_ROUTE_RXPEN | LEUART_ROUTE_TXPEN | LEUART_LOCATION;

  /* Set RXDMAWU to wake up the DMA controller in EM2 */
  LEUART_RxDmaInEM2Enable(LEUART0, true);

  /* Finally enable it */
  LEUART_Enable(LEUART0, leuartEnable);
}

/***************************************************************************//**
 * @brief  Setup DMA
 *
 * @details
 *   This function initializes DMA controller.
 *   It configures the DMA channel to be used for LEUART0 transmit
 *   and receive. The primary descriptor for channel0 is configured for
 *   a single data byte transfer. For continous data reception and transmission
 *   using LEUART DMA loopmode is enabled for channel0.
 *   In the end DMA transfer cycle is configured to basicMode where
 *   the channel source address, destination address, and
 *   the transfercount per dma cycle have been specified.
 *
 ******************************************************************************/
void setupDma(void)
{
  /* DMA configuration structs */
  DMA_Init_TypeDef       dmaInit;
  DMA_CfgChannel_TypeDef channelCfg;
  DMA_CfgDescr_TypeDef   descrCfg;
  DMA_CfgLoop_TypeDef    loopCfg;

  /* Initializing the DMA */
  dmaInit.hprot        = 0;
  dmaInit.controlBlock = dmaControlBlock;
  DMA_Init(&dmaInit);

  /* Setting up channel */
  channelCfg.highPri   = false; /* Can't use with peripherals */
  channelCfg.enableInt = false; /* Interrupt not needed in loop transfer mode */

  /* Configure channel 0 */
  /*Setting up DMA transfer trigger request*/
  channelCfg.select = DMAREQ_LEUART0_RXDATAV;
  channelCfg.cb     = NULL;
  DMA_CfgChannel(0, &channelCfg);

  /* Setting up channel descriptor */
  /* Destination is LEUART_Tx register and doesn't move */
  descrCfg.dstInc = dmaDataIncNone;

  /* Source is LEUART_RX register and transfers 8 bits each time */
  descrCfg.srcInc = dmaDataIncNone;
  descrCfg.size   = dmaDataSize1;

  /* We have time to arbitrate again for each sample */
  descrCfg.arbRate = dmaArbitrate1;
  descrCfg.hprot   = 0;

  /* Configure primary descriptor  */
  DMA_CfgDescr(0, true, &descrCfg);

  /* Configure loop transfer mode */
  loopCfg.enable = true;
  loopCfg.nMinus1 = 0;  /* Single transfer per DMA cycle*/
  DMA_CfgLoop(0, &loopCfg);

  /* Activate basic dma cycle using channel0 */
  DMA_ActivateBasic(0,
                    true,
                    false,
                    (void *)&LEUART0->TXDATA,
                    (void *)&LEUART0->RXDATA,
                    0);
}

/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/
int main(void)
{
  /* Chip errata */
  CHIP_Init();

  /* Initialize LEUART */
  setupLeuart();

  /* Setup DMA */
  setupDma();

  while (1) {
    /* On every wakeup enter EM2 again */
    EMU_EnterEM2(true);
  }
}