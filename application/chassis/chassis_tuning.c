#include "chassis_internal.h"
#include "chassis_energy_config.h"
#include "chassis_power_config.h"
#include "runtime_tuning.h"
#include <math.h>

ChassisPower_Init_Config_s power_module_config;
static uint32_t power_config_version;
void ChassisTuningRegister(void)
{
    const ChassisPowerOuterLoopConfig_s *o = &chassis_outer_loop_config;
    const ChassisPowerBudgetConfig_s *b = &chassis_budget_config;
    const float power[] = {o->kp, o->ki, o->measurement_lpf_tau_s,
        o->activation_margin_w, o->error_deadband_w, o->max_positive_correction_w,
        o->positive_slew_rate_w_per_s, o->budget_drop_reset_w, o->antiwindup_gain_per_s,
        o->low_demand_delay_s, o->low_demand_decay_w_per_s,
        b->max_boost_w, b->rise_rate_w_per_s, b->demand_margin_w,
        b->empty_energy_ratio, b->full_energy_ratio, b->buffer.max_boost_w,
        b->buffer.recovery_power_w, b->buffer.recovery_target_time_s};
    RuntimeTuningRegister(TUNING_POWER, power, CHASSIS_POWER_CONTROL_STAGE);
    const ChassisSpinConfig_s *s = &chassis_spin_config;
    const float spin[] = {s->fixed_wz, s->min_wz, s->max_wz, s->gain_wz_per_w_s,
        s->rise_wz_per_s, s->fall_wz_per_s, s->power_lpf_tau_s, s->power_deadband_w,
        s->settle_time_s, s->budget_drop_reset_w, s->sine_amplitude_wz,
        s->sine_period_s, s->sine_ramp_s, s->sine_rise_wz_per_s,
        s->sine_fall_wz_per_s, (float)s->sine_direction};
    RuntimeTuningRegister(TUNING_SPIN, spin, CHASSIS_SPIN_MODE);
}
bool ChassisTuningApplyPower(const float *v)
{
    const float upper[] = {100, 100, 10, 1000, 100, 1000, 10000, 1000,
        1000, 60, 1000, 1000, 10000, 1000, 1, 1, 1000, 1000, 60};
    for (unsigned i = 0; i < 19; ++i)
        if (!isfinite(v[i]) || v[i] < 0 || v[i] > upper[i]) return false;
    ChassisPowerOuterLoopConfig_s outer = {
        .kp=v[0], .ki=v[1], .measurement_lpf_tau_s=v[2],
        .activation_margin_w=v[3], .error_deadband_w=v[4],
        .max_positive_correction_w=v[5], .positive_slew_rate_w_per_s=v[6],
        .budget_drop_reset_w=v[7], .antiwindup_gain_per_s=v[8],
        .low_demand_delay_s=v[9], .low_demand_decay_w_per_s=v[10]};
    ChassisPowerBudgetConfig_s budget = chassis_budget_config;
    budget.max_boost_w=v[11]; budget.rise_rate_w_per_s=v[12];
    budget.demand_margin_w=v[13]; budget.empty_energy_ratio=v[14];
    budget.full_energy_ratio=v[15]; budget.buffer.max_boost_w=v[16];
    budget.buffer.recovery_power_w=v[17]; budget.buffer.recovery_target_time_s=v[18];
    if (!ChassisPowerOuterLoopConfigIsValid(&outer) ||
        !ChassisPowerBudgetConfigIsValid(&budget) ||
        !ChassisBufferBudgetConfigIsValid(&budget.buffer)) return false;
    if (chassis_power_ready) {
        ChassisPower_Init_Config_s next=power_module_config;
        next.outer=outer;next.budget=budget;
        if (!ChassisPowerRequestConfig(&next,power_config_version+1)) return false;
        ++power_config_version;power_module_config=next;
    }
    chassis_outer_loop_config = outer;
    chassis_budget_config = budget;
    return true;
}
bool ChassisTuningApplySpin(const float *v)
{
    const float upper[] = {8000,8000,8000,1000,10000,10000,10,1000,60,
        1000,6000,120,60,10000,10000};
    for (unsigned i = 0; i < 15; ++i)
        if (!isfinite(v[i]) || v[i] < 0 || v[i] > upper[i]) return false;
    if (v[15] != -1 && v[15] != 1) return false;
    ChassisSpinConfig_s spin = {
        .fixed_wz=v[0], .min_wz=v[1], .max_wz=v[2], .gain_wz_per_w_s=v[3],
        .rise_wz_per_s=v[4], .fall_wz_per_s=v[5], .power_lpf_tau_s=v[6],
        .power_deadband_w=v[7], .settle_time_s=v[8], .budget_drop_reset_w=v[9],
        .sine_amplitude_wz=v[10], .sine_period_s=v[11], .sine_ramp_s=v[12],
        .sine_rise_wz_per_s=v[13], .sine_fall_wz_per_s=v[14],
        .sine_direction=(int8_t)v[15]};
    /* Persisted/runtime payload remains exactly 16 legacy fields. Calibration
     * can only come from this firmware, never from bytes after old Flash data. */
    spin.extension_enabled=CHASSIS_SPIN_EXTENSION_ENABLE!=0;
    spin.hard_wz=CHASSIS_SPIN_HARD_WZ;
    spin.body_hard_rps=CHASSIS_SPIN_BODY_HARD_RPS;
    spin.wheel_hard_rpm=CHASSIS_SPIN_WHEEL_HARD_RPM;
    spin.extension_rise_wz_per_s=CHASSIS_SPIN_EXTENSION_RISE_RATE;
    if (!ChassisSpinConfigIsValid(&spin)) return false;
#if CHASSIS_SPIN_DYNAMIC_ENABLED
    if (chassis_cmd_recv.chassis_mode == CHASSIS_ROTATE) {
        if (spin.sine_direction != chassis_spin_config.sine_direction ||
            !ChassisSpinRetune(&spin, &chassis_spin_state)) return false;
    } else chassis_spin_state = (ChassisSpinState_s){0};
#endif
    chassis_spin_config = spin;
    return true;
}

