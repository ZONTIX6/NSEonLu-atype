#include "dji_tuning_adapter.h"
#include "runtime_tuning.h"
#include "autotune_protocol.h"
#include "vehicle_safety.h"
#include "board_config.h"
#include "bsp_dwt.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>
#include <string.h>

/* Policy state belongs to the registered vehicle group, not the motor driver.
 * Cycle pointers are borrowed only during that same motor task iteration. */
static DJIMotorOutputGroup_s *tuning_group;
static DJIMotorInstance *const *tuning_motors;
static const DJIMotorCycle_s *tuning_cycle;
static const DJIMotorGroupControl_s *tuning_control;
static float *tuning_references;
static bool autotune_owned,autotune_inhibit;
static uint32_t autotune_trial;
static PIDInstance autotune_original[4];
static AutotuneDrive autotune_drive;

static void ApplyAutotuneCycle(void)
{
    if (!tuning_group) return;
    const uint32_t now=HAL_GetTick();
    if(tuning_group->owns_raw_current) {
        const PIDAutotuneFeedback denied={.now_ms=now,.permitted=false};
        AutotuneMotorStep(&denied,&autotune_drive);
        if(autotune_drive.owned) AutotuneCancel();
        autotune_owned=false;autotune_inhibit=false;
        return;
    }
    PIDAutotuneFeedback f={.now_ms=now,.can_ok=true,.feedback_ok=true,
        .permitted=VehicleSafetyAllowsAutotune(now) && tuning_control->autotune_permitted && tuning_control->control_enabled &&
            (uint32_t)(now-tuning_control->tick_ms)<=10 && !autotune_inhibit};
    for(unsigned w=0;w<4;++w) {
        unsigned i=tuning_group->motor_indices[w];
        f.rpm[w]=tuning_cycle->measure[i].speed_aps/6;
        f.rx_ms[w]=tuning_cycle->measure[i].rx_tick_ms;f.rx_sequence[w]=tuning_cycle->measure[i].rx_sequence;
        f.feedback_ok &= tuning_cycle->online[i] && (uint32_t)(now-tuning_cycle->measure[i].rx_tick_ms)<=20;
        f.permitted &= fabsf(tuning_control->target_rpm[w])<=5;
        PIDInstance *pid=autotune_owned ? &autotune_original[w] : &tuning_motors[i]->motor_controller.speed_PID;
        const Motor_Control_Setting_s *settings=&tuning_motors[i]->motor_settings;
        AutotuneConfiguration config={.improve=pid->Improve,.coef_a=pid->CoefA,.coef_b=pid->CoefB,
            .output_rc=pid->Output_LPF_RC,.derivative_rc=pid->Derivative_LPF_RC,
            .speed_smooth=SPEED_SMOOTH_COEF,.current_smooth=CURRENT_SMOOTH_COEF,
            .output_limit=fminf(pid->MaxOut,tuning_group->max_current_command),
            .motor_flags=(uint32_t)settings->close_loop_type | ((uint32_t)settings->outer_loop_type<<8) |
                ((uint32_t)settings->motor_reverse_flag<<16) | ((uint32_t)settings->feedback_reverse_flag<<20) |
                ((uint32_t)settings->speed_feedback_source<<24) | ((uint32_t)settings->feedforward_flag<<28)};
        AutotuneSetConfiguration(w,&config);
    }
    for (unsigned i=0;i<tuning_cycle->count;++i) {
        bool chassis=false;
        for(unsigned w=0;w<4;++w) chassis |= i==tuning_group->motor_indices[w];
        if (!chassis) f.permitted &= tuning_cycle->stop[i]==MOTOR_STOP && tuning_cycle->online[i] &&
            (uint32_t)(now-tuning_cycle->measure[i].rx_tick_ms)<=20 &&
            isfinite(tuning_cycle->measure[i].speed_aps) && fabsf(tuning_cycle->measure[i].speed_aps)<=300;
    }
    AutotuneMotorStep(&f,&autotune_drive);
    if(autotune_drive.owned && !autotune_owned) {
        for(unsigned w=0;w<4;++w) autotune_original[w]=tuning_motors[tuning_group->motor_indices[w]]->motor_controller.speed_PID;
        autotune_trial=0;
    }
    if(!autotune_drive.owned && autotune_owned) {
        for(unsigned w=0;w<4;++w) {
            PIDInstance *pid=&tuning_motors[tuning_group->motor_indices[w]]->motor_controller.speed_PID;
            *pid=autotune_original[w];DJIMotorResetPID(pid,tuning_cycle->measure[tuning_group->motor_indices[w]].speed_aps);
        }
        autotune_inhibit=true;
    }
    autotune_owned=autotune_drive.owned;
    if(autotune_owned && autotune_drive.running && autotune_drive.trial!=autotune_trial) {
        for(unsigned w=0;w<4;++w) {
            PIDInstance *pid=&tuning_motors[tuning_group->motor_indices[w]]->motor_controller.speed_PID;
            *pid=autotune_original[w];DJIMotorResetPID(pid,tuning_cycle->measure[tuning_group->motor_indices[w]].speed_aps);
        }
        PIDInstance *pid=&tuning_motors[tuning_group->motor_indices[autotune_drive.test.wheel]]->motor_controller.speed_PID;
        pid->Kp=autotune_drive.test.kp;pid->Ki=autotune_drive.test.ki;pid->Kd=autotune_drive.test.kd;
        pid->MaxOut=fminf(pid->MaxOut,autotune_drive.test.current_limit);
        pid->IntegralLimit=fminf(pid->IntegralLimit,pid->MaxOut);
        autotune_trial=autotune_drive.trial;
    }
    if(autotune_owned || autotune_inhibit) for(unsigned w=0;w<4;++w) {
        unsigned i=tuning_group->motor_indices[w];
        bool selected=autotune_owned && autotune_drive.running && w==autotune_drive.test.wheel;
        tuning_cycle->stop[i]=selected ? MOTOR_ENALBED : MOTOR_STOP;
        tuning_references[i]=selected ? autotune_drive.target_rpm*6 : 0;
    }
    if (autotune_owned || autotune_inhibit || ATYPE_BENCH_MODE) {
        for(unsigned i=0;i<tuning_cycle->count;++i) {
            bool chassis=false;
            for(unsigned w=0;w<4;++w) chassis |= i==tuning_group->motor_indices[w];
            if (!chassis) {tuning_cycle->stop[i]=MOTOR_STOP;tuning_references[i]=0;}
        }
    }
    if(autotune_inhibit && tuning_control->zero_force && (uint32_t)(now-tuning_control->tick_ms)<=10) autotune_inhibit=false;
}
static void RegisterPIDTuning(void)
{
    float values[24];
    for (unsigned i = 0; i < 4; ++i) {
        PIDInstance *pid = &tuning_motors[tuning_group->motor_indices[i]]->motor_controller.speed_PID;
        const float wheel[] = {pid->Kp, pid->Ki, pid->Kd, pid->MaxOut, pid->IntegralLimit, pid->DeadBand};
        memcpy(&values[i * 6], wheel, sizeof wheel);
    }
    RuntimeTuningRegister(TUNING_PID, values, tuning_group->owns_raw_current ? 1 : 0);
}
static bool ApplyPIDTuning(const float *v)
{
    if (!tuning_group || tuning_group->owns_raw_current) return false;
    for (unsigned i = 0; i < 4; ++i) {
        const float *w = &v[i * 6];
        for (unsigned j = 0; j < 6; ++j)
            if (!isfinite(w[j]) || w[j] < 0) return false;
        if (w[0] > 1000 || w[1] > 1000 || w[2] > 1000 ||
            w[3] <= 0 || w[3] > tuning_group->max_current_command ||
            w[4] > w[3] || w[5] > 10000) return false;
    }
    /* Runs in the motor task, before any wheel computes its next PID output. */
    for (unsigned i = 0; i < 4; ++i) {
        const float *w = &v[i * 6];
        PIDInstance *pid = &tuning_motors[tuning_group->motor_indices[i]]->motor_controller.speed_PID;
        pid->Kp=w[0]; pid->Ki=w[1]; pid->Kd=w[2];
        pid->MaxOut=w[3]; pid->IntegralLimit=w[4]; pid->DeadBand=w[5];
        pid->Iout=pid->ITerm=pid->Last_ITerm=0;
        pid->Dout=pid->Last_Dout=0;
        pid->Output=fminf(fmaxf(pid->Output, -pid->MaxOut), pid->MaxOut);
        pid->Last_Output=pid->Output;
    }
    return true;
}

static void Registered(DJIMotorOutputGroup_s *group, DJIMotorInstance *const *motors)
{
    tuning_group=group;tuning_motors=motors;
    RegisterPIDTuning();
    if(!group->owns_raw_current) RuntimeTuningRestore(TUNING_PID, ApplyPIDTuning);
}
static void Process(void) {RuntimeTuningProcess(TUNING_PID, ApplyPIDTuning);}
static void Prepare(const DJIMotorCycle_s *cycle, const DJIMotorGroupControl_s *control,
                    float *references)
{
    tuning_cycle=cycle;tuning_control=control;tuning_references=references;
    ApplyAutotuneCycle();
}
static void Gate(const DJIMotorCycle_s *cycle)
{
    tuning_cycle=cycle;
    /* A new stop/control message can arrive while PID/power calculations run. */
    if(tuning_group && (autotune_owned || autotune_inhibit)) {
        taskENTER_CRITICAL();
        bool permitted=VehicleSafetyAllowsAutotune(HAL_GetTick()) && tuning_group->control.autotune_permitted && tuning_group->control.control_enabled &&
            (uint32_t)(HAL_GetTick()-tuning_group->control.tick_ms)<=10;
        taskEXIT_CRITICAL();
        if(!permitted) {
            if(autotune_owned) AutotuneCancel();
            for(unsigned w=0;w<4;++w) {unsigned i=tuning_group->motor_indices[w];tuning_cycle->stop[i]=MOTOR_STOP;tuning_cycle->current[i]=0;}
        }
    }

}
static void Cancel(void) {if(autotune_owned) AutotuneCancel();}
static void Record(const DJIMotorCycle_s *cycle, const float *raw_output, bool transmitted)
{
    tuning_cycle=cycle;
    if(tuning_group && autotune_owned) {
        unsigned w=autotune_drive.test.wheel,i=tuning_group->motor_indices[w];
        AutotuneObserve(HAL_GetTick(),tuning_cycle->measure[i].speed_aps/6,raw_output[i],
            tuning_cycle->current[i],fabsf(tuning_cycle->current[i])>=tuning_motors[i]->motor_controller.speed_PID.MaxOut*.98f,transmitted);
    }
}
const DJIMotorControlPolicy_s *DJITuningAdapterPolicy(void)
{
    static const DJIMotorControlPolicy_s policy={.registered=Registered,.process=Process,
        .prepare=Prepare,.gate=Gate,.cancel=Cancel,.record=Record};
    return &policy;
}
