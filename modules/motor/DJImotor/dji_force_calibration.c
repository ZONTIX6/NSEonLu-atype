#include "dji_force_calibration.h"
#include "bsp_dwt.h"
#include "vehicle_safety.h"
#include "power_meter.h"
#include "runtime_tuning.h"
#include <string.h>
#include <math.h>
/* CPU-only CCM; USB always receives a copied block in ordinary SRAM. */
static uint8_t storage[FC_RECORD_CAPACITY][FC_RECORD_BYTES] __attribute__((section(".ccmram"),aligned(4)));
static FcEngine engine;
static FcCapture capture;
static FcPermit published,cycle_permit;
static DJIMotorInstance *motors[4];
static uint8_t indices[4];
static void (*link_reader)(bool *,uint32_t *);
static FcReason cancellation;
static FcRequest pending,stop_request;
static FcCommandResult result;
static bool queued,stop_queued,result_ready,initialized,owned,recording;
static uint32_t origin_cycles,ticks_per_us,last_cycles,now_us,prepare_cycles,validate_cycles,cycle_seq,cycle_period_us;
static FcRecord control_record;
static uint32_t lock(void) {uint32_t p=__get_PRIMASK();__disable_irq();return p;}
static void unlock(uint32_t p) {__set_PRIMASK(p);}
/* Incremental engine clock works across CYCCNT wrap; capture time uses its own origin. */
static uint32_t clock_now(void)
{
    uint32_t cycles=DWT_GetCycleCount();now_us+=(cycles-last_cycles)/ticks_per_us;last_cycles=cycles;return now_us;
}
static uint32_t relative(uint32_t cycles) {return (cycles-origin_cycles)/ticks_per_us;}
static bool session_active(void) {return engine.status.state>=FC_PREPARING && engine.status.state<=FC_TAIL;}
static FcInput input(const DJIMotorCycle_s *cycle,bool latest)
{
    bool open=false;uint32_t generation=0;link_reader(&open,&generation);
    uint32_t now=HAL_GetTick(),cycles=DWT_GetCycleCount();
    const FcPermit *p=latest ? &published : &cycle_permit;
    const uint32_t age=now-p->command_received_ms;
    PowerMeterStatus_s meter={0};(void)PowerMeterGetStatus(&meter);
    FcInput i={.now_us=clock_now(),.usb_open=open,.usb_generation=generation,
        .bench_permitted=p->bench_permitted && VehicleSafetyAllowsAutotune(now) && VehicleSafetyAllowsDrive(now) &&
            p->safety_epoch==VehicleSafetyEpoch(),
        .command_fresh=age<=10 && p->source_age_ms<=10-age && p->yaw_age_ms<=10-age,
        .aux_stopped=p->aux_stopped && DJIMotorAuxiliaryStopped(motors,now),.load_w=meter.power_w,.budget_w=p->power.power_ceiling_w,
        .meter_fresh=meter.online && (uint32_t)(now-meter.last_rx_tick_ms)<=20,
        .budget_valid=p->power.control_enabled && !p->power.zero_force && (uint32_t)(now-p->power.tick_ms)<=10,
        .zero_submitted=engine.status.zero_submitted};
    for(unsigned w=0;w<4;w++) {
        DJI_Motor_Measure_s m=latest ? motors[w]->measure : cycle->measure[indices[w]];
        i.feedback_seq[w]=m.rx_sequence;i.raw_rpm[w]=m.raw_rpm;i.used_rpm[w]=m.speed_aps/6;
        i.feedback_age_us[w]=(cycles-m.rx_cycles)/ticks_per_us;i.temperature[w]=m.temperature;
        if(!DaemonIsOnline(motors[w]->daemon) || (!m.rx_sequence && !m.rx_cycles) ||
            (uint32_t)(now-m.rx_tick_ms)>5) i.feedback_age_us[w]=UINT32_MAX;
    }
    return i;
}
bool DJIForceCalibrationInit(DJIMotorInstance *const wheel[4],const FcLimits *limits,void (*read_link)(bool *,uint32_t *))
{
    if(initialized || !wheel || !limits || !read_link) return false;
    uint32_t frequency=DWT_GetFrequencyHz();
    if(frequency<1000000 || frequency%1000000 ||
       (uint64_t)(limits->max_on_ms+limits->pre_ms+limits->tail_ms)*frequency/1000>=UINT32_MAX) return false;
    for(unsigned w=0;w<4;w++) {
        if(!wheel[w] || !DJIMotorFindIndex(wheel[w],&indices[w])) return false;
        motors[w]=wheel[w];
    }
    FcInit(&engine,limits);if(!engine.valid_limits) return false;
    FcCaptureInit(&capture,&storage[0][0],FC_RECORD_CAPACITY);
    ticks_per_us=frequency/1000000;last_cycles=DWT_GetCycleCount();link_reader=read_link;
    if(!DJIMotorRegisterFeedbackObserver(DJIForceCalibrationFeedback)) return false;
    initialized=true;return true;
}
void DJIForceCalibrationPublish(const FcPermit *p)
{
    if(!initialized || !p) return;
    uint32_t key=lock();published=*p;
    for(unsigned w=0;w<4;w++) motors[w]->stop_flag=p->bench_permitted ? MOTOR_ENALBED : MOTOR_STOP;
    unlock(key);
}
void DJIForceCalibrationReadControl(void) {cycle_permit=published;}
void DJIForceCalibrationCancel(FcReason reason)
{
    uint32_t key=lock();if(!cancellation) cancellation=reason;queued=false;unlock(key);
}
bool DJIForceCalibrationSubmit(const FcRequest *r)
{
    if(!initialized || !r) return false;
    uint32_t key=lock();bool open=false;uint32_t generation=0;link_reader(&open,&generation);
    if(!open || generation!=r->generation) {unlock(key);return false;}
    if(r->op==FC_OP_STOP) {
        if(r->session && r->session!=engine.status.session) {unlock(key);return false;}
        stop_request=*r;stop_queued=true;queued=false;
        if(!cancellation) cancellation=FC_REASON_STOP;
        unlock(key);return true;
    }
    if(queued || result_ready || stop_queued || cancellation) {unlock(key);return false;}
    pending=*r;queued=true;unlock(key);return true;
}
static void status(FcStatus *s)
{
    FcGetStatus(&engine,s);s->records=capture.count;s->expected_records=capture.capacity;
    s->complete=capture.frozen && capture.complete;
    if(capture.meta.capture_id) {
        s->capture_id=capture.meta.capture_id;s->capture_session=capture.meta.session;s->capture_generation=capture.meta.generation;
    }
}
static void consume(const FcInput *i)
{
    if(cancellation) {FcCancel(&engine,cancellation,i->now_us);cancellation=FC_REASON_NONE;}
    FcRequest r;
    if(stop_queued) {r=stop_request;stop_queued=false;result_ready=false;}
    else if(queued && !result_ready) {r=pending;queued=false;}
    else return;
    FcCommandError error=FC_ERROR_NONE;
    if(r.generation!=i->usb_generation || !i->usb_open) error=FC_ERROR_GENERATION;
    else if(r.op==FC_OP_ARM) {
        if(session_active()) error=FC_ERROR_BUSY;
        else if(!RuntimeTuningBeginCalibration()) error=FC_ERROR_OWNERSHIP;
        else {uint32_t session;if(!FcArm(&engine,i,&session)) {RuntimeTuningEndCalibration();error=FC_ERROR_GUARD;} else owned=true;}
    } else if(r.op==FC_OP_START) {
        if(r.session!=engine.status.session) error=FC_ERROR_SESSION;
        else if(!FcStart(&engine,r.session,&r.pulse,i)) error=engine.status.state==FC_READY ? FC_ERROR_RANGE : FC_ERROR_GUARD;
        else {
            FcCaptureMeta meta={.capture_id=engine.status.capture_id,.session=r.session,.generation=r.generation,
                .pulse=r.pulse,.limits=engine.status.limits,.speed_lpf_pm=850,.current_lpf_pm=900};
            origin_cycles=DWT_GetCycleCount();FcCaptureBegin(&capture,&meta);recording=true;
        }
    } else if(r.op==FC_OP_PING) {
        if(!FcHeartbeat(&engine,r.session,r.generation,i->now_us)) error=FC_ERROR_SESSION;
    } else if(r.op==FC_OP_STOP) FcCancel(&engine,FC_REASON_STOP,i->now_us);
    else if(r.op==FC_OP_CLEAR) {
        if(session_active()) error=FC_ERROR_BUSY;
        else if(r.capture_id!=capture.meta.capture_id) error=FC_ERROR_CAPTURE;
        else {FcCaptureInit(&capture,&storage[0][0],FC_RECORD_CAPACITY);engine.status.capture_id=0;
            engine.status.capture_session=engine.status.capture_generation=0;engine.status.state=FC_IDLE;}
    } else error=FC_ERROR_RANGE;
    result.request_id=r.request_id;result.error=error;status(&result.status);result_ready=true;
}
void DJIForceCalibrationPrepare(const DJIMotorCycle_s *cycle)
{
    if(!initialized) return;
    uint32_t key=lock();prepare_cycles=DWT_GetCycleCount();uint32_t previous=engine.last_us;
    FcInput i=input(cycle,false);cycle_period_us=i.now_us-previous;consume(&i);
    if(recording && capture.count>=capture.capacity-FC_RESERVED_RECORDS) FcCancel(&engine,FC_REASON_CAPACITY,i.now_us);
    FcDrive drive=FcStep(&engine,&i);
    for(unsigned w=0;w<4;w++) cycle->current[indices[w]]=drive.current[w];
    const unsigned w=engine.status.pulse.wheel;const DJI_Motor_Measure_s *m=&cycle->measure[indices[w]];
    control_record=(FcRecord){.type=FC_CONTROL,.wheel=(uint8_t)w,.state=(uint8_t)drive.state,
        .sequence=++cycle_seq,.time_us=relative(prepare_cycles),
        .feedback_time_us=(int32_t)(m->rx_cycles-origin_cycles)/(int32_t)ticks_per_us,
        .raw_rpm=m->raw_rpm,.raw_current=m->raw_current,.used_rpm=m->speed_aps/6,
        .requested=drive.current[w],.temperature=m->temperature,.cycle_us=cycle_period_us};
    unlock(key);
}
bool DJIForceCalibrationValidate(const DJIMotorCycle_s *cycle,const int16_t commands[DJI_MOTOR_CNT],const bool enabled[DJI_MOTOR_CNT],uint32_t now)
{
    (void)now;if(!initialized) return false;
    uint32_t key=lock();validate_cycles=DWT_GetCycleCount();
    if(cancellation) FcCancel(&engine,cancellation,clock_now());
    FcInput i=input(cycle,true);int16_t current[4];bool permitted=true,zero=true;
    for(unsigned w=0;w<4;w++) {
        unsigned n=indices[w];current[w]=commands[n];zero &= current[w]==0;
        permitted &= enabled[n] && motors[w]->stop_flag!=MOTOR_STOP && cycle->stop[n]!=MOTOR_STOP &&
            (uint32_t)(DWT_GetCycleCount()-cycle->measure[n].rx_cycles)/ticks_per_us<=FC_MAX_FEEDBACK_US;
    }
    if(!permitted && !zero) FcCancel(&engine,FC_REASON_PERMISSION,i.now_us);
    const bool valid=(zero || permitted) && FcValidateFinal(&engine,&i,current) && (!cancellation || zero);
    unlock(key);return valid;
}
static uint16_t duration(uint32_t cycles)
{
    uint32_t us=cycles/ticks_per_us;
    if(us>UINT16_MAX) {control_record.flags|=FC_FLAG_TIME_OVERFLOW;return UINT16_MAX;}
    return (uint16_t)us;
}
void DJIForceCalibrationRecord(const DJIMotorCycle_s *cycle,const int16_t commands[DJI_MOTOR_CNT],uint8_t attempted,uint8_t successful)
{
    (void)cycle;if(!initialized) return;
    uint32_t key=lock();uint8_t mask=(uint8_t)(1U<<motors[0]->sender_group);
    for(unsigned w=0;w<4;w++) {control_record.final[w]=commands[indices[w]];
        if(motors[w]->stop_flag!=MOTOR_STOP) control_record.enabled_mask|=(uint8_t)(1U<<w);}
    control_record.attempted=attempted&mask;control_record.successful=successful&mask;
    control_record.prepare_us=duration(validate_cycles-prepare_cycles);
    control_record.submit_us=duration(DWT_GetCycleCount()-validate_cycles);
    FcRecordSubmission(&engine,clock_now(),control_record.final,(attempted&successful&mask)==mask);
    control_record.state=(uint8_t)engine.status.state;control_record.reason=(uint16_t)engine.status.reason;
    if(recording) {
        if(!FcCaptureAppend(&capture,&control_record)) FcCancel(&engine,FC_REASON_CAPACITY,now_us);
        if(engine.status.state==FC_FROZEN || engine.status.state==FC_ABORTED) {
            FcCaptureFreeze(&capture,engine.status.reason,engine.status.complete);recording=false;
        }
    }
    if(owned && !session_active()) {RuntimeTuningEndCalibration();owned=false;}
    unlock(key);
}
void DJIForceCalibrationFeedback(const DJIMotorInstance *motor,int16_t rpm,int16_t current,uint32_t cycles)
{
    if(!initialized) return;
    uint32_t key=lock();unsigned w=engine.status.pulse.wheel;
    if(recording && motor==motors[w]) {
        FcRecord r={.type=FC_RX,.wheel=(uint8_t)w,.state=(uint8_t)engine.status.state,
            .sequence=motor->measure.rx_sequence,.time_us=relative(cycles),.raw_rpm=rpm,.raw_current=current,
            .temperature=motor->measure.temperature};
        if(!FcCaptureAppend(&capture,&r) || capture.count>=capture.capacity-FC_RESERVED_RECORDS) {
            if(!cancellation) cancellation=FC_REASON_CAPACITY;
        }
    }
    unlock(key);
}
void DJIForceCalibrationGetStatus(FcStatus *s)
{
    uint32_t key=lock();status(s);
    if(initialized) {bool open;link_reader(&open,&s->generation);}
    unlock(key);
}
bool DJIForceCalibrationReply(FcCommandResult *r)
{
    uint32_t key=lock();bool ready=result_ready;if(ready) *r=result;unlock(key);return ready;
}
void DJIForceCalibrationReplyAccepted(uint32_t id)
{
    uint32_t key=lock();if(result_ready && result.request_id==id) result_ready=false;unlock(key);
}
bool DJIForceCalibrationRead(uint32_t id,uint16_t first,uint8_t count,uint8_t *bytes,size_t capacity)
{
    uint32_t key=lock();bool ok=id==capture.meta.capture_id && engine.status.zero_submitted &&
        FcCaptureRead(&capture,first,count,bytes,capacity);unlock(key);return ok;
}
