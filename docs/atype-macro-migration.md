# A 板宏迁移对照与输入基线

日期：2026-10-22。输入快照提交：`b86436332c9af97b04fe7541e9f050e2b32cbe76`，本地分支 `codex/atype-input-baseline`。该提交从原 `e93d04e` 保存了开始实施时的 8 个未提交文件；融合没有用历史 HEAD 覆盖这些内容。原始文件及 SHA256 如下。

| 文件 | SHA256 |
| --- | --- |
| `application/chassis/chassis.c` | `22b3984ec34603f9baddc1ae2d4664319880bfd3c1e0e2d3943b3e99b4705668` |
| `application/robot_def.h` | `017ef067798ca6ada54c4b3f44d6a3da88905cd43f515563333a2a6a391dfb43` |
| `Inc/board_config.h` | `020cb60a67b085271d4cfb6cce5656e14eff1c7e752ab8702340800a26ad78d1` |
| `modules/algorithm/chassis_power_config.h` | `43ed12bef4a790de09604f87155ec71f4c9443a615cb0bb470a99248fc1d9a8c` |
| `modules/algorithm/chassis_spin_config.h` | `33932ea83be9e0d70c75b0ece332954a1640d6f4fda0855292e4722da92b9ffb` |
| `modules/motor/DJImotor/dji_motor.c` | `74007a8785dd67fea259140d0176b7c857b104bda5da1130d78a82774e06b68e` |
| `modules/telemetry/rtt_telemetry.h` | `a68756c96aba198de2544d66f730b8d151a2d31b5e9051b00f58698485275cf6` |
| `modules/telemetry/tuning_overrides.h` | `cf4c41bd07b5d476a230c870be321c3a524d107cf362df0c89d1a07f5359289b` |

## 构建配置宏

“旧值”取工作区输入快照，未修改过的配置取原 HEAD；“/”表示互斥分支中的定义条目，生效分支以实际构建为准。默认自旋旧值为 SINE(2)，新值为 FIXED(0)。未改变数值的参数保留原单位；仅增加 `#ifndef` 的项用于接收构建覆盖。

| 宏 | 旧值 | 新值 | 原位置 | 最终位置 | 单位 | 原因 | 生效条件 | 验证 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `POWER_METER_ENABLE` | `1; 1` | `1` | `application/robot_def.h; modules/algorithm/chassis_power_config.h` | `modules/algorithm/chassis_power_config.h` | `无量纲/选择值` | `默认值集中到 chassis_power_config.h` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_POWER_CONTROL_STAGE` | `3` | `3` | `modules/algorithm/chassis_power_config.h` | `modules/algorithm/chassis_power_config.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_POWER_CAPTURE_ENABLE` | `0` | `0` | `modules/algorithm/chassis_power_config.h` | `modules/algorithm/chassis_power_config.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `设1时要求LOG_OUTPUT=2或3，USART2不得占用` | `配置矩阵/主机回归` |
| `CHASSIS_SPIN_ADAPTIVE_ENABLE` | `1` | `0` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_MODE` | `SINE (2)` | `FIXED (0)，显式覆盖优先` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_BUFFER_POWER_ENABLE` | `1` | `1` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_POWER_SOURCE_MODE` | `CHASSIS_POWER_SOURCE_DIRECT` | `CHASSIS_POWER_SOURCE_SUPERCAP` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_POWER_CALIBRATION_ENABLE` | `0` | `0` | `modules/telemetry/rtt_telemetry.h` | `modules/telemetry/rtt_telemetry.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_POWER_BENCH_TEST` | `1` | `0` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_POWER_BENCH_LIMIT_W` | `50.0f` | `50.0f` | `application/robot_def.h` | `application/robot_def.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `USB_TELEMETRY_ENABLE` | `1` | `1` | `modules/telemetry/usb_telemetry_config.h` | `modules/telemetry/usb_telemetry_config.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `REFEREE_LEGACY_POWER_FIELDS_ENABLED` | `0` | `0` | `modules/referee/referee_power.h` | `modules/referee/referee_power.h` | `布尔` | `保留显式旧功率字段选择，16字节长度不能单独判定功率有效` | `仅核实协议后设1` | `tests/referee_power` |
| `REFEREE_ENABLE` | `无` | `1` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `LOG_OUTPUT` | `无` | `1 /* 0: off, 1: RTT, 2: USART2, 3: both */` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `ATYPE_BENCH_MODE` | `无` | `0` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_DRIVE_TYPE` | `CHASSIS_DRIVE_MECANUM` | `CHASSIS_DRIVE_OMNI_X` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `VISION_USE_UART` | `未定义` | `默认定义（无数值）` | `application/robot_def.h` | `application/robot_def.h` | `存在性开关` | `视觉迁至参考USART3` | `未指定任何视觉传输时自动定义；与VCP互斥` | `默认固件/USB视觉配置` |
| `VISION_USE_VCP` | `定义（无数值）` | `默认未定义；构建可定义为1` | `application/robot_def.h` | `application/robot_def.h、config/atype-options.txt` | `存在性开关` | `默认USB留给遥测，保留USB视觉` | `启用时USB_TELEMETRY_ENABLE必须为0；不以=0关闭存在性宏` | `USB视觉固件` |
| `ONE_BOARD` | `定义（无数值）` | `未指定其他角色时自动定义` | `application/robot_def.h` | `application/robot_def.h` | `存在性开关` | `默认单板完整整车` | `三种角色只能定义一种；不以=0关闭` | `默认/双板固件` |
| `CHASSIS_BOARD` | `未定义` | `可选，构建定义为1` | `application/robot_def.h（注释入口）` | `application/robot_def.h、config/atype-options.txt` | `存在性开关` | `保留底盘板角色` | `与ONE_BOARD/GIMBAL_BOARD互斥` | `atype-chassis/dual-bench` |
| `GIMBAL_BOARD` | `未定义` | `可选，构建定义为1` | `application/robot_def.h（注释入口）` | `application/robot_def.h、config/atype-options.txt` | `存在性开关` | `保留云台板角色` | `与ONE_BOARD/CHASSIS_BOARD互斥` | `atype-gimbal` |
| `OLED_USE_I2C` | `无` | `0 /* A-board SPI1 OLED; I2C2 backend remains optional. */` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `ATYPE_LINK_MODULES` | `无` | `默认不定义；验证设1` | `无` | `config/atype.mk、CMakeLists.txt` | `布尔` | `强制保留可选模块实现入口` | `设1时读取atype-link-symbols.txt` | `optional/i2c-oled及nm检查` |
| `BOARD_AUX_UART2_DEVICE` | `无` | `0` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_AUX_UART7_DEVICE` | `无` | `0` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_OLED_ENABLE` | `无` | `0` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_KEY_ENABLE` | `无` | `0` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `YAW_CHASSIS_ALIGN_ECD` | `3734` | `350` | `application/robot_def.h` | `application/robot_def.h` | `编码器计数` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `YAW_ECD_GREATER_THAN_4096` | `0` | `0` | `application/robot_def.h` | `application/robot_def.h` | `编码器计数` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `PITCH_HORIZON_ECD` | `3412` | `3412` | `application/robot_def.h` | `application/robot_def.h` | `编码器计数` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `PITCH_MAX_ANGLE` | `0` | `30` | `application/robot_def.h` | `application/robot_def.h` | `°` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `PITCH_MIN_ANGLE` | `0` | `-30` | `application/robot_def.h` | `application/robot_def.h` | `°` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `ONE_BULLET_DELTA_ANGLE` | `36` | `45` | `application/robot_def.h` | `application/robot_def.h` | `°` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `REDUCTION_RATIO_LOADER` | `36.0f` | `36.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `NUM_PER_CIRCLE` | `10` | `8` | `application/robot_def.h` | `application/robot_def.h` | `发/圈` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `WHEEL_BASE` | `350` | `350` | `application/robot_def.h` | `application/robot_def.h` | `mm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `TRACK_WIDTH` | `300` | `300` | `application/robot_def.h` | `application/robot_def.h` | `mm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CENTER_GIMBAL_OFFSET_X` | `0` | `0` | `application/robot_def.h` | `application/robot_def.h` | `mm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CENTER_GIMBAL_OFFSET_Y` | `0` | `0` | `application/robot_def.h` | `application/robot_def.h` | `mm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `RADIUS_WHEEL` | `60` | `60` | `application/robot_def.h` | `application/robot_def.h` | `mm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `REDUCTION_RATIO_WHEEL` | `19.0f` | `19.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `OMNI_X_LF_DIRECTION` | `1.0f` | `1.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `OMNI_X_RF_DIRECTION` | `1.0f` | `1.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `OMNI_X_LB_DIRECTION` | `1.0f` | `1.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `OMNI_X_RB_DIRECTION` | `1.0f` | `1.0f` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `GYRO2GIMBAL_DIR_YAW` | `1` | `1` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `GYRO2GIMBAL_DIR_PITCH` | `1` | `1` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `GYRO2GIMBAL_DIR_ROLL` | `1` | `1` | `application/robot_def.h` | `application/robot_def.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_BUFFER_MAX_ENERGY_J` | `60.0f` | `60.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RECOVERY_ENTER_J` | `15.0f` | `15.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RECOVERY_EXIT_J` | `30.0f` | `30.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_HARD_FLOOR_J` | `5.0f` | `5.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_PREDICTION_WINDOW_S` | `0.5f` | `0.5f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_GUARD_WINDOW_S` | `0.3f` | `0.3f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_SETTLEMENT_DELAY_S` | `0.1f` | `0.1f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_ENERGY_MARGIN_J` | `3.0f` | `3.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_POWER_MARGIN_W` | `5.0f` | `5.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_BOOST_MAX_W` | `60.0f` | `60.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RECOVERY_POWER_W` | `20.0f` | `20.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S` | `1.0f` | `1.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_OBSERVER_MAX_STEP_S` | `0.020f` | `0.020f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RECOVERY_HOLD_S` | `0.1f` | `0.1f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_RISE_RATE_W_PER_S` | `600.0f` | `600.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `W/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S` | `20.0f` | `20.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `J/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_buffer_budget；阶段0～3` |
| `CHASSIS_POWER_DEMAND_MARGIN_W` | `5.0f` | `5.0f` | `modules/algorithm/chassis_energy_config.h` | `modules/algorithm/chassis_energy_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `CHASSIS_SPIN_FIXED_WZ` | `4000.0f` | `4000.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_MIN_WZ` | `1000.0f` | `1000.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_MAX_WZ` | `6000.0f` | `6000.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_POWER_GAIN` | `40.0f` | `40.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `命令尺度/(W·s)` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_RISE_RATE` | `400.0f` | `400.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_FALL_RATE` | `2000.0f` | `2000.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_POWER_LPF_TAU_S` | `0.1f` | `0.1f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_POWER_DEADBAND_W` | `1.0f` | `1.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SETTLE_TIME_S` | `0.2f` | `0.2f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_TRACKING_ERROR_RPM` | `500.0f` | `500.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `rpm` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_BUDGET_DROP_RESET_W` | `1.0f` | `1.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `W` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_CENTER_WZ` | `800.0f` | `800.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_MIN_WZ` | `200.0f` | `200.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_AMPLITUDE_WZ` | `200.0f` | `200.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_PERIOD_S` | `4.0f` | `4.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_RAMP_S` | `1.0f` | `1.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_RISE_RATE` | `400.0f` | `400.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_FALL_RATE` | `2000.0f` | `2000.0f` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `原Wz命令尺度/s` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `CHASSIS_SPIN_SINE_DIRECTION` | `1` | `1` | `modules/algorithm/chassis_spin_config.h` | `modules/algorithm/chassis_spin_config.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `USB_TELEMETRY_PERIOD_MS` | `20U` | `20U` | `modules/telemetry/usb_telemetry_config.h` | `modules/telemetry/usb_telemetry_config.h` | `ms` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `USB_TELEMETRY_HEADER_PERIOD_MS` | `1000U` | `1000U` | `modules/telemetry/usb_telemetry_config.h` | `modules/telemetry/usb_telemetry_config.h` | `ms` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `USB_TELEMETRY_BUFFER_SIZE` | `2048U` | `2048U` | `modules/telemetry/usb_telemetry_config.h` | `modules/telemetry/usb_telemetry_config.h` | `字节` | `保留输入数值与功能，构建定义优先` | `对应功能启用，范围/互斥检查通过` | `USB遥测/Node回归` |
| `DISABLE_LOG_SYSTEM` | `构建定义1（头默认0）` | `默认0，构建可设1` | `Makefile、bsp/log/bsp_log.h` | `bsp/log/bsp_log.h、config/atype-options.txt` | `布尔` | `恢复分级日志可见性，保留关闭入口` | `只控制分级宏；原始写接口仍按LOG_OUTPUT输出` | `tests/bsp_log/uart_log` |
| `BOARD_REMOTE_UART` | `无` | `(&huart1)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_VISION_UART` | `无` | `(&huart3)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_VTX_UART` | `无` | `(&huart8)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_REFEREE_UART` | `无` | `(&huart6)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_LOG_UART` | `无` | `(&huart2)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_SUPERCAP_CAN` | `无` | `(&hcan1)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `BOARD_POWER_METER_CAN` | `无` | `(&hcan1)` | `无` | `Inc/board_config.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `DEVICE_USART_CNT` | `3` | `6` | `bsp/usart/bsp_usart.h` | `bsp/usart/bsp_usart.h` | `无量纲/选择值` | `按已确认 A 板硬件或整车默认行为迁移` | `对应功能启用，范围/互斥检查通过` | `配置矩阵/主机回归` |
| `TUNING_PID_OVERRIDE` | `1` | `0` | `modules/telemetry/tuning_overrides.h` | `modules/telemetry/tuning_overrides.h` | `无量纲/选择值` | `拒绝旧 0.75 覆盖实车 Kp=4.5` | `开关启用、模式及范围匹配；有效 Flash 优先` | `配置矩阵/主机回归` |
| `TUNING_POWER_OVERRIDE` | `0` | `0` | `modules/telemetry/tuning_overrides.h` | `modules/telemetry/tuning_overrides.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `开关启用、模式及范围匹配；有效 Flash 优先` | `配置矩阵/主机回归` |
| `TUNING_SPIN_OVERRIDE` | `0` | `0` | `modules/telemetry/tuning_overrides.h` | `modules/telemetry/tuning_overrides.h` | `无量纲/选择值` | `保留输入数值与功能，构建定义优先` | `开关启用、模式及范围匹配；有效 Flash 优先` | `tests/chassis_spin；fixed/adaptive/sine 固件` |
| `UART1_USE` | `未正确定义；输入误写UART1_USEUART1_USE_REFEREE` | `已废止；显式传入会报错` | `Inc/board_config.h` | `Inc/board_config.h、Make/CMake检查` | `旧选择值` | `USART1固定DBUS，裁判与日志分开控制` | `改用REFEREE_ENABLE和LOG_OUTPUT` | `tests/atype/test_board.py` |
| `UART1_USE_REFEREE` | `0` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `UART1_USE_LOG` | `1` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `UART1_USE_LOG_RTT` | `2` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `UART1_REFEREE_ENABLED` | `(UART1_USE == UART1_USE_REFEREE)` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `UART1_LOG_ENABLED` | `(UART1_USE != UART1_USE_REFEREE)` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |
| `UART1_RTT_ENABLED` | `(UART1_USE != UART1_USE_LOG)` | `移除` | `Inc/board_config.h` | `Make/CMake 或移除` | `无量纲/选择值` | `USART1 固定 DBUS，旧构建选择报错` | `对应功能启用，范围/互斥检查通过` | `tests/atype/test_board.py；可选配置` |

## 源码与存储迁移

| 项目 | 旧值 → 新值 | 单位与原因 | 验证 |
| --- | --- | --- | --- |
| MCU/启动/链接/调试 | F407 → F427IIH6 | A 板设备头、向量、SVD、12 MHz HSE | Make/CMake、ELF 向量符号 |
| Flash/RAM/CCM | 768/128/64 → 1792/192/64 | KiB；参数日志改扇区 22/23 | ELF 段边界、BSP 写入边界测试 |
| schema | 1 → 2 | 拒绝旧 C 板 PID/功控/自旋记录 | CRC 正确旧记录拒绝与逐字掉电注入 |
| 电机/整车/INS/守护/USB | 1/5/1/10/20 → 2/5/1/10/20 | ms；实际控制 dt 仍为实测 | 整定、功控抖动测试与配置矩阵 |
| 底盘 PID Kp | 0.75 → 4.5 | 速度误差 °/s，恢复参考初值；其他功控输入数值保留 | 四轮参考 / CAN 分组回归 |
| 热量预留 | 1 → 7 | 发；修正参考源码延迟反馈下的超限 | 3200 组延迟与相位仿真 |
| PWM 适配 | 0.1 ms/50% → 0.0001 s/0.5 | BSP 语义保持秒、比例 | tests/atype/test_pwm_units.c |
| 轮速边界 | 解算原参考 aps → target_rpm=aps/6 → PID aps=rpm×6 | 禁止六倍放大 | 运动学纯函数与功控发布回归 |
| FreeRTOS heap | 20000 → 65536 | 字节；第一套输入 20000，参考为 30000；完整任务与遥测栈预算 | 链接 RAM 检查；运行栈余量待硬件 |
| 通用驱动 | SPI1/2 假设 → SPI1/5；I²C 从机容量修正 | 保留调用 API，修正 A 板绑定和容量 | 可选固件强制链接；实物待验 |

协议常量与寄存器定义保持常量；未把它们作为任意构建开关。完整初值、定义位置和条件分支见 [配置索引](configuration-index.md)。
