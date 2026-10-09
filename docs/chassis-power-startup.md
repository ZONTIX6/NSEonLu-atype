# 底盘启动保护与快速恢复

控制依据为 `superpowers/specs/2026-10-09-chassis-power-startup-recovery-design.md`。
第一阶段保留现有模型、PID、基础分配和超级电容预算来源。新阈值是调参起点，实际峰值、恢复和加速性能须上板验证。

## 配置

`CHASSIS_POWER_CONTROL_STAGE` 默认 0：0 观察、1 快恢复、2 启动保护、3 完整慢环。
`CHASSIS_POWER_CAPTURE_ENABLE` 默认 0，诊断固件置 1；与现有 17/26 列开关独立。
Make 和 CMake 均支持这两个配置及 `POWER_METER_ENABLE`。每个组合使用独立目录，避免复用不同宏的对象文件。

PowerShell 构建诊断基线：

```powershell
New-Item -ItemType Directory -Force build/power-observe | Out-Null
make -j8 BUILD_DIR=build/power-observe CHASSIS_POWER_CONTROL_STAGE=0 CHASSIS_POWER_CAPTURE_ENABLE=1 POWER_METER_ENABLE=1 UART1_USE=1
```

## 记录与排空

四轮低速、低电流持续 100 ms 后，至少 300 rpm 目标阶跃触发一次记录。
记录频率为 500 Hz，25 条前窗与 125 条后窗，共 150 条固定 96 字节记录。
记录开始后冻结整段历史；未排空期间的新触发只增加丢失计数。达到 t90 或 3 s 超时后停止本轮测试，再置零力模式，低优先级任务向 USART1 日志通道排空。
通道采用现有 250000 波特率，排空每帧至少间隔 21 ms，最多约 4.86 kB/s；队列满时保留当前帧重试。裁判串口模式不提供该通道。
串口原始数据保存为二进制，混入的普通日志不影响帧搜索：

```powershell
./scripts/decode-chassis-power-capture.ps1 -InputPath capture.bin -OutputPath capture.csv
```

工具检测 CRC、版本、序号、重复与缺帧，完整性失败时拒绝导出。一个文件内 capture_id 必须唯一；长时间记录应分文件保存。

## 二进制版本 1

每帧 102 字节：ASCII `CPC1`、96 字节载荷、CRC16/MODBUS。CRC 初值 0xffff，多项式 0xa001，覆盖前 100 字节。
整数、IEEE754 float32 和 CRC 均为小端；载荷通过逐字段编码产生，不依赖结构体布局。

| 载荷偏移 | 字段 |
|---|---|
| 0..7 | version:u8、kind:u8、capture_id:u16、sequence:u16、state:u8、flags:u8 |
| 8..15 | 控制 tick:u32、功率计接收 tick:u32 |
| 16..55 | raw_current、final_current、feedback_current、actual_rpm、target_rpm；各四个 int16，轮序 LF/RF/LB/RB |
| 56..87 | B、实测功率、实际转速模型功率、预测转速功率、C_fast、C_slow、E_excess、E_max；各 float32 |
| 88..95 | 四轮反馈年龄:u8、功率计年龄:u8、使能/驱动/制动掩码:u8 |

年龄饱和到 255 ms；flags 的 0x80 表示 int16 量化溢出或非有限数组值。电流使用原始命令/反馈单位，不解释为安培。
flags 的低三位依次为非有限需求、预测约束不可行、增长兜底；第 4/5 位记录控制阶段。
记录的 guard_model_w 使用实际最终命令与预测转速计算；建议命令、候选预测功率和建议预测功率通过 DJI 诊断快照读取。观察模式的建议值不冒充实际发送值。
kind=1 为控制快照；kind=2 为末尾摘要。摘要偏移 8 为启动 tick，16/20/24/28 分别为 t90 状态、t90 毫秒、丢失触发数、记录数，均 u32。
t90 状态：0 未开始、1 待完成、2 达到、3 无效、4 超时。t90 报告首次达到 90% 的时刻，经连续保持 20 ms 确认；目标变化、停机使尚未完成的指标无效。
接收功率通过零阶保持积分，重复 meter_tick 不代表独立测量；500 Hz 记录不提高功率计带宽。

## 启动与快恢复初值

启动须四轮在线使能，平均绝对转速低于 100 rpm、上次最终电流最大值低于 500，持续 100 ms；随后至少 300 rpm 阶跃且存在驱动需求。
反馈容忍最多 30 ms，阈值为 1.30 B_start，能量额度为 0.30 B_start × 0.030 s；模型预算不增加。
预算下降、制动、陈旧或离线结束容忍；500 ms 冷却在零力切换中保留，防止重新满足低速条件后反复领取额度。显式重新初始化清空状态。
转速差分只使用不同序号且接收时间间隔有效的 CAN 反馈；10 ms 加速度滤波、8 ms 正加速度预测。首两个有效反馈前的增长上限为 max_current_command / 5 ms。
方向滞回为进入近零 50 rpm、退出 100 rpm；需求进入 200、退出 100 命令单位。
快保护增益 0.5，低于 P_ref−0.5 W 的两个不同时间戳样本且保持至少 10 ms 后，以 60 W/s 释放；功率计超过 20 ms 陈旧时按 20 W/s 释放。
非单调二次可行区间与增长上限共同求解；无可行解或整数化后仍超阈值时驱动置零并报告不可行，制动候选不增加，也不抵扣负功率。
功率计关闭时无容忍、快慢补偿，保留预测约束和反馈不足兜底。反馈超过 5 ms 陈旧时停止外推，Daemon 离线后输出零。

## 验证

主机记录与解码测试：

```powershell
make -C tests/chassis_power_capture CC=gcc test
./tests/chassis_power_capture/test_decoder.ps1
```

上板分别记录架空/落地、20/30/40 W、正反直行、横移、自旋，各至少五次；记录负载、电压和超级电容状态。
先用阶段 0 采基线，再依次使用 1、2、3，每次只改变一个控制因素。
验收目标沿用设计：t90 中位增幅不超过 5%，同时报告最差值；窗口结束后超限不持续；充足需求下恢复争取 250 ms 内；稳态驱动 2 s 平均误差争取 ±0.5 W。
同时检查最新 map、任务栈高水位和真实控制耗时，电机任务总耗时须满足 1 ms 并保留余量。
