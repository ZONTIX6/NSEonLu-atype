# 双 A 板 IMU 与右偏 116 mm：软件验证记录

## 配置依据

- 用户确认图示坐标为 +X 前、+Y 右；116 mm 是底盘中心到下方 MPU6500 芯片的右向距离。因此内部前/左/上位置向量为 `(0,-0.116,0) m`。
- 两板朝向相同。官方 A 板位号图 U3 的 1 脚、MPU6500 图14和照片共同推导出传感器 +X 后、+Y 右、+Z 上；安装四元数 `[0,0,0,1]`。来源和推导见 [双板说明](../../dual-a-imu-integration.md)。根代理独立查看位号图与芯片轴图，复核了子代理推导。
- 恢复 CAN1 `0x520`、九帧 CRC 传输、源板样本序号/时间/年龄；移除 DM 模块解析、配置和接收端去重力。采集板只采样/温控/发送；主板融合只读，不接管控制。

## 回归与构建

以下检查通过：

- `make -B -C tests/chassis_imu_can_protocol test`：编解码、CRC、分片拒绝、无效样本与回绕。
- `make -C tests/chassis_imu_can_runtime test`：TX、RX、JustFloat 接收、超龄与邮箱取消。
- `make -B -C tests/imu_capture test`：UART/CAN 采集固件启动及任务隔离。
- `make -C tests/chassis_remote_fusion test`：源时钟配对、降级/恢复，以及生产配置的正反旋转、角加速、纯平移和组合运动补偿。
- `make -C tests/chassis_velocity_observer test`、`tests/chassis_velocity_center`、`tests/ins_service`：观测器、中心补偿和 INS 回归。
- `make -C tests/vofa_telemetry -j4 test`：15 个遥测目标，包括 A 板来源、源年龄、旧来源 bit 清除及现有功率/卡尔曼档。
- `python -m unittest discover -s tests/board_flash`：17 项，两板预设、独立目录、构建失败不下载。
- Python VOFA 融合重放 13 项、CSV 11 项、VOFA 配置 6 项、架构检查 12 项。
- `scripts/build-board.ps1 -Action Build -Board 1` 和 `-Board 2`：均生成 ELF/HEX/BIN；主板 FLASH 185632 B / RAM 156480 B，采集板 FLASH 72056 B / RAM 125144 B。构建使用 Make；CMake 改动做了源码检查，未运行 CMake 构建。
- 生成结构体/配置索引并检查差异；`git diff --check` 通过。

新增偏移夹具使用解析刚体运动加速度，不复用生产差分公式。零偏移配置先失败；改为内部 -Y 116 mm 后通过。随后用安装姿态 `[0,0,0,1]` 的夹具确认单位安装配置失败，再改为图纸推导值后通过。观察器夹具同步改为同一安装姿态，保持其“导航 +X 加速度对应车体前向速度”的物理含义。

独立代码复核发现旧发送限频按实际调用间隔要求20 ms，会把固定周期任务的 `161→180 ms` 正常抖动当成过快调用，导致漏组与融合重置。新增回归先失败；限频下限改为15 ms后通过，任务仍按20 ms调度，`194 ms` 快速追赶调用仍被抑制。复核修复后无未解决代码问题。

## 验证限制

额外尝试整个 `tests/vofa_analysis` 时，35项中31项通过，4项无关的功率模型拟合检查因当前 Python 缺少 `scipy` 失败；未修改环境或功率拟合代码。与本次变更相关的融合和CSV测试已单独通过。

没有连接硬件进行烧录或运动验证，没有推送远端。安装轴向是图纸与照片的推导，实物旋转符号、CAN 实际抖动、温控/零偏、旋转补偿残差仍待实测；`CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0` 保持不变。
