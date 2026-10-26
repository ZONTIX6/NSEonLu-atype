#ifndef FORCE_CALIBRATION_CAPTURE_H
#define FORCE_CALIBRATION_CAPTURE_H
#include "force_calibration.h"
#include <stddef.h>
enum { FC_RX=1, FC_CONTROL=2, FC_MARKER=3, FC_FLAG_TIME_OVERFLOW=1, FC_FLAG_FEEDBACK_GAP=2 };
typedef struct {
    uint8_t type,wheel,state,flags;
    uint32_t sequence,time_us;int32_t feedback_time_us;
    int16_t raw_rpm,raw_current;float used_rpm;int16_t requested,final[4];
    uint8_t temperature,enabled_mask;uint32_t cycle_us;
    uint16_t prepare_us,submit_us,reason;uint8_t attempted,successful;
} FcRecord;
typedef struct {
    uint32_t capture_id,session,generation;FcPulse pulse;FcLimits limits;
    uint16_t speed_lpf_pm,current_lpf_pm;
} FcCaptureMeta;
typedef struct {
    uint8_t *storage;uint16_t capacity,count;FcCaptureMeta meta;
    uint32_t dropped,rx_gaps,rx_duplicates,rx_min_us,rx_max_us,control_min_us,control_max_us;
    uint32_t last_rx_us,last_control_us,last_rx_seq;
    FcReason reason;bool frozen,complete,quality,has_rx,has_control;
} FcCapture;
void FcCaptureInit(FcCapture *capture,uint8_t *storage,uint16_t capacity);
void FcCaptureBegin(FcCapture *capture,const FcCaptureMeta *metadata);
bool FcCaptureAppend(FcCapture *capture,const FcRecord *record);
void FcCaptureFreeze(FcCapture *capture,FcReason reason,bool complete);
bool FcCaptureRead(const FcCapture *capture,uint16_t first,uint8_t count,uint8_t *bytes,size_t capacity);
bool FcEncodeRecord(const FcRecord *record,uint8_t bytes[48]);
#endif
