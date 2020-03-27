/***************************************************************************//**
 * @file
 * @brief Menu Functions of the Range Test Software Example.
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rail.h"

#include "bsp.h"
#include "app_common.h"

#include "pushButton.h"
#include "graphics.h"
#include "menu.h"
#include "seq.h"

#include "rangeTest.h"
#include "gatt_db.h"
#include "bluetooth.h"

// ----------------------------------------------------------------------------
// Defines and Macros
#define ROUND_UP(VAL, STEP) (((VAL + (STEP / 2)) / STEP) * STEP)

// ----------------------------------------------------------------------------
// Function prototypes
bool menuShowInfo(bool);
bool menuSetRFMode(bool);
bool menuSetRFFrequency(bool);
bool menuSetRFPA(bool);
bool menuSetPhy(bool init);
bool menuSetChannel(bool);
bool menuSetPktsNum(bool);
bool menuSetPktsLen(bool);
bool menuSetReTransmit(bool);
bool menuSetIDDest(bool);
bool menuSetIDSrc(bool);
bool menuSetMAWindow(bool);
bool menuSetLogMode(bool);
bool menuRunDemo(bool);

// Menu items
uint8_t menuShowInfoDisplay(char *buff[]);
uint8_t menuSetRFModeDisplay(char *buff[]);
uint8_t menuSetRFFrequencyDisplay(char *buff[]);
uint8_t menuSetRFPADisplay(char *buff[]);
uint8_t menuSetPhyDisplay(char *buff[]);
uint8_t menuSetChannelDisplay(char *buff[]);
uint8_t menuSetPktsNumDisplay(char *buff[]);
uint8_t menuSetPktsLenDisplay(char *buff[]);
uint8_t menuSetReTransmitDisplay(char *buff[]);
uint8_t menuSetIDDestDisplay(char *buff[]);
uint8_t menuSetIDSrcDisplay(char *buff[]);
uint8_t menuSetMAWindowDisplay(char *buff[]);
uint8_t menuSetLogModeDisplay(char *buff[]);
uint8_t menuRunDemoDisplay(char *buff[]);

char *  menuPrint(char* item, char* value);

static void resetPayloadLength(void);
static void handlePayloadLengthOverlfow(void);

// ----------------------------------------------------------------------------

// ------------------------------------
// Menu related variables
static uint8_t menuIdx;
static uint8_t menuDispStart;
char scrBuffer[21u];
static MenuItemChangeList menuItemChanges = { { { eAppMenu_ShowInfo, 0  } } };
// ------------------------------------

/**************************************************************************//**
 * @brief Array of menu state structure instances.
 * @note  First item must be the first element too.
 *        Since some menu items now depend on others, the order of the menu
 *        items do matter! For example: PHY has to precede RFPA, RFFrequency,
 *        Channel and PktsLen items
 *****************************************************************************/
/// Array of menu state structure instances.
static const menuItem_t menuItem[] = {
  { eAppMenu_ShowInfo, eAppMenu_SetRFMode, \
    menuShowInfo, menuShowInfoDisplay, NO_HIDE, 0 },
  { eAppMenu_SetRFMode, eAppMenu_SetPhy, \
    menuSetRFMode, menuSetRFModeDisplay, NO_HIDE, 1 },
  { eAppMenu_SetPhy, eAppMenu_SetRFPA, \
    menuSetPhy, menuSetPhyDisplay, RX_HIDE, 1 },
  { eAppMenu_SetRFPA, eAppMenu_SetRFFrequency, \
    menuSetRFPA, menuSetRFPADisplay, RX_HIDE, 1 },
  { eAppMenu_SetRFFrequency, eAppMenu_SetChannel, \
    menuSetRFFrequency, menuSetRFFrequencyDisplay, NO_HIDE, 1 },
  { eAppMenu_SetChannel, eAppMenu_SetPktsLen, \
    menuSetChannel, menuSetChannelDisplay, RX_HIDE, 1 },
  { eAppMenu_SetPktsLen, eAppMenu_SetPktsNum, \
    menuSetPktsLen, menuSetPktsLenDisplay, NO_HIDE, 1 },
  { eAppMenu_SetPktsNum, eAppMenu_SetIDDest, \
    menuSetPktsNum, menuSetPktsNumDisplay, RX_HIDE, 1 },
/*  {eAppMenu_SetReTransmit  , eAppMenu_SetIDDest        ,            \
        menuSetReTransmit    , menuSetReTransmitDisplay  , RX_HIDE, 1 }, */
  { eAppMenu_SetIDDest, eAppMenu_SetIDSrc, \
    menuSetIDDest, menuSetIDDestDisplay, NO_HIDE, 1 },
  { eAppMenu_SetIDSrc, eAppMenu_SetMAWindow, \
    menuSetIDSrc, menuSetIDSrcDisplay, NO_HIDE, 1 },
  { eAppMenu_SetMAWindow, eAppMenu_SetLogMode, \
    menuSetMAWindow, menuSetMAWindowDisplay, TX_HIDE, 1 },
  { eAppMenu_SetLogMode, eAppMenu_RunDemo, \
    menuSetLogMode, menuSetLogModeDisplay, NO_HIDE, 1 },
  { eAppMenu_RunDemo, 0, \
    menuRunDemo, menuRunDemoDisplay, NO_HIDE, 0 },
};

/**************************************************************************//**
 * @brief  Trigger function that shows the info screen upon a key press
 *         when the Show Information menu item is selected.
 *
 * @param  init: Request resetting.
 *
 * @return Always true.
 *****************************************************************************/
bool menuShowInfo(bool init)
{
  // Nothing to do with init
  if (!init) {
    seqSet(SEQ_INIT);
  }
  return true;
}

/**************************************************************************//**
 * @brief  This function selects between available radio modes.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetRFMode(bool init)
{
  rangeTest.radioMode++;
  if ((rangeTest.radioMode >= NUMOF_RADIO_MODES) || (init)) {
    rangeTest.radioMode = RADIO_MODE_RX;  // 0u
  }
  charInd bci = {
    .id = gattdb_radioMode,
    .value = rangeTest.radioMode
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function selects (cycles) between destination IDs.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetIDDest(bool init)
{
  rangeTest.destID++;
  if ((rangeTest.destID > 32u) || (init)) {
    rangeTest.destID = 0u;
  }
  charInd bci = {
    .id = gattdb_destID,
    .value = rangeTest.destID
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function selects (cycles) between source IDs.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetIDSrc(bool init)
{
  rangeTest.srcID++;
  if ((rangeTest.srcID > 32u) || (init)) {
    rangeTest.srcID = 0u;
  }
  charInd bci = {
    .id = gattdb_srcID,
    .value = rangeTest.srcID
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function selects (cycles) between trasmitted packet count
 *         options.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetPktsNum(bool init)
{
  static uint16_t pktsNum[8u] =
  { 500u, 1000u, 2500u, 5000u, 10000u, 25000u, 50000u, RANGETEST_TX_REPEAT };
  static uint8_t i = 1u;

  if (!init) {
    i++;
    if (i >= sizeof(pktsNum) / sizeof(*pktsNum)) {
      i = 0u;
    }
  }
  rangeTest.pktsReq = pktsNum[i];
  charInd bci = {
    .id = gattdb_pktsReq,
    .value = rangeTest.pktsReq
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function sets payload length based on the current PHY.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetPktsLen(bool init)
{
  if (init) {
    resetPayloadLength();
  } else {
    rangeTest.payloadLength++;
    handlePayloadLengthOverlfow();
  }
  charInd bci = {
    .id = gattdb_payload,
    .value = rangeTest.payloadLength
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function selects between number of retransmit counts.
 * @note   This function is not used.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetReTransmit(bool init)
{
  rangeTest.retransmitCnt++;
  if ((rangeTest.retransmitCnt > 10u) || (init)) {
    rangeTest.retransmitCnt = 0u;
  }
  return true;
}

/**************************************************************************//**
 * @brief  This function selects (cycles) between moving average window
 *         sizes.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetMAWindow(bool init)
{
  if ((rangeTest.maSize == 128u) || (init)) {
    rangeTest.maSize = 32u;
  } else {
    rangeTest.maSize <<= 1u;
  }
  charInd bci = {
    .id = gattdb_maSize,
    .value = rangeTest.maSize
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function selects if logging is enabled or not.
 *
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetLogMode(bool init)
{
  rangeTest.log = !rangeTest.log;

  if (init) {
    rangeTest.log = false;
  }
  charInd bci = {
    .id = gattdb_log,
    .value = rangeTest.log
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function executes the Range Test in the selected mode.
 *
 * @param  init: User (re)init request.
 *
 * @return False when init requested, true if not.
 *****************************************************************************/
bool menuRunDemo(bool init)
{
  // Nothing to init here
  if (init) {
    return false;
  }
  // (Re-)Init
  rangeTestInit();

  GRAPHICS_RSSIClear();
  if (RADIO_MODE_RX == rangeTest.radioMode) {
    // Bluetooth: close opened connection or stop advertising
    bluetoothDeactivate();
    // Start test timer
    proprietaryUpdateTestTimer(rangeTest.pktsReq);
  }
  // Start Demo
  seqSet(SEQ_TRX);

  if (RADIO_MODE_RX == rangeTest.radioMode) {
//    rangeTestRx();
    rangeTest.isRunning = true;
  }

  return true;
}

/**************************************************************************//**
 * @brief  This function would be used to select from radio frequencies.
 * @note   This function is not implemented (read-only).
 *
 * @param  init: User (re)init request.
 *
 * @return Always false.
 *****************************************************************************/
bool menuSetRFFrequency(bool init)
{
  return false;
}

/**************************************************************************//**
 * @brief  This function sets the output power.
 *
 * @param  init: User (re)init request.
 *
 * @return True if value was changed, false if not.
 *****************************************************************************/
bool menuSetRFPA(bool init)
{
  if (init) {
    // Init the rangetest power field to the current power level
    // to sync demanded and current value.
    rangeTest.txPower = RAIL_GetTxPowerDbm(rangeTestGetRailHandle());
    if ((rangeTest.txPower % TX_POWER_INC) != 0 ) {
      rangeTest.txPower = ROUND_UP(rangeTest.txPower, TX_POWER_INC);
      RAIL_SetTxPowerDbm(rangeTestGetRailHandle(), rangeTest.txPower);
    }
  } else {
    rangeTest.txPower += TX_POWER_INC;
    // Limit and cycle requested power.
    if ( rangeTest.txPower > TX_POWER_MAX) {
      rangeTest.txPower = TX_POWER_MIN;
    }
    // Return value ignored here
    RAIL_SetTxPowerDbm(rangeTestGetRailHandle(), rangeTest.txPower);
  }
  charInd bci = {
    .id = gattdb_txPower,
    .value = rangeTest.txPower
  };
  bluetoothEnqueueIndication(&bci);
  return true;
}

/**************************************************************************//**
 * @brief  This function sets the physical layer.
 *
 * @param  init: User (re)init request.
 *
 * @return True if value was changed, false if not.
 *****************************************************************************/
bool menuSetPhy(bool init)
{
  uint8_t i;

  if (init) {
    rangeTest.phy = (uint8_t)PHY_DEFAULT;
  } else {
    i = rangeTest.phy;
    // try to find for next supported PHY
    do {
      i++;
      if (i == COUNTOF(rangeTestPhys)) {
        i = 0;
      }
    } while (i != rangeTest.phy && !rangeTestPhys[i].isSupported);
    rangeTest.phy = i;
  }
  // Update payload::ValidRange based on the active PHY
  bluetoothUpdatePayloadValidRange();
  charInd bci = {
    .id = gattdb_phy,
    .value = rangeTest.phy
  };
  bluetoothEnqueueIndication(&bci);

  // Initialise all dependent menu items.
  menuSetRFFrequency(true);
  menuSetRFPA(true);
  menuSetChannel(true);
  menuSetPktsLen(true);
  return true;
}

/**************************************************************************//**
 * @brief  This function would select (cycles) which channel to use for TX/RX.
 * @note   This function is not implemented (read-only).
 * @param  init: User (re)init request.
 *
 * @return Always true.
 *****************************************************************************/
bool menuSetChannel(bool init)
{
  if (PHY_IEEE_802154 == rangeTest.phy) {
    rangeTest.channel = IEEE802154_CHANNEL;
  } else if (PHY_BLE_1mbps == rangeTest.phy
             || PHY_BLE_2mbps == rangeTest.phy
             || PHY_BLE_Coded_125kbps == rangeTest.phy
             || PHY_BLE_Coded_500kbps == rangeTest.phy) {
    rangeTest.channel = BLE_PHYSICAL_CH;
  }
  charInd bci = {
    .id = gattdb_channel,
    .value = rangeTest.channel
  };
  bluetoothEnqueueIndication(&bci);
  return false;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         to show some information about the Range Test Example.
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu-
 *****************************************************************************/
uint8_t menuShowInfoDisplay(char *buff[])
{
  if (buff) {
    //static /*const?*/ char *txt = "Show Information";
    *buff = "Show Information";
  }
  return ICON_SHOW;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         to set the Range Test mode (RX or TX role).
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetRFModeDisplay(char *buff[])
{
  if (buff) {
    *buff = menuPrint("Mode:",
                      (rangeTest.radioMode == RADIO_MODE_TX) ? ("TX") \
                      : ((rangeTest.radioMode == RADIO_MODE_RX) ? ("RX") : ("TRX")));
  }
  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         to shows the frequency.
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu
 *         ICON_NONE means that this menu item is read-only.
 *****************************************************************************/
uint8_t menuSetRFFrequencyDisplay(char *buff[])
{
  char pVal[11u];

  if (buff) {
    // TODO: replace hard-coded value
    sprintf(pVal, "%uMHz", 2450);
    *buff = menuPrint("Frequency:", pVal);
  }
  return ICON_NONE;
}

uint8_t menuSetRFPADisplay(char *buff[])
{
  // Enough to hold a sign and two digits,
  // one decimal point and one decimal digist and dBm */
  char pVal[17u];
  // Temporary power variable for actual RAIL output power.
  int16_t power;
  // Temporary power variable for the requested power.
  int16_t reqpower;

  if (buff) {
    power = (int16_t) RAIL_GetTxPowerDbm(rangeTestGetRailHandle());
    reqpower = rangeTest.txPower;
    snprintf(pVal,
             17u,
             "%s%u.%u/%s%u.%udBm",
             ((reqpower >= 0) ? "+" : "-"),
             (uint8_t)(((reqpower >= 0) ? (reqpower) : (-reqpower)) / 10),
             (uint8_t)(((reqpower >= 0) ? (reqpower) : (-reqpower)) % 10),
             ((power >= 0) ? "+" : "-"),
             (uint8_t)(((power >= 0) ? (power) : (-power)) / 10),
             (uint8_t)(((power >= 0) ? (power) : (-power)) % 10));
    *buff = menuPrint("Power:", pVal);
  }
  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         that shows/sets the physical layer the radio uses for transmission.
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in the menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetPhyDisplay(char *buff[])
{
  char pVal[24u];

  if (buff) {
    sprintf(pVal, "%s", rangeTestPhys[rangeTest.phy].name);
    *buff = menuPrint("Phy: ", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         that sets the channel.
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetChannelDisplay(char *buff[])
{
  char pVal[6u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.channel);
    *buff = menuPrint("Channel number:", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         that shows/sets the number of packets to be transmitted.
 *
 * @param  buff[]: Buffer that is to be filled out with the string
 *                 that will be printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetPktsNumDisplay(char *buff[])
{
  char pVal[6u];

  if (buff) {
    if (rangeTest.pktsReq != RANGETEST_TX_REPEAT) {
      sprintf(pVal, "%u", rangeTest.pktsReq);
    }
    *buff = menuPrint("Packet Count:",
                      (rangeTest.pktsReq != RANGETEST_TX_REPEAT) ? (pVal) : ("Repeat"));
  }
  ;

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         that shows/sets the channel.
 *
 * @param  Buffer that is to be filled out with the string that will be printed
 *         in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetPktsLenDisplay(char *buff[])
{
  char pVal[4u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.payloadLength);
    *buff = menuPrint("Payload length:", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         that shows the re-transmission count.
 * @note   This function is not used.
 *
 * @param  buff[]: Buffer that is to be filled out with the string that will be
 *                 printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetReTransmitDisplay(char *buff[])
{
  char pVal[4u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.retransmitCnt);
    *buff = menuPrint("Re-Transmit:",
                      (!rangeTest.retransmitCnt) ? ("Off") : (pVal));
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu to sets/shows
 *         the destination ID.
 *
 * @param  buff[]: Buffer that is to be filled out with the string that will be
 *                 printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetIDDestDisplay(char *buff[])
{
  char pVal[4u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.destID);
    *buff = menuPrint("Remote ID:", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief      This function sets the printed menu text for the menu
 *             to sets/shows the source ID.
 *
 * @param[out] buff[]: Buffer that is to be filled out with the string
 *                           that will be printed in th menu
 *
 * @return     Icon type to be used by the caller for this menu
 *****************************************************************************/
uint8_t menuSetIDSrcDisplay(char *buff[])
{
  char pVal[4u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.srcID);
    *buff = menuPrint("Self ID:", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu to sets/shows
 *         the size of the moving average window.
 *
 * @param  buff[]: Buffer that is to be filled out with the string that will be
 *                 printed in th menu
 *
 * @return Icon type to be used by the caller for this menu
 *****************************************************************************/
uint8_t menuSetMAWindowDisplay(char *buff[])
{
  char pVal[4u];

  if (buff) {
    sprintf(pVal, "%u", rangeTest.maSize);
    *buff = menuPrint("MA Window size:", pVal);
  }

  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu
 *         to shows/sets .
 *
 * @param  buff[]: Buffer that is to be filled out with the string that will be
 *                 printed in th menu.
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuSetLogModeDisplay(char *buff[])
{
  if (buff) {
    *buff = menuPrint("UART log enable:",
                      (rangeTest.log) ? ("Yes") : ("No"));
  }
  return ICON_PLUS;
}

/**************************************************************************//**
 * @brief  This function sets the printed menu text for the menu for executing
 *         the Range Test.
 *
 * @param  buff[]: Buffer that is to be filled out with the string that will be
 *                 printed in th menu
 *
 * @return Icon type to be used by the caller for this menu.
 *****************************************************************************/
uint8_t menuRunDemoDisplay(char *buff[])
{
  if (buff) {
    *buff = "Start Range Test";
  }
  return ICON_START;
}

/**************************************************************************//**
 * @brief  Checks isf the given item is the first element.
 *
 * @param  item: Menu item which is to be cheked if first or not.
 *
 * @return True if this is the first item.
 *****************************************************************************/
bool menuIsFirstItem(uint8_t item)
{
  return (menuItem[0u].menuItem == item);
}

/**************************************************************************//**
 * @brief  Checks isf the given item is the last element.
 *
 * @param  index: Menu item which is to be cheked if last or not.
 *
 * @return True if this is the last item.
 *****************************************************************************/
bool menuIsLastItem(uint8_t index)
{
  if (menuItem[index].menuNextItem == 0) {
    return true;
  }
  return false;
}

/**************************************************************************//**
 * @brief  Handles the list display for menu states that have string values.
 *         Extra aligment is done to pad out the value to  given
 *
 * @param  item: Item string.
 * @param  value: Value string.
 * @param  col: cClumn to align the value to.
 *
 * @return Pointer to (global) string buffer.
 *****************************************************************************/
char* menuPrintAligned(char* item, char* value, uint8_t col)
{
  uint8_t ii = 0u;

  while (*item) {
    scrBuffer[ii] = *item;
    item++;
    ii++;
  }

  while (ii < (col - strlen(value))) {
    scrBuffer[ii] = ' ';
    ii++;
  }

  while (*value) {
    scrBuffer[ii] = *value;
    value++;
    ii++;
  }
  scrBuffer[ii] = 0u;

  return scrBuffer;
}

/**************************************************************************//**
 * @brief  Handles the list display for menu states that have string values.
 *
 * @param  item: Item string.
 * @param  value: Value string.
 *
 * @return Pointer to (global) string buffer.
 *****************************************************************************/
char* menuPrint(char* item, char* value)
{
  return (menuPrintAligned(item, value, 20u));
}

/**************************************************************************//**
 * @brief  Initilization of the menu on the display by going through the actions
 *         of all menu items.
 *
 * @param  None.
 *
 * @return None.
 *****************************************************************************/
void menuInit()
{
  // Go through menu item handlers and let them init themselves

  uint8_t i = 0u;

  while (true) {
    menuItem[i].action(true);

    if (menuIsLastItem(i)) {
      break;
    }
    i++;
  }
}

/**************************************************************************//**
 * @brief  Handles the list display for menu states that have string values.
 *
 * @param  item: Item string.
 *
 * @return Pointer to (global) string buffer.
 *****************************************************************************/
char *menuItemStr(uint8_t item)
{
  char *ret;

  menuItem[item].repr(&ret);
  return ret;
}

/**************************************************************************//**
 * @brief  Triggers the display function of the selected item but it just
 *         determines the type of action icon used for this menu.
 *
 * @param  item: The current menu item index.
 *
 * @return The icon of the chosen item.
 *****************************************************************************/
uint8_t menuItemIcon(uint8_t item)
{
  return menuItem[item].repr(NULL);
}

/**************************************************************************//**
 * @brief  Function that checks if the menu item of a given index is visible
 *         in the menu on the display or not.
 *
 * @param  item: index of the menu item.
 * @param  mode: enum value (menuHideInfo_e) that determines in which mode
 *         should the menu item be visible.
 *
 * @return Pointer to (global) string buffer.
 *****************************************************************************/
bool menuItemHidden(uint8_t item, uint8_t mode)
{
  return (menuItem[item].menuHidden == mode);
}

/**************************************************************************//**
 * @brief  Returns the index of the currently selected menu item.
 *
 * @param  None.
 *
 * @return The index of the currently selected menu item.
 *****************************************************************************/
uint8_t menuGetActIdx()
{
  return menuIdx;
}

/**************************************************************************//**
 * @brief  Returns the menu index that is the top visible menu line.
 *
 * @param  None.
 *
 * @return Pointer to (global) string buffer.
 *****************************************************************************/
uint8_t menuGetDispStartIdx()
{
  return menuDispStart;
}

/**************************************************************************//**
 * @brief  Input handler for the init screen.
 *
 * @param  pInp: State of the pushbuttons .
 *
 * @return Returns true if the button status demands a display update.
 *****************************************************************************/
bool initHandleInput(pbState_t *pInp)
{
  static uint16_t delay = 500u;

  while ((PB_SHORT != pInp->pb[PB0]) && (PB_SHORT != pInp->pb[PB1])) {
    // Timeout only valid for first time init screen
    if (delay) {
      --delay;
      if (!delay) {
        break;
      }
    }
    return false;
  }
  seqSet(SEQ_MENU);

  return true;
}

/**************************************************************************//**
 * @brief  This function handles the button presses and move the menu menu and
 *         menu highlight or carry out the operation associated with the menu.
 *
 * @param  pInp: Pointer to the pushbutton states.
 *
 * @return True if buttin press was registered.
 *****************************************************************************/
bool menuHandleInput(pbState_t *pInp)
{
  bool retVal = false;
  MenuItemChange mic;

  if (pInp->pb[PB1] > PB_RELEASED) {
    // Increase menu index or wrap around
    if (menuIsLastItem(menuIdx)) {
      menuIdx = 0u;
      menuDispStart = 0u;
    } else {
      menuIdx++;
      // Scroll down if cursor past halfway and bottom option not yet visible
      if ( (menuIdx >= (GRAPHICS_MENU_DISP_SIZE / 2u))
           && ((COUNTOF(menuItem) - menuIdx) > (GRAPHICS_MENU_DISP_SIZE / 2u)) ) {
        menuDispStart++;
      }
    }

    retVal = true;
  }

  if (pInp->pb[PB0] > PB_RELEASED) {
    if (pInp->pb[PB0] > PB_SHORT && (!menuItem[menuIdx].menuLong)) {
      return false;
    }
    // Call trigger handler
    retVal = menuItem[menuIdx].action(false);
  }

  // Menu item changed over Bluetooth
  // TODO: do multiple in batch
  if (!menuItemChangeDequeue(&mic)) {
    retVal = menuItemChangeApply(&mic);
  }

  return retVal;
}

/**************************************************************************//**
 * @brief  Handles button presses in the TRX mode.
 *
 * @param  pInp: Pointer to the pushbutton states.
 *
 * @return True if one button was pushed.
 *****************************************************************************/
bool runTRXHandleInput(pbState_t *pInp)
{
  bool retVal = false;
  MenuItemChange mic;

  if (pInp->pb[PB0] > PB_RELEASED) {
    if (RADIO_MODE_TX == rangeTest.radioMode) {
      rangeTest.isRunning = !rangeTest.isRunning;
      charInd bci = {
        .id = gattdb_isRunning,
        .value = rangeTest.isRunning
      };
      bluetoothEnqueueIndication(&bci);
      // If just started reset counters
      if (rangeTest.isRunning) {
        rangeTest.pktsSent = 0u;
        proprietaryStartPeriodicTx();
        bluetoothPktsSentIndications(true);
        // Stopped
      } else {
        proprietaryStopPeriodicTx();
        bluetoothPktsSentIndications(false);
      }
      return true;
    }
  }

  if (pInp->pb[PB1] == PB_SHORT) {
    // Idle Radio
    RAIL_Idle(rangeTestGetRailHandle(), RAIL_IDLE, true);
    if (RADIO_MODE_RX == rangeTest.radioMode) {
      rangeTest.isRunning = !rangeTest.isRunning;
      // Bluetooth: start connectable advertising
      bluetoothActivate();
      // Stop test timer
      proprietaryUpdateTestTimer(0);
    } else if (RADIO_MODE_TX == rangeTest.radioMode) {
      proprietaryStopPeriodicTx();
      bluetoothPktsSentIndications(false);
    }
    seqSet(SEQ_MENU);
    return true;
  }

  // Menu item changed over Bluetooth
  // TODO: do multiple in batch
  if (!menuItemChangeDequeue(&mic)) {
    retVal = menuItemChangeApply(&mic);
  }
  return retVal;
}

bool menuItemChangeEnqueue(MenuItemChange* mic)
{
  if (menuItemChanges.size >= MENU_ITEM_CHANGE_LIST_SIZE) {
    return true;
  }
  menuItemChanges.items[menuItemChanges.head].index = mic->index;
  menuItemChanges.items[menuItemChanges.head].value = mic->value;
  menuItemChanges.head = (menuItemChanges.head < MENU_ITEM_CHANGE_LIST_SIZE - 1) \
                         ? (menuItemChanges.head + 1) : 0;
  menuItemChanges.size++;
  return false;
}

bool menuItemChangeDequeue(MenuItemChange* mic)
{
  if (menuItemChanges.size <= 0) {
    return true;
  }
  mic->index = menuItemChanges.items[menuItemChanges.tail].index;
  mic->value = menuItemChanges.items[menuItemChanges.tail].value;
  menuItemChanges.tail = (menuItemChanges.tail < MENU_ITEM_CHANGE_LIST_SIZE - 1) \
                         ? (menuItemChanges.tail + 1) : 0;
  menuItemChanges.size--;
  return false;
}

bool menuItemChangeApply(MenuItemChange* mic)
{
  bool ret = true;
  // Update cursor position
  menuIdx = mic->index;
  // Scroll to top.
  if (menuIdx < (GRAPHICS_MENU_DISP_SIZE / 2u)) {
    menuDispStart = 0;
    // Scroll down if cursor past halfway and bottom option not yet visible
  } else if ( (menuIdx >= (GRAPHICS_MENU_DISP_SIZE / 2u))
              && (menuIdx < (COUNTOF(menuItem) - (GRAPHICS_MENU_DISP_SIZE / 2u))) ) {
    menuDispStart = menuIdx + 1 - GRAPHICS_MENU_DISP_SIZE / 2u;
    // Scroll to bottom.
  } else if (menuIdx >= (COUNTOF(menuItem) - (GRAPHICS_MENU_DISP_SIZE / 2u))) {
    menuDispStart = COUNTOF(menuItem) - GRAPHICS_MENU_DISP_SIZE;
  }
  switch (mic->index) {
    case eAppMenu_SetPktsNum:
      rangeTest.pktsReq = mic->value;
      break;

    case eAppMenu_SetPhy:
      ret = false;
      for (uint8_t i = 0; i < COUNTOF(rangeTestPhys); i++) {
        // search supported PHYs for match
        if (rangeTestPhys[i].isSupported
            && rangeTestPhys[i].phy == (RangeTestPhy)mic->value) {
          rangeTest.phy = mic->value;
          // Update payload::ValidRange based on the active PHY
          bluetoothUpdatePayloadValidRange();
          // Initialise all dependent menu items.
          menuSetRFFrequency(true);
          menuSetRFPA(true);
          menuSetChannel(true);
          menuSetPktsLen(true);
          ret = true;
          break;
        }
      }
      break;

    case eAppMenu_SetRFMode:
      rangeTest.radioMode = (sRadioModes_e)mic->value;
      break;

    case eAppMenu_SetRFPA:
      rangeTest.txPower = (int16_t)mic->value;
      RAIL_SetTxPowerDbm(rangeTestGetRailHandle(), rangeTest.txPower);
      break;

    case eAppMenu_SetIDDest:
      rangeTest.destID = (uint8_t)mic->value;
      break;

    case eAppMenu_SetIDSrc:
      rangeTest.srcID = (uint8_t)mic->value;
      break;

    case eAppMenu_SetPktsLen:
      rangeTest.payloadLength = (uint8_t)mic->value;
      break;

    case eAppMenu_SetMAWindow:
      rangeTest.maSize = (uint8_t)mic->value;
      break;

    case eAppMenu_SetLogMode:
      rangeTest.log = (bool)mic->value;
      break;

    case eAppMenu_RunDemo:
      rangeTest.isRunning = (bool)mic->value;
      if (rangeTest.isRunning) {
        // (Re-)Init
        rangeTestInit();
        GRAPHICS_RSSIClear();
        if (RADIO_MODE_RX == rangeTest.radioMode) {
          // Bluetooth: close opened connection or stop advertising
          bluetoothDeactivate();
          // Start test timer
          proprietaryUpdateTestTimer(rangeTest.pktsReq);
//          rangeTestRx();
        } else if (RADIO_MODE_TX == rangeTest.radioMode) {
          proprietaryStartPeriodicTx();
          bluetoothPktsSentIndications(true);
        }
        // Start Demo
        seqSet(SEQ_TRX);
        rangeTest.isRunning = true;
      } else {
        // Idle Radio
        RAIL_Idle(rangeTestGetRailHandle(), RAIL_IDLE, true);
        if (RADIO_MODE_RX == rangeTest.radioMode) {
          // Bluetooth: start connectable advertising
          bluetoothActivate();
        }
        if (RADIO_MODE_TX == rangeTest.radioMode) {
          proprietaryStopPeriodicTx();
          bluetoothPktsSentIndications(false);
        }
        seqSet(SEQ_MENU);
      }
      break;

    default:
      ret = false;
      break;
  }
  return ret;
}

uint16_t getPhyList(MenuItemValue* miv)
{
  uint8_t ret = true;
  uint8_t i = 0;
  static char phyList[255] = { 0 };
  // assert if id of PHY is two digit, that is not handled here
  APP_ASSERT_DBG((COUNTOF(rangeTestPhys) < 9), COUNTOF(rangeTestPhys));
  for (uint8_t j = 0; j < COUNTOF(rangeTestPhys); j++) {
    // list supported PHYs only
    if (rangeTestPhys[j].isSupported) {
      // <phy id>:<phy name>,<phy id>:<phy name>...
      // string length = id length + semicolon + phy name length + comma
      uint8_t sl = strlen(rangeTestPhys[j].name) + 3;
      // string too long to fit
      if ((i + sl) >= 255) {
        break;
      }
      sprintf(&phyList[i], "%u:%s,", (uint8_t)rangeTestPhys[j].phy, rangeTestPhys[j].name);
      i += sl;
    }
  }
  miv->data = (uint8_t*)&phyList;
  miv->size = i;
  return ret;
}

bool menuItemValueGet(menuItemIndex_e mii, MenuItemValue* miv)
{
  uint8_t ret = true;
  uint16_t freq = PHY_FREQUENCY;
  switch (mii) {
    case eAppMenu_SetRFFrequency:
      // TODO
      miv->data = (uint8_t*)&freq;
      miv->size = sizeof(freq);
      break;

    case eAppMenu_SetPktsNum:
      miv->data = (uint8_t*)&rangeTest.pktsReq;
      miv->size = sizeof(rangeTest.pktsReq);
      break;

    case eAppMenu_SetPhy:
      miv->data = (uint8_t*)&rangeTest.phy;
      miv->size = sizeof(rangeTest.phy);
      break;

    case eAppMenu_SetChannel:
      miv->data = (uint8_t*)&rangeTest.channel;
      miv->size = sizeof(rangeTest.channel);
      break;

    case eAppMenu_SetRFMode:
      miv->data = (uint8_t*)&rangeTest.radioMode;
      miv->size = sizeof(rangeTest.radioMode);
      break;

    case eAppMenu_SetRFPA:
      miv->data = (uint8_t*)&rangeTest.txPower;
      miv->size = sizeof(rangeTest.txPower);
      break;

    case eAppMenu_SetIDDest:
      miv->data = (uint8_t*)&rangeTest.destID;
      miv->size = sizeof(rangeTest.destID);
      break;

    case eAppMenu_SetIDSrc:
      miv->data = (uint8_t*)&rangeTest.srcID;
      miv->size = sizeof(rangeTest.srcID);
      break;

    case eAppMenu_SetPktsLen:
      miv->data = (uint8_t*)&rangeTest.payloadLength;
      miv->size = sizeof(rangeTest.payloadLength);
      break;

    case eAppMenu_SetMAWindow:
      miv->data = (uint8_t*)&rangeTest.maSize;
      miv->size = sizeof(rangeTest.maSize);
      break;

    case eAppMenu_SetLogMode:
      miv->data = (uint8_t*)&rangeTest.log;
      miv->size = sizeof(rangeTest.log);
      break;

    case eAppMenu_RunDemo:
      miv->data = (uint8_t*)&rangeTest.isRunning;
      miv->size = sizeof(rangeTest.isRunning);
      break;

    default:
      ret = false;
      break;
  }
  return ret;
}

bool menuItemValueValidate(MenuItemChange* mic)
{
  uint8_t ret = true;
  switch (mic->index) {
    case eAppMenu_SetPhy:
      ret = false;
      for (uint8_t i = 0; i < COUNTOF(rangeTestPhys); i++) {
        // search supported PHYs for match
        if (rangeTestPhys[i].isSupported
            && rangeTestPhys[i].phy == (RangeTestPhy)mic->value) {
          ret = true;
          break;
        }
      }
      break;

    default:
      ret = false;
      break;
  }
  return ret;
}

/**************************************************************************//**
 * @brief  The function resets payload length to the min value
 *
 * @param  void
 *
 * @return void
 *****************************************************************************/
static void resetPayloadLength(void)
{
  rangeTest.payloadLength = PAYLOAD_LEN_MIN;
}

/**************************************************************************//**
 * @brief  The function handles payload length overflow of BLE and IEEE802154
 *
 * @param  void
 *
 * @return void
 *****************************************************************************/
static void handlePayloadLengthOverlfow(void)
{
  if (rangeTest.phy == PHY_IEEE_802154) {
    if (rangeTest.payloadLength > IEEE802154_PAYLOAD_LEN_MAX) {
      resetPayloadLength();
    }
  } else {
    // BLE
    if (rangeTest.payloadLength > BLE_PAYLOAD_LEN_MAX) {
      resetPayloadLength();
    }
  }
}
