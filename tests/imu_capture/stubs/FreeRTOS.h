#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H
#include <stddef.h>
#include <stdint.h>
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;
typedef struct { unsigned unused; } StaticTask_t;
#define configMINIMAL_STACK_SIZE 16
#define pdMS_TO_TICKS(ms) (ms)
#endif
