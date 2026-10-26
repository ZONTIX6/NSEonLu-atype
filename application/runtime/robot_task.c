#include "robot_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "robot.h"
#include "robot_def.h"
#include "ins_task.h"
#include "ins_bridge.h"
#include "motor_task.h"
#include "referee_task.h"
#include "master_process.h"
#include "daemon.h"
#include "HT04.h"
#include "buzzer.h"
#include "key.h"

#include "bsp_log.h"
#include "bsp_dwt.h"
#include "board_config.h"
#include "dji_motor.h"
#include "chassis_power.h"
#include "chassis_control_config.h"
#include "usb_telemetry.h"

static osThreadId insTaskHandle;
static osThreadId robotTaskHandle;
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
/* Actual execution time is separate from the controller's adjacent-cycle dt. */
volatile uint32_t chassis_force_motor_max_execution_us;
volatile UBaseType_t chassis_force_motor_stack_free_words;
#endif
static osThreadId motorTaskHandle;
static osThreadId daemonTaskHandle;
#if REFEREE_ENABLE && (defined(ONE_BOARD) || defined(CHASSIS_BOARD))
static osThreadId uiTaskHandle;
#endif
#if USB_TELEMETRY_ENABLE
static osThreadId usbTelemetryTaskHandle;
static void StartUSBTelemetryTask(void const *argument)
{
    (void)argument;
    USBTelemetryInit();
    TickType_t previous_wake = xTaskGetTickCount();
    for (;;) {
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
        static uint32_t force_stack_sample_tick;
        if((uint32_t)(HAL_GetTick()-force_stack_sample_tick)>=200) {
            force_stack_sample_tick=HAL_GetTick();
            chassis_force_motor_stack_free_words=uxTaskGetStackHighWaterMark(motorTaskHandle);
        }
#endif
        USBTelemetryTask();
        vTaskDelayUntil(&previous_wake, pdMS_TO_TICKS(USB_TELEMETRY_PERIOD_MS));
    }
}
#endif
#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED
static osThreadId powerCaptureTaskHandle;
/* Debugger-readable on-board validation; stack values are FreeRTOS words. */
volatile float chassis_power_motor_max_dt_ms;
volatile UBaseType_t chassis_power_motor_stack_free_words;
static void StartPowerCaptureTask(void const *argument)
{
    (void)argument;
    uint32_t stack_sample_tick = 0;
    for (;;) {
        ChassisPowerCaptureDrain();
        if ((uint32_t)(HAL_GetTick() - stack_sample_tick) >= 200) {
            stack_sample_tick = HAL_GetTick();
            chassis_power_motor_stack_free_words = uxTaskGetStackHighWaterMark(motorTaskHandle);
        }
        osDelay(5);
    }
}
#endif

static void StartINSTASK(void const *argument);
static void StartMOTORTASK(void const *argument);
static void StartDAEMONTASK(void const *argument);
static void StartROBOTTASK(void const *argument);
#if REFEREE_ENABLE && (defined(ONE_BOARD) || defined(CHASSIS_BOARD))
static void StartUITASK(void const *argument);
#endif

/**
 * @brief 初始化机器人任务,所有持续运行的任务都在这里初始化
 *
 */
void OSTaskInit(void)
{
    osThreadDef(instask, StartINSTASK, osPriorityAboveNormal, 0, 1024);
    insTaskHandle = osThreadCreate(osThread(instask), NULL); // 由于是阻塞读取传感器,为姿态解算设置较高优先级,确保以1khz的频率执行
    // // 后续修改为读取传感器数据准备好的中断处理,

    /* Diagnostic frame exceeds 1 KiB before nested algorithm/CSV calls. */
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
    /* Bounded allocator call chain exceeds 2 KiB; retain >=128 words headroom. */
    osThreadDef(motortask, StartMOTORTASK, osPriorityNormal, 0, 1024);
#else
    osThreadDef(motortask, StartMOTORTASK, osPriorityNormal, 0, 512);
#endif
    motorTaskHandle = osThreadCreate(osThread(motortask), NULL);

    /* Offline callbacks need stack for log formatting and interrupt context. */
    osThreadDef(daemontask, StartDAEMONTASK, osPriorityNormal, 0, 512);
    daemonTaskHandle = osThreadCreate(osThread(daemontask), NULL);

    osThreadDef(robottask, StartROBOTTASK, osPriorityNormal, 0, 1024);
    robotTaskHandle = osThreadCreate(osThread(robottask), NULL);

#if USB_TELEMETRY_ENABLE
    /* Bounded tuning-line parsing and reply formatting share this task. */
    osThreadDef(usbtelemetry, StartUSBTelemetryTask, osPriorityBelowNormal, 0, 768);
    usbTelemetryTaskHandle = osThreadCreate(osThread(usbtelemetry), NULL);
#endif

#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED
    osThreadDef(powercapture, StartPowerCaptureTask, osPriorityBelowNormal, 0, 256);
    powerCaptureTaskHandle = osThreadCreate(osThread(powercapture), NULL);
#endif

#if REFEREE_ENABLE && (defined(ONE_BOARD) || defined(CHASSIS_BOARD))
    osThreadDef(uitask, StartUITASK, osPriorityNormal, 0, 512);
    uiTaskHandle = osThreadCreate(osThread(uitask), NULL);
#endif

    HTMotorControlInit(); // 没有注册HT电机则不会执行
}

static __attribute__((noreturn)) void StartINSTASK(void const *argument)
{
    static float ins_start;
    static float ins_dt;
    INS_Init(); // 确保BMI088被正确初始化.
    LOGINFO("[freeRTOS] INS Task Start");
    for (;;)
    {
        // 1kHz
        ins_start = DWT_GetTimeline_ms();
        INSBridgeTask();
        ins_dt = DWT_GetTimeline_ms() - ins_start;
        if (ins_dt > 1)
            LOGERROR("[freeRTOS] INS Task is being DELAY! dt = [%f]", &ins_dt);
        // // 解算完成后发送视觉数据,但是当前的实现不太优雅,后续若添加硬件触发需要重新考虑结构的组织
        osDelay(1);
    }
}

static __attribute__((noreturn)) void StartMOTORTASK(void const *argument)
{
    static float motor_dt;
    static float motor_start;
    LOGINFO("[freeRTOS] MOTOR Task Start");
    for (;;)
    {
        motor_start = DWT_GetTimeline_ms();
        MotorControlTask();
        motor_dt = DWT_GetTimeline_ms() - motor_start;
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
        const uint32_t execution_us=(uint32_t)(motor_dt*1000);
        if(execution_us>chassis_force_motor_max_execution_us) chassis_force_motor_max_execution_us=execution_us;
#endif
#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED
        if (motor_dt > chassis_power_motor_max_dt_ms) chassis_power_motor_max_dt_ms = motor_dt;
#endif
        if (motor_dt > 2)
            LOGERROR("[freeRTOS] MOTOR Task is being DELAY! dt = [%f]", &motor_dt);
        osDelay(2);
    }
}

static __attribute__((noreturn)) void StartDAEMONTASK(void const *argument)
{
    static float daemon_dt;
    static float daemon_start;
    BuzzerInit();
    LOGINFO("[freeRTOS] Daemon Task Start");
    for (;;)
    {
        // 100Hz
        daemon_start = DWT_GetTimeline_ms();
        DaemonTask();
        BuzzerTask();
        KeyDetectTask();
        daemon_dt = DWT_GetTimeline_ms() - daemon_start;
        if (daemon_dt > 10)
            LOGERROR("[freeRTOS] Daemon Task is being DELAY! dt = [%f]", &daemon_dt);
        osDelay(10);
    }
}

static __attribute__((noreturn)) void StartROBOTTASK(void const *argument)
{
    static float robot_dt;
    static float robot_start;
    LOGINFO("[freeRTOS] ROBOT core Task Start");
    // 200Hz-500Hz,若有额外的控制任务如平衡步兵可能需要提升至1kHz
    for (;;)
    {
        robot_start = DWT_GetTimeline_ms();
        RobotTask();
        robot_dt = DWT_GetTimeline_ms() - robot_start;
        if (robot_dt > 5)
            LOGERROR("[freeRTOS] ROBOT core Task is being DELAY! dt = [%f]", &robot_dt);
        osDelay(5);
    }
}

#if REFEREE_ENABLE && (defined(ONE_BOARD) || defined(CHASSIS_BOARD))
static __attribute__((noreturn)) void StartUITASK(void const *argument)
{
    LOGINFO("[freeRTOS] UI Task Start");
    MyUIInit();
    LOGINFO("[freeRTOS] UI Init Done, communication with ref has established");
    for (;;)
    {
        // 每给裁判系统发送一包数据会挂起一次,详见UITask函数的refereeSend()
        UITask();
        osDelay(1); // 即使没有任何UI需要刷新,也挂起一次,防止卡在UITask中无法切换
    }
}
#endif
