#ifndef POWER_OBSERVER_H
#define POWER_OBSERVER_H
#include "power_prediction.h"
#define PP_HISTORY_LENGTH 32U
typedef struct {
    uint32_t tick_ms,control_version,model_version,config_version,strategy_version;
    int16_t command[4];
    uint8_t enabled_mask;
    bool submitted;
} PPFinalActuation;
typedef struct {
    uint32_t now_ms,rx_ms[4],sequence[4],model_version;
    float current[4],omega[4];
    uint8_t online_mask;
} PPMeasurement;
typedef struct {
    PPState state;
    PPFinalActuation history[PP_HISTORY_LENGTH];
    uint32_t sequence[4],rx_ms[4],last_record_ms;
    uint8_t head,count;
    bool sampled[4],recorded;
} PPObserver;
bool PPObserverRecord(PPObserver *observer,const PPFinalActuation *final);
bool PPObserverCommandAt(const PPObserver *observer,uint32_t sample_ms,uint32_t max_age_ms,
                         PPFinalActuation *final);
/* The endpoint predictor supports one held command during delay. Reject a
 * changing pipeline instead of pretending the latest submitted command executed. */
bool PPObserverStableCommand(const PPObserver *,uint32_t now_ms,uint32_t delay_ms,
                             uint32_t model_version,uint32_t config_version,PPFinalActuation *);
/* Receive-time observation only: callers must include unknown sample delay in
 * the model error envelope. This interface does not claim a synchronized EKF. */
bool PPObserverUpdate(PPObserver *observer,const PPMeasurement *measurement);
#endif
