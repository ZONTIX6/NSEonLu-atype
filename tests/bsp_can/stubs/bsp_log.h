#ifndef TEST_BSP_LOG_H
#define TEST_BSP_LOG_H
static inline void TestLog(const char *format,...) {(void)format;}
#define LOGINFO(...) TestLog(__VA_ARGS__)
#define LOGWARNING(...) TestLog(__VA_ARGS__)
#define LOGERROR(...) TestLog(__VA_ARGS__)
#endif
