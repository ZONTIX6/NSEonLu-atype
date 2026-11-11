#include "chassis_force_config.h"

/* 直接力控通用软件默认参数；物理标定参数在 chassis_force_vehicle_config.c 填写。
 * 以下为初始化值；未显式填写的字段保持 0，因此不能只设置标定标志就解除输出锁定。 */
ChassisForceConfig_s ChassisForceDefaultConfig(void)
{
    ChassisForceConfig_s c={
        .backend=1, /* 力控后端协议编号，固定为 1；不是选择速度/力控的构建开关。 */
        .schema_version=1, /* 参数结构版本，当前校验只接受 1。 */
        .calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0, /* 标定验收状态；默认宏为 0，未验收。 */
        .resume_ratio=.9f, /* 速度保护解除回滞比例，无单位；低于上限的 90% 后可恢复。 */
        .input_deadzone=.05f, /* 归一化摇杆径向死区，无单位；默认 5%。 */
        .input_gamma=1.5f, /* 输入力曲线的幂指数，无单位；控制小输入的细腻程度。 */
        .power_gain_nm_per_w_s=.01f, /* 功率误差到转矩变化率的增益，单位 N*m/(W*s)。 */
        .power_deadband_w=2, /* 负载功率跟踪死区，单位 W。 */
        .power_lpf_tau_s=.02f, /* 负载功率低通时间常数，单位 s。 */
        .power_recovery_samples=3, /* 恢复增长前所需连续新鲜功率样本数。 */
        .torque_probe_step_nm=.05f, /* 单次候选转矩增量，单位 N*m。 */
        .power_request_step_w=5, /* 单次能源申请的功率步长，单位 W。 */
        .grant_timeout_ms=50 /* 等待能源批准的超时时间，单位 ms。 */
    };
    c.checksum=ChassisForceConfigChecksum(&c); /* 摘要随全部默认字段自动计算。 */
    return c;
}
