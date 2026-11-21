#include "vofa_observation.h"

#if VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE == 3
#include "../../modules/motor/DJImotor/dji_traction.h"
#include "../../config/30_velocity_observation/chassis_traction_config.h"
#include "usart.h"
#include <limits.h>
#include <math.h>
#include <string.h>

enum { TRACTION_CHANNELS=72, TRACTION_SCHEMA=10 };
static struct { float channels[TRACTION_CHANNELS]; uint8_t tail[4]; }
    frame={.tail={0,0,128,127}};
_Static_assert(sizeof(float)==4 && sizeof frame==(TRACTION_CHANNELS+1U)*4U,"Traction frame layout");
_Static_assert(sizeof frame*10U*1000U<BOARD_UART7_BAUDRATE*20U,"Traction wire budget");

static void U32(float *f,unsigned index,uint32_t value)
{
    f[index]=(float)(value>>16);f[index+1]=(float)(value&UINT16_MAX);
}
static float Age(uint32_t now,uint32_t tick,bool known)
{
    const uint32_t age=now-tick;
    return known && age<=INT32_MAX ? (float)age:-1;
}

void VofaObservationSend(const USBTelemetrySample_s *sample,
    const ChassisPowerSnapshot_s *power,const ChassisFusedVelocitySnapshot_s *velocity,
    const SuperCapSnapshot_s *cap,const RefereePowerSnapshot_s *referee,uint32_t now_ms)
{
    (void)sample;(void)velocity;(void)cap;(void)referee;
    static bool started;
    static uint32_t last_ms,sequence;
    const uint32_t elapsed=now_ms-last_ms;
    if(started && elapsed<20U) return;
    last_ms=started ? now_ms-elapsed%20U:now_ms;started=true;
    const uint32_t this_sequence=sequence++;
    UART_HandleTypeDef *uart=BOARD_VOFA_UART;
    if(uart->gState!=HAL_UART_STATE_READY) return;
    static DJITractionSnapshot_s snapshot;
    const bool known=DJIPowerAdapterGetTractionSnapshot(&snapshot);
    const bool fresh=known && (uint32_t)(now_ms-snapshot.tick_ms)<=20U;
    const ChassisTractionOutput_s *o=&snapshot.traction;
    const bool power_fresh=power && power->registered && power->final_valid &&
        power->control_version==snapshot.control_version && (uint32_t)(now_ms-power->final_tick_ms)<=20U;
    float *f=frame.channels;
    memset(f,0,sizeof frame.channels);
    f[0]=TRACTION_SCHEMA;U32(f,1,now_ms);U32(f,3,this_sequence);
    f[5]=CHASSIS_TRACTION_MODE;
    f[6]=(known ? 1U:0U) | (fresh ? 2U:0U) | (fresh && o->valid ? 4U:0U) |
        (fresh && snapshot.can_submitted ? 8U:0U) | (CHASSIS_TRACTION_CALIBRATED ? 16U:0U) |
        (power_fresh ? 32U:0U);
    f[7]=known ? o->reason:CHASSIS_TRACTION_NO_REFERENCE;
    f[8]=o->spin_mask;f[9]=o->lock_mask;f[10]=o->intervened_mask;f[11]=o->active_brake_mask;
    U32(f,12,snapshot.cycles);U32(f,14,snapshot.intervention_cycles);
    U32(f,16,snapshot.submitted_intervention_cycles);
    f[18]=Age(now_ms,snapshot.reference.tick_ms,known && snapshot.reference.valid);
    f[19]=Age(now_ms,snapshot.reference.gyro_tick_ms,known && snapshot.reference.valid);
    for(unsigned w=0;w<4;++w) {
        f[20+w]=o->expected_mps[w];f[24+w]=o->actual_mps[w];
        f[28+w]=o->slip_mps[w];f[32+w]=snapshot.raw_current[w];
        f[36+w]=o->proposed_current[w];f[40+w]=snapshot.final_current[w];
        f[44+w]=snapshot.target_rpm[w];f[48+w]=snapshot.feedback_current[w];
        f[52+w]=o->threshold_mps[w];f[56+w]=o->scale[w];
    }
    f[60]=power_fresh ? power->power.effective_limit_w:0;
    f[61]=power_fresh && power->meter_online ? power->measured_power_w:0;
    f[62]=snapshot.enabled_mask;f[63]=Age(now_ms,snapshot.tick_ms,known);
    U32(f,64,snapshot.spin_cycles);U32(f,66,snapshot.lock_cycles);
    f[68]=snapshot.reference.vx_mps;f[69]=snapshot.reference.vy_mps;f[70]=snapshot.reference.wz_radps;
    f[71]=CHASSIS_TRACTION_GYRO_Z_SIGN;
    for(unsigned n=0;n<TRACTION_CHANNELS;++n) if(!isfinite(f[n])) f[n]=0;
    uint8_t *bytes=(uint8_t *)&frame;
    for(size_t n=0;n+4<=sizeof frame.channels;++n) if(memcmp(bytes+n,frame.tail,4)==0) {
        const size_t last=n+3;bytes[last-last%4]^=1U;
    }
    (void)HAL_UART_Transmit_IT(uart,bytes,sizeof frame);
}
#endif
