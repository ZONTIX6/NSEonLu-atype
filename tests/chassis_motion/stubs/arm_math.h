#ifndef TEST_ARM_MATH_H
#define TEST_ARM_MATH_H
#include <math.h>
#include <stdint.h>
/* Substitute only CMSIS math primitives; motion and angle wrapping are real. */
typedef struct {
    uint16_t numRows, numCols;
    float *pData;
} arm_matrix_instance_f32;
static inline float arm_cos_f32(float x) { return cosf(x); }
static inline float arm_sin_f32(float x) { return sinf(x); }
#endif
