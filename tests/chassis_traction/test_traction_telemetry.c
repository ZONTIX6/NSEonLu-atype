#include "vofa_observation.h"
#include "../../modules/motor/DJImotor/dji_traction.h"
#include "usart.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static DJITractionSnapshot_s snapshot;
static const uint8_t *bytes;
static unsigned sent;
UART_HandleTypeDef huart7={.gState=HAL_UART_STATE_READY};
bool DJIPowerAdapterGetTractionSnapshot(DJITractionSnapshot_s *out) {*out=snapshot;return out->available;}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,const uint8_t *data,uint16_t size)
{
    assert(uart==&huart7 && size==73*4);bytes=data;++sent;uart->gState=HAL_UART_STATE_BUSY_TX;return HAL_OK;
}
static float Field(unsigned n) {float v;memcpy(&v,bytes+n*4,4);return v;}
static void Send(uint32_t now) { VofaObservationSend(NULL,NULL,NULL,NULL,NULL,now); }
int main(void)
{
    Send(100);assert(sent==1 && Field(0)==10 && Field(5)==1 && Field(6)==0 && Field(18)==-1);
    uint8_t previous[73*4];memcpy(previous,bytes,sizeof previous);
    snapshot=(DJITractionSnapshot_s){.available=true,.can_submitted=true,.tick_ms=120,
        .enabled_mask=15,.cycles=UINT32_MAX,.intervention_cycles=0x12345678,
        .submitted_intervention_cycles=20,.spin_cycles=100,.lock_cycles=30,
        .reference={.valid=true,.tick_ms=100,.gyro_tick_ms=110,.vx_mps=1,.vy_mps=2,.wz_radps=3},
        .traction={.valid=true,.spin_mask=2,.lock_mask=4,.intervened_mask=6,.active_brake_mask=2}};
    for(unsigned w=0;w<4;++w) {
        snapshot.traction.expected_mps[w]=1+w;snapshot.traction.actual_mps[w]=2+w;
        snapshot.traction.slip_mps[w]=.1f*w;snapshot.traction.threshold_mps[w]=.5f;
        snapshot.raw_current[w]=1000+w;snapshot.traction.proposed_current[w]=-300.0f-(float)w;
        snapshot.final_current[w]=-100-(int)w;snapshot.target_rpm[w]=300+w;
        snapshot.feedback_current[w]=-50.0f-(float)w;snapshot.traction.scale[w]=.25f;
    }
    Send(120);assert(sent==1 && memcmp(previous,bytes,sizeof previous)==0); /* Busy buffer. */
    huart7.gState=HAL_UART_STATE_READY;Send(140);assert(sent==2);
    assert(Field(6)==15 && Field(8)==2 && Field(9)==4 && Field(10)==6 && Field(11)==2);
    assert(Field(12)==65535 && Field(13)==65535 && Field(14)==0x1234 && Field(15)==0x5678);
    assert(Field(18)==40 && Field(19)==30 && Field(63)==20);
    for(unsigned w=0;w<4;++w) {
        assert(Field(20+w)==1+w && Field(24+w)==2+w && Field(32+w)==1000+w);
        assert(Field(36+w)==-300-(float)w && Field(40+w)==-100-(float)w);
        assert(Field(44+w)==300+w && Field(48+w)==-50-(float)w);
        assert(Field(52+w)==.5f && Field(56+w)==.25f);
    }
    assert(Field(65)==100 && Field(67)==30 && Field(68)==1 && Field(69)==2 && Field(70)==3);
    huart7.gState=HAL_UART_STATE_READY;Send(141);assert(sent==2);
    snapshot.traction.expected_mps[0]=NAN;Send(160);assert(sent==3 && Field(6)==1 && Field(20)==0);
    for(unsigned n=0;n<72;++n) assert(isfinite(Field(n)));
    const uint8_t tail[]={0,0,128,127};assert(memcmp(bytes+72*4,tail,4)==0);
    for(unsigned n=0;n+4<=72*4;++n) assert(memcmp(bytes+n,tail,4)!=0);
    puts("PASS: traction telemetry schema, fields, counters, ages and asynchronous buffer");return 0;
}
