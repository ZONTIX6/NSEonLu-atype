#ifndef PREDICTIVE_POWER_CONFIG_H
#define PREDICTIVE_POWER_CONFIG_H
#define CHASSIS_PREDICTIVE_LEGACY 0 /* 模式编号 0：使用现有功率控制；编号本身不改，切换下方 MODE。 */
#define CHASSIS_PREDICTIVE_SHADOW 1 /* 模式编号 1：预测算法旁路计算供观察，实际控制继续使用现有路径。 */
#define CHASSIS_PREDICTIVE_CONTROL 2 /* 保留的接管模式编号 2；当前尚未开放，选用会被下面的编译检查拒绝。 */
/* 默认 0（LEGACY）；切换为 1（SHADOW）推荐用 make ... CHASSIS_PREDICTIVE_POWER_MODE=1，
 * CMake 传 -DCHASSIS_PREDICTIVE_POWER_MODE=1；也可在没有构建覆盖时改下方默认值。
 * 编译命令优先于此默认值；切换时换构建目录并重新编译、烧录。当前只允许 0/1。 */
#ifndef CHASSIS_PREDICTIVE_POWER_MODE
#define CHASSIS_PREDICTIVE_POWER_MODE CHASSIS_PREDICTIVE_LEGACY /* 当前默认选择 0：现有功率控制。 */
#endif
#if CHASSIS_PREDICTIVE_POWER_MODE != 0 && CHASSIS_PREDICTIVE_POWER_MODE != 1
#error Predictive takeover is not released: use legacy (0) or shadow (1)
#endif
#endif
