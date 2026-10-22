#ifndef TEST_TASK_H
#define TEST_TASK_H

void TestEnterCritical(void);
#define taskENTER_CRITICAL() TestEnterCritical()
#define taskEXIT_CRITICAL() ((void)0)

#endif
