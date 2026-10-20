# PID 自动整定 V2 实施计划

> 按用户已批准的 V2 方案在当前会话内实施；完成后请求独立代码审查。实机试验留作明确的交付待办。

**Goal:** 实现可重复、多转速独立验收的速度环 Kp/Ki 整定，并完成全部可离线执行的验证。

**Architecture:** 保留 A1 引擎行为；A2 使用持续停稳、时间加权统计和反馈序号。网页把指标判定、候选策略、预算和串口运行分开，使用现有 SET/文件/Flash 流程显式采用。

**Tech Stack:** C11 / FreeRTOS、原生浏览器 JavaScript、Node test、GCC 主机测试及 ARM 固件构建。

## Global Constraints

- Kp 0.01～5、Ki 0～10、Kd 固定；不扩展转速、电流、超速保护或 1000 ms 租期、180 秒会话。
- 每轮独立会话，结束恢复原参数，下一轮需用户完成零力重新准备；不自动续会话。
- 新候选最多 18、总试验最多 50、额外复核最多 4；独立验收 18 次不复用搜索样本。
- 保留工作区既有改动，仅提交本次内容。无实机结论或统计置信度声明。

## A2 协议表（ASCII，CRLF，每行小于 512 字节）

请求保留 A1 的 BEGIN/revision、TRIAL/session/wheel/rpm/hold/cap/kp/ki/kd、STATUS/session、END/session 字段和 float32 十六进制格式，前缀改为 `@A2`。另加：

| 请求 / 回复 | 固定字段 |
| --- | --- |
| `@A2,id,CAP` | `@A2,id,CAP,2,31,180000,1000,20`：版本、能力位（停稳/时间加权/反馈质量/配置快照/参数回显）、会话 ms、租期 ms、反馈最大间隔 ms |
| `@A2,id,CONFIG,wheel` | `@A2,id,CONFIG,wheel,improve,coefA,coefB,outputRC,derivativeRC,speedSmooth,currentSmooth,motorFlags`；六个实数为 float32 hex，其余 uint32 |
| 错误 | `@A2,id,ERR,reason`，不改变 A1 错误格式 |
| 状态结果 | `@A2,id,OK,session,state,wheel,trial,reason,samples,rmse_milli,overshoot_milli,settle_ms,steady_milli,currentTV_milli,limited_pm,window_ms,coverage_ms,feedback_count,max_feedback_ms,max_stat_ms,stat_limit_ms,current_limit,kp,ki,kd,phase,elapsed_ms` |

所有整数限 uint32；rpm 指标 ×1000，电流总变差为原始指令单位/秒 ×1000，限流时间千分比，未稳定 sentinel 4294967295。极端缩放溢出须饱和并将结果标为质量失败；相应网页不接受推荐。phase 0 等待停稳、1 阶跃、2 等待试后停稳。elapsed 是本会话经过 ms。A1 结果字段和含义保持原样。

### Task 1：固件指标、停稳和协议

Files: `modules/telemetry/pid_autotune.{h,c}`、`autotune_protocol.{h,c}`、`usb_telemetry_stream.c`、`modules/motor/DJImotor/dji_motor.c`、`tests/pid_autotune/*`、DJI 集成测试。

- [x] 添加并先运行失败测试：持续停稳/超时、手算不等周期 RMSE/稳态误差/限流/TV、反馈去重、缺口、回绕。
- [x] 实现 A2 引擎及真实反馈序号接入，无动态分配或历史数组。
- [x] 添加 A2 CAP/CONFIG/结果及参数回显，检查边界、截断、非法数值、跨版本会话和异常停止，运行 A1 回归。

### Task 2：指标、策略、预算和报告

Files: `tools/usb-telemetry/autotune-{metrics,strategy,budget,report}.js`、`autotune.js`、对应 `tests/*.test.cjs`。

- [x] 先添加确定性模拟测试：已好参数、远处改善、Ki=0、边界、偶然低分、漂移、限流、跨转速退化、预算、取消。
- [x] 分离 `evaluate(metrics, options)`、`searchWheel(run, original, options)`、`TrialBudget`；实现基线重测、分段搜索/去重、漂移、复筛和 18 次独立验收。
- [x] A2 客户端严格校验请求 ID、session/trial/wheel/参数、长度和整数，迟到回复不推进；保留可标注的 A1 客户端兼容性。
- [x] 报告导出包含快照、阈值、种子、所有记录/失败和适用条件，支持 JSON 与逐试验 CSV。

### Task 3：仪表盘接入与生命周期

Files: `autotune-ui.js`、`index.html`、`autotune.css`、`protocol.js`、浏览器验证脚本。

- [x] 能力确认后才允许实机 V2；旧固件显示升级提示。合成演示使用同一策略和完整指标。
- [x] 四轮逐个 BEGIN/END，零力准备后显式继续；快照变化/断连失效，保留探索报告。
- [x] 显示阶段、有效/无效/预算、三档中位数和范围、硬门槛及退化；补 JSON/CSV 下载。
- [x] 采用前复核各配置组和 PID 值/版本，保留文件同步、Flash 保存及重试逻辑。

### Task 4：验证和交付

- [x] Node、C 引擎/协议、DJI/USB/Flash 回归；USB 与视觉固件构建。
- [x] 浏览器单轮/四轮、旧固件、取消/断连/迟到、冲突及保存重试，桌面与窄屏可视检查。
- [x] 已请求独立核心审查并完成主执行者最终复核，更新 README 与验证记录（独立审查最后阶段因工具额度中止，详见验证记录）。
- [x] 本地提交；列出保护实测、指标对照、至少 10 组整定、独立复现和 V1 交替比较等剩余实机步骤。
