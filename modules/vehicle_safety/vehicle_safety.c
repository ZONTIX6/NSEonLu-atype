#include "vehicle_safety.h"
#include <stddef.h>
#ifdef STM32F427xx
#include "FreeRTOS.h"
#include "task.h"
#define LOCK() taskENTER_CRITICAL()
#define UNLOCK() taskEXIT_CRITICAL()
#else
#define LOCK() ((void)0)
#define UNLOCK() ((void)0)
#endif

static bool valid, ready, bench;
static bool aux_stopped;
static uint32_t tick;
static uint32_t safety_epoch;

void VehicleSafetyPublish(const VehicleSafetyInput_s *input, uint32_t now_ms)
{
    if (input == NULL) return;
    LOCK();
    const bool was_ready=ready;
    const bool expired=valid && (uint32_t)(now_ms-tick)>10U;
    if (valid && (uint32_t)(now_ms-tick)>10U) ready=false;
    if (!input->input_online || input->emergency) ready = false;
    else if (input->reset) ready = true;
    if(!valid || expired || was_ready!=ready) ++safety_epoch;
    bench = input->bench && input->input_online && !input->emergency;
    aux_stopped=input->aux_stopped;
    tick = now_ms;
    valid = true;
    UNLOCK();
}

uint32_t VehicleSafetyEpoch(void)
{
    LOCK();const uint32_t epoch=safety_epoch;UNLOCK();return epoch;
}

bool VehicleSafetyAllowsDrive(uint32_t now_ms)
{
    LOCK();
    bool allowed = valid && ready && (uint32_t)(now_ms - tick) <= 10U;
    UNLOCK();
    return allowed;
}

bool VehicleSafetyAllowsAutotune(uint32_t now_ms)
{
    LOCK();
    bool allowed = valid && ready && bench && (uint32_t)(now_ms - tick) <= 10U;
    UNLOCK();
    return allowed;
}

bool VehicleSafetyIsStopped(uint32_t now_ms)
{
    LOCK();
    bool stopped = valid && !ready && (uint32_t)(now_ms - tick) <= 10U;
    UNLOCK();
    return stopped;
}

bool VehicleSafetyAuxStopped(uint32_t now_ms)
{
    LOCK();
    bool stopped=valid && aux_stopped && (uint32_t)(now_ms-tick)<=10U;
    UNLOCK();
    return stopped;
}
