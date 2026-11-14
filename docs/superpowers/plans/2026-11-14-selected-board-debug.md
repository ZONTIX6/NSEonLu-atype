# F5 跟随板卡配置 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** F5 与 attach 的 ELF 跟随 `BOARD_FLASH_TARGET`，修复全部入口固定第一块板的问题。

**Architecture:** 保留两板独立的构建目录。`build-board.ps1 -Action Debug` 构建选中板并复制 ELF 到固定的调试入口；`-Action Symbols` 只准备选中板已有的 ELF。启动任务不再覆盖板号，失败时不保留上一块板的调试副本。

**Tech Stack:** Windows PowerShell、VS Code Cortex-Debug、Python unittest、GNU Arm Make。

## Global Constraints

- 用户选择 F5 跟随 `board_flash_config.h` 的 `BOARD_FLASH_TARGET`。
- 下载器由调试配置选择；Build 日志不将配置中的 Flash 下载器误报为实际调试下载器。
- 保留用户已有配置/PDF/.agents 改动；不操作硬件、不推送。
- 验证通过后自动本地提交本任务改动。

---

### Task 1: 统一配置选择、构建与调试 ELF

**Files:**
- Modify: `scripts/build-board.ps1`
- Modify: `.vscode/tasks.json`, `.vscode/launch.json`
- Modify: `tests/board_flash/test_board_flash.py`
- Modify: `README.md`, `config/README.md`

**Interfaces:**
- Consumes: `BOARD_FLASH_TARGET`、两板现有 `BUILD_DIR`、VS Code `preLaunchTask`。
- Produces: `-Action Debug`（构建并准备 ELF）、`-Action Symbols`（只准备已有 ELF），固定输出 `build/selected-board.elf`。

- [x] 通过真实 launch→task→script 链路复现第二块板被覆盖为第一块的问题。临时项目使用两份内容不同的 ELF 和 Make stub，断言加载的内容来自选中板：

```python
for board in (2, 1):
    # Set BOARD_FLASH_TARGET in the temporary project, then execute preLaunchTask.
    self.assertEqual(executable.read_bytes(), f"board-{board}-elf".encode())
```

- [x] 扩展脚本动作；Debug/Symbols 在读取配置前删除固定调试副本；Debug 使用现有 Make 参数构建，Symbols 跳过构建，随后检查所选 ELF 并复制：

```powershell
$debugElf=Join-Path $boardProjectRoot 'build/selected-board.elf'
if($Action -in @('Debug','Symbols')) {
    if(Test-Path -LiteralPath $debugElf) { Remove-Item -LiteralPath $debugElf -Force }
}
# After configuration validation and any required successful build:
$firmware=Join-Path $boardProjectRoot "$buildDirectory/basic_framework.elf"
if(-not (Test-Path -LiteralPath $firmware -PathType Leaf)) { throw "Built firmware is missing: $firmware" }
Copy-Item -LiteralPath $firmware -Destination $debugElf -Force
```

- [x] F5 使用 `build selected board for debug`；attach 使用 `prepare selected board symbols`。全部 executable 使用 `${workspaceRoot}/build/selected-board.elf`。仅 Flash 输出 programmer；同步两处使用说明。
- [x] 测试配置错误、构建失败、ELF 缺失时旧副本消失；attach 不执行 Make；自定义板目录仍能加载正确 ELF。
- [x] 执行 `python tests/board_flash/test_board_flash.py`，分别用 `-Action Debug -Board 2` 和 `-Board 1` 实际构建，对比副本与源 ELF 的 SHA256；不启动调试/烧录。
- [x] 独立审查并执行 `git diff --check`，仅暂存本任务文件，提交 `fix: follow selected board for VS Code debugging`。

执行已由用户授权；当前会话实现，子代理独立检查 Cortex-Debug 生命周期和最终改动，不另行等待执行方式确认。

## 验证结果

- 原实现下新增切板测试失败，三个 F5 入口均得到 `board-1-elf` 而非所选 `board-2-elf`；修复后全部 23 项测试通过。
- 本机 VS Code 与 Cortex-Debug 1.12.1 代码确认：preLaunchTask 先于 ELF 校验，因此初次尚无调试副本也能正常准备。
- 实际 Debug 构建板 2 后切回板 1，两次调试副本 SHA256 均与对应构建目录 ELF 相同。
- 独立审查及新增四项测试复验通过；未启动硬件调试或烧录。
- 共用调试副本用于单会话；切换板卡前应结束当前调试。
