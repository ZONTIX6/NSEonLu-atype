#include "referee_power.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const uint8_t status[17] = {1,1,0,0,0,0,20,0,200,0,80,0,2,0,0,0,2};
    const uint8_t heat[16] = {0,0,0,0,0,0,0,0,60,0,11,0,22,0,33,0};
    for (unsigned extended=0; extended<2; ++extended) {
        RefereePowerState_s state = {0};
        RefereeHeatSnapshot_s snapshot;
        assert(RefereeHeatBuildSnapshot(&state, 0, &snapshot) && !snapshot.valid);
        assert(RefereePowerStateAcceptFrame(&state,0x0201,status,extended?17:13,100));
        assert(RefereePowerStateAcceptFrame(&state,0x0202,heat,extended?16:14,100));
        assert(RefereeHeatBuildSnapshot(&state,400,&snapshot) && snapshot.valid);
        assert(snapshot.current_heat==(extended?22:11));
        assert(snapshot.heat_limit==200 && snapshot.cooling_rate==20);
        assert(snapshot.robot_state_tick_ms==100 && snapshot.power_heat_tick_ms==100);
        assert(RefereeHeatBuildSnapshot(&state,401,&snapshot) && !snapshot.valid);
        assert(RefereePowerStateAcceptFrame(&state,0x0202,heat,extended?16:14,401));
        assert(RefereeHeatBuildSnapshot(&state,401,&snapshot) && !snapshot.valid);
        assert(RefereePowerStateAcceptFrame(&state,0x0201,status,extended?17:13,401));
        assert(RefereeHeatBuildSnapshot(&state,401,&snapshot) && snapshot.valid);
        state.robot_state_tick_ms=state.power_heat_tick_ms=0xfffffffeU;
        assert(RefereeHeatBuildSnapshot(&state,2,&snapshot) && snapshot.valid);
        state.heat_limit=0;
        assert(RefereeHeatBuildSnapshot(&state,2,&snapshot) && !snapshot.valid);
    }
    puts("PASS: referee heat layouts, independent expiry and recovery");
}
