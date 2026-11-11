#ifndef CHASSIS_POWER_MODEL_CONFIG_H
#define CHASSIS_POWER_MODEL_CONFIG_H

/* 速度后端共用的静态功率模型初值，主机验证工具读取同一组系数。
 * x = abs(最终 DJI 电流命令) / current_conversion，n = abs(电机转子 rpm)。
 * 每轮预测功率 P(W) = k0 + k1*x + k2*n + k3*x*n + k4*x*x + k5*n*n。
 * x 是拟合采用的命令缩放量，不能视为已验证的 A 或 mA；以下系数应连同缩放值
 * 成组标定、成组修改并重新构建。该旧静态模型不区分驱动和制动，也无已验证动态域。
 * INITIALIZER 是整组结构体初始化宏，不是模式开关；定义行即当前源码默认值。 */
#define CHASSIS_SPEED_POWER_MODEL_INITIALIZER { \
    .k0 = 1.3123816, /* 常数损耗项，单位 W。 */ \
    .k1 = -0.25836727, /* 一次缩放电流项，单位 W/缩放命令单位。 */ \
    .k2 = 0.0007424524, /* 一次转速项，单位 W/rpm。 */ \
    .k3 = 0.0024271158, /* 电流与转速交叉项，单位 W/(缩放命令单位*rpm)。 */ \
    .k4 = 0.081198552, /* 二次缩放电流项，单位 W/(缩放命令单位的平方)。 */ \
    .k5 = 3.890596e-08, /* 二次转速项，单位 W/rpm^2。 */ \
    .current_conversion = 1000.0f, /* 每个缩放单位对应的 CAN 命令数；不是电流 A 换算系数。 */ \
}

#endif
