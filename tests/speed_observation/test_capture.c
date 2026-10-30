#include "chassis_speed_capture.h"
#include "crc16.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static CSCCapture capture;
static uint8_t frame[CSC_MAX_FRAME_BYTES], retry[CSC_MAX_FRAME_BYTES];

static void test_full_window_and_stable_retry(void)
{
    CSCMetadata m;
    CSCMetadataInit(&m, 7, 168000000);
    CSCRawRecord r = {{0}};
    assert(CSCArm(&capture, 7, &m));
    for (unsigned i=0;i<CSC_RAW_CAPACITY;++i) {
        CSCWriteU32(r.bytes+4,i+1);
        CSCWriteU32(r.bytes+8,100+i/4);
        CSCWriteU32(r.bytes+12,1000+i*100);
        assert(CSCAppendRaw(&capture,&r));
    }
    assert(capture.state==CSC_FROZEN);
    assert(capture.freeze_reason==CSC_FREEZE_RAW_FULL);
    assert(capture.rejected_records==0);
    assert(!CSCAppendRaw(&capture,&r));
    assert(capture.outside_window==1);
    assert(!CSCArm(&capture,8,&m));
    assert(CSCBeginDrain(&capture));
    uint16_t n=0,again=0;
    assert(CSCPeekFrame(&capture,frame,sizeof frame,&n));
    assert(n==1044 && frame[6]==CSC_KIND_METADATA);
    assert(CSCFrameIsValid(frame,n));
    assert(!CSCPeekFrame(&capture,retry,20,&again));
    assert(CSCPeekFrame(&capture,retry,sizeof retry,&again));
    assert(again==n && !memcmp(frame,retry,n));
    CSCFrameAccepted(&capture);
    for(unsigned i=0;i<CSC_RAW_CAPACITY;++i) {
        assert(CSCPeekFrame(&capture,frame,sizeof frame,&n));
        assert(frame[6]==CSC_KIND_RAW && CSCReadU32(frame+14)==i);
        assert(CSCReadU32(frame+18+4)==i+1);
        CSCFrameAccepted(&capture);
    }
    assert(CSCPeekFrame(&capture,frame,sizeof frame,&n));
    assert(frame[6]==CSC_KIND_END);
    assert(!CSCAppendRaw(&capture,&r));
    CSCTransportRetry(&capture);
    assert(CSCPeekFrame(&capture,retry,sizeof retry,&again));
    assert(n==again && !memcmp(frame,retry,n));
    CSCDrainComplete(&capture,true);
    assert(capture.state==CSC_DRAINING);
    CSCFrameAccepted(&capture);
    assert(!CSCPeekFrame(&capture,frame,sizeof frame,&n));
    CSCDrainComplete(&capture,false);
    assert(capture.state==CSC_DRAINING);
    CSCDrainComplete(&capture,true);
    assert(capture.state==CSC_IDLE);
}

static void test_cycle_freeze_and_invalid_input(void)
{
    CSCMetadata m;
    CSCMetadataInit(&m,11,1000000);
    assert(CSCArm(&capture,11,&m));
    CSCCycleRecord cycle={{0}};
    CSCWriteU32(cycle.bytes,123);
    CSCWriteU32(cycle.bytes+4,1000);
    CSCWriteU32(cycle.bytes+8,1500);
    assert(!CSCAppendCycle(&capture,NULL));
    assert(capture.rejected_records==1);
    for(unsigned i=0;i<CSC_CYCLE_CAPACITY;++i) assert(CSCAppendCycle(&capture,&cycle));
    assert(capture.freeze_reason==CSC_FREEZE_CYCLE_FULL);
    assert(CSCBeginDrain(&capture));
    assert(CSCReadU32(capture.end_payload+4)==32);
    assert(CSCReadU32(capture.end_payload+44)==3);
    assert(CSCReadU32(capture.end_payload+24)==123);
    assert(CSCReadU32(capture.end_payload+36)==1500);
}

static void write_fixture(const char *path)
{
    memset(&capture,0,sizeof capture);
    CSCMetadata m;
    CSCMetadataInit(&m,42,168000000);
    assert(CSCArm(&capture,42,&m));
    for(unsigned i=0;i<2;++i) {
        CSCRawRecord r={{0}};
        r.bytes[0]=2;r.bytes[1]=37;
        CSCWriteU32(r.bytes+4,i ? 0 : UINT32_MAX);
        CSCWriteU32(r.bytes+8,100);
        CSCWriteU32(r.bytes+12,1000+100*i);
        CSCWriteU16(r.bytes+16,(uint16_t)(int16_t)-600);
        CSCWriteU16(r.bytes+18,(uint16_t)(int16_t)-123);
        CSCWriteF32(r.bytes+20,-599.5f);
        CSCWriteU16(r.bytes+24,(uint16_t)(int16_t)-122);
        CSCWriteU32(r.bytes+36,i);
        assert(CSCAppendRaw(&capture,&r));
    }
    CSCCycleRecord c={{0}};
    CSCWriteU32(c.bytes,100);CSCWriteU64(c.bytes+48,100000);
    CSCWriteF32(c.bytes+80+40,1.25f);
    assert(CSCAppendCycle(&capture,&c));
    CSCFreeze(&capture);
    assert(CSCBeginDrain(&capture));
    FILE *f=fopen(path,"wb");assert(f);
    uint16_t n;
    while(CSCPeekFrame(&capture,frame,sizeof frame,&n)) {
        assert(fwrite(frame,1,n,f)==n);CSCFrameAccepted(&capture);
    }
    assert(fclose(f)==0);
}

int main(int argc,char **argv)
{
    if(argc==3 && !strcmp(argv[1],"--check-raw")) {
        FILE *f=fopen(argv[2],"rb");assert(f);
        const size_t n=fread(frame,1,sizeof frame,f);assert(fclose(f)==0);
        assert(n==60 && CSCFrameIsValid(frame,(uint16_t)n));
        assert(frame[18]==3);
        assert((int16_t)CSCReadU16(frame+18+18)==-60);
        assert(CSCReadF32(frame+18+20)==-49.5f);
        assert(CSCReadU32(frame+18+36)==12);
        return 0;
    }
    assert(crc_16((const uint8_t *)"123456789",9)==0x4b37);
    test_full_window_and_stable_retry();
    test_cycle_freeze_and_invalid_input();
    if(argc==2) write_fixture(argv[1]);
    puts("PASS: CSC1 bounded capture, complete frames and immutable retries");
    return 0;
}
