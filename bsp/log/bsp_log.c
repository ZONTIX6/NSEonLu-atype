#include "bsp_log.h"

#include "SEGGER_RTT.h"
#include "SEGGER_RTT_Conf.h"
#include <stdio.h>
#include <stdlib.h>


void BSPLogInit()
{
    SEGGER_RTT_Init();
}

unsigned BSPLogWrite(const void *data, unsigned length)
{
    return SEGGER_RTT_Write(BUFFER_INDEX, data, length);
}

unsigned BSPLogWriteString(const char *text)
{
    return SEGGER_RTT_WriteString(BUFFER_INDEX, text);
}

void BSPLogClear(void)
{
    (void)BSPLogWriteString("  " RTT_CTRL_CLEAR);
}

int PrintLog(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = SEGGER_RTT_vprintf(BUFFER_INDEX, fmt, &args); // 一次可以开启多个buffer(多个终端),我们只用一个
    va_end(args);
    return n;
}

void Float2Str(char *str, float va)
{
    int flag = va < 0;
    int head = (int)va;
    int point = (int)((va - head) * 1000);
    head = abs(head);
    point = abs(point);
    if (flag)
        sprintf(str, "-%d.%d", head, point);
    else
        sprintf(str, "%d.%d", head, point);
}
