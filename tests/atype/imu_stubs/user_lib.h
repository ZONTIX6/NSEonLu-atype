#pragma once
#include <math.h>
static inline float float_constrain(float v,float lo,float hi) {return fminf(hi,fmaxf(lo,v));}
static inline float float_rounding(float v) {return roundf(v);}
#define arm_cos_f32 cosf
#define arm_sin_f32 sinf
