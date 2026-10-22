#include "jscope.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t mask;
static unsigned free_bytes=1024,writes;
uint32_t __get_PRIMASK(void) {return mask;}
void __disable_irq(void) {mask=1;}
void __enable_irq(void) {mask=0;}
void __set_PRIMASK(uint32_t value) {mask=value;}
int SEGGER_RTT_ConfigUpBuffer(unsigned index,const char *name,void *buffer,unsigned size,unsigned flags)
{(void)buffer;(void)flags;assert(index==1 && size==1024 && strcmp(name,"JScope_f4f4f4f4")==0);return 0;}
unsigned SEGGER_RTT_Write(unsigned index,const void *data,unsigned length)
{(void)data;assert(index==1 && length==16);++writes;return length;}
unsigned SEGGER_RTT_GetAvailWriteSpace(unsigned index) {assert(mask==1 && index==1);return free_bytes;}
unsigned SEGGER_RTT_WriteNoLock(unsigned index,const void *data,unsigned length)
{assert(mask==1);return SEGGER_RTT_Write(index,data,length);}
int main(void)
{
    assert(JScopeInit()==0);
    float sample[4]={1,2,3,4};
    mask=1;assert(JScopeWriteSamples(sample,1,4)==16 && mask==1);
    free_bytes=0;assert(JScopeWriteSamples(sample,1,4)==0 && mask==1);
    free_bytes=1024;mask=0;assert(JScopeWriteSamples(sample,1,4)==16 && mask==0);
    assert(writes==2 && JScopeWrite(1,2,3,4)==16);
    puts("PASS: JScope four floats on RTT channel 1, restoring prior interrupt mask");
}
