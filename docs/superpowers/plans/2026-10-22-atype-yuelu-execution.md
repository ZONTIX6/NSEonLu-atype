# A 板跃鹿封装实施记录

基线：`451a8a9`（前序集成 `f4f70c7`）。执行依据：同目录 `2026-10-22-atype-yuelu-encapsulation.md`。下列记录只代表实际完成的工作；硬件项目须单独实测。

## 阶段 A：行为与依赖基线

- 新增阶段 0/3 输入流夹具 `tests/dji_chassis_power_rtt/baseline-stage*.csv`，固定启动、驱动、反转/制动、超功率、恢复、失联、整定退出的四轮整数命令、控制版本、预算和瞬态状态。原有 RTT 行与超电命令的逐字节断言继续保留。
- 复用已有九电机乱序、非底盘隔离、整定退出、快照交错和最终整数输出测试；未删除已有场景。
- 新增 `tests/architecture/test_layers.py`，按 Make 实际 include 搜索顺序解析头文件，检查模块反向依赖、纯算法硬件依赖、公共功控头与板级访问；检查器自身有相对路径、歧义和注释过滤测试。`baseline.json` 明确记录 55 处既有违规，新增违规或未移除的过期记录均使检查失败；最终验收须使用 `--strict`。
- 基线验证：四组指定主机测试、阶段 0/3 DJI 测试、架构检查通过。完整 `validate-atype.ps1 -FirmwareMatrix -CMake` 全部通过（主机、portable、Python、Node、16 个 Make 配置、3 个 CMake profile）。日志位于 `build/validation/`，日志目录为可重建产物。

## 阶段 B：超级电容设备契约

- 增加纯数据头、整体快照、输入帧诊断与命令失败计数；设备实例移入实现文件，保留旧 getter 供阶段 C 迁移。
- 原协议编码不变；状态与输入反馈的新鲜度独立；20/21 ms、无效帧时间不刷新、回绕和状态 Daemon 隔离均有断言。
- 测试先因缺少快照接口失败，再实现通过。补齐未初始化清零、空指针、重复初始化、Daemon 失败重试、两类 CAN 注册失败和半初始化回调安全场景。
- 验证：`tests/super_cap`、`tests/super_cap_protocol`、portable 超电测试、功率计测试、DJI 回归和架构检查通过。

## 后续阶段

C～H 待逐项实施和验证。硬件验收尚未执行。
