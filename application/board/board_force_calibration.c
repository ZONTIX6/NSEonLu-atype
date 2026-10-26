#include "board_force_calibration.h"
FcLimits BoardForceCalibrationLimits(void)
{
    /* 独立台架边界：CAN 命令、转子 rpm、毫秒、摄氏度；不是物理标定结果。 */
    return (FcLimits){.max_current=1500,.max_on_ms=500,.max_rpm=1000,
        .stationary_rpm=20,.stationary_ms=100,.heartbeat_ms=150,.pre_ms=50,
        .tail_ms=100,.stop_temperature_c=50};
}
