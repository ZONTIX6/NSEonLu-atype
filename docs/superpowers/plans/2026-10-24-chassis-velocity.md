# 底盘速度实施计划

目标：四轮 CAN 实测转子 rpm → 几何中心速度 → 固定指针/一致快照 → CSV v4 → 网页数值与曲线。用户追加要求：目标和实际共用 `chassis_velocity` 指针，`target/actual` 均为 m/s、圈/s。。

- [x] 在 `modules/algorithm/chassis_velocity.h/.c` 增加无硬件依赖的 `ChassisEstimateVelocity(config,input,output)`；测试先验证 19:1、60 mm 半径下 1900 rpm 为轮周线速 0.6283185 m/s，再验证纯前进、右移、旋转及组合、方向校正、非法数据/几何、20 ms 边界与时钟回绕。执行 `make -B -C tests/chassis_velocity test CC=gcc`，先红后绿。
- [x] `application/chassis/chassis.h/.c` 补 `EstimateSpeed()`、固定只读 `chassis_velocity`、`ChassisGetVelocitySnapshot()`；用短临界区复制四轮原始反馈和在线状态，计算后短临界区发布；`ChassisTelemetrySnapshot_s` 嵌入速度。不改双板报文或现有控制输出。
- [x] `modules/telemetry/usb_telemetry*` 追加 CSV v4 字段并编码，使用速度自身有效性/最老反馈时间做 20 ms 新鲜度检查。修改 C 测试验证带符号单位和旧字段顺序、最大长度、传输版本。
- [x] `tools/usb-telemetry/protocol.js` 接收 v1–v4，v4 新量换算为 m/s、圈/s；旧版本补无效值导出 v4。先写 Node 用例验证新单位、旧版本、过期及导出再实现。
- [x] `index.html/app.js` 新增目标/实际速度数值和六条双轴曲线、演示数据，复用已有图表交互。浏览器验证有效/过期/旧版本、暂停、导出和手机宽度。
- [x] 完成相关 C/Node 测试、ARM vehicle 和麦轮构建、代码审查；记录指针用法和配置限制。仅提交本次改动，保留用户已有 Kp 和 spin 配置改动。

验证记录：41 个 C 套件通过；52 个 Node 测试通过；速度面板与 IMU、选择器、串口、功控状态浏览器检查通过；vehicle 全向轮及麦轮 ARM 构建完成。独立审查无阻塞项。当前未做实车验证。
