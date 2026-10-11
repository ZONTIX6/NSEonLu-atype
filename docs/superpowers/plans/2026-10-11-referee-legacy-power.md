# 裁判系统旧版实时功率兼容 Implementation Plan

> **For agentic workers:** Execute the following task inline in this session, with a read-only reviewer after validation. Steps use checkbox syntax for tracking.

**Goal:** 接收用户已确认的 16 字节 `0x0202`，提供实时功率调试变量，同时兼容 14 字节协议。

**Architecture:** 在既有接收分发与功率解析两层允许两种准确长度。根据布局解码功率及热量，沿用现有原子快照和新鲜度判断。

**Tech Stack:** C11、STM32F407、FreeRTOS、MinGW GCC 主机测试、ARM GNU Toolchain。

## Global Constraints

- 14 字节帧的功率位置是保留字段，不能当作有效实测功率。
- 16 字节帧的功率位于偏移 4，缓冲能量位于偏移 8，42 mm 热量位于偏移 14。
- 300 ms 超时和 CRC 检查沿用既有规则。
- 不更换独立 CAN 功率计的控制反馈来源。
- 保留工作区已有用户修改，只提交本任务文件。

### Task 1: 接收、解析、调试接口与回归验证

**Files:**
- Modify: `modules/referee/referee_power.h`, `modules/referee/referee_power.c`, `modules/referee/rm_referee.c`
- Test: `tests/referee_power/test_referee_power.c`, `tests/referee_integration/test_referee_integration.c`
- Create: `docs/referee-power-debug.md`

**Interfaces:**
- Consumes: `RefereePowerStateAcceptFrame()`、`RefereeGetPowerSnapshot()` 和完整裁判系统字节流。
- Produces: 原始状态 `chassis_power_w` / `has_chassis_power`，快照 `chassis_power_w` / `chassis_power_valid`。

- [x] **Step 1: 添加旧版帧回归并确认失败。**

```c
static const uint8_t legacy[16] = {
    0xA8U, 0x61U, 0xF4U, 0x01U, 0U, 0U, 0x48U, 0x41U,
    55U, 0U, 10U, 0U, 20U, 0U, 30U, 0U
};
ASSERT_TRUE(RefereePowerStateAcceptFrame(&state, 0x0202U, legacy, 16U, 100U));
ASSERT_TRUE(snapshot.valid);
ASSERT_EQ_U32(55U, snapshot.buffer_energy_j);
```

集成测试通过现有 `BuildFrame()`、`TestPlatformFeed()` 输入真实 CRC 帧，断言快照可用、原始功率为 12.5 W、42 mm 热量为 30。

```powershell
make -B -C tests/referee_power test
make -B -C tests/referee_integration test
```

- [x] **Step 2: 增加公开功率成员并补充语义回归。**

```c
/* RefereePowerState_s */
float chassis_power_w;
bool has_chassis_power;
/* RefereePowerSnapshot_s */
float chassis_power_w;
bool chassis_power_valid;
```

对 16 字节解析、16→14 切换、非有限功率、超时和坏帧保留旧状态分别断言实际功率和有效标志；保持测试在实现解析前失败。

- [x] **Step 3: 实现两种长度的接收与解码。**

```c
if (payload_length != REFEREE_POWER_HEAT_PAYLOAD_LENGTH &&
    payload_length != REFEREE_POWER_HEAT_LEGACY_PAYLOAD_LENGTH)
    return false;
```

新增小端浮点读取，有限功率填入原始状态；14 字节和非有限功率清零实时功率。分发层允许 16 字节功率帧；只复制现有结构大小，旧版 42 mm 热量从 `payload[14]` 读取。有效快照发布功率及可用标志。

- [x] **Step 4: 文档、主机回归及固件验证。**

```powershell
make -B -C tests/referee_power test
make -B -C tests/referee_integration test
make -B -C tests/referee_stream_parser test
make -B -C tests/chassis_power_link test
New-Item -ItemType Directory -Path build/referee-legacy-power -Force | Out-Null
make -j8 BUILD_DIR=build/referee-legacy-power UART1_USE=0
```

记录可烧录文件位置及 Watch 表达式，并说明原始状态与超时快照、独立 CAN 功率计变量的区别。

- [x] **Step 5: 只读代码审查，修复实际问题后提交。**

只读审查未发现阻塞问题；四组主机测试及隔离目录、默认调试目录的 ARM 固件构建均通过。本提交只包含本任务的八个文件。

```powershell
git diff --check -- modules/referee/referee_power.h modules/referee/referee_power.c modules/referee/rm_referee.c tests/referee_power/test_referee_power.c tests/referee_integration/test_referee_integration.c
git add modules/referee/referee_power.h modules/referee/referee_power.c modules/referee/rm_referee.c tests/referee_power/test_referee_power.c tests/referee_integration/test_referee_integration.c docs/referee-power-debug.md docs/superpowers/specs/2026-10-11-referee-legacy-power-design.md docs/superpowers/plans/2026-10-11-referee-legacy-power.md
git commit -m "fix: receive legacy referee chassis power frames"
```
