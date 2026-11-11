#ifndef TEST_CMSIS_OS_H
#define TEST_CMSIS_OS_H
#include <stdint.h>
typedef enum { osPriorityBelowNormal=-1, osPriorityNormal=0, osPriorityAboveNormal=1 } osPriority;
typedef struct {
    const char *name;
    void (*function)(void const *);
    osPriority priority;
    unsigned stack;
} osThreadDef_t;
typedef const osThreadDef_t *osThreadId;
#define osThreadDef(name,func,priority,instances,stack) \
    const osThreadDef_t os_thread_def_##name={#name,func,priority,stack}
#define osThread(name) (&os_thread_def_##name)
osThreadId osThreadCreate(const osThreadDef_t *definition,void *argument);
void osDelay(uint32_t ms);
void osThreadTerminate(osThreadId thread);
#endif
