#ifndef WHEEL_SPEED_KF_H
#define WHEEL_SPEED_KF_H
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PP_KF_OK, PP_KF_INITIALIZED, PP_KF_DUPLICATE, PP_KF_TIME,
    PP_KF_VALUE, PP_KF_INNOVATION, PP_KF_STALE, PP_KF_CONFIG, PP_KF_NUMERIC
} PPWheelKFReason;
typedef struct {
    uint32_t version, max_gap_us, max_age_us, max_horizon_us;
    float q, r, initial_p00, initial_p11, gate_squared;
    uint8_t max_rejections;
} PPWheelKFConfig;
typedef struct {
    uint64_t rx_time_us;
    uint32_t sequence, source_epoch;
    float omega_rad_s;
    bool online;
} PPWheelSample;
typedef struct {
    float omega, alpha, p00, p01, p11, innovation, innovation_variance;
    uint64_t corrected_time_us, output_time_us;
    uint32_t sequence, source_epoch, config_version;
    bool valid;
    PPWheelKFReason reason;
} PPWheelEstimate;
typedef struct {
    PPWheelEstimate corrected;
    uint64_t seen_time_us;
    uint32_t seen_sequence, seen_source_epoch;
    uint8_t rejected_consecutive;
    bool seen, initialized;
} PPWheelKF;

bool PPWheelKFConfigValid(const PPWheelKFConfig *config);
void PPWheelKFReset(PPWheelKF *filter);
/* Every receive event is consumed at most once, including rejected events.
 * Time is receive time, not an assertion of synchronized motor sample time. */
PPWheelKFReason PPWheelKFCorrect(PPWheelKF *filter,const PPWheelKFConfig *config,
                                const PPWheelSample *sample);
/* Pure projection: never advances the corrected state or its freshness.
 * Freshness at now and the requested future horizon are bounded separately. */
bool PPWheelKFPredict(const PPWheelKF *filter,const PPWheelKFConfig *config,
                     uint64_t now_us,uint32_t horizon_us,PPWheelEstimate *output);
#endif
