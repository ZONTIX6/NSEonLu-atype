#include "chassis_motion.h"
#include <stdio.h>

static unsigned failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)
#define NEAR(a,b,tol) CHECK(isfinite(b) && fabsf((a)-(b)) < (tol))

/* Remove the independent static trim to observe only the operator response. */
static void OperatorBody(Chassis_Ctrl_Cmd_s cmd, float lead, float *x, float *y)
{
    float trim_x, trim_y;
    ChassisResolveMotion(&cmd, 4000, lead, x, y);
    cmd.vx = cmd.vy = 0;
    ChassisResolveMotion(&cmd, 4000, lead, &trim_x, &trim_y);
    *x -= trim_x;
    *y -= trim_y;
}

/* A rotating body with a transport delay reproduces the observed sideways
 * motion. Lead must cancel it for both spin directions and every input axis. */
static void DelayedPlant(void)
{
    const float rates[] = {388.0f, -388.0f, 194.0f, -194.0f};
    const float commands[][2] = {{1000,0}, {-1000,0}, {0,1000}, {0,-1000}, {600,800}};
    for (unsigned r=0; r<sizeof rates/sizeof rates[0]; ++r) {
        for (unsigned c=0; c<sizeof commands/sizeof commands[0]; ++c) {
            ChassisSpinPhase_s state={0};
            Chassis_Ctrl_Cmd_s cmd={.chassis_mode=CHASSIS_ROTATE,
                .vx=commands[c][0], .vy=commands[c][1]};
            float lead=0;
            for (unsigned ms=0; ms<=1500; ms+=5) {
                cmd.offset_angle=remainderf(179.0f+rates[r]*(float)ms*.001f,360.0f);
                lead=ChassisSpinPhaseUpdate(&state,&cmd,ms);
            }
            float bx,by;
            OperatorBody(cmd,lead,&bx,&by);
            const float delay=CHASSIS_SPIN_PHASE_COMPENSATION_ENABLE ? CHASSIS_SPIN_PHASE_PREDICTION_MS : 0;
            const float angle=(cmd.offset_angle+rates[r]*delay*.001f)*DEGREE_2_RAD;
            const float actual_x=bx*cosf(angle)+by*sinf(angle);
            const float actual_y=-bx*sinf(angle)+by*cosf(angle);
            NEAR(cmd.vx,actual_x,.08f);
            NEAR(cmd.vy,actual_y,.08f);
            NEAR(1000,hypotf(bx,by),.01f);
            /* The phase estimator and transform cannot rewrite the measured angle. */
            CHECK(cmd.offset_angle>=-180 && cmd.offset_angle<=180);
        }
    }
}

static void ModeAndTrimScope(void)
{
    const chassis_mode_e modes[]={CHASSIS_NO_FOLLOW,CHASSIS_FOLLOW_GIMBAL_YAW,CHASSIS_ZERO_FORCE};
    for(unsigned i=0;i<sizeof modes/sizeof modes[0];++i) {
        Chassis_Ctrl_Cmd_s cmd={.chassis_mode=modes[i],.offset_angle=30,.vx=500,.vy=-200};
        float x0,y0,x1,y1;
        ChassisResolveMotion(&cmd,4000,0,&x0,&y0);
        ChassisResolveMotion(&cmd,4000,15,&x1,&y1);
        NEAR(x0,x1,.001f); NEAR(y0,y1,.001f);
    }
    /* A lead may not turn the existing static trim or create operator motion. */
    Chassis_Ctrl_Cmd_s cmd={.chassis_mode=CHASSIS_ROTATE,.offset_angle=73};
    float x0,y0,x1,y1;
    ChassisResolveMotion(&cmd,4000,0,&x0,&y0);
    ChassisResolveMotion(&cmd,4000,15,&x1,&y1);
    NEAR(x0,x1,.001f); NEAR(y0,y1,.001f);
    NEAR(73,cmd.offset_angle,.001f);
    NEAR(4000,cmd.wz,.001f);
}

static void HistoryAndBounds(void)
{
    ChassisSpinPhase_s state={0};
    Chassis_Ctrl_Cmd_s cmd={.chassis_mode=CHASSIS_ROTATE,.offset_angle=179};
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,UINT32_MAX-4U),.001f);
    cmd.offset_angle=-179;
    float lead=ChassisSpinPhaseUpdate(&state,&cmd,0);
    CHECK(isfinite(lead) && lead>=0 && lead<=CHASSIS_SPIN_PHASE_MAX_DEG);
    NEAR(lead,ChassisSpinPhaseUpdate(&state,&cmd,0),.001f);
    for(unsigned ms=5;ms<=1000;ms+=5) {
        cmd.offset_angle=remainderf(-179.0f+2.0f*ms,360.0f);
        lead=ChassisSpinPhaseUpdate(&state,&cmd,ms);
        CHECK(isfinite(lead) && lead<=CHASSIS_SPIN_PHASE_MAX_DEG);
    }
    if(CHASSIS_SPIN_PHASE_COMPENSATION_ENABLE && CHASSIS_SPIN_PHASE_PREDICTION_MS>0)
        NEAR(CHASSIS_SPIN_PHASE_MAX_DEG,lead,.001f);
    else NEAR(0,lead,.001f);
    /* An unchanged angle decays to zero, rather than retaining the last rate. */
    for(unsigned ms=1005;ms<=2000;ms+=5) lead=ChassisSpinPhaseUpdate(&state,&cmd,ms);
    NEAR(0,lead,.001f);
    cmd.offset_angle=NAN;
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,2005),.001f);
    CHECK(!state.initialized);
    cmd.offset_angle=40;
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,2010),.001f);
    cmd.offset_angle=42;
    ChassisSpinPhaseUpdate(&state,&cmd,2015);
    cmd.offset_angle=100;
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,2050),.001f);
    cmd.chassis_mode=CHASSIS_NO_FOLLOW;
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,2055),.001f);
    CHECK(!state.initialized && state.rate_dps==0);
    cmd.chassis_mode=CHASSIS_ROTATE;
    cmd.offset_angle=-130;
    NEAR(0,ChassisSpinPhaseUpdate(&state,&cmd,2060),.001f);
}

int main(void)
{
    DelayedPlant();
    ModeAndTrimScope();
    HistoryAndBounds();
    if(failures) return 1;
    puts("PASS: spin translation phase, signed delay, wrap, trim and reset");
    return 0;
}
