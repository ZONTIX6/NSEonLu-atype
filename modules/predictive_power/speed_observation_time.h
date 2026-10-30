#ifndef SPEED_OBSERVATION_TIME_H
#define SPEED_OBSERVATION_TIME_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t now_us;
    uint32_t cycles, tick_ms, hz, source_epoch, fraction;
    bool initialized;
} PPObservationClock;
/* First valid call establishes a local time origin. Ambiguous intervals or
 * frequency changes rebase and advance source_epoch, returning false once. */
bool PPObservationClockUpdate(PPObservationClock *clock,uint32_t cycles,
                              uint32_t tick_ms,uint32_t hz);
bool PPObservationReceiveTime(const PPObservationClock *clock,
    uint32_t rx_cycles,uint32_t rx_tick_ms,uint32_t max_age_us,uint64_t *rx_us);
#endif
