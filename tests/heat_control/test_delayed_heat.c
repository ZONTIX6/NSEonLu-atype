#include "heat_control.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static Subscriber_t sub;
static HeatControlData_s message;
static unsigned pending;
static uint32_t now;
Subscriber_t *SubRegister(char *name,uint8_t len)
{(void)name;assert(len==sizeof message);pending=0;return &sub;}
uint8_t SubGetMessage(Subscriber_t *s,void *out)
{assert(s==&sub);if(!pending)return 0;memcpy(out,&message,sizeof message);pending=0;return 1;}
float DWT_GetDeltaT(uint32_t *count) {++*count;return 0.005f;}
uint32_t HAL_GetTick(void) {return now;}

int main(void)
{
    unsigned cases=0;
    for(unsigned delay=0;delay<=3;++delay)
    for(unsigned bullet=2;bullet<=10;bullet+=8)
    for(unsigned offset=0;offset<20;++offset)
    for(unsigned initial_phase=0;initial_phase<20;++initial_phase) {
        now=0;
        HeatControlInstance instance;
        HeatControl_Init_Config_s config={(float)bullet,30};
        HeatControlInit(&instance,&config);
        HeatControlData_s samples[4];
        for(unsigned i=0;i<4;++i) samples[i]=(HeatControlData_s){
            .heat_limit=300,.cooling_rate=20,.valid=1};
        float heat=0, phase=initial_phase/20.0f;
        message=samples[0];pending=1;
        for(unsigned step=0;step<12000;++step) {
            now=step*5U;
            if(step%20==offset) {
                message=samples[delay?delay-1:0];pending=1;
                for(unsigned i=delay;i>0;--i) samples[i]=samples[i-1];
                samples[0]=(HeatControlData_s){.current_heat=(uint16_t)heat,
                    .heat_limit=300,.cooling_rate=20,.valid=1,
                    .robot_state_tick_ms=now,.power_heat_tick_ms=now};
                if(!delay) message=samples[0];
            }
            phase+=HeatControlCalculate(&instance,1)*0.005f;
            while(phase>=1) {
                phase-=1;heat+=bullet;
                if(heat>=300) {
                    fprintf(stderr,"heat=%f delay=%u bullet=%u offset=%u phase=%u tick=%u\n",
                        heat,delay,bullet,offset,initial_phase,now);
                    return 1;
                }
            }
            if((step+1)%20==0) heat=fmaxf(0,heat-2);
        }
        ++cases;
    }
    printf("PASS: %u delayed heat simulations (0..300 ms, 60 s each)\n",cases);
}
