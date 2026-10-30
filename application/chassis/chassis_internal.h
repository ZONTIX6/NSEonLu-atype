#ifndef CHASSIS_INTERNAL_H
#define CHASSIS_INTERNAL_H

/* Private to this application: never include from another application/module.
 * The public chassis.h remains the only interface for external consumers. */
#include "chassis.h"
#include "robot_def.h"
#include "dji_motor.h"
#include "chassis_power.h"
#include "chassis_spin.h"
#include "chassis_spin_config.h"
#include "message_center.h"

#define CHASSIS_POWER_SAFETY_FACTOR 0.98f

/* Device/message registrations owned by chassis_init.c. */
extern DJIMotorInstance *motor_lf, *motor_rf, *motor_lb, *motor_rb;
extern bool chassis_power_ready;
extern Publisher_t *heat_pub;
#ifdef ONE_BOARD
extern Publisher_t *chassis_pub;
extern Subscriber_t *chassis_sub;
#endif
#ifdef CHASSIS_BOARD
#include "can_comm.h"
extern CANCommInstance *chasiss_can_comm;
#endif

/* Control state owned by chassis.c, shared only for initialization/retuning. */
extern Chassis_Ctrl_Cmd_s chassis_cmd_recv;
extern uint32_t chassis_budget_dwt_count;
#if CHASSIS_SPIN_DYNAMIC_ENABLED
extern ChassisSpinState_s chassis_spin_state;
#endif

/* Validated configuration owned by chassis_tuning.c. */
extern ChassisSpinConfig_s chassis_spin_config;
extern ChassisPowerBudgetConfig_s chassis_budget_config;
extern ChassisPowerOuterLoopConfig_s chassis_outer_loop_config;
extern ChassisPower_Init_Config_s power_module_config;
void ChassisTuningRegister(void);
bool ChassisTuningApplyPower(const float *values);
bool ChassisTuningApplySpin(const float *values);

#endif
