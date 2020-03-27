/***************************************************************************//**
 * @file
 * @brief Application specific overrides of weak functions defined as part of
 * the test application.
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

// ----------- NVM usage in Application ---------------------------------------
#if defined EMBER_AF_PLUGIN_PSSTORE
extern void store_init(void);
#endif

#define EMBER_GPD_NV_DATA_TAG 0xA9A1

static uint8_t *p;
static uint8_t length;
static uint8_t flags;

/** @brief This is called by framework to initialise the NVM system
 *
 */
void emberGpdAfPluginNvInitCallback(void)
{
#if defined EMBER_AF_PLUGIN_PSSTORE
  // Initialise the NV
  store_init();
#endif
}

/** @brief Called to the application to give a chance to load or store the GPD Context
 *.        in a non volatile context. Thsi can help the application to use any other
 *         non volatile storage.
 *
 * @param nvmData The pointer to the data that needs saving or retrieving to or from
 *                the non-volatile memory.
 * @param sizeOfNvmData The size of the data non-volatile data.
 * @param loadStore indication wether to load or store the context.
 * Ver.: always
 *
 * @return true if application handled it.
 */
bool emberGpdAfPluginNvSaveAndLoadCallback(EmberGpd_t * gpd,
                                           uint8_t * nvmData,
                                           uint8_t sizeOfNvmData,
                                           EmebrGpdNvLoadStore_t loadStore)
{
  if (loadStore == EMEBER_GPD_AF_CALLBACK_LOAD_GPD_FROM_NVM) {
  #if defined EMBER_AF_PLUGIN_PSSTORE
    if (0 != store_read(EMBER_GPD_NV_DATA_TAG, &flags, &length, &p)) {
      // Fresh chip , erase, create a storage with default setting.
      store_eraseall();
      // First write to the NVM shadow so that it updated with default ones
      emberGpdCopyToShadow(gpd);
      // Write the data to NVM
      store_write(EMBER_GPD_NV_DATA_TAG,
                  flags,
                  sizeOfNvmData,
                  (void *)nvmData);
    } else {
      memcpy(nvmData, p, sizeOfNvmData);
    }
  #endif
  } else if (loadStore == EMEBER_GPD_AF_CALLBACK_STORE_GPD_TO_NVM) {
  #if defined EMBER_AF_PLUGIN_PSSTORE
    store_write(EMBER_GPD_NV_DATA_TAG,
                flags,
                sizeOfNvmData,
                (void *)nvmData);
  #endif
  } else {
  }
  return false;
}
// ----------- END NVM usage in Application -----------------------------------

// ------------Sending an operational command ---------------------------------
static void sendToggle(EmberGpd_t * gpd)
{
  uint8_t command[] = { GP_CMD_TOGGLE };
  emberAfGpdfSend(EMBER_GPD_NWK_FC_FRAME_TYPE_DATA,
                  gpd,
                  command,
                  sizeof(command),
                  EMBER_AF_PLUGIN_APPS_CMD_RESEND_NUMBER);
}
// ------------Sending an operational command ---------------------------------

// -------------- Timer and button drivern application state machine ----------
#define BUTTON_PRESSED  1
#define BUTTON_RELEASED 0

// Type defines
typedef struct ButtonArray{
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} ButtonArray_t;

static const ButtonArray_t buttonArray[BSP_NO_OF_BUTTONS] = BSP_GPIO_BUTTONARRAY_INIT;
static bool buttonPressed = false;
static unsigned int buttonPin = 0;
static uint32_t button0LongPressTimerStartValue = 0;

static volatile uint32_t appTimeCount;

static void gpioCallback(uint8_t pin)
{
  // Check if any of the button pressed
  uint8_t buttonState = (GPIO_PinInGet(buttonArray[0].port, buttonArray[0].pin)) ? BUTTON_RELEASED : BUTTON_PRESSED;
  buttonState |= (GPIO_PinInGet(buttonArray[1].port, buttonArray[1].pin)) ? BUTTON_RELEASED : BUTTON_PRESSED;
  if (buttonState == BUTTON_PRESSED) {
    if (BSP_BUTTON0_PIN == pin) {
      // load a timer to count long press
      button0LongPressTimerStartValue = appTimeCount;
      buttonPressed = true;
      buttonPin = BSP_BUTTON0_PIN;
    } else if (BSP_BUTTON1_PIN == pin) {
      buttonPressed = true;
      buttonPin = BSP_BUTTON1_PIN;
    }
  } else {
    // Button released
    if (BSP_BUTTON0_PIN == pin) {
      button0LongPressTimerStartValue = 0;
    } else if (BSP_BUTTON1_PIN == pin) {
    }
  }
}

static void emberGpdCryoTimerInit(void)
{
  CMU_ClockEnable(cmuClock_CRYOTIMER, true);
  CRYOTIMER_Init_TypeDef  cryoInit = CRYOTIMER_INIT_DEFAULT;
  cryoInit.enable = false;
  cryoInit.osc = cryotimerOscULFRCO;
  cryoInit.presc = cryotimerPresc_1;
  cryoInit.period = cryotimerPeriod_256; //about 0.25s
  cryoInit.em4Wakeup = true;
  CRYOTIMER_Init(&cryoInit);

  NVIC_ClearPendingIRQ(CRYOTIMER_IRQn);
  NVIC_EnableIRQ(CRYOTIMER_IRQn);
  CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
  CRYOTIMER_IntDisable(CRYOTIMER_IEN_PERIOD);
}

// Low Power Mode with option to force EM4 mode.
static void gpdEnterLowPowerMode(bool forceEm4)
{
  EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
  if (forceEm4) {
    em4Init.retainLfxo = true;
    em4Init.pinRetentionMode = emuPinRetentionLatch;
    em4Init.em4State = emuEM4Shutoff;
    EMU_EM4Init(&em4Init);
    SLEEP_ForceSleepInEM4();
  } else {
    SLEEP_Sleep();
  }
}
// Application base loop timing with sleep
static void gpdSleepWithTimer(bool forceEm4)
{
  // Start the Timer
  CRYOTIMER_IntClear(CRYOTIMER_IEN_PERIOD);
  CRYOTIMER_IntEnable(CRYOTIMER_IEN_PERIOD);
  CRYOTIMER_Enable(true);
  //Sleep when the device is commissioned
  // TODO : pass the flag based on the GPD state
  gpdEnterLowPowerMode(forceEm4);
}

// Cryotimer Intterrupt
void CRYOTIMER_IRQHandler(void)
{
  CRYOTIMER_IntClear(CRYOTIMER_IF_PERIOD);
  appTimeCount++;
  __DSB();
}

/** @brief Called by framework from the application main enry to inform the user
 * as the first call to the main.
 *
 */
void emberGpdAfPluginMainCallback(EmberGpd_t * gpd)
{
  // Initialise timer for application state machine with sleep consideration
  emberGpdCryoTimerInit();

  // Enable the buttons on the board
  for (int i = 0; i < BSP_NO_OF_BUTTONS; i++) {
    GPIO_PinModeSet(buttonArray[i].port, buttonArray[i].pin, gpioModeInputPull, 1);
  }

  // Button Interrupt Config
  GPIOINT_Init();
  GPIOINT_CallbackRegister(buttonArray[0].pin, gpioCallback);
  GPIOINT_CallbackRegister(buttonArray[1].pin, gpioCallback);
  GPIO_IntConfig(buttonArray[0].port, buttonArray[0].pin, true, true, true);
  GPIO_IntConfig(buttonArray[1].port, buttonArray[1].pin, false, true, true);

  buttonPressed = false;
  buttonPin = 0;

  // Loop forever
  while (true) {
    if (buttonPressed) {
      if (buttonPin == BSP_BUTTON0_PIN) {
        emberGpdAfPluginCommission(gpd);
      } else if (buttonPin == BSP_BUTTON1_PIN) {
        sendToggle(gpd);
      }
      emberGpdStoreSecDataToNV(gpd);
      buttonPin = 0;
      buttonPressed = false;
    }
    uint32_t expiredTime = appTimeCount - button0LongPressTimerStartValue;
    if (button0LongPressTimerStartValue
        && expiredTime > GPD_APP_BUTTON_LONG_PRESS_TIME_IN_QS) {
      button0LongPressTimerStartValue = 0;
      emberGpdAfPluginDeCommission(gpd);
    }
    // Enter sleep with timer untill time to enter EM4
    bool enterEm4 = (((appTimeCount > GPD_APP_TIME_IN_QS_TO_ENTER_EM4) \
                      && (GPD_APP_TIME_IN_QS_TO_ENTER_EM4 != 0xFF)) ? true : false);
    gpdSleepWithTimer(enterEm4);
  }
}
