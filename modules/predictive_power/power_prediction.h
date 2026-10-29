#ifndef POWER_PREDICTION_H
#define POWER_PREDICTION_H
#include <stdbool.h>
#include <stdint.h>

/* Rotor rad/s, equivalent feedback-current units and physical boundary watts.
 * Current units become amperes only after an independently validated scaling. */
typedef struct {
    uint32_t schema,version,checksum;
    float command_scale,horizon_s,max_dt_s,max_speed,max_current,max_command;
    float current_tau[4],current_gain[4],speed_gain[4],speed_drag[4];
    float base_w,resistance,mechanical,speed_loss,speed_squared_loss;
    float power_margin_w,speed_error,current_error,acceleration_bound;
    uint32_t command_delay_ms;
    bool braking_valid;
} PPModel;
typedef struct {
    float current[4],omega[4];
    float applied_command[4];
    uint32_t tick_ms,model_version;
    bool valid;
    bool applied_valid;
} PPState;
typedef struct {
    float current[4],omega[4],power_mean_w,power_upper_w;
    /* P+(x)=sum(h*x*x+g*x)+c, x=CAN command / command_scale.
     * Valid only in each frozen command-sign box and declared state domain. */
    float h[4],g[4],c;
    bool valid;
} PPPrediction;
uint32_t PPModelChecksum(const PPModel *model);
bool PPModelValid(const PPModel *model);
bool PPPredict(const PPModel *model,const PPState *state,const float command[4],
               float horizon_s,PPPrediction *output);
#endif
