#include "force_calibration_capture.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    FcRecord r={.type=FC_RX,.wheel=1,.time_us=0x01020304,.raw_rpm=-123,
        .raw_current=-300,.used_rpm=12.5f,.feedback_time_us=-1000,.final={1,2,-3,-4}};
    uint8_t b[48];assert(FcEncodeRecord(&r,b));
    assert(b[8]==4 && b[9]==3 && b[10]==2 && b[11]==1);
    assert(b[16]==0x85 && b[17]==0xff && b[18]==0xd4 && b[19]==0xfe);
    assert(b[20]==0 && b[21]==0 && b[22]==0x48 && b[23]==0x41);
    assert(b[26]==1 && b[28]==2 && b[30]==0xfd && b[32]==0xfc);
    uint8_t storage[4*48],before[4*48],read[96];FcCapture c;
    FcCaptureInit(&c,storage,4);FcCaptureMeta meta={.capture_id=4,.session=3,.generation=9};
    FcCaptureBegin(&c,&meta);assert(!FcCaptureRead(&c,0,1,read,sizeof read));
    r.time_us=0;r.sequence=UINT32_MAX;
    for(unsigned k=0;k<4;k++) {r.time_us+=1000;++r.sequence;assert(FcCaptureAppend(&c,&r));}
    memcpy(before,storage,sizeof before);assert(!FcCaptureAppend(&c,&r));
    assert(!memcmp(before,storage,sizeof before) && c.dropped==1 && !c.quality);
    FcCaptureFreeze(&c,FC_REASON_CAPACITY,false);
    assert(FcCaptureRead(&c,0,2,read,sizeof read) && !memcmp(read,storage,sizeof read));
    assert(FcCaptureRead(&c,0,2,read,sizeof read));
    assert(!FcCaptureRead(&c,3,2,read,sizeof read));assert(!FcCaptureRead(&c,0,3,read,sizeof read));
    assert(!FcCaptureAppend(&c,&r));assert(c.meta.session==3 && c.meta.generation==9);
    FcCaptureBegin(&c,&meta);r.sequence=9;r.time_us=0;assert(FcCaptureAppend(&c,&r));
    r.time_us=1000;assert(FcCaptureAppend(&c,&r));assert(c.rx_duplicates==1 && !c.quality);
    r.sequence=12;r.time_us=5000;assert(FcCaptureAppend(&c,&r));assert(c.rx_gaps==2);
    FcCaptureFreeze(&c,FC_REASON_NONE,true);assert(!c.complete);
    puts("capture: exact LE bytes, immutable bounded storage and quality OK");return 0;
}
