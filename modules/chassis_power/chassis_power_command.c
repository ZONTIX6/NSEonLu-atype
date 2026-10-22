#include "chassis_power_policy.h"
#include <math.h>
#include <stddef.h>

bool ChassisPowerBuildCommand(const ChassisPowerCommandInput_s *input,
                              ChassisPowerCommandOutput_s *output)
{
    if (input == NULL || output == NULL) return false;
    ChassisPowerCommandOutput_s result={0};
    if(input->referee.valid) {
        result.command.enable_dcdc=input->referee.chassis_output_enabled &&
            input->referee.chassis_power_limit_w>0 && !input->chassis_zero_force;
        result.command.referee_power_limit_w=input->referee.chassis_power_limit_w;
        result.command.referee_buffer_energy_j=input->referee.buffer_energy_j;
        result.base_power_limit_w=input->referee.chassis_power_limit_w;
    } else if(input->bench_enabled && isfinite(input->bench_limit_w) && input->bench_limit_w>0) {
        result.command.enable_dcdc=!input->chassis_zero_force &&
            (!input->referee.chassis_output_valid || input->referee.chassis_output_enabled);
        result.command.referee_power_limit_w=input->bench_limit_w>=UINT16_MAX ?
            UINT16_MAX : (uint16_t)(input->bench_limit_w+.5f);
        result.base_power_limit_w=input->bench_limit_w;
    }
    *output=result;
    return true;
}
