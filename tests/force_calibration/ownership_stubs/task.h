void FcTestEnterCritical(void);
#define taskENTER_CRITICAL() FcTestEnterCritical()
#define taskEXIT_CRITICAL() ((void)0)
