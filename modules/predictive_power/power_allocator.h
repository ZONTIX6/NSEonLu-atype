#ifndef POWER_ALLOCATOR_H
#define POWER_ALLOCATOR_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    /* min x'Qx + linear'x; Q positive definite, x=raw command/scale. */
    float q[4][4],linear[4],power_h[4],power_g[4],power_c;
    float low[4],high[4],scale,budget_w;
} PPAllocationProblem;
typedef struct {
    int16_t command[4];
    float power_w,objective,residual;
    uint32_t iterations,elapsed_us;
    bool feasible;
} PPAllocationResult;
bool PPAllocationCheck(const PPAllocationProblem *problem,const int16_t command[4],float *power_w);
bool PPAllocate(const PPAllocationProblem *problem,PPAllocationResult *result);
#endif
