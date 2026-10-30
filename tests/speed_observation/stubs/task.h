#ifndef SPEED_TEST_TASK_H
#define SPEED_TEST_TASK_H
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
void vTaskSuspendAll(void);
int xTaskResumeAll(void);
#endif
