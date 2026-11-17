# 小陀螺 yaw 固定偏差：第一轮观测固件

目的：在调整积分限幅或加入补偿前，确认 yaw 目标是否变化、速度环积分是否受限，以及最终输出是否受限或被门控。

## 实现

- 默认 `VOFA_OBSERVATION_PROFILE=2` 升级到 schema 7、108 通道；schema 6 前 102 路的位置、单位和原有效位含义保留。
- I102–I107 追加实际采用的角度目标、速度反馈、速度环积分、最终 CAN 命令、控制状态位与控制周期年龄。控制状态由当次运行的 PID 限幅和 CAN 提交结果产生。
- 电机任务在提交完成后原子发布完整周期，底盘观察器将其冻结进原 16 槽 FIFO；不在遥测发送时读取活跃 PID。停机、反馈过期未执行速度环时有独立状态标志，不能把占位零解释为真实反馈。
- I5 bit 10 表示控制快照新鲜且数值有限，与使能、CAN 邮箱接受、IMU/编码器有效性分别判断。
- 原始 schema 4/5/6 定义保留，分析工具兼容 schema 7 的连续性检查与卡尔曼回放。
- yaw PID 保持角度 Kp=8，速度 Kp=50/Ki=200，积分限幅 3000、总输出限幅 20000。本轮未加入角度偏置或前馈。

## 已通过验证

```text
make -C tests/vofa_telemetry -j4 test
make -C tests/chassis_velocity_observer -j4 test
make -C tests/dji_chassis_power_rtt -j4 test
make -C tests/chassis_force -j4 test
python -B -m unittest discover -s tests/vofa_analysis -p 'test_*.py'
python -B scripts/generate-vofa-schema.py --check
git diff --check （仅本任务路径）
```

Python 分析测试共 44 项通过，使用现有 Windows Python 3.12（已装 NumPy/SciPy）；内置 Python 缺 SciPy，首次运行的 4 个拟合测试因依赖失败，切换解释器后全套通过。未更改环境依赖。

新增验证覆盖：完成周期和待执行目标分离、真实速度反馈、积分/两环输出限位标志、符号处理后最终命令、停机/过期/安全门控归零、CAN 失败、控制快照过期/非有限数/时钟回绕，以及 FIFO 延迟发送仍保留捕获值。串口仿真按实际帧字节和波特率计算发送时长，400 次抖动事件与 70 ms 消费者暂停后全部排空，无丢失。

## 整车构建

构建使用当前工作区已有配置；保留原有未提交改动，包括 `config/21_chassis_spin/chassis_spin_config.h`，本任务没有修改或提交这些配置。构建目录独立于日常 Board 1 输出。

```powershell
make -j8 PROFILE=atype-vehicle BUILD_DIR=build/yaw-control-observation `
  GCC_PATH=C:/Users/xuzih/AppData/Local/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin `
  ATYPE_BENCH_MODE=0 CHASSIS_CONTROL_MODE=0 CHASSIS_FORCE_CALIBRATION_ENABLE=0 `
  CHASSIS_IMU_CAN_RX_ENABLE=1 CHASSIS_REMOTE_IMU_FUSION_ENABLE=1 `
  CHASSIS_VELOCITY_FUSION_CALIBRATED=0 CHASSIS_VELOCITY_FUSION_ENABLE=1 `
  IMU_CAPTURE_CAN_TX_ENABLE=0 IMU_CAPTURE_ONLY=0 ONE_BOARD=1 USB_TELEMETRY_ENABLE=1 `
  VOFA_OBSERVATION_PROFILE=2 VOFA_TELEMETRY_ENABLE=1 VOFA_TELEMETRY_MODE=2
```

- 完整 ARM 构建通过，无编译警告或错误。
- RAM 163976 B / 192 KB（83.40%）；FLASH 181480 B。
- 编译器静态栈：`DJIMotorControl` 208 B，`ChassisObserveFusedVelocity` 720 B（不含子调用）。
- BIN：`build/yaw-control-observation/basic_framework.bin`，181480 字节。
- SHA256：`2690c56fa5dacbd2cf25fbaff9bd6bc5e3973128363b83eea95fa8baa4222189`。
- ELF/HEX 与 `build.log` 在同一目录。

108 路加帧尾 436 字节，在 230400 / 8N1 下占线约 18.92 ms；50 Hz 理论余量约 5.4%。已通过固件静态预算和主机模拟，实际任务调度与无线链路仍须复测。未烧录或做本轮实车验证。

## 下一轮实测

使用主板 Board 1 固件，VOFA+ 设 JustFloat、230400 / 8N1，确认 I0=7。yaw 输入保持不动，切入前录 3 秒、小陀螺运行 5 秒、退出后录 3 秒，重复至少 5 次，完整导出 I0–I107。

先核对目标是否恒定和命令是否正常送出；若持续角误差伴随积分顶住 ±3000，但总输出有余量，再单独提高积分限幅做对照。没有新数据前不能把“积分不足”作为确定根因，也不能宣称偏差已消失。字段与判定步骤见 [VOFA 文档](../../vofa-telemetry.md#小陀螺-yaw-固定偏差第一轮控制环诊断)。
