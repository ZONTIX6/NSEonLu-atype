#include "cmsis_os.h"
#include "task.h"
#include "robot_task.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static osThreadDef_t tasks[3];
static unsigned count,ins_init,ins_runs,telemetry_init,telemetry_runs,optional,usb,terminated;
static jmp_buf yielded;
osThreadId osThreadCreate(const osThreadDef_t *definition,void *argument)
{
    assert(argument==NULL && count<3);
    tasks[count]=*definition;
    return &tasks[count++];
}
void INS_Init(void) { ++ins_init; }
void INS_Task(void) { ++ins_runs; }
void RobotTelemetryInit(void) { ++telemetry_init; }
void RobotTelemetryTask(void) { ++telemetry_runs; }
TickType_t xTaskGetTickCount(void) { return 123; }
void osDelay(uint32_t ms) { assert(ms==1); longjmp(yielded,1); }
void vTaskDelayUntil(TickType_t *previous,TickType_t period)
{
    assert(*previous==123 && period==20);
    longjmp(yielded,1);
}
void BoardOptionalInit(void) { ++optional; }
void MX_USB_DEVICE_Init(void) { ++usb; }
void osThreadTerminate(osThreadId thread) { assert(thread==NULL); ++terminated; }
void StartDefaultTask(void const *argument);

int main(void)
{
    OSTaskInit();
    assert(count==2);
    assert(strcmp(tasks[0].name,"imucapture")==0 && tasks[0].priority==osPriorityAboveNormal);
    assert(strcmp(tasks[1].name,"telemetry")==0 && tasks[1].priority==osPriorityBelowNormal);
    if (!setjmp(yielded)) tasks[0].function(NULL);
    assert(ins_init==1 && ins_runs==1 && telemetry_runs==0);
    if (!setjmp(yielded)) tasks[1].function(NULL);
    assert(telemetry_init==1 && telemetry_runs==1);
    StartDefaultTask(NULL);
    assert(usb==1 && optional==0 && terminated==1);
    puts("PASS: IMU-only task set and optional-device isolation");
    return 0;
}
