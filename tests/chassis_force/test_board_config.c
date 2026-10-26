#include "board_chassis_force.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const ChassisForceConfig_s c=BoardChassisForceConfig();
    assert(c.calibration_valid); /* Build deliberately sets the macro to 1. */
    assert(c.checksum==ChassisForceConfigChecksum(&c));
    assert(!ChassisForceConfigValid(&c)); /* Unmeasured values still prevent actuation. */
    for(unsigned w=0;w<4;++w) {
        assert(c.wheel[w].radius_m==RADIUS_WHEEL*.001f); /* Reuse existing vehicle geometry. */
        assert(c.wheel[w].k_out_nm_a==0);
        assert(c.wheel[w].current_limit==15000.0f); /* Reuse the original speed/PID command cap. */
    }
    puts("PASS: unmeasured board calibration stays explicit and locked even with VALID=1");
    return 0;
}
