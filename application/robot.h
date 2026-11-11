#ifndef ROBOT_H
#define ROBOT_H

/* Robot 使用 config/06_robot/robot_config.h 中的角色宏，兼容单板及底盘/云台分板配置。 */

/**
 * @brief 机器人初始化,请在开启rtos之前调用.这也是唯一需要放入main函数的函数
 * 
 */
void RobotInit();

/**
 * @brief 机器人任务,放入实时系统以一定频率运行,内部会调用各个应用的任务
 * 
 */
void RobotTask();

#endif
