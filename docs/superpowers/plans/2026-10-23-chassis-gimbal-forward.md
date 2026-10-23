# 底盘跟随策略宏切换实施计划

> **For agentic workers:** 按本计划在当前会话逐项实施并审查；使用 test-driven-development 验证生产计算逻辑。

**Goal:** 默认以云台实测朝向解释平移输入、底盘不主动跟转，允许编译宏恢复旧策略。

**Architecture:** 保留 CMD 入口、模式枚举和通信结构。将 ChassisTask 的旋转模式选择与坐标转换移到 application/chassis/chassis_motion.h 的静态内联纯计算函数，供任务和主机测试共用；仅原跟随分支新增宏选择。自旋目标仍由现有策略提供，功控、自动整定与安全输出保持原路径。

**Tech Stack:** C11、CMSIS-DSP、GNU Make、CMake、ARM GCC、主机 GCC。

## Global Constraints

- `CHASSIS_FOLLOW_MODE` 默认 `1`，`#ifndef` 允许构建覆盖；只接受 `0/1`。
- `0` 保留 `clamp(-50 × theta_format(offset_angle), -4500, +4500)`。
- `1` 每周期显式设置 `wz = 0`；不新增绝对航向锁定。
- 相对角继续来自 yaw 实测编码器和 `YAW_CHASSIS_ALIGN_ECD`；只进行一次原有平移转换。
- 保留 `CHASSIS_NO_FOLLOW` 的模式值和自动整定限制；不改变通信结构大小。
- 不提交会话开始前已有的 PID、台架功率回退、遥测和诊断改动。
- 固件验证仅构建，不烧录；实车验收仍需架空检查及低速测试。

### Task 1: 实现与测试跟随策略

**Files:**
- Create: `application/chassis/chassis_motion.h`
- Modify: `application/chassis/chassis.c`, `application/robot_def.h`, `config/atype-options.txt`
- Create: `tests/chassis_motion/Makefile`, `tests/chassis_motion/test_chassis_motion.c`, `tests/chassis_motion/stubs/arm_math.h`, `tests/chassis_motion/stubs/bsp_usart.h`

**Interfaces:**
- Consumes: 原有 `Chassis_Ctrl_Cmd_s`、`theta_format(float)`、CMSIS 三角函数和自旋策略目标。
- Produces: `static inline void ChassisResolveMotion(Chassis_Ctrl_Cmd_s *cmd, float spin_wz, float *vx, float *vy)`；更新 `cmd->wz`，输出底盘坐标平移速度。

- [x] 先写共享生产接口测试。独立表格期望覆盖 `±30° → ∓1500`、`±90°/±180°` 限幅、`330°/-330°` 环绕；新策略遍历角度为零。

```c
Chassis_Ctrl_Cmd_s cmd = {.chassis_mode=CHASSIS_FOLLOW_GIMBAL_YAW,
                         .offset_angle=30, .wz=4000};
float vx, vy;
ChassisResolveMotion(&cmd, 4000, &vx, &vy);
/* 独立构建：旧策略期望 -1500，新策略期望 0。 */
```

- [x] 运行 `make -C tests/chassis_motion test CC=gcc`，确认缺少新接口导致失败，再补实现。
- [x] 在 robot_def.h 增加默认值与非法值检查，在选项清单加入同名宏。

```c
#ifndef CHASSIS_FOLLOW_MODE
#define CHASSIS_FOLLOW_MODE 1
#endif
#if CHASSIS_FOLLOW_MODE != 0 && CHASSIS_FOLLOW_MODE != 1
#error CHASSIS_FOLLOW_MODE must be 0 or 1.
#endif
```

- [x] 将现有模式选择与三角转换移入共享函数，跟随分支使用 `#if CHASSIS_FOLLOW_MODE == 0` 保留旧表达式，另一分支写零。ChassisTask 传入原动态或固定自旋目标。
- [x] 运行默认、显式 `0/1` 三组主机测试。用前后左右独立期望表覆盖 `0/±90/±180°`；检查跨 `0/360°` 和 `±180°` 连续性、小陀螺切出不残留旋转以及无平移输入。
- [x] 对 `-1/2/255` 编译 `#include "robot_def.h"`，要求非零退出且诊断指向 `CHASSIS_FOLLOW_MODE`。

### Task 2: 文档、构建与审查

**Files:** `docs/chassis.md`, `docs/configuration-guide.md`, `docs/configuration-index.md`。

**Interfaces:** 使用 Task 1 的 `CHASSIS_FOLLOW_MODE=0/1`，由 Make/CMake 共用选项列表透传。

- [x] 文档说明默认策略、恢复方式、构建目录、两种操作入口、实测角度与非航向锁定边界；索引记录宏定义及生产条件分支，不纳入无关工作区变化。
- [x] 回归 `make -C tests/chassis_kinematics test CC=gcc`、`make -C tests/chassis_spin test CC=gcc`、`make -C tests/atype test`、功控与 DJI 输出许可相关主机测试。
- [x] 独立目录运行 `make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-original CHASSIS_FOLLOW_MODE=0` 和 `make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-gimbal-forward CHASSIS_FOLLOW_MODE=1`；默认省略宏也构建。核对 configuration.txt 的实际宏。
- [x] CMake 使用 `-DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=0/1` 分别配置和构建，核对 compile_commands.json；非法值构建必须失败。
- [x] 使用 requesting-code-review 派出只读审查，修复实际问题。审查重点为原公式兼容、转换未重复、安全许可未扩大、工作区已有改动未混入。
- [x] 检查 `git diff --check`；只暂存本任务文件与共享文件中本任务的 hunks，创建本地提交并报告编号。

## 验证记录

2026-10-23 软件验证结果：

- TDD：先确认缺少接口导致编译失败，再用原生产公式确认新策略断言失败；加入宏分支后通过。
- `tests/chassis_motion`：省略宏、显式 `0/1` 全部通过；非法 `-1/2/255` 均由预处理拒绝。测试共用生产函数与 `user_lib.c`；仅 CMSIS 三角原语由主机 libm 替代。
- 已通过运动学、自旋、atype 安全、功控模块、功控链接、DJI 功控集成、CAN 失联回归，共七组现有主机测试。
- Make：`follow-original`、`follow-gimbal-forward`、`follow-default` 完整构建通过；默认与显式 `1` 的 `.bin` 哈希相同。双板 `CHASSIS_BOARD=1`、麦轮 `CHASSIS_DRIVE_TYPE=0`、正弦 `CHASSIS_SPIN_MODE=2` 组合也通过。
- CMake：`cmake-follow-0/1` 完整构建通过；核对 chassis.c 的 compile_commands.json 及 Make configuration.txt，确认宏被实际传入编译器。两条构建路径的非法 `2` 都因指定宏检查失败。
- 两策略的镜像布局检查通过；从 Git 暂存内容生成的独立副本排除了原有 PID 与台架配置修改，两种策略固件和新主机测试再次通过。
- 日志保存在 `build/follow-validation/`。未烧录，未执行实车验收；实际输入符号、机械零位、轮向和低速移动仍需按方案检查。

- 独立只读审查通过：未发现需要修复的问题，确认未暂存的 PID 与台架配置没有混入提交。
