/***************************************************************************//**
 * @file
 * @brief A minimal project structure, used as a starting point for custom
 * Dynamic-Multiprotocol applications. The project has the basic
 * functionality enabling peripheral connectivity, without GATT
 * services. It runs on top of Micrium OS RTOS and multiprotocol RAIL.
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

#include "bsp/siliconlabs/generic/include/bsp_os.h"

#include <common/include/common.h>
#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/rtos_prio.h>
#include <common/include/platform_mgr.h>
#include <cpu/include/cpu.h>
#include <kernel/include/os.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "hal-config.h"
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"
#include "infrastructure.h"

#include "bsphalconfig.h"
#include "bsp.h"

#include "em_cmu.h"
#include "sleep.h"

#include "rtos_bluetooth.h"
#include "rtos_gecko.h"
#include "gatt_db.h"

#include "rail.h"
#include "rail_types.h"
#if !defined(_SILICON_LABS_32B_SERIES_2)
#include "rail_config.h"
#endif

// Ex Main Start task
#define EX_MAIN_START_TASK_PRIO           21u
#define EX_MAIN_START_TASK_STK_SIZE       512u
static CPU_STK exMainStartTaskStk[EX_MAIN_START_TASK_STK_SIZE];
static OS_TCB  exMainStartTaskTCB;
static void    exMainStartTask(void *p_arg);

#define APP_CFG_TASK_BLUETOOTH_LL_PRIO    3u
#define APP_CFG_TASK_BLUETOOTH_STACK_PRIO 4u

// Bluetooth Application task
#define BLUETOOTH_APP_TASK_PRIO           5u
#define BLUETOOTH_APP_TASK_STACK_SIZE     (1024 / sizeof(CPU_STK))
static CPU_STK bluetoothAppTaskStk[BLUETOOTH_APP_TASK_STACK_SIZE];
static OS_TCB  bluetoothAppTaskTCB;
static void    bluetoothAppTask(void *p_arg);

// Proprietary Application task
#define PROPRIETARY_APP_TASK_PRIO         6u
#define PROPRIETARY_APP_TASK_STACK_SIZE   (1024 / sizeof(CPU_STK))
static CPU_STK proprietaryAppTaskStk[PROPRIETARY_APP_TASK_STACK_SIZE];
static OS_TCB  proprietaryAppTaskTCB;
static void    proprietaryAppTask(void *p_arg);

// Tick Task Configuration
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
#define TICK_TASK_PRIO                    0u
#define TICK_TASK_STK_SIZE                256u
static CPU_STK TickTaskStk[TICK_TASK_STK_SIZE];
#define TICK_TASK_CFG                     .TickTaskCfg = \
{                                                        \
    .StkBasePtr = &TickTaskStk[0],                       \
    .StkSize    = (TICK_TASK_STK_SIZE),                  \
    .Prio       = TICK_TASK_PRIO,                        \
    .RateHz     = 1000u                                  \
},
#else
#define TICK_TASK_CFG
#endif

// Idle Task Configuration
#if (OS_CFG_TASK_IDLE_EN == DEF_ENABLED)
#define IDLE_TASK_STK_SIZE                256u
static CPU_STK IdleTaskStk[IDLE_TASK_STK_SIZE];
#define IDLE_TASK_CFG                     .IdleTask = \
{                                                     \
    .StkBasePtr = &IdleTaskStk[0],                    \
    .StkSize    = IDLE_TASK_STK_SIZE                  \
},
#else
#define IDLE_TASK_CFG
#endif

// Timer Task Configuration
#if (OS_CFG_TMR_EN == DEF_ENABLED)
#define TIMER_TASK_PRIO                   4u
#define TIMER_TASK_STK_SIZE               256u
static CPU_STK TimerTaskStk[TIMER_TASK_STK_SIZE];
#define TIMER_TASK_CFG                    .TmrTaskCfg = \
{                                                       \
    .StkBasePtr = &TimerTaskStk[0],                     \
    .StkSize    = TIMER_TASK_STK_SIZE,                  \
    .Prio       = TIMER_TASK_PRIO,                      \
    .RateHz     = 10u                                   \
},
#else
#define TIMER_TASK_CFG
#endif

// ISR Configuration
#define ISR_STK_SIZE                      256u
static CPU_STK ISRStk[ISR_STK_SIZE];
#define ISR_CFG                           .ISR = \
{                                                \
    .StkBasePtr = (CPU_STK*)&ISRStk[0],          \
    .StkSize    = (ISR_STK_SIZE)                 \
},

/* Define RTOS_DEBUG_MODE=DEF_ENABLED at the project level,
 * for enabling debug information for Micrium Probe.*/
#if (RTOS_DEBUG_MODE == DEF_ENABLED)
#define STAT_TASK_CFG          .StatTaskCfg = \
{                                             \
    .StkBasePtr = DEF_NULL,                   \
    .StkSize    = 256u,                       \
    .Prio       = KERNEL_STAT_TASK_PRIO_DFLT, \
    .RateHz     = 10u                         \
},

#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    IDLE_TASK_CFG                     \
    TICK_TASK_CFG                     \
    TIMER_TASK_CFG                    \
    STAT_TASK_CFG                     \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL       \
}
#else
#define OS_INIT_CFG_APP {     \
    ISR_CFG                   \
    IDLE_TASK_CFG             \
    TICK_TASK_CFG             \
    TIMER_TASK_CFG            \
    .MsgPoolSize   = 0u,      \
    .TaskStkLimit  = 0u,      \
    .MemSeg        = DEF_NULL \
}
#endif

#define COMMON_INIT_CFG_APP {   \
    .CommonMemSegPtr = DEF_NULL \
}

#define PLATFORM_MGR_INIT_CFG_APP { \
    .PoolBlkQtyInit = 0u,           \
    .PoolBlkQtyMax  = 0u            \
}

const OS_INIT_CFG           OS_InitCfg          = OS_INIT_CFG_APP;
const COMMON_INIT_CFG       Common_InitCfg      = COMMON_INIT_CFG_APP;
const PLATFORM_MGR_INIT_CFG PlatformMgr_InitCfg = PLATFORM_MGR_INIT_CFG_APP;

// Micrium OS hooks
static void idleHook(void);
static void setupHooks(void);

// Maximum number of Bluetooth connections.
#define MAX_CONNECTIONS 1
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
// Configuration parameters (see gecko_configuration.h)
static const gecko_configuration_t bluetooth_config =
{
  .config_flags = GECKO_CONFIG_FLAG_RTOS,
  .sleep.flags = 0,
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .gattdb = &bg_gattdb_data,
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
  .scheduler_callback = BluetoothLLCallback,
  .stack_schedule_callback = BluetoothUpdate,
#if (HAL_PA_ENABLE)
  .pa.config_enable = 1, // Set this to be a valid PA config
#if defined(FEATURE_PA_INPUT_FROM_VBAT)
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#else
  .pa.input = GECKO_RADIO_PA_INPUT_DCDC,
#endif // defined(FEATURE_PA_INPUT_FROM_VBAT)
#endif // (HAL_PA_ENABLE)
  .rf.flags = GECKO_RF_CONFIG_ANTENNA,                 /* Enable antenna configuration. */
  .rf.antenna = GECKO_RF_ANTENNA,                      /* Select antenna path! */
  .mbedtls.flags = GECKO_MBEDTLS_FLAGS_NO_MBEDTLS_DEVICE_INIT,
  .mbedtls.dev_number = 0,
};
static uint8_t boot_to_dfu = 0;

// Proprietary radio
static RAIL_Handle_t railHandle = NULL;
static RAILSched_Config_t railSchedState;
static void radioEventHandler(RAIL_Handle_t railHandle,
                              RAIL_Events_t events);
static RAIL_Config_t railCfg = {
  .eventsCallback = &radioEventHandler,
  .protocol = NULL,
  .scheduler = &railSchedState,
};

/* OS Event Flag Group */
OS_FLAG_GRP  proprietary_event_flags;
/* Dummy flag to prevent cyclic execution of the proprietary task function code.
 * this flag will not be posted by default. */
#define DUMMY_FLAG  ((OS_FLAGS)0x01)

/**************************************************************************//**
 * Main.
 *
 * @returns Returns 1.
 *
 * This is the standard entry point for C applications. It is assumed that your
 * code will call main() once you have performed all necessary initialization.
 *****************************************************************************/
int main(void)
{
  RTOS_ERR err;

  // Initialize device.
  initMcu();
  // Initialize board.
  initBoard();
  // Initialize application.
  initApp();
  // Initialize the Kernel.
  OSInit(&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  // Initialize Kernel tick source.
  BSP_TickInit();
  // Setup the Micrium OS hooks.
  setupHooks();
  // Create the Ex Main Start task
  OSTaskCreate(&exMainStartTaskTCB,
               "Ex Main Start Task",
               exMainStartTask,
               DEF_NULL,
               EX_MAIN_START_TASK_PRIO,
               &exMainStartTaskStk[0],
               (EX_MAIN_START_TASK_STK_SIZE / 10u),
               EX_MAIN_START_TASK_STK_SIZE,
               0u,
               0u,
               DEF_NULL,
               (OS_OPT_TASK_STK_CLR),
               &err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
  // Start the kernel.
  OSStart(&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}

/***************************************************************************//**
 * This is the idle hook.
 *
 * This will be called by the Micrium OS idle task when there is no other task
 * ready to run. We just enter the lowest possible energy mode.
 ******************************************************************************/
void SleepAndSyncProtimer(void);
static void idleHook(void)
{
  // Put MCU in the lowest sleep mode available, usually EM2.
  SleepAndSyncProtimer();
}

/***************************************************************************//**
 * Setup the Micrium OS hooks.
 *
 * Setup the Micrium OS hooks. We are only using the idle task hook in this
 * example. See the Mcirium OS documentation for more information on the other
 * available hooks.
 ******************************************************************************/
static void setupHooks(void)
{
  CPU_SR_ALLOC();
  CPU_CRITICAL_ENTER();
  // Don't allow EM3, since we use LF clocks.
  SLEEP_SleepBlockBegin(sleepEM3);
  OS_AppIdleTaskHookPtr = idleHook;
  CPU_CRITICAL_EXIT();
}

/***************************************************************************//**
 * Setup the bluetooth init function.
 *
 * @return error code for the gecko_init function
 *
 * All bluetooth specific initialization code should be here like gecko_init(),
 * gecko_init_whitelisting(), gecko_init_multiprotocol() and so on.
 ******************************************************************************/
static errorcode_t initialize_bluetooth()
{
  errorcode_t err = gecko_init(&bluetooth_config);
  if (err == bg_err_success) {
    gecko_init_multiprotocol(NULL);
  }
  APP_RTOS_ASSERT_DBG((err == bg_err_success), 1);
  return err;
}

/**************************************************************************//**
 * Task to initialise Bluetooth and Proprietary tasks.
 *
 * @param p_arg Pointer to an optional data area which can pass parameters to
 *              the task when the task executes.
 *
 * This is the task that will be called by from main() when all services are
 * initialized successfully.
 *****************************************************************************/
static void exMainStartTask(void *p_arg)
{
  PP_UNUSED_PARAM(p_arg);
  RTOS_ERR err;

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
  // Initialize CPU Usage.
  OSStatTaskCPUUsageInit(&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE),; );
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  // Initialize interrupts disabled measurement.
  CPU_IntDisMeasMaxCurReset();
#endif

  // Initialize the flag group for the proprietary task
  OSFlagCreate(&proprietary_event_flags, "Prop. flags", (OS_FLAGS)0, &err);

  // Create the Bluetooth Application task
  OSTaskCreate(&bluetoothAppTaskTCB,
               "Bluetooth App Task",
               bluetoothAppTask,
               0u,
               BLUETOOTH_APP_TASK_PRIO,
               &bluetoothAppTaskStk[0u],
               (BLUETOOTH_APP_TASK_STACK_SIZE / 10u),
               BLUETOOTH_APP_TASK_STACK_SIZE,
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  // Create the Proprietary Application task
  OSTaskCreate(&proprietaryAppTaskTCB,
               "Proprietary App Task",
               proprietaryAppTask,
               0u,
               PROPRIETARY_APP_TASK_PRIO,
               &proprietaryAppTaskStk[0u],
               (PROPRIETARY_APP_TASK_STACK_SIZE / 10u),
               PROPRIETARY_APP_TASK_STACK_SIZE,
               0u,
               0u,
               0u,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  // Done starting everyone else so let's exit.
  OSTaskDel((OS_TCB *)0, &err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
}

/**************************************************************************//**
 * Bluetooth Application task.
 *
 * @param p_arg Pointer to an optional data area which can pass parameters to
 *              the task when the task executes.
 *
 * This is a minimal Bluetooth Application task that starts advertising after
 * boot and supports OTA upgrade feature.
 *****************************************************************************/
static void bluetoothAppTask(void *p_arg)
{
  PP_UNUSED_PARAM(p_arg);
  RTOS_ERR osErr;
  errorcode_t initErr;
  struct gecko_msg_le_gap_start_advertising_rsp_t* pRspAdv;
  struct gecko_msg_le_gap_set_advertise_timing_rsp_t* pRspAdvT;
  struct gecko_msg_gatt_server_send_user_write_response_rsp_t* pRspWrRsp;
  struct gecko_msg_le_connection_close_rsp_t* pRspConnCl;

  // Create Bluetooth Link Layer and stack tasks
  bluetooth_start(APP_CFG_TASK_BLUETOOTH_LL_PRIO,
                  APP_CFG_TASK_BLUETOOTH_STACK_PRIO,
                  initialize_bluetooth);

  while (DEF_TRUE) {
    OSFlagPend(&bluetooth_event_flags,
               (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_WAITING,
               (OS_TICK)0,
               OS_OPT_PEND_BLOCKING       \
               + OS_OPT_PEND_FLAG_SET_ANY \
               + OS_OPT_PEND_FLAG_CONSUME,
               NULL,
               &osErr);
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(osErr) == RTOS_ERR_NONE), 1);

    // Handle stack events
    switch (BGLIB_MSG_ID(bluetooth_evt->header)) {
      // This boot event is generated when the system boots up after reset.
      // Do not call any stack commands before receiving the boot event.
      // Here the system is set to start advertising immediately after boot
      // procedure.
      case gecko_evt_system_boot_id:
        // Set advertising parameters. 100ms advertisement interval.
        // The first parameter is advertising set handle
        // The next two parameters are minimum and maximum advertising
        // interval, both in units of (milliseconds * 1.6).
        // The last two parameters are duration and maxevents left as default.
        pRspAdvT = gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0);
        APP_ASSERT_DBG((pRspAdvT->result == bg_err_success), pRspAdvT->result);
        // Start general advertising and enable connections.
        pRspAdv = gecko_cmd_le_gap_start_advertising(0,
                                                     le_gap_general_discoverable,
                                                     le_gap_connectable_scannable);
        APP_ASSERT_DBG((pRspAdv->result == bg_err_success), pRspAdv->result);
        break;

      case gecko_evt_le_connection_closed_id:
        // Check if need to boot to dfu mode.
        if (boot_to_dfu) {
          // Enter to DFU OTA mode.
          gecko_cmd_system_reset(2);
        } else {
          // Restart advertising after client has disconnected.
          pRspAdv = gecko_cmd_le_gap_start_advertising(0,
                                                       le_gap_general_discoverable,
                                                       le_gap_connectable_scannable);
          APP_ASSERT_DBG((pRspAdv->result == bg_err_success), pRspAdv->result);
        }
        break;

      // Events related to OTA upgrading
      // Check if the user-type OTA Control Characteristic was written.
      // If ota_control was written, boot the device into Device Firmware
      // Upgrade (DFU) mode.
      case gecko_evt_gatt_server_user_write_request_id:
        if (bluetooth_evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
          // Set flag to enter to OTA mode.
          boot_to_dfu = 1;
          // Send response to Write Request.
          pRspWrRsp = gecko_cmd_gatt_server_send_user_write_response(
            bluetooth_evt->data.evt_gatt_server_user_write_request.connection,
            gattdb_ota_control,
            bg_err_success);
          APP_ASSERT_DBG((pRspWrRsp->result == bg_err_success), pRspWrRsp->result);
          // Close connection to enter to DFU OTA mode.
          pRspConnCl = gecko_cmd_le_connection_close(bluetooth_evt->data.evt_gatt_server_user_write_request.connection);
          APP_ASSERT_DBG((pRspConnCl->result == bg_err_success), pRspConnCl->result);
        }
        break;

      default:
        break;
    }

    OSFlagPost(&bluetooth_event_flags,
               (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_HANDLED,
               OS_OPT_POST_FLAG_SET,
               &osErr);
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(osErr) == RTOS_ERR_NONE), 1);
  }
}

/**************************************************************************//**
 * Proprietary Application task.
 *
 * @param p_arg Pointer to an optional data area which can pass parameters to
 *              the task when the task executes.
 *
 * This is a minimal Proprietary Application task that only configures the
 * radio.
 *****************************************************************************/
static void proprietaryAppTask(void *p_arg)
{
  PP_UNUSED_PARAM(p_arg);
  RTOS_ERR err;

  // Create each RAIL handle with their own configuration structures
  railHandle = RAIL_Init(&railCfg, NULL);
  // Configure radio according to the generated radio settings
  RAIL_TxPowerConfig_t railTxPowerConfig = {
#if HAL_PA_2P4_LOWPOWER
    .mode = RAIL_TX_POWER_MODE_2P4_LP,
#else
    .mode = RAIL_TX_POWER_MODE_2P4_HP,
#endif
    .voltage = HAL_PA_VOLTAGE,
    .rampTime = HAL_PA_RAMP,
  };

#if !defined(_SILICON_LABS_32B_SERIES_2)
  if (channelConfigs[0]->configs[0].baseFrequency < 1000000000UL) {
    // Use the Sub-GHz PA if required
    railTxPowerConfig.mode = RAIL_TX_POWER_MODE_SUBGIG;
  }
#endif
  if (RAIL_ConfigTxPower(railHandle, &railTxPowerConfig) != RAIL_STATUS_NO_ERROR) {
    while (1) ;
  }
  // We must reapply the Tx power after changing the PA above
  RAIL_SetTxPower(railHandle, HAL_PA_POWER);
#if !defined(_SILICON_LABS_32B_SERIES_2)
  RAIL_ConfigChannels(railHandle, channelConfigs[0], NULL);
#else
  //
  // Put your Radio Configuration here!
  //
#endif
  // Configure the most useful callbacks plus catch a few errors
  RAIL_ConfigEvents(railHandle,
                    RAIL_EVENTS_ALL,
                    RAIL_EVENTS_ALL);

  while (DEF_TRUE) {
    // Wait for the dummy flag. Use this flag to stop waiting with the execution of your code.
    OSFlagPend(&proprietary_event_flags,
               DUMMY_FLAG,
               (OS_TICK)0,
               OS_OPT_PEND_BLOCKING       \
               + OS_OPT_PEND_FLAG_SET_ANY \
               + OS_OPT_PEND_FLAG_CONSUME,
               NULL,
               &err);
    APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);
    //
    // Put your code here!
    //
  }
}

static void radioEventHandler(RAIL_Handle_t railHandle,
                              RAIL_Events_t events)
{
  PP_UNUSED_PARAM(railHandle);
  PP_UNUSED_PARAM(events);
}
