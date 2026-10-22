# INS 服务与 A 板装配

每板一个 INS/EKF。`ins_service.c` 持有校准、滤波、温控和健康状态，不持有 MPU、PWM 或视觉对象。应用在 BSP/DWT 就绪后注册设备，再通过 `INSServiceConfigure` 注入采样、加热和 context；配置按值复制，context 在服务运行期间一直有效，重复配置返回 false。

```c
static bool Read(void *context, IMUSample_s *sample) {
    return MPU6500Read(context, sample);
}
static void Heat(void *context, float duty) {
    (void)context;
    IMU_Temperature_Update(duty);
}
INSServiceConfig_s service = {.sample=Read, .heat=Heat, .context=imu};
if (!INSServiceConfigure(&service)) return false;
attitude_t *attitude = INS_Init();
```

`IMUSample_s` 输入为机体三轴加速度 m/s²、角速度 rad/s、摄氏温度、同一 HAL 毫秒时钟的 tick 和 valid。采样回调必须报告本次事务结果，不能复用旧缓冲并更新时间。非有限数、年龄超过 10 ms 或非法温度/加速度拒绝进入解算；加热回调接收 0..1，占空比转换只在服务的温控 PID 边界执行。

`INS_Init()` 在应用启动阶段执行阻塞校准，返回地址固定的姿态对象。保留 6000 次采样、15 s 校准超时和现有偏置回退；采样失败直接终止初始化、加热归零，此次启动即使通信恢复也不启用 EKF。重复初始化返回同一对象，不重复注册或重做校准。返回非空地址不表示健康，必须另查 `INSIsHealthy` 或快照。

初始化成功后，原 INS 任务每 1 ms 调用 `INS_Task`。运行中失败样本不更新 EKF 或时间戳，加热归零并置无效；正常新样本可恢复运行健康。`attitude_t` 的角度和角速度均为 °、°/s，加速度为 m/s²，保留现有 Pitch/Roll 映射。电机原反馈指针保持稳定，需要完整一致数据的消费者使用 `INSGetSnapshot`：其原子复制姿态、tick 和 valid，年龄超过 10 ms 返回 false，保留最后姿态供诊断；空输出返回 false。

A 板 `application/board_devices.c` 在 `BSPInit` 后、各应用初始化前装配 SPI5/PF6 和 TIM3 CH2/PB5。MPU 每次寄存器读取为地址加数据的同一 SPI 事务，样本为 15 字节全双工事务。当前超时配置 500 μs，由 DWT 周期计数约束，启动关中断时仍有效；值和 1 ms 周期的实际余量待硬件测量。量程保持 ±8g、1000 °/s；温度以有符号 int16 解码。MPU 已注册 SPI 后的初始化失败会锁定注册，避免残留 BSP 对象重复注册；未发布对象不可读取，失败输出清零。

加热 PWM 周期 100 μs，初始占空比 0.5，目标温度 40 °C；装配、校准或采样失败关闭加热。PWM 使用 ARR+1 计数，1 MHz 下 ARR=99、50% CCR=50。可选 OLED/按键仍在调度器启动后的原阶段初始化。

`application/ins_bridge.c` 在原 1 ms 任务内执行服务及视觉组合。单板/云台板保持每次任务发送视觉帧，只用有效快照刷新姿态；底盘板不发送视觉。服务自身不依赖视觉。

验证：`tests/mpu6500`、`tests/bsp_spi`、`tests/bsp_pwm`、`tests/ins_service` 和 `tests/atype`。后续硬件验收需核对 SPI 片选窗口、实际耗时、PWM、IMU 轴向、温控失败和视觉发送节拍。
