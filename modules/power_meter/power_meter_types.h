#ifndef POWER_METER_TYPES_H
#define POWER_METER_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool online;
    float shunt_voltage_v;
    float bus_voltage_v;
    float current_a;
    float power_w;
    uint32_t last_rx_tick_ms;
    uint32_t rx_sequence; /* Accepted receive events, including same-tick frames. */
} PowerMeterStatus_s;

#endif
