#include <stddef.h>
#define SEGGER_RTT_MODE_NO_BLOCK_SKIP 0
int SEGGER_RTT_ConfigUpBuffer(unsigned,const char *,void *,unsigned,unsigned);
unsigned SEGGER_RTT_Write(unsigned,const void *,unsigned);
unsigned SEGGER_RTT_GetAvailWriteSpace(unsigned);
unsigned SEGGER_RTT_WriteNoLock(unsigned,const void *,unsigned);
