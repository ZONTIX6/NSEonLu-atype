#include "dji_power_adapter.h"
#include "bsp_dwt.h"
#include "power_meter.h"
#include "chassis_power_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

static DJIMotorOutputGroup_s adapter_group;
static DJIMotorInstance *adapter_motors[4];
static ChassisPowerControlSnapshot_s adapter_control, adapter_cycle_control;
static uint32_t adapter_dwt_count;
static ChassisPowerCycleInput_s adapter_input;
static ChassisPowerCycleOutput_s adapter_output;

static void AdapterReadControl(void *context)
{
    (void)context;
    adapter_cycle_control=adapter_control;
}

static void AdapterLimit(void *context,const DJIMotorCycle_s *cycle)
{
    (void)context;
    ChassisPowerCycleInput_s *input=&adapter_input;
    *input=(ChassisPowerCycleInput_s){.control=adapter_cycle_control,
        .dt_s=DWT_GetDeltaT(&adapter_dwt_count)};
#if POWER_METER_ENABLE
    (void)PowerMeterGetStatus(&input->meter);
#endif
    input->now_ms=HAL_GetTick();
    for(unsigned w=0;w<4;++w) {
        const unsigned index=adapter_group.motor_indices[w];
        input->raw_current[w]=cycle->current[index];
        input->actual_rpm[w]=cycle->actual_aps[index]/6.0f;
        input->target_rpm[w]=cycle->target_aps[index]/6.0f;
        input->feedback_current[w]=cycle->measure[index].real_current;
        input->feedback_tick_ms[w]=cycle->measure[index].rx_tick_ms;
        input->feedback_sequence[w]=cycle->measure[index].rx_sequence;
        if(cycle->online[index]) input->online_mask |= (uint8_t)(1U<<w);
        if(cycle->stop[index]!=MOTOR_STOP) input->enabled_mask |= (uint8_t)(1U<<w);
    }
    (void)ChassisPowerLimit(input,&adapter_output);
    for(unsigned w=0;w<4;++w) cycle->current[adapter_group.motor_indices[w]]=adapter_output.current[w];
}

static void AdapterRecordFinal(void *context,const DJIMotorCycle_s *cycle,
    const int16_t commands[DJI_MOTOR_CNT],const bool enabled[DJI_MOTOR_CNT],
    uint8_t submitted_groups,uint8_t successful_groups)
{
    (void)context;
    ChassisPowerFinalInput_s final={.now_ms=HAL_GetTick(),.control_version=adapter_cycle_control.version,
        .can_submitted=true};
    for(unsigned w=0;w<4;++w) {
        const unsigned index=adapter_group.motor_indices[w];
        const uint8_t group=(uint8_t)(1U<<cycle->motors[index]->sender_group);
        final.current_command[w]=commands[index];
        final.can_submitted &= (submitted_groups & successful_groups & group)!=0;
        if(enabled[index]) final.enabled_mask |= (uint8_t)(1U<<w);
    }
    (void)ChassisPowerRecordFinal(&final);
}

bool DJIPowerAdapterInit(const DJIPowerAdapterConfig_s *config)
{
    if(config==NULL || config->count!=4 || adapter_group.registered) return false;
    DJIMotorOutputGroup_s group={.max_current_command=config->max_current_command};
    ChassisPowerSnapshot_s power;
    if(!ChassisPowerGetSnapshot(&power)) return false;
    for(unsigned w=0;w<4;++w) {
        if(config->motors[w]==NULL || config->motors[w]->motor_type!=M3508 ||
            !DJIMotorFindIndex(config->motors[w],&group.motor_indices[w])) return false;
        for(unsigned j=0;j<w;++j) if(config->motors[w]==config->motors[j]) return false;
    }
    adapter_group=group;
    const DJIMotorOutputHooks_s hooks={.read_control=AdapterReadControl,.limit=AdapterLimit,
        .record_final=AdapterRecordFinal};
    if(!DJIMotorRegisterOutputGroup(&adapter_group,&hooks)) return false;
    memcpy(adapter_motors,config->motors,sizeof adapter_motors);
    (void)DWT_GetDeltaT(&adapter_dwt_count);
    RTTTelemetryWriteChassisPowerHeader();
    return true;
}

bool DJIPowerAdapterPublish(const ChassisPowerControlSnapshot_s *control)
{
    if(control==NULL || !adapter_group.registered) return false;
    taskENTER_CRITICAL();
    adapter_control=*control;
    adapter_group.control=(DJIMotorGroupControl_s){.version=control->version,.tick_ms=control->tick_ms,
        .control_enabled=control->control_enabled,.autotune_permitted=control->autotune_permitted,
        .zero_force=control->zero_force};
    memcpy(adapter_group.control.target_rpm,control->target_rpm,sizeof control->target_rpm);
    for(unsigned w=0;w<4;++w) {
        adapter_motors[w]->motor_controller.pid_ref=control->target_rpm[w]*6.0f;
        adapter_motors[w]->stop_flag=control->control_enabled ? MOTOR_ENALBED : MOTOR_STOP;
    }
    taskEXIT_CRITICAL();
    return true;
}
