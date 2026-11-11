#include "robot_telemetry.h"
#include "ins_task.h"
#include "usart.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart7 = {.gState = HAL_UART_STATE_READY};
static uint32_t now;
static unsigned frames;
static int32_t fields[64];
static INSMotionSnapshot_s imu = {
    .valid = true, .sequence = 0x1234abcdU, .quaternion = {1, 0, 0, 0},
    .accel_n_mps2 = {.125f, -.25f, .5f}, .gyro_sensor_radps = {1, -2, 3},
};
uint32_t HAL_GetTick(void) { return now; }
bool INSGetMotionSnapshot(INSMotionSnapshot_s *out) { *out = imu; return imu.valid; }

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart, const uint8_t *data, uint16_t length)
{
    assert(uart == &huart7 && length < 769);
    char row[769];
    memcpy(row, data, length);
    row[length] = 0;
    char *next = row;
    for (unsigned i = 0; i < 64; ++i) {
        char *end;
        fields[i] = (int32_t)strtol(next, &end, 10);
        assert(end != next && *end == (i == 63 ? '\n' : ','));
        next = end + 1;
    }
    assert(next == row + length);
    ++frames;
    return HAL_OK;
}

int main(void)
{
    RobotTelemetryInit();
    RobotTelemetryTask();
    assert(frames == 1);
    assert(fields[0] == 2 && fields[5] == (16384 | 16 | 1));
    assert(fields[6] == -2 && fields[7] == -1 && fields[63] == 1);
    assert(fields[8] == 0 && fields[9] == 0 && fields[11] == -1 && fields[12] == -1);
    for (unsigned i = 21; i <= 32; ++i) assert(fields[i] == 0);
    assert(fields[33] == 0 && fields[34] == 4660 && fields[35] == 43981);
    assert(fields[36] == 1000000 && fields[40] == 125 && fields[41] == -250);
    assert(fields[42] == 500 && fields[43] == 1000 && fields[44] == -2000 && fields[45] == 3000);
    for (unsigned i = 52; i <= 59; ++i) assert(fields[i] == 0);
    assert(fields[60] == -1 && fields[61] == -1 && fields[62] == -1);
    now = 80;
    RobotTelemetryTask();
    assert(frames == 2 && fields[33] == 80 && !(fields[5] & 16));
    for (unsigned i = 36; i <= 45; ++i) assert(fields[i] == 0);
    now = 160;
    imu.tick_ms = now;
    imu.valid = false;
    RobotTelemetryTask();
    assert(frames == 3 && !(fields[5] & 16) && fields[36] == 0);
    puts("PASS: IMU-only runtime emits source-tagged data without robot or motor dependencies");
    return 0;
}
