/***************************************************************************//**
 * @file
 * @brief Driver for the SPV1840LR5H-B MEMS Microphone
 * @version 5.2.1
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
#include "em_adc.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_letimer.h"
#include "dmadrv.h"

#include "thunderboard/util.h"
#include "thunderboard/board.h"
#include "native_gecko.h"
#include "mic.h"
#include <math.h>
#include "common.h"
#include "filter.h"
#include "adpcm.h"

/***************************************************************************//**
 * @defgroup Mic MIC - Microphone Driver
 * @{
 * @brief Driver for the Knowles SPV1840LR5H-B MEMS Microphone
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup Mic_Locals MEMS Microphone Local Variables
 * @{
 * @brief MEMS microphone local variables
 ******************************************************************************/

static unsigned int dmadrvChannelId; /**< The channel Id assigned by DMADRV                         */
static uint16_t *sampleBuffer0; /**< First buffer used to store the microphone samples               */
static uint16_t *sampleBuffer1; /**< Second buffer used to store the microphone samples               */
#ifndef MIC_CONFIG_USE_LETIMER
static uint32_t      cmuClkoutSel1;        /**< Variable to store the current clock output mode           */
#endif /* MIC_CONFIG_USE_LETIMER */

#define SAMPLE_RATE_8k       (8000)     /**< 8k sample rate */
#define SAMPLE_RATE_16k      (16000)        /**< 16k sample rate */

/** @} {end defgroup Mic_Locals} */

/** Timer Frequency used. */
#define TIMER_CLK_FREQ ((uint32)32768)
/** Convert msec to timer ticks. */
#define TIMER_MS_2_TIMERTICK(ms) ((TIMER_CLK_FREQ * ms) / 1000)

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @defgroup Mic_Functions MEMS Microphone Functions
 * @{
 * @brief MEMS microphone driver and support functions
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static bool dmaCompleteCallbackPingPong(unsigned int channel, unsigned int sequenceNo, void *userParam);
static void adcEnable(bool enable);

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Initializes MEMS microphone and sets up the DMA, ADC and clocking
 *
 * @param[in] fs
 *    The desired sample rate in Hz
 *
 * @param[in] buffer
 *    Pointer to the sample buffer to store the ADC data
 *
 * @param[in] len
 *    The size of the sample buffer
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t MIC_init(sample_rate_t sr, audio_data_buff_t *buffer)
{
  uint32_t status;
  uint32_t auxhfrcoFreq;
  uint32_t fs;

  /* Set fs value depending on selected sample rate */
  fs = setSampleRate(sr);

  ADC_Init_TypeDef adcInit = ADC_INIT_DEFAULT;
  ADC_InitScan_TypeDef adcInitScan = ADC_INITSCAN_DEFAULT;

  CMU_ClockEnable(cmuClock_ADC0, true);
  CMU_ClockEnable(cmuClock_PRS, true);
  CMU->ADCCTRL = CMU_ADCCTRL_ADC0CLKSEL_AUXHFRCO;

  auxhfrcoFreq = CMU_ClockFreqGet(cmuClock_AUX);

  /* Enable microphone circuit and wait for it to settle properly */
  BOARD_micEnable(true);

  /* Setup ADC */
  adcInit.em2ClockConfig = adcEm2ClockOnDemand;
  adcInit.timebase = ADC_TimebaseCalc(auxhfrcoFreq);
  adcInit.prescale = ADC_PrescaleCalc(MIC_ADC_CLOCK_FREQ, auxhfrcoFreq);
  ADC_Init(ADC0, &adcInit);

  /* Setup ADC channel */
  adcInitScan.reference = adcRef2V5;
  adcInitScan.prsEnable = true;
  adcInitScan.prsSel = MIC_CONFIG_ADC_PRSSEL;
  adcInitScan.scanDmaEm2Wu = true;

  if (getVobleConfig()->adcResolution == adc_12bit) {
    adcInitScan.acqTime = adcAcqTime16;
    adcInitScan.resolution = adcRes12Bit;
  } else {
    adcInitScan.acqTime = adcAcqTime8;
    adcInitScan.resolution = adcRes8Bit;
  }

  /* Add microphone scan channel */
  ADC_ScanInputClear(&adcInitScan);
  ADC_ScanSingleEndedInputAdd(&adcInitScan, adcScanInputGroup0, MIC_CONFIG_ADC_POSSEL);

  ADC_InitScan(ADC0, &adcInitScan);

  /* Setup PRS channel to trigger ADC */
  PRS_SourceAsyncSignalSet(MIC_CONFIG_PRS_CH, MIC_CONFIG_PRS_SOURCE, MIC_CONFIG_PRS_SIGNAL);

#if MIC_CONFIG_USE_LETIMER

  /* Setup LETIMER to trigger ADC in EM2 */
  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  uint16_t timerTop;
  uint32_t timerFreq;

  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
  CMU_ClockEnable(MIC_CONFIG_TIMER_CMU_CLK, true);
  timerFreq = CMU_ClockFreqGet(MIC_CONFIG_TIMER_CMU_CLK);

  letimerInit.comp0Top = true;
  letimerInit.enable = false;
  letimerInit.ufoa0 = letimerUFOAPulse;

  LETIMER_Init(MIC_CONFIG_TIMER, &letimerInit);
  LETIMER_RepeatSet(MIC_CONFIG_TIMER, 0, 1);

  timerTop = timerFreq / fs - 1;
  fs = timerFreq / (timerTop + 1);
  LETIMER_CompareSet(MIC_CONFIG_TIMER, 0, timerTop);

#else

  if ( fs == 1000 ) {
    /* Use ULFRCO which runs at 1 kHz */
    cmuClkoutSel1 = CMU_CTRL_CLKOUTSEL1_ULFRCOQ;
  } else {
    /* Use LFXO clock which runs at 32.768 kHz */
    cmuClkoutSel1 = CMU_CTRL_CLKOUTSEL1_LFXOQ;
    fs = 32768;
  }

#endif

  /* Setup DMA driver to move samples from ADC to memory */
  DMADRV_Init();
  status = DMADRV_AllocateChannel(&dmadrvChannelId, NULL);
  if ( status != ECODE_EMDRV_DMADRV_OK ) {
    return status;
  }

  sampleBuffer0 = buffer->buffer0;
  sampleBuffer1 = buffer->buffer1;

  status = DMADRV_PeripheralMemoryPingPong(dmadrvChannelId,
                                           dmadrvPeripheralSignal_ADC0_SCAN,
                                           (void *) sampleBuffer0,
                                           (void *) sampleBuffer1,
                                           (void *) &(ADC0->SCANDATA),
                                           true,
                                           MIC_SAMPLE_BUFFER_SIZE,
                                           dmadrvDataSize2,
                                           dmaCompleteCallbackPingPong,
                                           NULL);
  if ( status != ECODE_EMDRV_DMADRV_OK ) {
    return status;
  }

  adcEnable(true);

  return 0;
}

/***************************************************************************//**
 * @brief
 *    Powers down the MEMS microphone stops the ADC and frees up the DMA channel
 *
 * @return
 *    None
 ******************************************************************************/
void MIC_deInit(void)
{
  /* Stop sampling */
  adcEnable(false);

  /* Clear PRS channel configuration */
  PRS_SourceAsyncSignalSet(MIC_CONFIG_PRS_CH, 0, 0);

  /* Power down microphone */
  BOARD_micEnable(false);

  DMADRV_FreeChannel(dmadrvChannelId);

  return;
}

/***************************************************************************//**
 * @brief
 *    Gets the sample buffer 0
 *
 * @return
 *    Returns a pointer to the sample buffer 0
 ******************************************************************************/
uint16_t *MIC_getSampleBuffer0(void)
{
  return sampleBuffer0;
}

/***************************************************************************//**
 * @brief
 *    Gets the sample buffer 1
 *
 * @return
 *    Returns a pointer to the sample buffer 1
 ******************************************************************************/
uint16_t *MIC_getSampleBuffer1(void)
{
  return sampleBuffer1;
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Called when the DMA complete interrupt fired
 *
 * @param[in] channel
 *    DMA channel
 *
 * @param[in] sequenceNo
 *    Sequence number
 *
 * @param[in] userParam
 *    User parameters
 *
 * @return
 *    Returns false to stop transfers
 ******************************************************************************/
bool dmaCompleteCallbackPingPong(unsigned int channel, unsigned int sequenceNo, void *userParam)
{
  GPIO_PinOutSet(gpioPortF, 3);
  gecko_external_signal((sequenceNo % 2) ? EXT_SIGNAL_BUFFER0_READY : EXT_SIGNAL_BUFFER1_READY);
  return true;
}

/***************************************************************************//**
 * @brief
 *    Enables the ADC by enabling its clock
 *
 * @param[in] enable
 *    If true enables the ADC, if false disables.
 *
 * @return
 *    None
 ******************************************************************************/
static void adcEnable(bool enable)
{
#if MIC_CONFIG_USE_LETIMER
  /* Enable LETIMER to trigger ADC */
  LETIMER_Enable(MIC_CONFIG_TIMER, enable);
#else
  if ( enable ) {
    /* Set up CLKOUT1 to start sampling */
    CMU->CTRL |=  cmuClkoutSel1;
  } else {
    CMU->CTRL &= ~cmuClkoutSel1;
  }
#endif

  return;
}

/** @endcond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** @} {end defgroup Mic_Functions} */

/** @} {end defgroup Mic} */
