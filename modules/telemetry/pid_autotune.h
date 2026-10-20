#ifndef PID_AUTOTUNE_H
#define PID_AUTOTUNE_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {PID_AUTO_IDLE,PID_AUTO_READY,PID_AUTO_RUNNING,PID_AUTO_DONE,PID_AUTO_ABORTED} PIDAutotuneState;
typedef enum {PID_AUTO_NONE,PID_AUTO_STOPPED,PID_AUTO_TIMEOUT,PID_AUTO_GUARD,
    PID_AUTO_FEEDBACK,PID_AUTO_OVERSPEED,PID_AUTO_LIMITED,PID_AUTO_NO_RESPONSE,
    PID_AUTO_CONFIG,PID_AUTO_CAN,PID_AUTO_QUALITY,PID_AUTO_STATIONARY} PIDAutotuneReason;
typedef struct {unsigned wheel; float rpm; uint32_t hold_ms; float current_limit,kp,ki,kd;} PIDAutotuneTrial;
typedef struct {
    uint32_t now_ms; bool permitted,feedback_ok,can_ok; float rpm[4];
    uint32_t rx_sequence[4],rx_ms[4];
} PIDAutotuneFeedback;
typedef struct {
    bool active;
    uint32_t session,trial,lease_ms,begin_ms,start_ms,samples,limited_samples,steady_samples,last_outside;
    PIDAutotuneState state;
    PIDAutotuneReason reason;
    PIDAutotuneTrial test;
    double error2,steady_sum,current_variation;
    float peak,previous_current,rmse,overshoot,steady,jitter;
    uint32_t settle_ms,limited_pm;
    bool v2,stationary_seen,observed,metrics_closed,previous_limited;
    unsigned phase;
    uint32_t stationary_since,last_tick,step_ms,previous_ms,previous_sequence,previous_rx_ms;
    uint32_t window_ms,coverage_ms,steady_ms,limited_ms,feedback_count,max_feedback_ms,max_stat_ms,min_stat_ms,stat_limit_ms;
    float previous_error;
} PIDAutotune;
bool PIDAutotuneBegin(PIDAutotune *a,uint32_t session,const PIDAutotuneFeedback *f);
bool PIDAutotuneBeginV2(PIDAutotune *a,uint32_t session,const PIDAutotuneFeedback *f);
bool PIDAutotuneStart(PIDAutotune *a,const PIDAutotuneTrial *test,const PIDAutotuneFeedback *f);
void PIDAutotuneKeepAlive(PIDAutotune *a,uint32_t now);
void PIDAutotuneStop(PIDAutotune *a,PIDAutotuneReason reason);
void PIDAutotuneTick(PIDAutotune *a,const PIDAutotuneFeedback *f);
float PIDAutotuneTarget(const PIDAutotune *a,uint32_t now);
void PIDAutotuneObserve(PIDAutotune *a,uint32_t now,float rpm,float raw_current,float final_current,bool limited);
void PIDAutotuneObserveV2(PIDAutotune *a,const PIDAutotuneFeedback *f,float rpm,float raw_current,float final_current,bool limited);
#endif
