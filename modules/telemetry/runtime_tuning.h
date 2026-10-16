#ifndef RUNTIME_TUNING_H
#define RUNTIME_TUNING_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { TUNING_PID, TUNING_POWER, TUNING_SPIN, TUNING_GROUP_COUNT } TuningGroup;
#define TUNING_MAX_VALUES 24U
#define TUNING_LINE_SIZE 512U
/* PID: each LF/RF/LB/RB has Kp, Ki, Kd, MaxOut, IntegralLimit, DeadBand.
 * Power: outer loop's 11 fields, boost, rise, demand margin, empty/full ratio,
 *        buffer boost, recovery power, recovery target time.
 * Spin: ChassisSpinConfig fields in declaration order, including direction.
 * Register once from initialized objects; Process only in the owning task.
 * Protocol and reply functions belong to the USB telemetry task. */
unsigned RuntimeTuningCount(TuningGroup group);
void RuntimeTuningRegister(TuningGroup group, const float *values, unsigned mode);
/* Initialization only: validate PC-saved build defaults through the consumer. */
void RuntimeTuningRestore(TuningGroup group, bool (*apply)(const float *values));
void RuntimeTuningSubmit(const char *line);
void RuntimeTuningProcess(TuningGroup group, bool (*apply)(const float *values));
size_t RuntimeTuningReply(char *buffer, size_t capacity);
void RuntimeTuningReplySent(void);
void RuntimeTuningCancel(void);
#endif
