#ifndef TEST_VOFA_FREERTOS_H
#define TEST_VOFA_FREERTOS_H
void TestEnterCritical(void);
void TestExitCritical(void);
#define taskENTER_CRITICAL() TestEnterCritical()
#define taskEXIT_CRITICAL() TestExitCritical()
#endif
