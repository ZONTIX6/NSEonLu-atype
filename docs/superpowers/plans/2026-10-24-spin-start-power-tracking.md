# 非零起转与直供功率跟踪实施计划

**Goal:** 自适应自转从非零下限起转，消除普通升速和有界模型补偿被额外门槛拦截的问题。

**Architecture:** 保留现有自转功率误差控制器、缓冲状态机和保护分层，只区分普通增长与严格扩展跟随，并将模型 guard 的补偿口径与实测上限分开。

**Tech Stack:** C11 主机测试、STM32 ARM GCC、现有 Make 构建。

## 全局约束

- 当前 `min_wz=1000` 为健康进入时的非零初始目标；故障与退出输出 0。
- 自适应默认 rise=3600 Wz/s，固定和正弦保留旧默认；扩展区仍为 600 Wz/s。
- 实测基础上限仍为 50 W、目标 49 W；不降低缓冲到达门槛。
- 保留原 +8 W 模型预算边界；只有实际应用的有效补偿可进入预测 guard。
- 用户原有两文件差异不提交。

## Task 1: 非零起转与普通跟随

Files: `modules/algorithm/chassis_spin.c`, `chassis_spin_config.h`, `tests/chassis_spin/test_chassis_spin{,_priority,_limits}.c`。

- [x] 先将进入/重入测试改为首个健康目标等于 min_wz，后续步长受 rise 限制；增长被拒绝时仍为零。运行 `make -B -C tests/chassis_spin test` 观察失败。
- [x] 增加直供软范围内逐轮误差 300 rpm、整车误差超过 .05 圈/s 时继续增长的测试，并保持持续掉队与扩展区严格跟随测试。
- [x] 用健康增长条件为尚未建立的自适应目标设置 min_wz，之后从已发布目标累加。将整车跟随与整车候选领先限值仅用于严格扩展区，普通范围保留逐轮候选领先/趋势与持续回收；首次非零需求和确认的趋势放行除外。
- [x] 设置自适应默认 rise=3600，执行自转全部模式回归。

## Task 2: 有界模型补偿与生产闭环

Files: `modules/algorithm/chassis_power_transient.c`, `tests/chassis_power_transient/test_chassis_power_transient.c`, `tests/chassis_power_module/test_spin_policy.c`。

- [x] 增加 50 W 下已应用 +8 W 模型补偿能越过 50 W 预测边界的失败测试，同时实测超过 50 W、旧帧、制动、反馈过期和异常 dt 都撤销正补偿。
- [x] 预测 guard 的物理瓦数边界换算时，加上 `eta * max(slow_applied_w, 0)`；实测快保护比较及原模型预算上限保持独立。
- [x] 生产模块闭环接入完整直供旋转几何、当前 Kp 对应的速度环比例系数与独立负载，覆盖 50 W、负向模型误差、基础确认和缓冲增补。更新旧测试中把模型瓦数等同于实测物理上限的断言。
- [x] 执行 `make -B -C tests/chassis_power_transient test`、`make -B -C tests/chassis_power_module test`，连同预算与外环测试通过。

## Task 3: 验证、文档和提交

- [x] 更新 `docs/chassis-power-priority.md` 的起转、默认 rise 和补偿口径，说明 Flash 优先与实车验证限制。
- [x] 完成相关主机回归与直供/固定/正弦/电容固件构建。
- [x] 按 requesting-code-review 技能派发独立只读审查，处理重要问题后复验。
- [x] `git diff --check`；分块暂存仅本任务差异，创建本地提交并报告编号。

## 验证记录

- 自转固定/自适应/正弦、模式覆盖、非零进入与重入、增长拒绝、300 rpm 普通负载误差、持续掉队、扩展边界全部通过。
- 补充硬边界退出滞回回归：普通趋势放宽不得覆盖 hard_hold；测试先失败，加入 healthy 与 !hard_hold 后通过。
- 瞬态保护覆盖已应用补偿及实测 51 W、计量离线/关闭/过期、反馈过期、制动、异常 dt 撤销；预算、外环、持续超功率、功控链路、DJI RTT 集成通过。
- 生产模块 32 种 source/meter/buffer/stage 组合通过；50 W 独立带惯性负载使用 .75*(6 deg/s per rpm)=4.5/rpm。无偏差与模型高估场景均达到准入范围并进入缓冲辅助。
- 用旧 transient 源码重编译同一偏差闭环：基础峰值 44.16 W、无辅助；新代码基础峰值 47.33 W、进入辅助，最高慢环修正约 4.77 W。数值仅为合成负载验证，不是实车测量。
- 四套固件成功：build/spin-start-direct、spin-start-cap、spin-start-fixed、spin-start-sine，直供产物在 build/spin-start-direct/basic_framework.hex。
- requesting-code-review 独立审查结论 Ready；重要迟滞问题已修复并复验，无遗留 Critical/Important。
- 实车尚未烧录验证；有效旧 Flash 仍覆盖 min_wz/rise，保留用户原有 PID、自适应开关和 fixed/max Wz 差异。
