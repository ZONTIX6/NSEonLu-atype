#ifndef FORCE_CALIBRATION_H
#define FORCE_CALIBRATION_H
#include "force_calibration_config.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum { FC_IDLE, FC_PREPARING, FC_READY, FC_PRE, FC_RISE, FC_HOLD,
    FC_FALL, FC_TAIL, FC_FROZEN, FC_ABORTED } FcState;
typedef enum { FC_REASON_NONE, FC_REASON_STOP, FC_REASON_USB, FC_REASON_HEARTBEAT,
    FC_REASON_PERMISSION, FC_REASON_FEEDBACK, FC_REASON_OVERSPEED, FC_REASON_TEMPERATURE,
    FC_REASON_POWER, FC_REASON_TIMING, FC_REASON_CAN, FC_REASON_CAPACITY, FC_REASON_PROTOCOL } FcReason;
typedef struct {
    int16_t max_current;
    uint16_t max_on_ms, max_rpm, stationary_rpm, stationary_ms, heartbeat_ms, pre_ms, tail_ms;
    uint8_t stop_temperature_c;
} FcLimits;
typedef struct { uint8_t wheel; int16_t current; uint16_t rise_ms, hold_ms, fall_ms; } FcPulse;
typedef struct {
    uint32_t now_us, usb_generation, feedback_seq[4], feedback_age_us[4];
    int16_t raw_rpm[4];
    float used_rpm[4], load_w, budget_w;
    uint8_t temperature[4];
    bool usb_open, bench_permitted, command_fresh, aux_stopped, meter_fresh, budget_valid, zero_submitted;
} FcInput;
typedef struct { int16_t current[4]; FcState state; FcReason reason; bool recording; } FcDrive;
typedef struct {
    uint32_t session, capture_id, generation, capture_session, capture_generation;
    FcState state; FcReason reason; FcPulse pulse; FcLimits limits;
    uint16_t records, expected_records;
    bool complete, zero_submitted;
    int16_t requested, final[4], raw_rpm[4];
    float used_rpm[4], load_w, budget_w;
    uint8_t temperature[4]; uint32_t feedback_age_us[4], zero_submitted_us;
} FcStatus;
typedef enum { FC_OP_ARM, FC_OP_START, FC_OP_PING, FC_OP_STOP, FC_OP_CLEAR } FcOperation;
typedef struct {
    FcOperation op; uint32_t request_id, generation, session, capture_id; FcPulse pulse;
} FcRequest;
typedef enum { FC_ERROR_NONE, FC_ERROR_BUSY, FC_ERROR_GENERATION, FC_ERROR_SESSION,
    FC_ERROR_GUARD, FC_ERROR_RANGE, FC_ERROR_CAPTURE, FC_ERROR_OWNERSHIP } FcCommandError;
typedef struct { uint32_t request_id; FcStatus status; FcCommandError error; } FcCommandResult;
typedef struct {
    FcStatus status;
    uint32_t last_us, heartbeat_us, stationary_us, start_us, tail_us, next_session, next_capture;
    float start_budget_w;
    bool valid_limits, clock_started, stationary, started;
} FcEngine;
void FcInit(FcEngine *engine, const FcLimits *limits);
bool FcArm(FcEngine *engine, const FcInput *input, uint32_t *session);
bool FcStart(FcEngine *engine, uint32_t session, const FcPulse *pulse, const FcInput *input);
bool FcHeartbeat(FcEngine *engine, uint32_t session, uint32_t generation, uint32_t now_us);
void FcCancel(FcEngine *engine, FcReason reason, uint32_t now_us);
FcDrive FcStep(FcEngine *engine, const FcInput *input);
bool FcValidateFinal(FcEngine *engine, const FcInput *input, const int16_t current[4]);
void FcRecordSubmission(FcEngine *engine, uint32_t now_us, const int16_t current[4], bool submitted);
void FcGetStatus(const FcEngine *engine, FcStatus *status);
#endif
