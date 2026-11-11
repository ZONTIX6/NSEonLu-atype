#ifndef TUNING_OVERRIDES_H
#define TUNING_OVERRIDES_H
/* 网页收到板端确认后，由本地服务写入的源码启动覆盖值；重新构建并烧录后生效。
 * 每组OVERRIDE：0=使用普通配置默认值，1=启用本组VALUES；MODE必须匹配运行模式。
 * 有效Flash保存值优先于本文件。注释放在定义行之前，网页同步时会保留说明。
 * 推荐通过网页填写与同步，不手工打乱数组顺序；完整字段名见runtime_tuning.h。 */
/* 底盘PID源码覆盖开关：0=关闭（默认），1=启用下方24个值；不影响云台/发射PID。 */
#define TUNING_PID_OVERRIDE 0
/* PID适用模式：0=普通速度后端；网页仅允许该模式，直接力控不恢复此PID组。 */
#define TUNING_PID_MODE 0
/* 四轮顺序：左前、右前、左后、右后；每轮依次Kp、Ki、Kd、MaxOut、IntegralLimit、DeadBand。
 * 增益量纲随速度环；误差/死区为度每秒，输出及积分限幅为电机电流命令值。 */
#define TUNING_PID_VALUES {0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f}
/* 功率源码覆盖开关：0=关闭（默认），1=启用下方19个值；全0占位值不能直接作为有效配置。 */
#define TUNING_POWER_OVERRIDE 0
/* 适用功控阶段：0=观测，1=快保护，2=启动保护，3=完整慢环；必须匹配CHASSIS_POWER_CONTROL_STAGE。 */
#define TUNING_POWER_MODE 3
/* 19项顺序：外环kp、ki、测量低通时间(s)、介入余量(W)、误差死区(W)、正修正上限(W)、
 * 正修正斜率(W/s)、预算跌落重置阈值(W)、抗饱和增益(1/s)、低需求延迟(s)、低需求衰减(W/s)、
 * 电容增补上限(W)、增补上升斜率(W/s)、需求余量(W)、空/满能量比例、缓冲增补上限(W)、恢复功率(W)、恢复时间(s)。 */
#define TUNING_POWER_VALUES {0}
/* 小陀螺源码覆盖开关：0=关闭（默认），1=启用下方16个值；全0占位值需先通过网页填写有效参数。 */
#define TUNING_SPIN_OVERRIDE 0
/* 适用自旋模式：0=固定，1=功率自适应，2=正弦；必须匹配CHASSIS_SPIN_MODE。 */
#define TUNING_SPIN_MODE 2
/* 16项顺序：固定/最小/最大Wz、功率增益、上升/下降斜率、功率低通时间(s)、死区(W)、
 * 稳定时间(s)、预算跌落阈值(W)、正弦幅值、周期(s)、渐入时间(s)、正弦上升/下降斜率、方向(+1/-1)。
 * Wz沿用原命令尺度；各项单位和含义见chassis_spin_config.h。 */
#define TUNING_SPIN_VALUES {0}
#endif
