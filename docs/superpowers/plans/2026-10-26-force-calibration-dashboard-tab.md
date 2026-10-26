# 力控标定统一仪表盘标签实施补充

用户已要求将 [已确认标签设计](../specs/2026-10-26-force-calibration-dashboard-tab-design.md) 加入正在实施的 [标定工具计划](2026-10-26-chassis-force-calibration-tools.md)。本补充替代原独立页串口装配，保留板端、记录格式与保护。

- [x] 先写共享路由、优先发送/未知所有权、force-v1 字段与单位测试，运行失败测试。
- [x] 实现 `serial-router.js`、`control-coordinator.js`、`force-telemetry.js`。行分发独立于绘图；停止取消未发送查询，所有权超时不解锁。
- [x] 标定 client 注入 send/连接状态，由 app 分发协议行；client 只取消自己的请求和心跳，不关闭共享端口。
- [x] 标定 UI 改为面板工厂，将原内容移入 index 第四标签；连接栏仅保留公共一套，停止入口保持可见。
- [x] 接入调参/整定互斥、标签切换等待后续零提交状态、后台停止、重连不恢复和 force-v1 独立原始历史/单位映射。
- [x] 旧 HTML 仅跳转 `/#force-calibration`，更新说明。运行全部 Node、模拟串口 Playwright、旧仪表盘检查及受影响板端测试。
- [x] 复核审查发现、记录实际结果和截图，提交本次相关文件；保留其他任务改动。

验证完成：72 项 Node、共享串口模拟操作、旧仪表盘与 V2 整定浏览器回归通过。详见 [验证记录](../verification/2026-10-26-chassis-force-calibration-tools.md)。
