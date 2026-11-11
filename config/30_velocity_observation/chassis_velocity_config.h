#ifndef CHASSIS_VELOCITY_CONFIG_H
#define CHASSIS_VELOCITY_CONFIG_H

/* 直接修改下方宏值并重新编译、烧录；这些阈值决定何时拒绝过期或不同步的样本。 */
#define CHASSIS_VELOCITY_MAX_AGE_MS 20U /* 轮速及普通速度快照最大允许年龄，单位毫秒；超过该值判无效。 */
#define CHASSIS_FUSED_VELOCITY_MAX_AGE_MS 60U /* 融合速度快照最大允许年龄，单位毫秒；轮速自身仍须满足上面的独立阈值。 */
#define CHASSIS_FUSION_RECEIVE_MAX_AGE_US 10000U /* 转换接收时间戳时允许的最大样本年龄，单位微秒。 */
#define CHASSIS_FUSION_RECEIVE_MAX_SKEW_US 3000U /* IMU、关节编码器和各轮样本间允许的最大时间偏差，单位微秒。 */

/* 融合开关由构建选项提供，不在此重复定义：
 * CHASSIS_VELOCITY_FUSION_ENABLE：默认 0 关闭，1 开启速度融合/观察路径；要求 ONE_BOARD 和速度后端。
 * CHASSIS_VELOCITY_FUSION_CALIBRATED：0 关闭本地云台融合，1 使用本地实测标定，必须同时 ENABLE=1。
 * 远端固定底盘IMU另用 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1（仍保持 CALIBRATED=0）；
 * 两条融合路径都未启用时，ENABLE=1只发布未标定轮速观察。
 * 用 make ... CHASSIS_VELOCITY_FUSION_ENABLE=1 或 CMake 的 -D 同名参数切换；
 * CALIBRATED 同理。标定头从对应 build 目录加载，切换时使用独立构建目录。
 * ENABLE 还决定数值计算文件的 -fno-fast-math 选项，因此不能只在头文件额外定义开关。 */
#endif
