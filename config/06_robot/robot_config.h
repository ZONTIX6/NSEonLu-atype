#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/* 切换方法：
 * 1. 单板：保留下方 #define ONE_BOARD；双板时把该行改为 #define CHASSIS_BOARD
 *    （底盘板）或 #define GIMBAL_BOARD（云台板），两块板分别编译、分别烧录。
 *    也可通过编译参数 -DCHASSIS_BOARD 或 -DGIMBAL_BOARD 选择角色，使默认分支不生效。
 *    三种角色只能定义一种；下方注释示例不能在默认 ONE_BOARD 已生效后直接取消注释。
 * 2. 视觉默认 UART：保留 #define VISION_USE_UART；改用 USB 时把该行改成
 *    #define VISION_USE_VCP，或传入编译参数 -DVISION_USE_VCP。
 *    VCP 与 USB 遥测共用接口，必须同时在 usb_telemetry_config.h 关闭 USB_TELEMETRY_ENABLE。
 * 修改后重新编译；将这些选择宏写成 0 仍表示“已定义”，不会禁用对应功能。
 */

/* 主控板角色与视觉通信配置；修改参数后重新编译。 */

/* 开发板类型定义,烧录时注意不要弄错对应功能;修改定义后需要重新编译,只能存在一个定义! */
#if !defined(ONE_BOARD) && !defined(CHASSIS_BOARD) && !defined(GIMBAL_BOARD)
/* 板卡角色的默认选择：单板控制整车。此类宏按“是否定义”判断，不能用值 0 关闭。 */
#define ONE_BOARD // 单板控制整车
#endif
// #define CHASSIS_BOARD //底盘板
// #define GIMBAL_BOARD  //云台板

#if !defined(VISION_USE_VCP) && !defined(VISION_USE_UART)
/* 视觉通信默认走串口；与 VISION_USE_VCP 二选一。UART 使用 BOARD_VISION_UART 映射的串口。 */
#define VISION_USE_UART
#endif
#if defined(VISION_USE_VCP) && defined(VISION_USE_UART)
#error Select one vision transport.
#endif
#include "../31_telemetry/usb_telemetry_config.h"
#if defined(VISION_USE_VCP) && USB_TELEMETRY_ENABLE
#error USB vision and USB telemetry are mutually exclusive.
#endif
// #define VISION_USE_UART // 使用串口发送视觉数据

// 检查是否出现主控板定义冲突,只允许一个开发板定义存在,否则编译会自动报错
#if (defined(ONE_BOARD) && defined(CHASSIS_BOARD)) || \
    (defined(ONE_BOARD) && defined(GIMBAL_BOARD)) ||  \
    (defined(CHASSIS_BOARD) && defined(GIMBAL_BOARD))
#error Conflict board definition! You can only define one board type.
#endif

#endif
