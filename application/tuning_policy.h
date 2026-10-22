#ifndef TUNING_POLICY_H
#define TUNING_POLICY_H
#include <stdbool.h>
#include <stdint.h>
void TuningPolicyInit(void);
bool TuningPolicyCanSave(void *context,uint32_t now_ms);
#endif
