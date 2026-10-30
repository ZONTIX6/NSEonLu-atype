#ifndef CHASSIS_SPEED_CAPTURE_H
#define CHASSIS_SPEED_CAPTURE_H
#include <stdbool.h>
#include <stdint.h>

#define CSC_RAW_CAPACITY 192U
#define CSC_CYCLE_CAPACITY 32U
#define CSC_RAW_BYTES 40U
#define CSC_CYCLE_BYTES 512U
#define CSC_METADATA_BYTES 1024U
#define CSC_END_BYTES 48U
#define CSC_MAX_FRAME_BYTES 1044U
#define CSC_KIND_METADATA 1U
#define CSC_KIND_RAW 2U
#define CSC_KIND_CYCLE 3U
#define CSC_KIND_END 4U
#define CSC_FREEZE_RAW_FULL 1U
#define CSC_FREEZE_CYCLE_FULL 2U
#define CSC_FREEZE_MANUAL 3U
#define CSC_FREEZE_VERSION 4U
#define CSC_FREEZE_ERROR 5U

typedef enum { CSC_IDLE, CSC_ARMED, CSC_RECORDING, CSC_FROZEN, CSC_DRAINING } CSCState;
typedef struct { uint8_t bytes[CSC_RAW_BYTES]; } CSCRawRecord;
typedef struct { uint8_t bytes[CSC_CYCLE_BYTES]; } CSCCycleRecord;
typedef struct { uint8_t bytes[CSC_METADATA_BYTES]; } CSCMetadata;
typedef struct {
    CSCRawRecord raw[CSC_RAW_CAPACITY];
    CSCCycleRecord cycle[CSC_CYCLE_CAPACITY];
    CSCMetadata metadata;
    uint8_t end_payload[CSC_END_BYTES];
    uint32_t capture_id,rejected_records,outside_window,transport_retries;
    uint32_t session_outside_window,session_transport_retries;
    uint32_t start_tick_ms,end_tick_ms,start_cycles,end_cycles;
    uint16_t raw_count,cycle_count,drain_index;
    uint8_t drain_kind,freeze_reason;
    CSCState state;
} CSCCapture;

/* The caller serializes Arm/Append/Freeze/BeginDrain with a bounded critical
 * section. Append only copies one pre-encoded record and publishes counters;
 * it never waits, computes CRC, allocates or transmits. Prepare metadata before
 * entering the critical section. Drain operates only on frozen immutable data.
 * No instance is allocated here: disabled firmware need not reserve capture RAM. */
bool CSCArm(CSCCapture *,uint32_t capture_id,const CSCMetadata *);
bool CSCAppendRaw(CSCCapture *,const CSCRawRecord *);
bool CSCAppendCycle(CSCCapture *,const CSCCycleRecord *);
void CSCFreeze(CSCCapture *);
void CSCFreezeReason(CSCCapture *,uint8_t reason);
bool CSCBeginDrain(CSCCapture *);
bool CSCPeekFrame(const CSCCapture *,uint8_t *dst,uint16_t capacity,uint16_t *length);
void CSCFrameAccepted(CSCCapture *);
void CSCTransportRetry(CSCCapture *);
void CSCDrainComplete(CSCCapture *,bool transport_idle);
bool CSCFrameIsValid(const uint8_t *,uint16_t length);
void CSCMetadataInit(CSCMetadata *,uint32_t capture_id,uint32_t hz);

/* Explicit little-endian wire helpers; no alignment or native struct ABI. */
void CSCWriteU16(uint8_t *,uint16_t);
void CSCWriteU32(uint8_t *,uint32_t);
void CSCWriteU64(uint8_t *,uint64_t);
void CSCWriteF32(uint8_t *,float);
uint16_t CSCReadU16(const uint8_t *);
uint32_t CSCReadU32(const uint8_t *);
uint64_t CSCReadU64(const uint8_t *);
float CSCReadF32(const uint8_t *);
#endif
