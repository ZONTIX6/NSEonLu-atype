# 底盘直接力控：实施与验证记录

2026-10-25。软件实施完成；未烧录、未完成车辆物理标定及实车验收。

## 模块边界

| 位置 | 职责 |
| --- | --- |
| `modules/chassis_force/` | 纯 C 力意图协议、参数校验、几何、输入整形、功率自适应转矩、约束分配及状态机 |
| `application/board_chassis_force.[ch]` | 车辆标定、功率配置及合法宏组合装配 |
| `application/chassis/chassis_force_app.c` | 现有任务中的消息、安全、能源政策及反馈装配 |
| `modules/chassis_power/chassis_power_force.[ch]` | 每周期冻结共用预算、独立输入/负载功率账目和完成记录 |
| `modules/motor/DJImotor/dji_force_adapter.[ch]` | 四轮直接电流所有权、快照、最终许可检查及发送结果回投 |
| `modules/telemetry/chassis_force_telemetry.[ch]` | 独立 `force-v1` USB CSV；慢任务格式化 |

默认 `CHASSIS_CONTROL_MODE=0` 保留速控。`MODE=1` 的四轮跳过软件角度、速度、电流 PID 和旧符号处理；其他电机保留原路径。FOLLOW 在力控中只执行云台方向平移，不自动跟转。应用主线与交付验证为 ONE_BOARD 单板；协议编解码不改变旧速度报文结构。

## 构建

每种宏组合使用独立构建目录。模块迁移前产生的旧 `.d` 文件可能引用旧路径，应使用新目录重新构建。

```powershell
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/speed-modular CHASSIS_CONTROL_MODE=0
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/force-modular CHASSIS_CONTROL_MODE=1
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/force-modular-brake CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_EXIT_MODE=1 CHASSIS_POWER_SOURCE_MODE=1
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/force-modular-fixed-direct CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 POWER_METER_ENABLE=0 CHASSIS_BUFFER_POWER_ENABLE=0
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/force-modular-fixed-cap CHASSIS_CONTROL_MODE=1 CHASSIS_FORCE_SPIN_MODE=0 CHASSIS_POWER_SOURCE_MODE=1

cmake -S . -B build/cmake-speed-debug -G "MinGW Makefiles" -DCHASSIS_CONTROL_MODE=0
cmake --build build/cmake-speed-debug -j4
cmake -S . -B build/cmake-force-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCHASSIS_CONTROL_MODE=1
cmake --build build/cmake-force-release -j4
```

以上 Make 五种组合与 CMake Debug 速控 / Release 力控均已成功。Release 数值源的最终参数为 `-Ofast ... -fno-fast-math`。修复了旧 `QuaternionEKF` 逆平方根中的指针别名访问，使优化构建通过编译器检查。

## 软件验证

- `make -C tests/chassis_force test`：几何与单位、标定锁定、44 字节协议、序号与 tick 回绕、年龄、非单调功率解析分配与独立密集扫描、退出制动、功率样本不重复积分、量化增量、制动时禁止增长、NaN 故障、实际适配器及发送回投。
- 原生测试还以 `-Ofast -fno-fast-math` 覆盖 EXIT=0/1 × SPIN=0/1 四组合。
- `make -C tests/vehicle_motion test_force`：直接读取原始输入；摇杆、DBUS 键盘与图传键盘坐标一致，倍率明确，发布时刻计入有效年龄。
- 原车辆运动、DJI 驱动、功率模块、缓冲预算、安全、超级电容、USB 遥测及 VTX 测试通过。驱动测试确认四轮 PID 调用为零、普通第五电机仍走 PID，记录电流与 CAN 字节一致，邮箱等待期间停机使全组归零。
- `tests/chassis_force/test_build_contracts.py` 使用实际 CMake 编译参数验证 14 项合法/非法宏组合、Release 数值选项，以及速控无力控符号、力控无旧自旋/速控适配器符号。
- 架构测试确认模块无新增应用反向依赖或硬件依赖。独立审查发现的问题均已修正，并补入回归测试。

能源验证覆盖：不以零输出/模型空载损耗申请增补；候选申请有界；发送完成同 tick 的历史反馈不能确认新目标；051/052 独立年龄；发送失败及失联撤销增长；物理 W 与模型补偿分离；配置版本采用。固定模式允许已确认的正向模型学习；自适应模式冻结正向学习并保留负向收紧。输入侧能量估计继续使用 P_input，转矩环使用 P_load。

## 上车前的剩余工作

生产配置中没有填写可使能的通用物理参数。默认标定锁定；单独把 `CHASSIS_FORCE_CALIBRATION_VALID` 改为 1 仍无法驱动。应在板级配置中测量并填写 COM/轮向、输出端 K_out、电流/轮力、温度/堵转、启动及速度保护等参数，再审核有效标志。

仍需实车确认功率计节点/方向、源端硬件与匹配 F334、最高转速和制动行为。当前图传链路约 30 Hz，不能持续满足本设计的 10 ms 操作源门槛；保持严格失效停机，不能把在线看门狗代替新鲜度。

力控电机任务栈已从 512 增为 1024 words。当前 GCC 静态调用链已知部分约 2.3 KiB，尚不包含全部数学库与 RTOS/FPU 开销；必须测量高水位并达到至少 128 words 余量。USB 默认开启时可监视 `chassis_force_motor_stack_free_words` 和 `chassis_force_motor_max_execution_us`。force-v1 提供输入、估计、命令、分配、guard 与 CAN 分段 DWT 时间；最坏力控 <=200 us、整电机任务 <=1 ms 的门槛仍需硬件测量。

上述软件测试不代表已测得轮地力、功率跟踪精度、最坏实时耗时或实车安全边界。
