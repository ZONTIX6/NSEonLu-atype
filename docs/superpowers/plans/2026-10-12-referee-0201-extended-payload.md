# Referee 0x0201 Extended Payload Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Accept the referee system's CRC-valid 17-byte `0x0201` payload while preserving standard 13-byte compatibility and using only the documented first 13 bytes for control.

**Architecture:** Add one named compatibility length in the referee power interface. Apply the same 13-or-17 predicate in the isolated power-state parser and the integrated frame dispatcher; continue copying exactly 13 bytes into the standard protocol structure. Cover both layers with the captured hardware payload before changing production code.

**Tech Stack:** C11, GCC host tests, GNU Make, STM32F407 arm-none-eabi firmware build

## Global Constraints

- Accept `0x0201` payload lengths 13 and 17 only.
- Read standard fields only from bytes 0 through 12.
- Do not use or assign business meaning to the trailing `00 00 F0 41` bytes.
- Keep `payload[12]` bit 1 as the chassis output enable source.
- Do not change `0x0202` 14/16-byte compatibility behavior.
- Do not include the user's unrelated dirty working-tree files in any commit.

---

### Task 1: Power-state parser compatibility

**Files:**
- Modify: `tests/referee_power/test_referee_power.c`
- Modify: `modules/referee/referee_power.h`
- Modify: `modules/referee/referee_power.c`

**Interfaces:**
- Consumes: `RefereePowerStateAcceptFrame(RefereePowerState_s *, uint16_t, const uint8_t *, uint16_t, uint32_t)`
- Produces: `REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH` with value `17U`; `RefereePowerStateAcceptFrame` accepts robot-state lengths 13 and 17.

- [ ] **Step 1: Add the failing hardware-payload test**

Add this fixture and test to `tests/referee_power/test_referee_power.c`, then call it from `main`:

```c
static const uint8_t extended_robot_state[17] = {
    0x03U, 0x01U, 0xC8U, 0x00U, 0xC8U, 0x00U, 0x28U, 0x00U,
    0xF0U, 0x00U, 0x50U, 0x00U, 0x00U, 0x00U, 0x00U, 0xF0U, 0x41U,
};

static void TestAcceptsExtendedRobotStateUsingStandardPrefix(void)
{
    RefereePowerState_s state;
    RefereePowerStateReset(&state);

    ASSERT_TRUE(RefereePowerStateAcceptFrame(
        &state, 0x0201U, extended_robot_state, 17U, 250U));
    ASSERT_TRUE(state.has_robot_state);
    ASSERT_EQ_U32(80U, state.chassis_power_limit_w);
    ASSERT_FALSE(state.chassis_output_enabled);
    ASSERT_EQ_U32(250U, state.robot_state_tick_ms);
}
```

- [ ] **Step 2: Run the focused test and verify RED**

Run: `make -C tests/referee_power clean test`

Expected: FAIL at the new `ASSERT_TRUE` because 17-byte `0x0201` currently returns false.

- [ ] **Step 3: Implement the minimum parser change**

In `referee_power.h`, add:

```c
#define REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH 17U
```

In `referee_power.c`, replace the exact 13-byte check with:

```c
if (payload_length != REFEREE_ROBOT_STATE_PAYLOAD_LENGTH &&
    payload_length != REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH)
{
    return false;
}
```

Keep all standard field offsets unchanged.

- [ ] **Step 4: Run the focused suite and verify GREEN**

Run: `make -C tests/referee_power clean test`

Expected: both normal and legacy binaries print `PASS: referee power tests`.

### Task 2: Integrated frame dispatch and bounded copy

**Files:**
- Modify: `tests/referee_integration/test_referee_integration.c`
- Modify: `modules/referee/rm_referee.c`

**Interfaces:**
- Consumes: `REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH` and the parser behavior from Task 1.
- Produces: CRC-valid 17-byte `0x0201` frames reach `RefereePowerStateAcceptFrame`, update `GameRobotState` from exactly 13 bytes, and do not increment `payload_length_error_count`.

- [ ] **Step 1: Add the failing end-to-end test**

Create a 17-byte frame from the captured payload with the existing `BuildFrame` helper. Feed it after `StartReferee()`, then assert:

```c
ASSERT_EQ_U32(3U, info->GameRobotState.robot_id);
ASSERT_EQ_U32(80U, info->GameRobotState.chassis_power_limit);
ASSERT_TRUE(RefereeGetLinkDiagnostics(&diagnostics));
ASSERT_EQ_U32(0U, diagnostics.payload_length_error_count);
ASSERT_TRUE(RefereeGetPowerSnapshot(&snapshot));
ASSERT_TRUE(snapshot.chassis_output_valid);
ASSERT_FALSE(snapshot.chassis_output_enabled);
```

Call the new test from integration-test `main`.

- [ ] **Step 2: Run integration tests and verify RED**

Run: `make -C tests/referee_integration clean test`

Expected: FAIL because the dispatcher counts the 17-byte `0x0201` as a payload-length error and leaves robot state unavailable.

- [ ] **Step 3: Implement the dispatcher compatibility predicate**

In `HandleValidFrame`, add a named boolean:

```c
const bool extended_robot_state = command_id == ID_game_robot_state &&
    payload_length == REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH;
```

Exclude `extended_robot_state` from the length-error branch alongside `extended_power_heat`. In the successful robot-state branch, accept either robot-state length but retain:

```c
memcpy(&referee_info.GameRobotState, payload, LEN_game_robot_state);
```

- [ ] **Step 4: Run both focused suites**

Run:

```text
make -C tests/referee_power clean test
make -C tests/referee_integration clean test
```

Expected: all four normal/legacy test binaries pass.

- [ ] **Step 5: Build the firmware**

Run: `make -j4`

Expected: `build/basic_framework.elf`, `.hex`, and `.bin` build successfully with no new compiler diagnostics. If unrelated pre-existing dirty configuration prevents the build, record the exact diagnostic and leave those files untouched.

- [ ] **Step 6: Review and commit only task files**

Run `git diff --check` and inspect the diff. Stage only:

```text
modules/referee/referee_power.h
modules/referee/referee_power.c
modules/referee/rm_referee.c
tests/referee_power/test_referee_power.c
tests/referee_integration/test_referee_integration.c
```

Commit message: `fix: accept extended referee robot state frames`

### Task 3: Hardware acceptance check

**Files:**
- No source changes expected.

**Interfaces:**
- Consumes: firmware produced by Task 2.
- Produces: observed acceptance evidence from the physical referee link.

- [ ] **Step 1: Flash and observe the same 17-byte frame**

At the `HandleValidFrame` breakpoint, confirm `command_id == 0x0201` and `payload_length == 17` continue to occur.

- [ ] **Step 2: Verify state after the handler returns**

Observe:

```text
referee_power_state.has_robot_state = true
referee_power_state.chassis_power_limit_w = 80
referee_power_state.chassis_output_enabled = false
payload_length_error_count does not increase for this frame
```

The final false output flag is expected for the captured frame because byte 12 equals zero.
