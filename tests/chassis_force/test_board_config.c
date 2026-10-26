#include "board/board_chassis_force.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ChassisForceConfig_s c=BoardChassisForceConfig();
    assert(c.calibration_valid); /* Build deliberately sets the macro to 1. */
    assert(c.checksum==ChassisForceConfigChecksum(&c));
    assert(!ChassisForceConfigValid(&c)); /* Unmeasured values still prevent actuation. */
    for(unsigned w=0;w<4;++w) {
        assert(c.wheel[w].radius_m==RADIUS_WHEEL*.001f); /* Reuse existing vehicle geometry. */
        assert(c.wheel[w].k_out_nm_a==0);
        assert(c.wheel[w].current_limit==15000.0f); /* Reuse the original speed/PID command cap. */
        assert(c.wheel[w].current_slew>0);
        assert(c.wheel[w].force_limit_n==0); /* Tire/ground capability is still unmeasured. */
    }
    assert(c.force_max_n>0 && c.spin_torque_nm>0 && c.spin_start_torque_nm>0);
    assert(c.spin_start_torque_nm<=c.spin_torque_max_nm && c.spin_torque_nm<=c.spin_torque_max_nm);
    assert(c.force_rise_n_s>0 && c.force_fall_n_s>0 && c.torque_rise_nm_s>0 && c.torque_fall_nm_s>0);
    assert(c.velocity_limit_mps>0 && c.omega_limit_rad_s>0 && c.wheel_limit_rpm>0);
    assert(c.startup_current_limit>0 && c.stall_current_command>0 && c.stall_rpm>0 && c.stall_timeout_ms>0);
    /* Stall detection must remain reachable under the near-zero startup cap. */
    assert(c.stall_current_command<=c.startup_current_limit && c.startup_current_limit<=c.wheel[0].current_limit);
    assert(c.temp_derate_c>0 && c.temp_stop_c>c.temp_derate_c && c.temp_stop_c<=100);
    assert(c.brake_torque_nm>0 && c.brake_torque_nm<=c.spin_torque_max_nm);
    assert(c.brake_stop_rad_s>0 && c.brake_stop_rad_s<c.omega_limit_rad_s && c.brake_timeout_ms>0);
    assert(c.speed_prediction_rpm==0); /* A guessed prediction envelope cannot pass calibration. */
    /* Synthetic test data only: filling exactly the remaining measurement groups
     * must leave no accidental zero-valued tuning fields behind. */
    for(unsigned w=0;w<4;++w) {c.wheel[w].k_out_nm_a=.3f;c.wheel[w].force_limit_n=5;}
    c.speed_prediction_rpm=100;c.checksum=ChassisForceConfigChecksum(&c);
    assert(ChassisForceConfigValid(&c));
    puts("PASS: unmeasured board calibration stays explicit and locked even with VALID=1");
    return 0;
}
