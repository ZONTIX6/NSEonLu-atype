#ifndef BOARD_DEVICES_H
#define BOARD_DEVICES_H
#include <stdbool.h>
#include "mpu6500driver.h"
bool BoardDevicesInit(void);
MPU6500Instance *BoardIMUDevice(void);
#endif
