/***************************************************************************//**
 * @file
 * @brief Implements the Thermometer (GATT Server) Role of the Health
 * Thermometer Profile, which enables a Collector device to connect and
 * interact with a Thermometer.  The device acts as a connection
 * Peripheral. The sample application is based on Micrium OS RTOS.
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
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                            INCLUDE FILES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#include "bsp/siliconlabs/generic/include/bsp_os.h"

#include <cpu/include/cpu.h>
#include <common/include/common.h>
#include <kernel/include/os.h>

#include <common/include/lib_def.h>
#include <common/include/rtos_utils.h>
#include <common/include/toolchains.h>
#include <common/include/rtos_prio.h>
#include  <common/include/platform_mgr.h>

#include "sleep.h"
#include <stdio.h>

#include "rtos_bluetooth.h"
//Bluetooth API definition
#include "rtos_gecko.h"
//GATT DB
#include "gatt_db.h"

/* Board Headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"
#include "hal-config-app-common.h"

#include "em_cmu.h"

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                             LOCAL DEFINES
 *********************************************************************************************************
 *********************************************************************************************************
 */

#define OTA

#define  EX_MAIN_START_TASK_PRIO            21u
#define  EX_MAIN_START_TASK_STK_SIZE        512u

#define  APP_CFG_TASK_START_PRIO            2u
#define  APP_CFG_TASK_BLUETOOTH_LL_PRIO     3u
#define  APP_CFG_TASK_BLUETOOTH_STACK_PRIO  4u
#define  APP_CFG_TASK_APPLICATION_PRIO      5u
#define  APP_CFG_TASK_THERMOMETER_PRIO      6u

#define  APP_CFG_TASK_THERMOMETER_STK_SIZE  (1024 / sizeof(CPU_STK))
#define  APPLICATION_STACK_SIZE             (1024 / sizeof(CPU_STK))

// MTM: Added configs for all Micrium OS tasks

// Tick Task Configuration
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
#define  TICK_TASK_PRIO             0u
#define  TICK_TASK_STK_SIZE         256u
#define  TICK_TASK_CFG              .TickTaskCfg = \
{                                                  \
    .StkBasePtr = &TickTaskStk[0],                 \
    .StkSize    = (TICK_TASK_STK_SIZE),            \
    .Prio       = TICK_TASK_PRIO,                  \
    .RateHz     = 1000u                            \
},
#else
#define  TICK_TASK_CFG
#endif

// Idle Task Configuration
#if (OS_CFG_TASK_IDLE_EN == DEF_ENABLED)
#define  IDLE_TASK_STK_SIZE         256u
#define  IDLE_TASK_CFG              .IdleTask = \
{                                               \
    .StkBasePtr  = &IdleTaskStk[0],             \
    .StkSize     = IDLE_TASK_STK_SIZE           \
},
#else
#define  IDLE_TASK_CFG
#endif

// Timer Task Configuration
#if (OS_CFG_TMR_EN == DEF_ENABLED)
#define  TIMER_TASK_PRIO            4u
#define  TIMER_TASK_STK_SIZE        256u
#define  TIMER_TASK_CFG             .TmrTaskCfg = \
{                                                 \
    .StkBasePtr = &TimerTaskStk[0],               \
    .StkSize    = TIMER_TASK_STK_SIZE,            \
    .Prio       = TIMER_TASK_PRIO,                \
    .RateHz     = 10u                             \
},
#else
#define  TIMER_TASK_CFG
#endif

// ISR Configuration
#define  ISR_STK_SIZE               256u
#define  ISR_CFG                        .ISR = \
{                                              \
    .StkBasePtr  = (CPU_STK*) &ISRStk[0],      \
    .StkSize     = (ISR_STK_SIZE)              \
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
#define  OS_INIT_CFG_APP            { \
    ISR_CFG                           \
    IDLE_TASK_CFG                     \
    TICK_TASK_CFG                     \
    TIMER_TASK_CFG                    \
    .MsgPoolSize     = 0u,            \
    .TaskStkLimit    = 0u,            \
    .MemSeg          = DEF_NULL       \
}
#endif

#define  COMMON_INIT_CFG_APP        { \
    .CommonMemSegPtr = DEF_NULL       \
}

#define  PLATFORM_MGR_INIT_CFG_APP  { \
    .PoolBlkQtyInit = 0u,             \
    .PoolBlkQtyMax  = 0u              \
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                        LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 *********************************************************************************************************
 */

/*
 * Bluetooth stack configuration
 */

#define MAX_CONNECTIONS 1
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
/* Gecko configuration parameters (see gecko_configuration.h) */
static const gecko_configuration_t bluetooth_config =
{
  .config_flags = GECKO_CONFIG_FLAG_RTOS,
#if defined(FEATURE_LFXO)
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
#else
  .sleep.flags = 0,
#endif // LFXO
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .gattdb = &bg_gattdb_data,
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
#ifdef OTA
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
#endif
};

// Thermometer Task.
static  OS_TCB   App_TaskThermometerTCB;
static  CPU_STK  App_TaskThermometerStk[APP_CFG_TASK_THERMOMETER_STK_SIZE];

// Event Handler Task
static  OS_TCB   ApplicationTaskTCB;
static CPU_STK ApplicationTaskStk[APPLICATION_STACK_SIZE];

// Idle Task
#if (OS_CFG_TASK_IDLE_EN == DEF_ENABLED)
static  CPU_STK  IdleTaskStk[IDLE_TASK_STK_SIZE];
#endif

// Tick Task
#if (OS_CFG_TASK_TICK_EN == DEF_ENABLED)
static  CPU_STK  TickTaskStk[TICK_TASK_STK_SIZE];
#endif

// Timer Task
#if (OS_CFG_TMR_EN == DEF_ENABLED)
static  CPU_STK  TimerTaskStk[TIMER_TASK_STK_SIZE];
#endif

// ISR Stack
static  CPU_STK  ISRStk[ISR_STK_SIZE];

const   OS_INIT_CFG             OS_InitCfg          = OS_INIT_CFG_APP;
const   COMMON_INIT_CFG         Common_InitCfg      = COMMON_INIT_CFG_APP;
const   PLATFORM_MGR_INIT_CFG   PlatformMgr_InitCfg = PLATFORM_MGR_INIT_CFG_APP;

enum bg_thermometer_temperature_measurement_flag{
  bg_thermometer_temperature_measurement_flag_units    =0x1,
  bg_thermometer_temperature_measurement_flag_timestamp=0x2,
  bg_thermometer_temperature_measurement_flag_type     =0x4,
};

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                       LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 *********************************************************************************************************
 */

static  void     App_TaskThermometer      (void *p_arg);
static  void     BluetoothApplicationTask (void *p_arg);

static  void     idleHook(void);
static  void     setupHooks(void);
#ifdef OTA
static uint8_t boot_to_dfu = 0;
#endif

static inline uint32_t bg_uint32_to_float(uint32_t mantissa, int32_t exponent);
static inline void bg_thermometer_create_measurement(uint8_t* buffer, uint32_t measurement, int fahrenheit);
/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                          GLOBAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C applications. It is assumed that your code will
 *               call main() once you have performed all necessary initialization.
 *
 * Argument(s) : None.
 *
 * Return(s)   : None.
 *
 * Note(s)     : None.
 *********************************************************************************************************
 */
int main(void)
{
  RTOS_ERR  err;

  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();

  CMU_ClockEnable(cmuClock_PRS, true);

  //system already initialized by enter_DefaultMode_from_RESET
  //BSP_SystemInit();                                           /* Initialize System.                                   */

  // MTM: Not needed anymore
  //OS_ConfigureTickTask(&tickTaskCfg);

  OSInit(&err);                                                 /* Initialize the Kernel.                               */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  // MTM: Rename start task to Thermometer task
  OSTaskCreate(&App_TaskThermometerTCB,                         /* Create the Start Task.                               */
               "Thermometer Task",
               App_TaskThermometer,
               0,
               APP_CFG_TASK_THERMOMETER_PRIO,
               &App_TaskThermometerStk[0],
               (APP_CFG_TASK_THERMOMETER_STK_SIZE / 10u),
               APP_CFG_TASK_THERMOMETER_STK_SIZE,
               0,
               0,
               0,
               (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               &err);
  /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  OSStart(&err);                                                /* Start the kernel.                                    */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  return (1);
}

/*
 *********************************************************************************************************
 *********************************************************************************************************
 *                                           LOCAL FUNCTIONS
 *********************************************************************************************************
 *********************************************************************************************************
 */
/**
 * Convert mantissa & exponent values to IEEE float type
 */
static inline uint32_t bg_uint32_to_float(uint32_t mantissa, int32_t exponent)
{
  return (mantissa & 0xffffff) | (uint32_t)(exponent << 24);
}

/**
 * Create temperature measurement value from IEEE float and temperature type flag
 */
static inline void bg_thermometer_create_measurement(uint8_t* buffer, uint32_t measurement, int fahrenheit)
{
  buffer[0] = fahrenheit ? bg_thermometer_temperature_measurement_flag_units : 0;
  buffer[1] = measurement & 0xff;
  buffer[2] = (measurement >> 8) & 0xff;
  buffer[3] = (measurement >> 16) & 0xff;
  buffer[4] = (measurement >> 24) & 0xff;
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
  APP_RTOS_ASSERT_DBG((err == bg_err_success), 1);
  return err;
}

/*
 *********************************************************************************************************
 *                                          Ex_MainStartTask()
 *
 * Description : This is the task that will be called by the Startup when all services are initializes
 *               successfully.
 *
 * Argument(s) : p_arg   Argument passed from task creation. Unused, in this case.
 *
 * Return(s)   : None.
 *
 * Notes       : None.
 *********************************************************************************************************
 */
static  void  App_TaskThermometer(void *p_arg)
{
  RTOS_ERR  err;
  int temperature_counter = 20;

  PP_UNUSED_PARAM(p_arg);                                       /* Prevent compiler warning.                            */

  // MTM: Move to AFTER Micrium OS starts
  BSP_TickInit();                                               /* Initialize Kernel tick source.                       */
  setupHooks();

  bluetooth_start(APP_CFG_TASK_BLUETOOTH_LL_PRIO,
                  APP_CFG_TASK_BLUETOOTH_STACK_PRIO,
                  initialize_bluetooth);

#if (OS_CFG_STAT_TASK_EN == DEF_ENABLED)
  OSStatTaskCPUUsageInit(&err);                                 /* Initialize CPU Usage.                                */
                                                                /*   Check error code.                                  */
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE),; );
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();                                  /* Initialize interrupts disabled measurement.          */
#endif

  BSP_OS_Init();                                                /* Initialize the BSP. It is expected that the BSP ...  */
                                                                /* ... will register all the hardware controller to ... */
                                                                /* ... the platform manager at this moment.             */

  // Create task for Event handler
  OSTaskCreate((OS_TCB     *)&ApplicationTaskTCB,
               (CPU_CHAR   *)"Bluetooth Application Task",
               (OS_TASK_PTR ) BluetoothApplicationTask,
               (void       *) 0u,
               (OS_PRIO     ) APP_CFG_TASK_APPLICATION_PRIO,
               (CPU_STK    *)&ApplicationTaskStk[0u],
               (CPU_STK_SIZE)(APPLICATION_STACK_SIZE / 10u),
               (CPU_STK_SIZE) APPLICATION_STACK_SIZE,
               (OS_MSG_QTY  ) 0u,
               (OS_TICK     ) 0u,
               (void       *) 0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (RTOS_ERR   *)&err);
  APP_RTOS_ASSERT_DBG((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), 1);

  // Start the Thermometer Loop. This call does NOT return.
  while (DEF_TRUE) {
    OSTimeDlyHMSM(0, 0, 1, 0,
                  OS_OPT_TIME_DLY | OS_OPT_TIME_HMSM_NON_STRICT,
                  &err);

    temperature_counter++;
    if (temperature_counter > 40) {
      temperature_counter = 20;
    }

    uint8_t temp_buffer[5];
    bg_thermometer_create_measurement(temp_buffer,
                                      bg_uint32_to_float(temperature_counter, 0),
                                      0);
    gecko_cmd_gatt_server_send_characteristic_notification(0xff, gattdb_temperature_measurement, 5, temp_buffer);
  }

  /* Done starting everyone else so let's exit */
  // MTM: Remove Delete
  //OSTaskDel((OS_TCB *)0, &err);
}

void BluetoothEventHandler(struct gecko_cmd_packet* evt)
{
  switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:
    case gecko_evt_le_connection_closed_id:
#ifdef OTA
      if (boot_to_dfu) {
        gecko_cmd_system_reset(2);
      }
#endif
      //Start advertisement at boot, and after disconnection
      gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
      break;
#ifdef OTA
    case gecko_evt_gatt_server_user_write_request_id:
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
        //boot to dfu mode after disconnecting
        boot_to_dfu = 1;
        gecko_cmd_gatt_server_send_user_write_response(evt->data.evt_gatt_server_user_write_request.connection, gattdb_ota_control, bg_err_success);
        gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection);
      }
      break;
#endif
  }
}
/***************************************************************************//**
 * @brief
 *   This is the idle hook.
 *
 * @detail
 *   This will be called by the Micrium OS idle task when there is no other
 *   task ready to run. We just enter the lowest possible energy mode.
 ******************************************************************************/
void SleepAndSyncProtimer();
static void idleHook(void)
{
  /* Put MCU in the lowest sleep mode available, usually EM2 */
  SleepAndSyncProtimer();
}

/***************************************************************************//**
 * @brief
 *   Setup the Micrium OS hooks. We are only using the idle task hook in this
 *   example. See the Mcirium OS documentation for more information on the
 *   other available hooks.
 ******************************************************************************/
static void setupHooks(void)
{
  CPU_SR_ALLOC();

  CPU_CRITICAL_ENTER();
  /* Don't allow EM3, since we use LF clocks. */
  SLEEP_SleepBlockBegin(sleepEM3);
  OS_AppIdleTaskHookPtr = idleHook;
  CPU_CRITICAL_EXIT();
}

/*********************************************************************************************************
 *                                             BluetoothApplicationTask()
 *
 * Description : Bluetooth Application task.
 *
 * Argument(s) : p_arg       the argument passed by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */
static  void  BluetoothApplicationTask(void *p_arg)
{
  RTOS_ERR      os_err;
  (void)p_arg;

  while (DEF_TRUE) {
    OSFlagPend(&bluetooth_event_flags, (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_WAITING,
               0,
               OS_OPT_PEND_BLOCKING + OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME,
               NULL,
               &os_err);
    BluetoothEventHandler((struct gecko_cmd_packet*)bluetooth_evt);

    OSFlagPost(&bluetooth_event_flags, (OS_FLAGS)BLUETOOTH_EVENT_FLAG_EVT_HANDLED, OS_OPT_POST_FLAG_SET, &os_err);
  }
}
