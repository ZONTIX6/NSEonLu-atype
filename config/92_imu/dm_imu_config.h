#ifndef DM_IMU_CONFIG_H
#define DM_IMU_CONFIG_H

/* DM-IMU-L1 主动上报；通过厂家上位机设 CAN / 1 Mbps / 100 Hz 并保存。
 * 开启加速度、角速度、四元数，可保留欧拉角。主板不向模块写配置。
 * MST_ID 必须与 RX_ID 一致；0x11 是厂家示例值，不保证是出厂值。 */
#define DM_IMU_CAN_BUS 2 /* 用户接线：CAN2；1=CAN1，2=CAN2。 */
#define DM_IMU_CAN_RX_ID 0x11U /* 模块数据回报 MST_ID，标准帧；不得与同总线设备重叠。 */
#define DM_IMU_GRAVITY_MPS2 9.80665f /* 导航系 +Z 重力扣除量；实车检查静止/倾斜残差。 */

#if DM_IMU_CAN_BUS != 1 && DM_IMU_CAN_BUS != 2
#error DM_IMU_CAN_BUS must be 1 or 2.
#endif
#if DM_IMU_CAN_RX_ID > 0x7FFU
#error DM_IMU_CAN_RX_ID must be a standard CAN identifier.
#endif
#endif
