#ifndef ENERGY_SCHEDULER_H
#define ENERGY_SCHEDULER_H
#include <stdbool.h>
#include <stdint.h>
#define PP_ENERGY_NODES 10U
#define PP_ENERGY_BLOCKS 5U
typedef struct {
    float buffer_max_j,buffer_floor_j,buffer_terminal_j;
    float cap_max_j,cap_floor_j,cap_terminal_j,eta_charge,eta_discharge;
    float charge_max_w,discharge_max_w,input_max_w,load_max_w,input_slew_w_s;
    float path_loss_w,leak_w;
} PPEnergyConfig;
typedef struct {
    uint32_t now_ms,strategy_version,model_version,capability_version;
    float buffer_low_j,buffer_high_j,cap_low_j,cap_high_j;
    float referee_w,demand_w,current_request_w,input_low_w,input_high_w,load_floor_w;
    bool permitted,buffer_valid,capacitor,cap_valid,strong_confirmation;
} PPEnergyInput;
typedef struct {
    float request_w[PP_ENERGY_BLOCKS],load_w[PP_ENERGY_BLOCKS];
    uint32_t created_ms,strategy_version,model_version,capability_version;
    bool valid,recovering;
} PPEnergyPlan;
typedef struct {
    float buffer_low_j,buffer_high_j,cap_low_j,cap_high_j;
    float minimum_buffer_j,minimum_cap_j;
    bool feasible;
} PPEnergyTrace;
/* Original interval energy equations, including the low-load overcharge case. */
bool PPEnergyCheck(const PPEnergyConfig *,const PPEnergyInput *,const PPEnergyPlan *,PPEnergyTrace *);
/* Conservative 10-node / 5-block feasibility planner. It deliberately grants no
 * future benefit to a new input request without an identified response bound.
 * This is not the full energy QP MPC described in the architecture proposal. */
bool PPEnergySchedule(const PPEnergyConfig *,const PPEnergyInput *,PPEnergyPlan *);
#endif
