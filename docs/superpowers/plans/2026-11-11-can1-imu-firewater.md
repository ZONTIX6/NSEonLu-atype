# CAN1 汇总底盘 IMU 与 FireWater

**目标：** 第二块固定于底盘的 A 板经 CAN1 向第一块主板传输 IMU，电脑只连接主板 UART7，在一份 FireWater CSV 中观察轮速和底盘 IMU。

**设计：** 两端使用独立、默认关闭的 `IMU_CAPTURE_CAN_TX_ENABLE` 与 `CHASSIS_IMU_CAN_RX_ENABLE`。CAN1 保持 1 Mbps，新增标准数据 ID `0x520`，不复用电机、超电或功率计 ID。每 20 ms 发送一个一致 IMU 快照，9 个 8 字节帧，包含分片序号与传输 tag；54 字节内容含协议版本、源有效位、源年龄、序号、源时钟、四元数、加速度、角速度与 CRC16。接收端只发布完整组，10 ms 内严格按序接收，拒绝混组/丢片/重复/CRC 或数值错误。

**时间与范围：** 无两板时钟同步。主板只能计算本机接收年龄、组传输时长，加上发送时源年龄形成已知年龄下界，不能用主板时间减第二板时钟。CAN 接收数据只用于记录，主板 `CALIBRATED=0`，不将远端 IMU 塞进已有云台杆臂融合器。记录开关开启后不自动退回本地 IMU，断链必须标为无效。

**FireWater：** 接收模式输出 schema 3 固定 72 列，80 ms 最小周期；前 64 列保留布局，I33–I45 的 IMU 改用远端来源，I63 仍为主板角色 0，I5 bit15 标明 CAN 源。追加 I64=2（CAN1 底盘 IMU）、I65接收年龄、I66发送时源年龄、I67–I68源时钟高低位、I69组传输时长、I70–I71拒绝组计数高低位。未启用 CAN 的既有 schema 2/JustFloat 保持兼容。

## 工作与验证

- [x] `modules/imu/chassis_imu_can_protocol.[ch]`：显式小端序及 CRC16/CCITT-FALSE；固定内存重组；主机验证正常、失效、丢片/乱序/混组/重复/超时/长度/CRC/版本/非有限数、时钟回绕与恢复。
- [x] `application/runtime/chassis_imu_can.[ch]`：CAN1 单实例注册、20 ms 发送、单片最多等 1 ms、整组最多 10 ms，失败中止并取消本模块待发邮箱；IRQ 重组、临界区快照、接收和源新鲜度检查。验证源时钟与主板时钟无关、断链无效、不发送电机命令；模拟邮箱因高优先级反馈占用 0.5 ms 后可释放的情况。
- [x] 初始化/任务/Make/CMake：接收主板与纯 IMU 发送板分别构建；发送板无需开启 UART 遥测，维持 INS/温控任务和低优先级通信任务，不创建电机控制任务。
- [x] FireWater schema 3：测试远端有效/过期/未收到/源失效与数据来源标签；保留 schema 2 和 JustFloat 测试。
- [x] 更新接线、两端烧录命令、VOFA 列说明；主机测试、两端 Make/CMake 构建及独立代码审查通过。结果见 `docs/superpowers/verification/2026-11-11-can1-imu-firewater.md`。
