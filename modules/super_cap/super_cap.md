# F334 超级电容设备模块

F427 A 板通过 CAN 与 F334 通信。模块内部持有一个设备实例，应用通过复制快照及发送命令使用设备。

```c
SuperCap_Init_Config_s config = {
    .can_config = {
        .can_handle = &hcan1, /* 仅在应用装配中选择硬件 */
        .tx_id = SUPERCAP_COMMAND_CAN_ID,
        .rx_id = SUPERCAP_STATUS_CAN_ID,
    },
    .offline_reload_count = 20,
    .input_power_feedback_enabled = true,
};
bool ready = SuperCapInit(&config);
SuperCapSnapshot_s snapshot;
bool valid = SuperCapGetSnapshot(now_ms, &snapshot);
SuperCapCommand_s command = {
    .enable_dcdc = true,
    .referee_power_limit_w = 100,
    .referee_buffer_energy_j = 50,
};
bool submitted = ready && SuperCapSendCommand(&command);
```

`SuperCapGetSnapshot` 在一个短临界区复制状态、输入功率、各自接收时间和诊断计数。两帧时间独立，一致复制不表示同时采样。`now_ms` 必须来自同一毫秒时钟；无符号差值处理 32 位回绕。输入反馈年龄 20 ms 有效、21 ms 无效。状态在线性保持 Daemon 默认 20 次、每次 10 ms 的语义；缓冲能量控制还须在功控策略中要求状态年龄 ≤20 ms。

命令 `0x061`、状态 `0x051`、可选输入反馈 `0x052` 均保持原有 DLC 8 与字节定义。仅有效状态帧刷新状态 Daemon，输入帧不维持状态在线。非法 DLC、非有限浮点和协议错误只增加拒绝计数。发送返回 true 仅表示 CAN 提交成功；设备确认的输入目标来自新鲜 `0x052`，不从发送命令推断。已发布设备的命令编码或 CAN 发送失败均计入 `command_fail_count`。

未初始化或初始化失败时，快照返回 false 并清零；空输出返回 false。初始化成功后拒绝重复注册。若 Daemon/CAN 已保存回调，后续注册失败将保留失效对象以避免悬空回调，并拒绝再次注册；未注册依赖前的分配失败可重试。半初始化回调不更新状态或刷新 Daemon。

`super_cap_types.h` 只依赖纯协议数据头，可供无 HAL 的主机功控使用。所有消费者通过整体快照读取状态；迁移用分项 getter 已删除。功率预算、裁判、台架、零力及恢复政策由功控和应用负责。

协议说明：`docs/supercap-can-protocol.md`；唯一编解码实现为 `super_cap_protocol.c`。
