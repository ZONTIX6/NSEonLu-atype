#ifndef CHASSIS_SPEED_OBSERVATION_H
#define CHASSIS_SPEED_OBSERVATION_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t sequence,action,capture_id,acknowledged_sequence,result;
} ChassisSpeedObservationRequest;
typedef struct {
    uint32_t period_us,execution_us,period_max_us,execution_max_us,overruns;
    uint32_t period_histogram[8],execution_histogram[8];
    uint32_t motor_stack_words,robot_stack_words,observation_stack_words,heap_free_bytes,heap_min_bytes;
    uint32_t config_result;
} ChassisSpeedObservationDiagnostics;
extern volatile ChassisSpeedObservationRequest chassis_speed_observation_request;
extern volatile ChassisSpeedObservationDiagnostics chassis_speed_observation_diagnostics;
void ChassisSpeedObservationInit(void);
void ChassisSpeedObservationMotorBegin(uint32_t cycles,uint32_t tick_ms);
void ChassisSpeedObservationMotorEnd(uint32_t cycles,uint32_t tick_ms);
void ChassisSpeedObservationTask(void);
/* Arm/Freeze run in the motor task, before this cycle's observer correction. */
bool ChassisSpeedObservationArm(uint32_t capture_id);
void ChassisSpeedObservationFreeze(void);
void ChassisSpeedObservationResources(uint32_t motor_words,uint32_t robot_words,
                                     uint32_t observer_words,uint32_t heap_free,uint32_t heap_min);
#endif
