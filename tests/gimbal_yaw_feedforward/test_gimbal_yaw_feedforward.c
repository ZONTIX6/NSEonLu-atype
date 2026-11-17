#include "../../application/gimbal/gimbal_yaw_feedforward.h"
#include "../../config/03_gimbal/gimbal_config.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

static unsigned failures;
#define CHECK(c) do {if (!(c)) {printf("FAIL line %d: %s\n",__LINE__,#c);++failures;}} while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b))<0.01f)

int main(void)
{
    GimbalYawFeedforward_s state={0};
    if (!YAW_SPIN_FEEDFORWARD_ENABLE || YAW_SPIN_FEEDFORWARD_GAIN<=0) {
        state.active=true;
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,100,100));
        CHECK(!state.active);
    } else {
        /* Ordinary follow rotation must not activate a new compensation. */
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,3,100,100));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,0,100,100));
        NEAR(1600,GimbalYawFeedforwardUpdate(&state,true,true,true,2,102,103));
        NEAR(5416,GimbalYawFeedforwardUpdate(&state,true,true,true,6.77f,104,105));
        /* Exiting the mode retains only the measured remaining rotation. */
        NEAR(2400,GimbalYawFeedforwardUpdate(&state,true,false,true,3,106,107));
        NEAR(80,GimbalYawFeedforwardUpdate(&state,true,false,true,0.1f,108,109));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,0.05f,110,110));
        CHECK(!state.active);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,3,112,112));
        NEAR(8000,GimbalYawFeedforwardUpdate(&state,true,true,true,FLT_MAX,114,114));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,-6.77f,116,116));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,-0.1f,118,118));
        CHECK(!state.active);

        NEAR(800,GimbalYawFeedforwardUpdate(&state,true,true,true,1,100,120));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,100,121));
        CHECK(!state.active);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,1,122,122));
        NEAR(800,GimbalYawFeedforwardUpdate(&state,true,true,true,1,123,123));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,false,true,true,1,124,124));
        CHECK(!state.active);
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,false,true,1,125,125));

        const float bad[]={NAN,INFINITY,-INFINITY};
        for (unsigned i=0;i<3;++i) {
            NEAR(800,GimbalYawFeedforwardUpdate(&state,true,true,true,1,130,130));
            NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,bad[i],132,132));
            CHECK(!state.active);
        }
        NEAR(800,GimbalYawFeedforwardUpdate(&state,true,true,true,1,140,140));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,false,1,142,142));
        CHECK(!state.active);
        /* Source-clock rollover is accepted; future timestamps are rejected. */
        NEAR(800,GimbalYawFeedforwardUpdate(&state,true,true,true,1,UINT32_MAX-9,10));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,UINT32_MAX-9,11));
        NEAR(0,GimbalYawFeedforwardUpdate(&state,true,true,true,1,201,200));
    }
    NEAR(0,GimbalYawFeedforwardUpdate(NULL,true,true,true,1,100,100));
    printf("yaw feedforward: %u failures\n",failures);
    return failures ? 1 : 0;
}
