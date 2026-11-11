#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "robot.h"

static unsigned bsp, imu, tasks, other,can;
static bool irq_disabled;
void __disable_irq(void) { assert(!irq_disabled); irq_disabled=true; }
void __enable_irq(void) { assert(irq_disabled); irq_disabled=false; }
void BSPInit(void) { assert(irq_disabled); ++bsp; }
bool BoardDevicesInit(void) { assert(irq_disabled && bsp==1); ++imu; return true; }
void OSTaskInit(void) { assert(irq_disabled && imu==1); ++tasks; }
void ChassisImuCanInit(void) { assert(irq_disabled && imu==1 && tasks==0); ++can; }
void TuningFlashInit(void) { ++other; }
void TuningPolicyInit(void) { ++other; }
int JScopeInit(void) { ++other; return 0; }
void RobotCMDInit(void) { ++other; }
void GimbalInit(void) { ++other; }
void ShootInit(void) { ++other; }
void ChassisInit(void) { ++other; }
void RobotCMDTask(void) { ++other; }
void GimbalTask(void) { ++other; }
void ShootTask(void) { ++other; }
void ChassisTask(void) { ++other; }

int main(void)
{
    RobotInit();
    assert(bsp==1 && imu==1 && tasks==1 && !irq_disabled);
    assert(other==0);
#if IMU_CAPTURE_CAN_TX_ENABLE
    assert(can==1);
#else
    assert(can==0);
#endif
    /* Even an accidental caller must not execute actuator applications. */
    RobotTask();
    assert(other==0);
    puts("PASS: IMU-only startup excludes robot applications and tuning");
    return 0;
}
