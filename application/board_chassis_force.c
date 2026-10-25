#include "board_chassis_force.h"

/* Vehicle calibration boundary. Fill measured physical limits and output-shaft
 * torque constants here, review CAN signs/COM, then enable the calibration flag.
 * Zero physical defaults intentionally prevent actuation even with FLAG=1. */
ChassisForceConfig_s BoardChassisForceConfig(void)
{
    ChassisForceConfig_s c=ChassisForceDefaultConfig();
    const float direction[4]={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION};
    const float dx[4]={-.7071067812f,.7071067812f,-.7071067812f,.7071067812f};
    const float dy[4]={.7071067812f,.7071067812f,-.7071067812f,-.7071067812f};
    for(unsigned w=0;w<4;++w) {
        c.wheel[w].x_m=(w<2 ? 1 : -1)*WHEEL_BASE*.0005f;
        c.wheel[w].y_m=(w%2==0 ? 1 : -1)*TRACK_WIDTH*.0005f;
        c.wheel[w].dx=dx[w]*direction[w];c.wheel[w].dy=dy[w]*direction[w];
        c.wheel[w].radius_m=RADIUS_WHEEL*.001f;c.wheel[w].reduction=REDUCTION_RATIO_WHEEL;
    }
    c.checksum=ChassisForceConfigChecksum(&c);return c;
}

ChassisPower_Init_Config_s BoardChassisForcePowerConfig(void)
{
    ChassisPower_Init_Config_s c={.stage=3,.force_control=true,.meter_enabled=POWER_METER_ENABLE!=0,
        .buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0,
        .use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP,
        .algorithm={.safety_factor=.98f,.small_error_threshold_rpm=500,.reserved_power_threshold_w=54,
            .per_motor_reserved_power_w=8,.max_current_command=15000},
        .transient=ChassisPowerTransientDefaultConfig(),
        .outer={.kp=1,.ki=1.5f,.measurement_lpf_tau_s=.030f,.activation_margin_w=5,.error_deadband_w=.3f,
            .max_positive_correction_w=8,.positive_slew_rate_w_per_s=60,.budget_drop_reset_w=1,
            .antiwindup_gain_per_s=10,.low_demand_delay_s=.5f,.low_demand_decay_w_per_s=1},
        .budget={.empty_energy_ratio=.1f,.full_energy_ratio=.3f,.max_boost_w=100,.rise_rate_w_per_s=200,
            .demand_margin_w=CHASSIS_POWER_DEMAND_MARGIN_W,
            .buffer={.max_energy_j=CHASSIS_BUFFER_MAX_ENERGY_J,.recovery_enter_j=CHASSIS_BUFFER_RECOVERY_ENTER_J,
                .recovery_exit_j=CHASSIS_BUFFER_RECOVERY_EXIT_J,.hard_floor_j=CHASSIS_BUFFER_HARD_FLOOR_J,
                .prediction_window_s=CHASSIS_BUFFER_PREDICTION_WINDOW_S,.guard_window_s=CHASSIS_BUFFER_GUARD_WINDOW_S,
                .settlement_delay_s=CHASSIS_BUFFER_SETTLEMENT_DELAY_S,.energy_margin_j=CHASSIS_BUFFER_ENERGY_MARGIN_J,
                .power_margin_w=CHASSIS_BUFFER_POWER_MARGIN_W,.max_boost_w=CHASSIS_BUFFER_BOOST_MAX_W,
                .recovery_power_w=CHASSIS_BUFFER_RECOVERY_POWER_W,.recovery_target_time_s=CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S,
                .observer_max_step_s=CHASSIS_BUFFER_OBSERVER_MAX_STEP_S,.recovery_hold_s=CHASSIS_BUFFER_RECOVERY_HOLD_S,
                .rise_rate_w_per_s=CHASSIS_BUFFER_RISE_RATE_W_PER_S,.safety_factor=.98f,
                .correction_rise_j_per_s=CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S}}};
    const MotorPowerModelConfig_s m={.k0=1.3123816f,.k1=-.25836727f,.k2=.0007424524f,
        .k3=.0024271158f,.k4=.081198552f,.k5=3.890596e-8f,.current_conversion=1000};
    for(unsigned w=0;w<4;++w) c.models[w]=m;
    return c;
}
