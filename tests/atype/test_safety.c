#include "vehicle_safety.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(!VehicleSafetyAllowsDrive(0));
    VehicleSafetyInput_s input = {.input_online=true, .reset=true};
    VehicleSafetyPublish(&input, 100);
    assert(VehicleSafetyAllowsDrive(100));
    assert(!VehicleSafetyAllowsDrive(111));
    input.emergency=true;
    VehicleSafetyPublish(&input, 112);
    assert(!VehicleSafetyAllowsDrive(112)); /* reset cannot override emergency */
    input.emergency=false; input.reset=false;
    VehicleSafetyPublish(&input, 113);
    assert(!VehicleSafetyAllowsDrive(113)); /* releasing dial alone cannot restart */
    input.reset=true; input.bench=true;
    VehicleSafetyPublish(&input, 114);
    assert(VehicleSafetyAllowsDrive(114));
    assert(VehicleSafetyAllowsAutotune(114));
    input.input_online=false;
    VehicleSafetyPublish(&input, 115);
    assert(!VehicleSafetyAllowsDrive(115));
    assert(!VehicleSafetyAllowsAutotune(115));
    input.input_online=true; input.bench=false;
    VehicleSafetyPublish(&input, 0xfffffffeU);
    assert(VehicleSafetyAllowsDrive(2)); /* tick wrap */
    assert(!VehicleSafetyAllowsAutotune(2));
    input.reset=false;
    VehicleSafetyPublish(&input,20);
    assert(!VehicleSafetyAllowsDrive(20)); /* stalled command producer must rearm */
    puts("PASS: vehicle safety latching, freshness and bench gate");
}
