#ifndef CHASSIS_FORCE_CONTROL_H
#define CHASSIS_FORCE_CONTROL_H
#include "chassis_force_geometry.h"
#include "chassis_force_allocator.h"
#include "chassis_force_command.h"
#include "chassis_force_spin_power.h"
typedef enum {
    CHASSIS_FORCE_DISABLED,CHASSIS_FORCE_UNCALIBRATED,CHASSIS_FORCE_READY,
    CHASSIS_FORCE_RUN,CHASSIS_FORCE_LIMITED,CHASSIS_FORCE_EXIT_BRAKING,CHASSIS_FORCE_FAULT
} ChassisForceState_e;
enum {
    CHASSIS_FORCE_ERROR_NONE=0,CHASSIS_FORCE_ERROR_NUMERIC=1,CHASSIS_FORCE_ERROR_AGE=2,
    CHASSIS_FORCE_ERROR_TIMING=4,CHASSIS_FORCE_ERROR_CAN=8,CHASSIS_FORCE_ERROR_CONFIG=16,
    CHASSIS_FORCE_ERROR_THERMAL=32,CHASSIS_FORCE_ERROR_STALL=64,CHASSIS_FORCE_ERROR_PROTOCOL=128
};
enum { CHASSIS_FORCE_LIMIT_ALLOCATION=1,CHASSIS_FORCE_LIMIT_SPEED=2,
    CHASSIS_FORCE_LIMIT_THERMAL=4,CHASSIS_FORCE_LIMIT_SLEW=8,CHASSIS_FORCE_LIMIT_INTEGER=16,
    CHASSIS_FORCE_LIMIT_ZERO_POWER=32,CHASSIS_FORCE_LIMIT_METER=64 };
typedef struct {
    ChassisForceIntent_s intent;
    uint32_t now_ms,command_received_ms,config_version,safety_epoch,transport_margin_ms;
    float dt_s,actual_rpm[4],feedback_current[4];
    uint32_t feedback_tick_ms[4];
    uint8_t temperature[4],online_mask,enabled_mask;
    bool control_enabled,growth_allowed,meter_valid,offer_allowed;
    uint32_t policy_version,meter_tick_ms;
    uint32_t external_faults;
    float load_power_w,physical_target_w,model_budget_w;
    MotorPowerModelConfig_s models[4];
    uint32_t (*clock_us)(void *context);
    void *clock_context;
} ChassisForceCycleInput_s;
typedef struct {
    uint32_t command_seq,cycle_seq,config_version,safety_epoch,tick_ms,policy_version;
    uint32_t calibration_checksum;
    ChassisForceState_e state;
    uint32_t faults,limits;
    float dt_s,force_g[2],wrench_request[3],wrench_sent_est[3],velocity[3],residual;
    float raw_current[4],allocated_current[4],alpha,beta,predicted_w;
    float raw_predicted_w[4],final_predicted_w[4];
    float torque_nm,physical_target_w,load_power_w,offered_power_w,offered_torque_nm,operating_power_w;
    float actual_rpm[4],feedback_current[4];
    uint8_t temperature[4],drive_mask,brake_mask;
    uint32_t feedback_age_ms[4],command_age_ms,source_age_ms,yaw_age_ms;
    int16_t current[4];
    bool valid,can_submitted,braking,brake_timed_out,growth_permitted,zero_over_budget;
    bool calibrated;
    bool feedback_valid;
    uint32_t estimate_us,command_us,allocation_us,guard_us,input_us,can_us;
} ChassisForceCycleOutput_s;
typedef struct {
    ChassisForceConfig_s config;
    ChassisForceGeometry_s geometry;
    ChassisForceCommandState_s command;
    ChassisForceSpinPowerState_s spin;
    ChassisForceAllocationInput_s hard_constraints;
    ChassisForceCycleOutput_s output;
    float sent_current[4],last_budget_w;
    uint32_t cycle_seq,faults,last_sent_ms,stall_since_ms[4],offer_since_ms,offer_policy_version;
    uint32_t config_version,safety_epoch;
    unsigned recovery_samples;
    int8_t spin_direction;
    uint8_t wheel_overspeed,stall_active;
    bool calibrated,clock_started,history_valid,last_submission_ok,pending_record;
    bool translation_overspeed,rotation_overspeed,offer_pending,offer_timed_out;
} ChassisForceController_s;
bool ChassisForceControlInit(const ChassisForceConfig_s *config,ChassisForceController_s *state);
bool ChassisForceControlStep(ChassisForceController_s *state,const ChassisForceCycleInput_s *input,ChassisForceCycleOutput_s *output);
bool ChassisForceControlValidateFinal(const ChassisForceController_s *state,const int16_t current[4]);
bool ChassisForceControlRecord(ChassisForceController_s *state,uint32_t cycle_seq,const int16_t current[4],bool submitted,uint32_t now_ms);
void ChassisForceControlCancel(ChassisForceController_s *state,uint32_t faults);
#endif
