#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../application/chassis/chassis.h"
#include "../../application/cmd/robot_cmd.c"
#include "../../application/gimbal/gimbal.c"
#include "../../application/shoot/shoot.c"

struct UART_HandleTypeDef { int unused; };
UART_HandleTypeDef huart1, huart3, huart8;
CAN_HandleTypeDef hcan1, hcan2;
static uint32_t now=1000;
static attitude_t imu;
static Vision_Recv_s vision;
static RemoteControlSnapshot_s remote_input;
static Vt02_Ctrl_t vtx_input;
static DJIMotorInstance motors[5];
static unsigned motor_count;
static Publisher_t pubs[12];
static unsigned pub_count;
typedef struct { Subscriber_t sub; Publisher_t *pub; unsigned char data[256]; int pending; } TestSub;
static TestSub subs[20];
static unsigned sub_count;
static ChassisVelocityFeedback_s wheel_feedback;
bool ChassisGetVelocitySnapshot(ChassisVelocityFeedback_s *out) {
    *out=wheel_feedback; return out->actual.valid;
}
uint32_t HAL_GetTick(void) { return now; }
float DWT_GetDeltaT(uint32_t *count) { *count=now; return .005f; }
bool RuntimeTuningTrialActive(void) { return false; }
attitude_t *INS_Init(void) { return &imu; }
bool INSIsHealthy(uint32_t tick) { (void)tick; return true; }
RC_ctrl_t *RemoteControlInit(UART_HandleTypeDef *h) { (void)h; return &remote_input.control; }
uint8_t RemoteControlGetSnapshot(RemoteControlSnapshot_s *s) { *s=remote_input; return 1; }
Vt02_Ctrl_t *Vt02ControlInit(UART_HandleTypeDef *h) { (void)h; return &vtx_input; }
uint8_t Vt02ControlGetSnapshot(Vt02_Ctrl_t *s) { *s=vtx_input; return 1; }
Vision_Recv_s *VisionInit(UART_HandleTypeDef *h) { (void)h; return &vision; }
Publisher_t *PubRegister(char *name,uint8_t len) {
    for(unsigned i=0;i<pub_count;i++) if(!strcmp(pubs[i].topic_name,name)) return &pubs[i];
    Publisher_t *p=&pubs[pub_count++]; strcpy(p->topic_name,name); p->data_len=len; return p;
}
Subscriber_t *SubRegister(char *name,uint8_t len) {
    TestSub *s=&subs[sub_count++]; s->pub=PubRegister(name,len); return &s->sub;
}
uint8_t PubPushMessage(Publisher_t *p,void *data) {
    for(unsigned i=0;i<sub_count;i++) if(subs[i].pub==p) {
        memcpy(subs[i].data,data,p->data_len); subs[i].pending=1;
    } return 1;
}
uint8_t SubGetMessage(Subscriber_t *sub,void *data) {
    TestSub *s=(TestSub *)sub; if(!s->pending) return 0;
    memcpy(data,s->data,s->pub->data_len); s->pending=0; return 1;
}
DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *c) {
    DJIMotorInstance *m=&motors[motor_count++]; m->motor_settings=c->controller_setting_init_config;
    return m;
}
void DJIMotorSetRef(DJIMotorInstance *m,float ref) { m->motor_controller.pid_ref=ref; }
void DJIMotorStop(DJIMotorInstance *m) { m->stop_flag=MOTOR_STOP; m->output_feedforward=0; }
void DJIMotorSetOutputFeedforward(DJIMotorInstance *m,float command,uint32_t source_tick) {
    m->output_feedforward=command; m->output_feedforward_tick_ms=source_tick;
}
void DJIMotorEnable(DJIMotorInstance *m) { m->stop_flag=MOTOR_ENALBED; }
void DJIMotorOuterLoop(DJIMotorInstance *m,Closeloop_Type_e loop) { m->motor_settings.outer_loop_type=loop; }
void DJIMotorChangeFeed(DJIMotorInstance *m,Closeloop_Type_e loop,Feedback_Source_e type) { (void)m;(void)loop;(void)type; }

static void FreshMotors(void) {
    for(unsigned i=0;i<motor_count;i++) { motors[i].measure.rx_sequence++; motors[i].measure.rx_tick_ms=now; }
}
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED
static void StepYawFeedforward(bool fresh_wheels) {
    now+=5; FreshMotors(); remote_input.last_rx_tick_ms=now;
    if(fresh_wheels) wheel_feedback.actual.tick_ms=now-2;
    RobotCMDTask(); GimbalTask();
}
#endif
static void Heat(unsigned heat,unsigned valid) {
    HeatControlData_s h={.current_heat=heat,.heat_limit=300,.cooling_rate=20,.valid=valid,
        .robot_state_tick_ms=now,.power_heat_tick_ms=now};
    PubPushMessage(PubRegister(HEAT_CONTROL_DATA_TOPIC,sizeof h),&h);
}
static void StepShoot(Shoot_Ctrl_Cmd_s c,unsigned heat,unsigned valid) {
    now+=5; FreshMotors(); Heat(heat,valid); PubPushMessage(shoot_cmd_pub,&c); ShootTask();
}
static void Ready(Shoot_Ctrl_Cmd_s c) {
    c.load_mode=LOAD_STOP;
    friction_l->measure.speed_aps=-4000; friction_r->measure.speed_aps=4000;
    for(int i=0;i<25;i++) StepShoot(c,0,1);
}
static int stopped(void) { return loader->stop_flag==MOTOR_STOP ||
    (loader->motor_settings.outer_loop_type==SPEED_LOOP && loader->motor_controller.pid_ref==0); }
int main(int argc,char **argv) {
    assert(argc==2); RobotCMDInit(); GimbalInit(); ShootInit(); FreshMotors();
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE
    if(!strcmp(argv[1],"force")) {
        Subscriber_t *sub=SubRegister("chassis_force_cmd",sizeof(ChassisForceIntentMessage_s));
        remote_input=(RemoteControlSnapshot_s){.online=1,.has_sample=1,.last_rx_tick_ms=now};
        remote_input.control.rc.switch_left=RC_SW_DOWN;remote_input.control.rc.switch_right=RC_SW_UP;
        GimbalTask();RobotCMDTask();
        ChassisForceIntentMessage_s message;
        assert(SubGetMessage(sub,&message));assert(message.published_ms==now);
        remote_input.control.rc.rocker_r1=-660;
        RobotCMDTask();assert(SubGetMessage(sub,&message));
        assert(message.intent.input_x==1 && message.intent.input_y==0);
        assert(message.intent.source_id==CHASSIS_FORCE_SOURCE_REMOTE && message.intent.force_scale==1);
        rc_snapshot[0].key[0].w=1;rc_snapshot[0].key[0].a=1;rc_snapshot[0].key_count[0][Key_C]=0;
        MouseKeySet();assert(force_intent.input_x>0 && force_intent.input_y>0);
        assert(fabsf(force_intent.force_scale-.4f)<.0001f);
        vt_snapshot.key[0]=rc_snapshot[0].key[0];VideoTransmissionControlSet();
        assert(force_intent.source_id==CHASSIS_FORCE_SOURCE_VTX);
        assert(fabsf(force_intent.input_x-.70710678f)<.00001f && fabsf(force_intent.input_y-.70710678f)<.00001f);
        ChassisForceIntentReceiver_s receiver={0};
        assert(ChassisForceIntentAccept(&receiver,&message.intent,message.published_ms));
        now+=11;assert(!ChassisForceIntentFresh(&receiver,now,0));
        puts("PASS: single-board force input directions/scales and publication age");return 0;
    }
#endif
    if(!strcmp(argv[1],"speed")) {
        vt_snapshot.key[0].w=1;
        for(unsigned i=0;i<4;i++) { vt_snapshot.key_count[0][Key_C]=i; VideoTransmissionControlSet();
            assert(fabsf(chassis_cmd_send.vx+3000*(.4f+.2f*i))<.01f); }
    } else if(!strcmp(argv[1],"manual")) {
        rc_snapshot[0].rc.switch_left=RC_SW_MID; rc_snapshot[0].rc.rocker_l_=100;
        vision.target_state=READY_TO_FIRE; RemoteControlSet(); assert(gimbal_cmd_send.yaw<0);
    } else if(!strcmp(argv[1],"gimbal_deadband")) {
        rc_snapshot[0].rc.switch_left=RC_SW_MID;
        gimbal_cmd_send.yaw=12.0f; gimbal_cmd_send.pitch=8.0f;
        rc_snapshot[0].rc.rocker_l_=30; rc_snapshot[0].rc.rocker_l1=-30;
        RemoteControlSet();
        assert(fabsf(gimbal_cmd_send.yaw-12.0f)<.0001f);
        assert(fabsf(gimbal_cmd_send.pitch-8.0f)<.0001f);
        rc_snapshot[0].rc.rocker_l_=31; rc_snapshot[0].rc.rocker_l1=-31;
        RemoteControlSet();
        assert(fabsf(gimbal_cmd_send.yaw-11.845f)<.0001f);
        assert(fabsf(gimbal_cmd_send.pitch-7.969f)<.0001f);
    } else if(!strcmp(argv[1],"yaw_feedforward")) {
#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED
        remote_input=(RemoteControlSnapshot_s){.online=1,.has_sample=1,.last_rx_tick_ms=now};
        remote_input.control.rc.switch_left=RC_SW_DOWN; remote_input.control.rc.switch_right=RC_SW_UP;
        GimbalTask(); RobotCMDTask(); assert(VehicleSafetyAllowsDrive(now));
        wheel_feedback.actual=(ChassisVelocity_s){.valid=true,.tick_ms=now-2,.wz_rpm=6.77f/0.104719755f};
        /* Deliberately different target: only actual wheel motion may drive FF. */
        wheel_feedback.target=(ChassisVelocity_s){.valid=true,.tick_ms=now,.wz_rpm=1000};
        remote_input.control.rc.switch_right=RC_SW_DOWN;
        RobotCMDTask(); GimbalTask();
        assert(gimbal_cmd_recv.yaw_spin_feedforward_requested);
        assert(yaw_motor->output_feedforward==0); /* Entry starts smoothly. */
        StepYawFeedforward(true);
        assert(yaw_motor->output_feedforward>0 && yaw_motor->output_feedforward<5416);
        assert(yaw_motor->output_feedforward_tick_ms==now-2);
        assert(pitch_motor->output_feedforward==0);
        for(unsigned i=0;i<200;++i) StepYawFeedforward(true);
        assert(fabsf(yaw_motor->output_feedforward-5416)<.1f);
        float previous_ref=yaw_motor->motor_controller.pid_ref;
        remote_input.control.rc.rocker_l_=100;
        RobotCMDTask(); GimbalTask();
        assert(yaw_motor->motor_controller.pid_ref!=previous_ref); /* Manual yaw still works. */
        assert(fabsf(yaw_motor->output_feedforward-5416)<.1f);
        remote_input.control.rc.rocker_l_=0;
        remote_input.control.rc.switch_right=RC_SW_UP;
        wheel_feedback.actual.wz_rpm=3/0.104719755f;
        StepYawFeedforward(true);
        assert(!gimbal_cmd_recv.yaw_spin_feedforward_requested);
        assert(yaw_motor->output_feedforward>2400 && yaw_motor->output_feedforward<5416);
        for(unsigned i=0;i<200;++i) StepYawFeedforward(true);
        assert(fabsf(yaw_motor->output_feedforward-2400)<.1f);
        wheel_feedback.actual.wz_rpm=0;
        StepYawFeedforward(true);
        assert(yaw_motor->output_feedforward>0 && yaw_motor->output_feedforward<2400);
        wheel_feedback.actual.wz_rpm=3/0.104719755f;
        float tail=yaw_motor->output_feedforward;
        StepYawFeedforward(true);
        assert(yaw_motor->output_feedforward>0 && yaw_motor->output_feedforward<tail);
        for(unsigned i=0;i<200;++i) StepYawFeedforward(true);
        assert(yaw_motor->output_feedforward==0); /* Follow cannot re-arm FF. */
        remote_input.control.rc.switch_right=RC_SW_DOWN;
        RobotCMDTask(); GimbalTask(); assert(yaw_motor->output_feedforward==0);
        StepYawFeedforward(true); assert(yaw_motor->output_feedforward>0);
        /* Keep safety/commands alive while only the wheel sample ages out. */
        for(unsigned i=0;i<4;++i) StepYawFeedforward(false);
        assert(yaw_motor->output_feedforward==0);
        wheel_feedback.actual.tick_ms=now;
        RobotCMDTask(); GimbalTask(); assert(yaw_motor->output_feedforward==0);
        StepYawFeedforward(true); assert(yaw_motor->output_feedforward>0);
        remote_input.online=0;
        RobotCMDTask(); GimbalTask();
        assert(!gimbal_cmd_recv.yaw_spin_feedforward_requested);
        assert(yaw_motor->output_feedforward==0 && yaw_motor->stop_flag==MOTOR_STOP);
        assert(!yaw_feedforward_state.active && yaw_feedforward_state.output==0);
#endif
    } else if(!strcmp(argv[1],"yaw")) {
        remote_input=(RemoteControlSnapshot_s){.online=1,.has_sample=1,.last_rx_tick_ms=now};
        remote_input.control.rc.switch_left=RC_SW_DOWN; remote_input.control.rc.switch_right=RC_SW_UP;
        GimbalTask(); RobotCMDTask(); assert(VehicleSafetyAllowsDrive(now));
        now+=25; remote_input.last_rx_tick_ms=now;
        GimbalTask(); RobotCMDTask(); assert(!VehicleSafetyAllowsDrive(now));
        FreshMotors(); remote_input.control.rc.switch_right=RC_SW_MID;
        GimbalTask(); RobotCMDTask(); assert(!VehicleSafetyAllowsDrive(now));
        remote_input.control.rc.switch_right=RC_SW_UP;
        GimbalTask(); RobotCMDTask(); assert(VehicleSafetyAllowsDrive(now));
    } else {
        Shoot_Ctrl_Cmd_s c={.shoot_mode=SHOOT_ON,.load_mode=LOAD_3_BULLET,
            .friction_mode=FRICTION_ON,.bullet_speed=SMALL_AMU_15};
        if(!strcmp(argv[1],"spinup")) {
            StepShoot(c,0,1); assert(stopped());
            c.load_mode=LOAD_STOP; Ready(c); c.load_mode=LOAD_1_BULLET;
            StepShoot(c,0,1); assert(!stopped());
        } else {
            Ready(c);
            if(!strcmp(argv[1],"heat")) {
                StepShoot(c,300,1); assert(stopped());
                c.load_mode=LOAD_STOP; StepShoot(c,0,1); c.load_mode=LOAD_3_BULLET;
                StepShoot(c,0,0); assert(stopped());
                c.load_mode=LOAD_STOP; StepShoot(c,0,1); c.load_mode=LOAD_3_BULLET;
                StepShoot(c,210,1); assert(stopped()); /* only 90 heat; needs 100 */
            } else if(!strcmp(argv[1],"friction")) {
                c.friction_mode=FRICTION_OFF; StepShoot(c,0,1); assert(stopped());
            } else if(!strcmp(argv[1],"stale")) {
                StepShoot(c,0,1); assert(!stopped());
                now+=301; FreshMotors(); PubPushMessage(shoot_cmd_pub,&c); ShootTask(); assert(stopped());
                StepShoot(c,0,1); assert(stopped()); /* held request cannot resume */
            } else if(!strcmp(argv[1],"motor_stale")) {
                StepShoot(c,0,1); assert(!stopped());
                now+=21; Heat(0,1); PubPushMessage(shoot_cmd_pub,&c); ShootTask(); assert(stopped());
                for(int i=0;i<70;i++) StepShoot(c,0,1);
                assert(stopped());
            } else if(!strcmp(argv[1],"modes")) {
                c.load_mode=LOAD_BURSTFIRE; c.shoot_rate=2; StepShoot(c,0,1); assert(!stopped());
                c.load_mode=LOAD_STOP; StepShoot(c,0,1); c.load_mode=LOAD_1_BULLET;
                StepShoot(c,0,1); assert(stopped()); /* burst heat not yet reported */
                c.load_mode=LOAD_STOP; for(int i=0;i<65;i++) StepShoot(c,0,1);
                c.load_mode=LOAD_3_BULLET; StepShoot(c,0,1); assert(!stopped());
                uint32_t feed_tick=now;
                loader->measure.total_angle=loader->motor_controller.pid_ref;
                c.load_mode=LOAD_BURSTFIRE; StepShoot(c,0,1); assert(stopped());
                /* Republish valid-looking heat with a pre-settlement source timestamp. */
                now+=305; FreshMotors();
                HeatControlData_s h={.heat_limit=300,.valid=1,.robot_state_tick_ms=now,
                    .power_heat_tick_ms=feed_tick+10};
                PubPushMessage(PubRegister(HEAT_CONTROL_DATA_TOPIC,sizeof h),&h);
                PubPushMessage(shoot_cmd_pub,&c); ShootTask(); assert(stopped());
                StepShoot(c,0,1); assert(!stopped());
            } else {
                StepShoot(c,0,1); assert(!stopped());
                float target=loader->motor_controller.pid_ref;
                c.load_mode=LOAD_STOP; StepShoot(c,0,1); assert(!stopped());
                loader->measure.total_angle=target; StepShoot(c,0,1); assert(stopped());
                c.load_mode=LOAD_3_BULLET; StepShoot(c,0,1); assert(stopped()); /* unreported heat */
                c.load_mode=LOAD_STOP; for(int i=0;i<65;i++) StepShoot(c,0,1);
                c.load_mode=LOAD_1_BULLET; StepShoot(c,0,1); assert(!stopped());
                c.friction_mode=FRICTION_OFF; StepShoot(c,0,1); assert(stopped());
                c.friction_mode=FRICTION_ON; for(int i=0;i<70;i++) StepShoot(c,0,1);
                assert(stopped()); /* restoring wheel speed cannot restart cancelled press */
                c.load_mode=LOAD_STOP; StepShoot(c,0,1); c.load_mode=LOAD_3_BULLET;
                StepShoot(c,0,1); assert(!stopped());
                target=loader->motor_controller.pid_ref; loader->measure.total_angle=target;
                for(int i=0;i<70;i++) StepShoot(c,0,1);
                assert(stopped()); /* holding a completed batch never repeats */
            }
        }
    }
    puts("PASS: vehicle motion integration"); return 0;
}
