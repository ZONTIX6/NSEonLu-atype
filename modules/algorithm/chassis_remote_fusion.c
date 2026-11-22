#include "chassis_remote_fusion.h"
#include "chassis_gimbal_imu.h"
#include "../../config/30_velocity_observation/chassis_wheel_redundancy_config.h"
#include <math.h>

#define HALF_RANGE UINT32_C(0x80000000)
#define REMOTE_MAX_SOURCE_AGE_MS 20U
#define REMOTE_MAX_KNOWN_AGE_MS 60U

static void Increment(uint32_t *value)
{
    if(*value!=UINT32_MAX) ++*value;
}

static void ResetPipeline(ChassisRemoteFusion_s *s)
{
    if(s->center.count || s->filter.initialized || s->held.valid) Increment(&s->resets);
    s->center=(ChassisVelocityCenter_s){0};
    ChassisVelocityFusionReset(&s->filter);
    s->redundancy_candidate_wheel=UINT8_MAX;
    s->redundancy_confirmations=0;
    s->held=(ChassisRemoteFusionOutput_s){0};
}

static bool ValidConfig(const ChassisRemoteFusionConfig_s *c)
{
    if(!c || !c->max_pair_span_ms || c->max_pair_span_ms>REMOTE_MAX_KNOWN_AGE_MS ||
       c->max_wheel_skew_ms>CHASSIS_VELOCITY_MAX_AGE_MS ||
       !isfinite(c->spin_accel_uncertainty_fraction) || c->spin_accel_uncertainty_fraction<0 ||
       !isfinite(c->max_accel_variance) || c->max_accel_variance<c->filter.accel_variance ||
       c->max_timing_jitter_ms>REMOTE_MAX_KNOWN_AGE_MS) return false;
    const ChassisVelocityFusionConfig_s *f=&c->filter;
    if(!isfinite(f->wheel_variance) || f->wheel_variance<=0 ||
       !isfinite(f->accel_variance) || f->accel_variance<=0 ||
       !isfinite(f->jerk_density) || f->jerk_density<=0 ||
       !isfinite(f->max_velocity_residual_mps) || f->max_velocity_residual_mps<=0 ||
       !f->max_age_us || f->max_age_us>=HALF_RANGE || f->max_skew_us>=HALF_RANGE ||
       !f->max_gap_us || f->max_gap_us>=HALF_RANGE) return false;
    float norm=0;
    for(unsigned i=0;i<4;++i) norm+=c->sensor_to_body_quaternion[i]*c->sensor_to_body_quaternion[i];
    if(!isfinite(norm) || norm<.98f || norm>1.02f) return false;
    for(unsigned i=0;i<3;++i) if(!isfinite(c->imu_offset_body_m[i])) return false;
    return true;
}

static void RotateSensorVectorToBody(const float q[4],const float sensor[3],float body[3])
{
    const float w=q[0],x=q[1],y=q[2],z=q[3];
    const float scale=2.0f/(w*w+x*x+y*y+z*z);
    body[0]=(1.0f-scale*(y*y+z*z))*sensor[0]+scale*(x*y-w*z)*sensor[1]+
        scale*(x*z+w*y)*sensor[2];
    body[1]=scale*(x*y+w*z)*sensor[0]+(1.0f-scale*(x*x+z*z))*sensor[1]+
        scale*(y*z-w*x)*sensor[2];
    body[2]=scale*(x*z-w*y)*sensor[0]+scale*(y*z+w*x)*sensor[1]+
        (1.0f-scale*(x*x+y*y))*sensor[2];
}

static float EffectiveAccelVariance(const ChassisRemoteFusionConfig_s *config,
                                    const float angular_velocity_body_radps[3])
{
    const float base=config->filter.accel_variance;
    const float fraction=config->spin_accel_uncertainty_fraction;
    if(fraction==0.0f) return base;

    const float *r=config->imu_offset_body_m;
    const float *w=angular_velocity_body_radps;
    /* Bound the residual from the modeled centripetal lever-arm acceleration. */
    const float wxr[3]={w[1]*r[2]-w[2]*r[1],w[2]*r[0]-w[0]*r[2],w[0]*r[1]-w[1]*r[0]};
    const float rotational_accel[3]={w[1]*wxr[2]-w[2]*wxr[1],
        w[2]*wxr[0]-w[0]*wxr[2],w[0]*wxr[1]-w[1]*wxr[0]};
    const float sigma=fraction*hypotf(hypotf(rotational_accel[0],rotational_accel[1]),
        rotational_accel[2]);
    const float max_sigma=sqrtf(config->max_accel_variance-base);
    if(!isfinite(sigma) || sigma>=max_sigma) return config->max_accel_variance;
    return base+sigma*sigma;
}

static bool PushWheel(ChassisRemoteFusion_s *s,const ChassisVelocity_s *wheel)
{
    if(s->wheel_count) {
        const unsigned newest=(s->wheel_head+CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY-1U)%
            CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY;
        const uint32_t step=wheel->tick_ms-s->wheels[newest].tick_ms;
        if(!step) return true;
        if(step>=HALF_RANGE) {
            s->wheel_head=s->wheel_count=0;
            return false;
        }
    }
    s->wheels[s->wheel_head]=*wheel;
    s->wheel_head=(s->wheel_head+1U)%CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY;
    if(s->wheel_count<CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY) ++s->wheel_count;
    return true;
}

typedef enum {PAIR_UNAVAILABLE,PAIR_WAIT,PAIR_READY} WheelPairResult_e;

static WheelPairResult_e PairWheel(const ChassisRemoteFusion_s *s,uint32_t tick,
                      uint32_t now_ms,uint32_t max_span,float velocity[2],
                      float wheel_mps[CHASSIS_WHEEL_COUNT],uint32_t *span)
{
    const ChassisVelocity_s *before=NULL,*after=NULL;
    uint32_t before_distance=UINT32_MAX,after_distance=UINT32_MAX;
    for(unsigned i=0;i<s->wheel_count;++i) {
        const unsigned index=(s->wheel_head+CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY-s->wheel_count+i)%
            CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY;
        const ChassisVelocity_s *w=&s->wheels[index];
        const uint32_t back=tick-w->tick_ms,forward=w->tick_ms-tick;
        if(back<=REMOTE_MAX_KNOWN_AGE_MS && back<before_distance) {before=w;before_distance=back;}
        if(forward<=REMOTE_MAX_KNOWN_AGE_MS && forward<after_distance) {after=w;after_distance=forward;}
    }
    if(!before) return PAIR_UNAVAILABLE;
    if(!after) {
        /* A newly received IMU sample can be slightly newer than all wheel
         * feedback. Keep the existing center/filter while its bounded bracket
         * is pending; clearing it on every 50 Hz arrival prevents warmup. */
        return before_distance<=max_span && now_ms-tick<=max_span ? PAIR_WAIT : PAIR_UNAVAILABLE;
    }
    *span=before_distance+after_distance;
    if(*span>max_span) return PAIR_UNAVAILABLE;
    const float fraction=*span ? (float)before_distance/(float)*span : 0;
    velocity[0]=before->vx_mps+fraction*(after->vx_mps-before->vx_mps);
    velocity[1]=before->vy_mps+fraction*(after->vy_mps-before->vy_mps);
    for(unsigned i=0;i<CHASSIS_WHEEL_COUNT;++i)
        wheel_mps[i]=before->wheel_mps[i]+fraction*(after->wheel_mps[i]-before->wheel_mps[i]);
    if(!isfinite(velocity[0]) || !isfinite(velocity[1])) return PAIR_UNAVAILABLE;
    for(unsigned i=0;i<CHASSIS_WHEEL_COUNT;++i)
        if(!isfinite(wheel_mps[i])) return PAIR_UNAVAILABLE;
    return PAIR_READY;
}

static void RememberSource(ChassisRemoteFusion_s *s,const ChassisImuCanSnapshot_s *r,
                           uint32_t estimated,uint32_t source_step,uint32_t receive_step)
{
    s->last_sequence=r->sample.sequence;
    s->last_source_ms=r->sample.tick_ms;
    s->last_receive_ms=r->first_rx_tick_ms;
    s->last_estimated_ms=estimated;
    s->source_step_ms=source_step;
    s->receive_step_ms=receive_step;
    s->have_source=true;
}

static void PublishHeld(const ChassisRemoteFusion_s *s,ChassisRemoteFusionOutput_s *out)
{
    out->status=CHASSIS_REMOTE_FUSED;
    out->tick_ms=s->held.tick_ms;
    out->vx_mps=s->held.vx_mps;
    out->vy_mps=s->held.vy_mps;
    out->pair_span_ms=s->held.pair_span_ms;
    for(unsigned i=0;i<2;++i) {
        out->paired_wheel_mps[i]=s->held.paired_wheel_mps[i];
        out->center_accel_n_mps2[i]=s->held.center_accel_n_mps2[i];
    }
    out->wheel_redundancy=s->held.wheel_redundancy;
    out->wheel_redundancy_confirmations=s->held.wheel_redundancy_confirmations;
    out->wheel_redundancy_applied=s->held.wheel_redundancy_applied;
}

static bool PredictReference(const ChassisRemoteFusion_s *state,
    const ChassisRemoteFusionConfig_s *config,
    const ChassisVelocityCenterSample_s *center, float reference[3])
{
    if(!state->filter.initialized) return false;
    const uint32_t elapsed_us=center->imu_us-state->filter.state_us;
    if(elapsed_us>=HALF_RANGE ||
       elapsed_us>CHASSIS_WHEEL_REDUNDANCY_REFERENCE_MAX_AGE_MS*1000U) return false;
    const float dt=(float)elapsed_us*1e-6f;
    const float nav_x=state->filter.axis[0].velocity+dt*state->filter.axis[0].accel;
    const float nav_y=state->filter.axis[1].velocity+dt*state->filter.axis[1].accel;
    const float a=center->body_to_nav_xy[0][0],b=center->body_to_nav_xy[0][1];
    const float c=center->body_to_nav_xy[1][0],d=center->body_to_nav_xy[1][1];
    const float det=a*d-b*c;
    if(!(det>.5f) || !isfinite(nav_x) || !isfinite(nav_y)) return false;
    const float body_x=(d*nav_x-b*nav_y)/det;
    const float body_y=(a*nav_y-c*nav_x)/det;
    reference[0]=body_x;
    reference[1]=body_y; /* Public chassis convention and wheel kinematics are forward/right. */
    reference[2]=config->wheel_redundancy_gyro_z_sign*center->angular_velocity_body_radps[2];
    return isfinite(reference[0]) && isfinite(reference[1]) && isfinite(reference[2]);
}

void ChassisRemoteFusionReset(ChassisRemoteFusion_s *state)
{
    if(state) *state=(ChassisRemoteFusion_s){0};
}

void ChassisRemoteFusionUpdate(ChassisRemoteFusion_s *state,
    const ChassisRemoteFusionConfig_s *config, const ChassisVelocity_s *wheel,
    uint32_t wheel_skew_ms, const ChassisImuCanSnapshot_s *remote,
    bool remote_usable, uint32_t now_ms, ChassisRemoteFusionOutput_s *output)
{
    if(!output) return;
    *output=(ChassisRemoteFusionOutput_s){.timing_estimated=true};
    if(!state) return;
    ChassisRemoteFusion_s *s=state;
    ChassisRemoteFusionOutput_s *out=output;
    out->source_step_ms=s->source_step_ms;
    out->receive_step_ms=s->receive_step_ms;
    if(remote && remote->available) {
        out->source_sequence=remote->sample.sequence;
        out->source_tick_ms=remote->sample.tick_ms;
        out->estimated_sample_ms=remote->first_rx_tick_ms-remote->sample.source_age_ms;
    }
    if(!wheel || !wheel->valid || now_ms-wheel->tick_ms>CHASSIS_VELOCITY_MAX_AGE_MS ||
       !isfinite(wheel->vx_mps) || !isfinite(wheel->vy_mps)) {
        s->wheel_count=s->wheel_head=0;
        ResetPipeline(s);
        out->status=CHASSIS_REMOTE_WHEEL_UNAVAILABLE;
        goto done;
    }
    out->valid=true;
    out->vx_mps=wheel->vx_mps;out->vy_mps=wheel->vy_mps;out->tick_ms=wheel->tick_ms;
    if(!ValidConfig(config)) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_CONFIG_INVALID;goto done;
    }
    if(s->have_now && now_ms-s->last_now_ms>=HALF_RANGE) {
        s->wheel_count=s->wheel_head=0;
        s->have_source=false;
        s->last_now_ms=now_ms;
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    s->last_now_ms=now_ms;s->have_now=true;
    if(wheel_skew_ms>config->max_wheel_skew_ms) {
        s->wheel_count=s->wheel_head=0;
        ResetPipeline(s);out->status=CHASSIS_REMOTE_NOT_ALIGNED;goto done;
    }
    if(!PushWheel(s,wheel)) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    if(!remote_usable || !remote || !remote->available || !remote->sample.valid ||
       remote->sample.source_age_ms>REMOTE_MAX_SOURCE_AGE_MS ||
       now_ms-remote->first_rx_tick_ms>REMOTE_MAX_KNOWN_AGE_MS-remote->sample.source_age_ms ||
       now_ms-remote->last_rx_tick_ms>REMOTE_MAX_KNOWN_AGE_MS ||
       remote->last_rx_tick_ms-remote->first_rx_tick_ms>CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_IMU_UNAVAILABLE;goto done;
    }
    const uint32_t estimated=out->estimated_sample_ms;
    const uint32_t source_step=remote->sample.tick_ms-s->last_source_ms;
    const uint32_t receive_step=remote->first_rx_tick_ms-s->last_receive_ms;
    const uint32_t estimate_step=estimated-s->last_estimated_ms;
    const uint32_t sequence_step=remote->sample.sequence-s->last_sequence;
    if(s->have_source && !sequence_step && !source_step) {
        if(s->held.valid && now_ms-s->held.tick_ms<=REMOTE_MAX_KNOWN_AGE_MS) PublishHeld(s,out);
        else if(s->held.valid) {ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;}
        else out->status=CHASSIS_REMOTE_WARMUP;
        goto done;
    }
    out->source_step_ms=s->have_source ? source_step : 0;
    out->receive_step_ms=s->have_source ? receive_step : 0;
    const uint32_t jitter=source_step>estimate_step ? source_step-estimate_step : estimate_step-source_step;
    if(s->have_source && (!sequence_step || sequence_step>=HALF_RANGE ||
       source_step<15000U/1000U || source_step>30000U/1000U ||
       !receive_step || receive_step>=HALF_RANGE || !estimate_step || estimate_step>=HALF_RANGE ||
       jitter>config->max_timing_jitter_ms)) {
        RememberSource(s,remote,estimated,source_step,receive_step);
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    ChassisGimbalImuGeometry_s geometry={0};
    /* Reuse the rigid-pose math with A-board axes as its fixed body frame. */
    for(unsigned i=0;i<3;++i) geometry.yaw_origin_b_m[i]=config->imu_offset_body_m[i];
    for(unsigned i=0;i<4;++i) geometry.sensor_to_head_quaternion[i]=config->sensor_to_body_quaternion[i];
    ChassisGimbalImuPose_s pose;
    bool finite=true;
    for(unsigned i=0;i<3;++i)
        finite &= isfinite(remote->sample.accel_n_mps2[i]) && isfinite(remote->sample.gyro_sensor_radps[i]);
    if(!finite || !ChassisGimbalImuPose(&geometry,remote->sample.quaternion,0,0,&pose)) {
        RememberSource(s,remote,estimated,out->source_step_ms,out->receive_step_ms);
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    float paired[2],paired_wheels[CHASSIS_WHEEL_COUNT];
    uint32_t span=0;
    const WheelPairResult_e pairing=PairWheel(s,estimated,now_ms,config->max_pair_span_ms,
        paired,paired_wheels,&span);
    if(pairing!=PAIR_READY) {
        /* This identity has not entered the filter. A later wheel sample may
         * complete its bracket, so do not consume it yet. */
        if(pairing==PAIR_UNAVAILABLE) ResetPipeline(s);
        out->status=CHASSIS_REMOTE_NOT_ALIGNED;
        if(pairing==PAIR_WAIT && s->held.valid && now_ms-s->held.tick_ms<=REMOTE_MAX_KNOWN_AGE_MS)
            PublishHeld(s,out);
        goto done;
    }
    out->pair_span_ms=span;
    RememberSource(s,remote,estimated,out->source_step_ms,out->receive_step_ms);
    /* The 2.4 m and 3.2 m translation captures require wheel +X to follow
     * native board +X. Keep Y and raw IMU/nav data unchanged. */
    ChassisVelocityCenterSample_s sample={.imu_us=remote->sample.tick_ms*1000U,
        .wheel_us=remote->sample.tick_ms*1000U,.wheel_b_mps={paired[0],paired[1]}};
    for(unsigned i=0;i<CHASSIS_WHEEL_COUNT;++i) sample.wheel_mps[i]=paired_wheels[i];
    RotateSensorVectorToBody(config->sensor_to_body_quaternion,
        remote->sample.gyro_sensor_radps,sample.angular_velocity_body_radps);
    for(unsigned i=0;i<3;++i) {
        sample.imu_offset_n_m[i]=pose.imu_offset_nav_m[i];
        sample.accel_n_mps2[i]=remote->sample.accel_n_mps2[i];
    }
    for(unsigned i=0;i<2;++i)
        for(unsigned j=0;j<2;++j) sample.body_to_nav_xy[i][j]=pose.body_to_nav[i][j];
    const unsigned previous_count=s->center.count;
    const uint32_t middle_local_ms=s->center_local_ms[1],middle_span=s->center_pair_span_ms[1];
    ChassisVelocityCenterSample_s center;
    const ChassisVelocityCenterResult_e result=ChassisVelocityCenterUpdate(&s->center,&sample,&center);
    if(result==CHASSIS_CENTER_RESET) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    if(result==CHASSIS_CENTER_WAIT) {
        if(s->center.count>previous_count) {
            s->center_local_ms[previous_count]=estimated;
            s->center_pair_span_ms[previous_count]=span;
        }
        out->status=CHASSIS_REMOTE_WARMUP;goto done;
    }
    s->center_local_ms[0]=s->center_local_ms[1];s->center_local_ms[1]=estimated;
    s->center_pair_span_ms[0]=s->center_pair_span_ms[1];s->center_pair_span_ms[1]=span;
    ChassisWheelRedundancyOutput_s redundancy={.excluded_wheel=UINT8_MAX,
        .reason=config->wheel_redundancy.enabled ? CHASSIS_WHEEL_REDUNDANCY_NO_REFERENCE :
                                                   CHASSIS_WHEEL_REDUNDANCY_OFF};
    bool redundancy_applied=false;
    uint8_t redundancy_confirmations=0;
    if(config->wheel_redundancy.enabled) {
        float reference[3]={0};
        ChassisWheelRedundancyInput_s redundancy_input={.valid=true,.online_mask=15,
            .reference_valid=PredictReference(s,config,&center,reference),
            .reference_vx_mps=reference[0],.reference_vy_mps=reference[1],
            .reference_wz_radps=reference[2]};
        if(ChassisEstimateVelocityWithWheelRedundancy(&config->wheel_kinematics,
               &config->wheel_redundancy,&redundancy_input,center.wheel_mps,&redundancy) &&
           redundancy.corrected) {
            if(s->redundancy_candidate_wheel==redundancy.excluded_wheel) {
                if(s->redundancy_confirmations<UINT8_MAX) ++s->redundancy_confirmations;
            } else {
                s->redundancy_candidate_wheel=redundancy.excluded_wheel;
                s->redundancy_confirmations=1;
            }
            redundancy_confirmations=s->redundancy_confirmations;
            if(CHASSIS_WHEEL_REDUNDANCY_MODE>=2 && config->wheel_redundancy_confirm_samples &&
               s->redundancy_confirmations>=config->wheel_redundancy_confirm_samples) {
                center.wheel_b_mps[0]=redundancy.selected_vx_mps;
                center.wheel_b_mps[1]=redundancy.selected_vy_mps;
                redundancy_applied=true;
            }
        } else {
            s->redundancy_candidate_wheel=UINT8_MAX;
            s->redundancy_confirmations=0;
        }
    }
    out->wheel_redundancy=redundancy;
    out->wheel_redundancy_confirmations=redundancy_confirmations;
    out->wheel_redundancy_applied=redundancy_applied;
    const uint32_t known_age=now_ms-remote->first_rx_tick_ms+remote->sample.source_age_ms;
    ChassisVelocityFusionInput_s input={
        .now_us=(remote->sample.tick_ms+known_age)*1000U,
        .wheel_us=center.imu_us,.imu_us=center.imu_us,.wheel_valid=true,.imu_valid=true};
    for(unsigned i=0;i<2;++i) {
        input.wheel_n_mps[i]=center.body_to_nav_xy[i][0]*center.wheel_b_mps[0]+
            center.body_to_nav_xy[i][1]*center.wheel_b_mps[1];
        input.accel_n_mps2[i]=center.accel_n_mps2[i];
    }
    ChassisVelocityFusionConfig_s effective_filter=config->filter;
    effective_filter.accel_variance=EffectiveAccelVariance(config,center.angular_velocity_body_radps);
    ChassisVelocityFusionOutput_s filtered;
    s->attempt=(ChassisRemoteFusionAttempt_s){
        .sequence=s->attempt.sequence+1U,
        .receiver_call_ms=now_ms,.middle_receiver_ms=middle_local_ms,
        .resets_before=s->resets,.pair_span_ms=middle_span,
        .source_step_ms=out->source_step_ms,.receive_step_ms=out->receive_step_ms,
        .input=input,.config=*config,.available=true,
        .effective_accel_variance=effective_filter.accel_variance,
        .initialized_before=s->filter.initialized};
    ChassisVelocityFusionUpdate(&s->filter,&effective_filter,&input,&filtered);
    s->attempt.output=filtered;
    const float a=center.body_to_nav_xy[0][0],b=center.body_to_nav_xy[0][1];
    const float c=center.body_to_nav_xy[1][0],d=center.body_to_nav_xy[1][1];
    const float det=a*d-b*c;
    if(!filtered.valid || !filtered.fused || !(det>.5f) || now_ms-middle_local_ms>REMOTE_MAX_KNOWN_AGE_MS) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    /* Invert nav -> board axes, which retain the public wheel signs. */
    const float vx=(d*filtered.velocity_n_mps[0]-b*filtered.velocity_n_mps[1])/det;
    const float vy=(a*filtered.velocity_n_mps[1]-c*filtered.velocity_n_mps[0])/det;
    if(!isfinite(vx) || !isfinite(vy)) {
        ResetPipeline(s);out->status=CHASSIS_REMOTE_REJECTED;goto done;
    }
    s->held=(ChassisRemoteFusionOutput_s){.valid=true,.tick_ms=middle_local_ms,
        .vx_mps=vx,.vy_mps=vy,.paired_wheel_mps={center.wheel_b_mps[0],center.wheel_b_mps[1]},
        .center_accel_n_mps2={center.accel_n_mps2[0],center.accel_n_mps2[1]},.pair_span_ms=middle_span,
        .wheel_redundancy=redundancy,.wheel_redundancy_confirmations=redundancy_confirmations,
        .wheel_redundancy_applied=redundancy_applied};
    Increment(&s->updates);
    PublishHeld(s,out);
done:
    out->updates=s->updates;out->resets=s->resets;
    out->attempt=s->attempt;
}
