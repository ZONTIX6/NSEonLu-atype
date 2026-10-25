#ifndef CHASSIS_FORCE_INTENT_H
#define CHASSIS_FORCE_INTENT_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "chassis_force_types.h"
#define CHASSIS_FORCE_WIRE_SIZE 44U
typedef struct {
    ChassisForceIntent_s intent;
    uint32_t published_ms;
} ChassisForceIntentMessage_s;
typedef struct {
    ChassisForceIntent_s intent;
    uint32_t received_ms;
    bool valid;
} ChassisForceIntentReceiver_s;
bool ChassisForceIntentValid(const ChassisForceIntent_s *intent);
bool ChassisForceIntentEncode(const ChassisForceIntent_s *intent,uint8_t bytes[44]);
bool ChassisForceIntentDecode(const uint8_t *bytes,size_t length,ChassisForceIntent_s *intent);
bool ChassisForceIntentAccept(ChassisForceIntentReceiver_s *receiver,const ChassisForceIntent_s *intent,uint32_t now_ms);
bool ChassisForceIntentFresh(const ChassisForceIntentReceiver_s *receiver,uint32_t now_ms,uint32_t transport_margin_ms);
/* All sources produce gimbal forward +x, left +y. No legacy speed coefficients. */
void ChassisForceIntentStick(float forward,float left,float full_scale,ChassisForceIntent_s *intent);
void ChassisForceIntentKeys(bool w,bool s,bool a,bool d,ChassisForceIntent_s *intent);
#endif
