#ifndef SHADOW_JSCOPE_H
#define SHADOW_JSCOPE_H
#include <stdint.h>
#include "shadow_scope_config.h"
typedef struct {
    uint32_t sent, dropped, stale, elapsed_max_us;
    int init_result;
} ShadowJScopeDiagnostics;
extern volatile ShadowJScopeDiagnostics shadow_jscope_diagnostics;
/* Single low-priority observation-task owner. No calls from the motor task. */
void ShadowJScopeInit(void);
void ShadowJScopePoll(uint32_t now_ms);
#endif
