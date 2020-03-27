/***************************************************************************//**
 * @file
 * @brief Driver for the SPV1840LR5H-B MEMS Microphone
 * @version 5.3.3
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
#include <common.h>
#include <math.h>
#include "em_device.h"
#include "em_cmu.h"
#include "em_prs.h"
#include "em_usart.h"
#include "em_ldma.h"
#include "dmadrv.h"

#include "thunderboard/util.h"
#include "thunderboard/board.h"
#include "native_gecko.h"

#include "mic.h"
#include "adpcm.h"

/**************************************************************************//**
* @addtogroup TBSense_BSP
* @{
******************************************************************************/

/***************************************************************************//**
 * @defgroup Mic_i2s MIC - Microphone Driver (I2S)
 * @{
 * @brief Driver for the Invensense ICS-43434 MEMS Microphone
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static unsigned int  dmaChannelLeft;       /**< The channel Id assigned by DMADRV                         */
static int16_t      *sampleBuffer0;         /**< Buffer used to store the microphone samples               */
static int16_t      *sampleBuffer1;         /**< Buffer used to store the microphone samples               */

/** @endcond */

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static bool dmaCompleteCallbackPingPong(unsigned int channel, unsigned int sequenceNo, void *userParam);

/** @endcond */

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
  uint32_t fs;

  /* Set fs value depending on selected sample rate */
  fs = setSampleRate(sr);

  USART_InitI2s_TypeDef usartInit = USART_INITI2S_DEFAULT;

  /* Enable microphone circuit and wait for it to settle properly */
  status = BOARD_micEnable(true);
  if ( status != BOARD_OK ) {
    return status;
  }

  /* Enable clocks */
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(MIC_USART_CLK, true);

  /* Setup GPIO pins */
  GPIO_PinModeSet(MIC_PORT_DATA, MIC_PIN_DATA, gpioModeInput, 0);
  GPIO_PinModeSet(MIC_PORT_CLK, MIC_PIN_CLK, gpioModePushPull, 0);
  GPIO_PinModeSet(MIC_PORT_WS, MIC_PIN_WS, gpioModePushPull, 0);

  /* Setup USART in I2S mode to get data from microphone */
  usartInit.sync.enable   = usartEnable;
  usartInit.sync.baudrate = fs * 64;
  usartInit.sync.autoTx   = true;
  usartInit.format        = usartI2sFormatW32D16;
  usartInit.dmaSplit      = true;
  usartInit.sync.databits = usartDatabits16;

  USART_InitI2s(MIC_USART, &usartInit);

  MIC_USART->ROUTELOC0 = (MIC_USART_LOC_DATA | MIC_USART_LOC_CLK | MIC_USART_LOC_WS);
  MIC_USART->ROUTEPEN  = (USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_CLKPEN | USART_ROUTEPEN_CSPEN);

  sampleBuffer0 = (int16_t *) buffer->buffer0;
  sampleBuffer1 = (int16_t *) buffer->buffer1;

  /* Setup DMA driver to move samples from USART to memory */
  DMADRV_Init();
  status = DMADRV_AllocateChannel(&dmaChannelLeft, NULL);
  if ( status != ECODE_EMDRV_DMADRV_OK ) {
    return status;
  }

  status = DMADRV_PeripheralMemoryPingPong(dmaChannelLeft,
                                           MIC_DMA_LEFT_SIGNAL,
                                           (void *) sampleBuffer0,
                                           (void *) sampleBuffer1,
                                           (void *) &(MIC_USART->RXDOUBLE),
                                           true,
                                           MIC_SAMPLE_BUFFER_SIZE,
                                           dmadrvDataSize2,
                                           dmaCompleteCallbackPingPong,
                                           NULL);

  return status;
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
  DMADRV_StopTransfer(dmaChannelLeft);

  /* Reset USART peripheral and disable IO pins */
  USART_Reset(MIC_USART);
  MIC_USART->I2SCTRL = 0;

  GPIO_PinModeSet(MIC_PORT_CLK, MIC_PIN_CLK, gpioModeDisabled, 0);
  GPIO_PinModeSet(MIC_PORT_DATA, MIC_PIN_DATA, gpioModeDisabled, 0);
  GPIO_PinModeSet(MIC_PORT_WS, MIC_PIN_WS, gpioModeDisabled, 0);

  /* Power down microphone */
  BOARD_micEnable(false);

  /* Free resources */
  DMADRV_FreeChannel(dmaChannelLeft);

  return;
}

/***************************************************************************//**
 * @brief
 *    Gets the sample buffer 0
 *
 * @return
 *    Returns a pointer to the sample buffer 0
 ******************************************************************************/
int16_t *MIC_getSampleBuffer0(void)
{
  return (int16_t *) sampleBuffer0;
}

/***************************************************************************//**
 * @brief
 *    Gets the sample buffer 1
 *
 * @return
 *    Returns a pointer to the sample buffer 1
 ******************************************************************************/
int16_t *MIC_getSampleBuffer1(void)
{
  return (int16_t *) sampleBuffer1;
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Called when the DMA complete interrupt fired.
      Funcion emits external signal depending on sequence number.
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
 *    Returns true
 ******************************************************************************/
static bool dmaCompleteCallbackPingPong(unsigned int channel, unsigned int sequenceNo, void *userParam)
{
  gecko_external_signal((sequenceNo % 2) ? EXT_SIGNAL_BUFFER0_READY : EXT_SIGNAL_BUFFER1_READY);
  return true;
}

/** @endcond */

/** @} {end defgroup Mic_i2s} */
/** @} {end addtogroup TBSense_BSP} */
