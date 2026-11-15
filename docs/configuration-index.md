# 完整宏与初始化参数索引

更新日期：2026-11-14。当前目标为 STM32F427 A 板。配套说明：[配置指南](configuration-guide.md)、[宏迁移表](atype-macro-migration.md)。

由 `scripts/generate-configuration-index.py` 扫描 397 个项目 C/H 文件，记录 4501 条预处理、配置初始化和 HAL/NVIC 项。预处理条目包含互斥分支与保护宏，不能将所有条目同时理解为有效配置。可从构建命令覆盖的选项见 `config/99_build/atype-options.txt`；普通源码 PID/功率模型初值也列在所属文件下。

`modules/imu/ins_task.c`、`BMI088driver.c`、`BMI088Middleware.c` 是未链接的旧 C 板实现；当前 INS 只链接 MPU6500 和公共数学接口。便携副本有独立 API，未加入固件链接。

## application/board/board_analog_key.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/board/board_analog_key.h#L1) | 预处理 | `#ifndef BOARD_ANALOG_KEY_H` |
| [2](../application/board/board_analog_key.h#L2) | 预处理 | `#define BOARD_ANALOG_KEY_H` |
| [6](../application/board/board_analog_key.h#L6) | 预处理 | `#endif` |

## application/board/board_chassis_force.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/board/board_chassis_force.h#L1) | 预处理 | `#ifndef BOARD_CHASSIS_FORCE_H` |
| [2](../application/board/board_chassis_force.h#L2) | 预处理 | `#define BOARD_CHASSIS_FORCE_H` |
| [6](../application/board/board_chassis_force.h#L6) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [13](../application/board/board_chassis_force.h#L13) | 预处理 | `#endif` |
| [14](../application/board/board_chassis_force.h#L14) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [15](../application/board/board_chassis_force.h#L15) | 预处理 | `#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_OMNI_X` |
| [17](../application/board/board_chassis_force.h#L17) | 预处理 | `#endif` |
| [18](../application/board/board_chassis_force.h#L18) | 预处理 | `#if CHASSIS_POWER_CONTROL_STAGE != 3` |
| [20](../application/board/board_chassis_force.h#L20) | 预处理 | `#endif` |
| [21](../application/board/board_chassis_force.h#L21) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE && !POWER_METER_ENABLE` |
| [23](../application/board/board_chassis_force.h#L23) | 预处理 | `#endif` |
| [24](../application/board/board_chassis_force.h#L24) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP && !CHASSIS_BUFFER_POWER_ENABLE` |
| [26](../application/board/board_chassis_force.h#L26) | 预处理 | `#endif` |
| [27](../application/board/board_chassis_force.h#L27) | 预处理 | `#endif` |
| [28](../application/board/board_chassis_force.h#L28) | 预处理 | `#endif` |

## application/board/board_devices.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [17](../application/board/board_devices.c#L17) | 配置初值 | `const PWM_Init_Config_s heater={.htim=&htim3,.channel=TIM_CHANNEL_2,` |
| [18](../application/board/board_devices.c#L18) | 配置初值 | `.period=.0001f,.dutyratio=.5f};` |
| [20](../application/board/board_devices.c#L20) | 配置初值 | `const MPU6500_Init_Config_s imu={.spi_config={.spi_handle=&hspi5,` |
| [21](../application/board/board_devices.c#L21) | 配置初值 | `.GPIOx=GPIOF,.cs_pin=GPIO_PIN_6,.cs_mode=SPI_CS_GPIO,.spi_work_mode=SPI_BLOCK_MODE},` |
| [22](../application/board/board_devices.c#L22) | 配置初值 | `.timeout_us=500};` |
| [26](../application/board/board_devices.c#L26) | 配置初值 | `const INSServiceConfig_s service={.sample=SampleIMU,.heat=HeatIMU,.context=board_imu};` |

## application/board/board_devices.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/board/board_devices.h#L1) | 预处理 | `#ifndef BOARD_DEVICES_H` |
| [2](../application/board/board_devices.h#L2) | 预处理 | `#define BOARD_DEVICES_H` |
| [5](../application/board/board_devices.h#L5) | 预处理 | `#endif` |

## application/board/board_force_calibration.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/board/board_force_calibration.h#L1) | 预处理 | `#ifndef BOARD_FORCE_CALIBRATION_H` |
| [2](../application/board/board_force_calibration.h#L2) | 预处理 | `#define BOARD_FORCE_CALIBRATION_H` |
| [4](../application/board/board_force_calibration.h#L4) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [7](../application/board/board_force_calibration.h#L7) | 预处理 | `#if !defined(ONE_BOARD)` |
| [9](../application/board/board_force_calibration.h#L9) | 预处理 | `#endif` |
| [10](../application/board/board_force_calibration.h#L10) | 预处理 | `#if CHASSIS_CONTROL_MODE != 1` |
| [12](../application/board/board_force_calibration.h#L12) | 预处理 | `#endif` |
| [13](../application/board/board_force_calibration.h#L13) | 预处理 | `#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_OMNI_X` |
| [15](../application/board/board_force_calibration.h#L15) | 预处理 | `#endif` |
| [16](../application/board/board_force_calibration.h#L16) | 预处理 | `#if CHASSIS_POWER_CONTROL_STAGE != 3` |
| [18](../application/board/board_force_calibration.h#L18) | 预处理 | `#endif` |
| [19](../application/board/board_force_calibration.h#L19) | 预处理 | `#if !ATYPE_BENCH_MODE` |
| [21](../application/board/board_force_calibration.h#L21) | 预处理 | `#endif` |
| [22](../application/board/board_force_calibration.h#L22) | 预处理 | `#if !USB_TELEMETRY_ENABLE` |
| [24](../application/board/board_force_calibration.h#L24) | 预处理 | `#endif` |
| [25](../application/board/board_force_calibration.h#L25) | 预处理 | `#if !POWER_METER_ENABLE` |
| [27](../application/board/board_force_calibration.h#L27) | 预处理 | `#endif` |
| [28](../application/board/board_force_calibration.h#L28) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE != 0` |
| [30](../application/board/board_force_calibration.h#L30) | 预处理 | `#endif` |
| [31](../application/board/board_force_calibration.h#L31) | 预处理 | `#endif` |
| [33](../application/board/board_force_calibration.h#L33) | 预处理 | `#endif` |

## application/board/board_optional.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../application/board/board_optional.c#L13) | 预处理 | `#if BOARD_AUX_UART2_DEVICE &#124;&#124; BOARD_AUX_UART7_DEVICE` |
| [20](../application/board/board_optional.c#L20) | 配置初值 | `Servo_Init_Config_s config={.servo_type=Bus_Servo,._handle=uart,.servo_id=1};` |
| [26](../application/board/board_optional.c#L26) | 预处理 | `#endif` |
| [30](../application/board/board_optional.c#L30) | 预处理 | `#if BOARD_AUX_UART2_DEVICE` |
| [32](../application/board/board_optional.c#L32) | 预处理 | `#endif` |
| [33](../application/board/board_optional.c#L33) | 预处理 | `#if BOARD_AUX_UART7_DEVICE` |
| [35](../application/board/board_optional.c#L35) | 预处理 | `#endif` |
| [36](../application/board/board_optional.c#L36) | 预处理 | `#if BOARD_OLED_ENABLE` |
| [37](../application/board/board_optional.c#L37) | 配置初值 | `OLED_Init_Config_s oled={.read_button=BoardAnalogKeyRead};` |
| [38](../application/board/board_optional.c#L38) | 预处理 | `#if OLED_USE_I2C` |
| [41](../application/board/board_optional.c#L41) | 预处理 | `#else` |
| [46](../application/board/board_optional.c#L46) | 预处理 | `#endif` |
| [48](../application/board/board_optional.c#L48) | 预处理 | `#endif` |
| [49](../application/board/board_optional.c#L49) | 预处理 | `#if BOARD_KEY_ENABLE` |
| [50](../application/board/board_optional.c#L50) | 配置初值 | `Key_Config_s key={.key_gpio_config={.GPIOx=KEY_GPIO_Port,.GPIO_Pin=KEY_Pin,` |
| [51](../application/board/board_optional.c#L51) | 配置初值 | `.exti_mode=GPIO_EXTI_MODE_RISING}};` |
| [53](../application/board/board_optional.c#L53) | 预处理 | `#endif` |

## application/board/board_optional.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/board/board_optional.h#L1) | 预处理 | `#ifndef BOARD_OPTIONAL_H` |
| [2](../application/board/board_optional.h#L2) | 预处理 | `#define BOARD_OPTIONAL_H` |
| [4](../application/board/board_optional.h#L4) | 预处理 | `#endif` |

## application/chassis/chassis.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [33](../application/chassis/chassis.c#L33) | 预处理 | `#if POWER_METER_ENABLE` |
| [35](../application/chassis/chassis.c#L35) | 预处理 | `#endif` |
| [50](../application/chassis/chassis.c#L50) | 预处理 | `#define HALF_WHEEL_BASE (WHEEL_BASE / 2.0f)     // 半轴距` |
| [51](../application/chassis/chassis.c#L51) | 预处理 | `#define HALF_TRACK_WIDTH (TRACK_WIDTH / 2.0f)   // 半轮距` |
| [52](../application/chassis/chassis.c#L52) | 预处理 | `#define PERIMETER_WHEEL (RADIUS_WHEEL * 2 * PI) // 轮子周长` |
| [90](../application/chassis/chassis.c#L90) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [94](../application/chassis/chassis.c#L94) | 预处理 | `#endif` |
| [102](../application/chassis/chassis.c#L102) | 预处理 | `#define LF_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH + CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE - CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)` |
| [103](../application/chassis/chassis.c#L103) | 预处理 | `#define RF_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH - CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE - CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)` |
| [104](../application/chassis/chassis.c#L104) | 预处理 | `#define LB_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH + CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE + CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)` |
| [105](../application/chassis/chassis.c#L105) | 预处理 | `#define RB_ROTATION_LEVER_SUM ((HALF_TRACK_WIDTH - CENTER_GIMBAL_OFFSET_X + HALF_WHEEL_BASE + CENTER_GIMBAL_OFFSET_Y) * DEGREE_2_RAD)` |
| [125](../application/chassis/chassis.c#L125) | 预处理 | `#if CHASSIS_DRIVE_TYPE == CHASSIS_DRIVE_OMNI_X` |
| [133](../application/chassis/chassis.c#L133) | 预处理 | `#else` |
| [135](../application/chassis/chassis.c#L135) | 预处理 | `#endif` |
| [169](../application/chassis/chassis.c#L169) | 配置初值 | `static const ChassisVelocityConfig_s velocity_config={` |
| [170](../application/chassis/chassis.c#L170) | 配置初值 | `.omni_x=CHASSIS_DRIVE_TYPE==CHASSIS_DRIVE_OMNI_X,` |
| [171](../application/chassis/chassis.c#L171) | 配置初值 | `.wheel_radius_m=RADIUS_WHEEL*.001f, .reduction_ratio=REDUCTION_RATIO_WHEEL,` |
| [172](../application/chassis/chassis.c#L172) | 配置初值 | `.wheel_base_m=WHEEL_BASE*.001f, .track_width_m=TRACK_WIDTH*.001f,` |
| [173](../application/chassis/chassis.c#L173) | 配置初值 | `.direction={OMNI_X_LF_DIRECTION,OMNI_X_RF_DIRECTION,OMNI_X_LB_DIRECTION,OMNI_X_RB_DIRECTION}};` |
| [175](../application/chassis/chassis.c#L175) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [214](../application/chassis/chassis.c#L214) | 预处理 | `#endif` |
| [258](../application/chassis/chassis.c#L258) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP` |
| [260](../application/chassis/chassis.c#L260) | 预处理 | `#endif` |
| [261](../application/chassis/chassis.c#L261) | 预处理 | `#if POWER_METER_ENABLE` |
| [263](../application/chassis/chassis.c#L263) | 预处理 | `#endif` |
| [266](../application/chassis/chassis.c#L266) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [280](../application/chassis/chassis.c#L280) | 预处理 | `#endif` |
| [290](../application/chassis/chassis.c#L290) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED && POWER_METER_ENABLE` |
| [296](../application/chassis/chassis.c#L296) | 预处理 | `#if CHASSIS_POWER_CONTROL_STAGE >= 1` |
| [301](../application/chassis/chassis.c#L301) | 预处理 | `#endif` |
| [303](../application/chassis/chassis.c#L303) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [314](../application/chassis/chassis.c#L314) | 预处理 | `#if CHASSIS_POWER_CONTROL_STAGE >= 2` |
| [317](../application/chassis/chassis.c#L317) | 预处理 | `#endif` |
| [343](../application/chassis/chassis.c#L343) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [356](../application/chassis/chassis.c#L356) | 预处理 | `#endif` |
| [357](../application/chassis/chassis.c#L357) | 预处理 | `#endif` |
| [358](../application/chassis/chassis.c#L358) | 预处理 | `#endif` |
| [359](../application/chassis/chassis.c#L359) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [363](../application/chassis/chassis.c#L363) | 预处理 | `#else` |
| [374](../application/chassis/chassis.c#L374) | 预处理 | `#endif` |
| [375](../application/chassis/chassis.c#L375) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [379](../application/chassis/chassis.c#L379) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [383](../application/chassis/chassis.c#L383) | 预处理 | `#endif` |
| [384](../application/chassis/chassis.c#L384) | 预处理 | `#endif` |
| [406](../application/chassis/chassis.c#L406) | 预处理 | `#ifdef ONE_BOARD` |
| [408](../application/chassis/chassis.c#L408) | 预处理 | `#endif` |
| [409](../application/chassis/chassis.c#L409) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [423](../application/chassis/chassis.c#L423) | 预处理 | `#endif // CHASSIS_BOARD` |
| [429](../application/chassis/chassis.c#L429) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [433](../application/chassis/chassis.c#L433) | 预处理 | `#else` |
| [435](../application/chassis/chassis.c#L435) | 预处理 | `#endif` |
| [448](../application/chassis/chassis.c#L448) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [450](../application/chassis/chassis.c#L450) | 预处理 | `#endif` |
| [480](../application/chassis/chassis.c#L480) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [482](../application/chassis/chassis.c#L482) | 预处理 | `#endif` |
| [502](../application/chassis/chassis.c#L502) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [512](../application/chassis/chassis.c#L512) | 预处理 | `#endif` |
| [519](../application/chassis/chassis.c#L519) | 预处理 | `#ifdef ONE_BOARD` |
| [521](../application/chassis/chassis.c#L521) | 预处理 | `#endif` |
| [522](../application/chassis/chassis.c#L522) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [527](../application/chassis/chassis.c#L527) | 预处理 | `#endif // CHASSIS_BOARD` |

## application/chassis/chassis.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/chassis/chassis.h#L1) | 预处理 | `#ifndef CHASSIS_H` |
| [2](../application/chassis/chassis.h#L2) | 预处理 | `#define CHASSIS_H` |
| [51](../application/chassis/chassis.h#L51) | 预处理 | `#endif // CHASSIS_H` |

## application/chassis/chassis_force_app.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [6](../application/chassis/chassis_force_app.c#L6) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [8](../application/chassis/chassis_force_app.c#L8) | 预处理 | `#endif` |
| [23](../application/chassis/chassis_force_app.c#L23) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [26](../application/chassis/chassis_force_app.c#L26) | 预处理 | `#else` |
| [29](../application/chassis/chassis_force_app.c#L29) | 预处理 | `#endif` |
| [57](../application/chassis/chassis_force_app.c#L57) | 配置初值 | `Motor_Init_Config_s m={.can_init_config.can_handle=&hcan1,.motor_type=M3508};` |
| [64](../application/chassis/chassis_force_app.c#L64) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [66](../application/chassis/chassis_force_app.c#L66) | 预处理 | `#endif` |
| [70](../application/chassis/chassis_force_app.c#L70) | 预处理 | `#if REFEREE_ENABLE` |
| [72](../application/chassis/chassis_force_app.c#L72) | 预处理 | `#else` |
| [74](../application/chassis/chassis_force_app.c#L74) | 预处理 | `#endif` |
| [75](../application/chassis/chassis_force_app.c#L75) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP` |
| [76](../application/chassis/chassis_force_app.c#L76) | 配置初值 | `SuperCap_Init_Config_s cap={.can_config={.can_handle=BOARD_SUPERCAP_CAN,.tx_id=SUPERCAP_COMMAND_CAN_ID,` |
| [77](../application/chassis/chassis_force_app.c#L77) | 配置初值 | `.rx_id=SUPERCAP_STATUS_CAN_ID},.offline_reload_count=20,.input_power_feedback_enabled=true};` |
| [79](../application/chassis/chassis_force_app.c#L79) | 预处理 | `#endif` |
| [80](../application/chassis/chassis_force_app.c#L80) | 预处理 | `#if POWER_METER_ENABLE` |
| [81](../application/chassis/chassis_force_app.c#L81) | 配置初值 | `PowerMeter_Init_Config_s meter={.can_config={.can_handle=BOARD_POWER_METER_CAN,.tx_id=POWER_METER_STATUS_CAN_ID,` |
| [82](../application/chassis/chassis_force_app.c#L82) | 配置初值 | `.rx_id=POWER_METER_STATUS_CAN_ID},.offline_reload_count=10};` |
| [84](../application/chassis/chassis_force_app.c#L84) | 预处理 | `#endif` |
| [85](../application/chassis/chassis_force_app.c#L85) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [86](../application/chassis/chassis_force_app.c#L86) | 配置初值 | `CANComm_Init_Config_s comm={.can_config={.can_handle=&hcan2,.tx_id=0x311,.rx_id=0x312},` |
| [87](../application/chassis/chassis_force_app.c#L87) | 配置初值 | `.recv_data_len=44,.send_data_len=sizeof(Chassis_Upload_Data_s),.daemon_count=10};` |
| [89](../application/chassis/chassis_force_app.c#L89) | 预处理 | `#else` |
| [92](../application/chassis/chassis_force_app.c#L92) | 预处理 | `#endif` |
| [101](../application/chassis/chassis_force_app.c#L101) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [104](../application/chassis/chassis_force_app.c#L104) | 预处理 | `#else` |
| [108](../application/chassis/chassis_force_app.c#L108) | 预处理 | `#endif` |
| [115](../application/chassis/chassis_force_app.c#L115) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [117](../application/chassis/chassis_force_app.c#L117) | 预处理 | `#else` |
| [119](../application/chassis/chassis_force_app.c#L119) | 预处理 | `#endif` |
| [123](../application/chassis/chassis_force_app.c#L123) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [130](../application/chassis/chassis_force_app.c#L130) | 预处理 | `#endif` |
| [143](../application/chassis/chassis_force_app.c#L143) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [148](../application/chassis/chassis_force_app.c#L148) | 预处理 | `#endif` |
| [150](../application/chassis/chassis_force_app.c#L150) | 预处理 | `#if POWER_METER_ENABLE` |
| [152](../application/chassis/chassis_force_app.c#L152) | 预处理 | `#endif` |
| [153](../application/chassis/chassis_force_app.c#L153) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP` |
| [155](../application/chassis/chassis_force_app.c#L155) | 预处理 | `#endif` |
| [166](../application/chassis/chassis_force_app.c#L166) | 预处理 | `#if !CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [175](../application/chassis/chassis_force_app.c#L175) | 预处理 | `#endif` |
| [176](../application/chassis/chassis_force_app.c#L176) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [184](../application/chassis/chassis_force_app.c#L184) | 预处理 | `#else` |
| [186](../application/chassis/chassis_force_app.c#L186) | 预处理 | `#endif` |
| [202](../application/chassis/chassis_force_app.c#L202) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [205](../application/chassis/chassis_force_app.c#L205) | 预处理 | `#else` |
| [207](../application/chassis/chassis_force_app.c#L207) | 预处理 | `#endif` |

## application/chassis/chassis_init.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [14](../application/chassis/chassis_init.c#L14) | 预处理 | `#if POWER_METER_ENABLE` |
| [16](../application/chassis/chassis_init.c#L16) | 预处理 | `#endif` |
| [18](../application/chassis/chassis_init.c#L18) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [22](../application/chassis/chassis_init.c#L22) | 预处理 | `#endif` |
| [23](../application/chassis/chassis_init.c#L23) | 预处理 | `#ifdef ONE_BOARD` |
| [26](../application/chassis/chassis_init.c#L26) | 预处理 | `#endif` |
| [40](../application/chassis/chassis_init.c#L40) | 配置初值 | `Motor_Init_Config_s chassis_motor_config = {` |
| [41](../application/chassis/chassis_init.c#L41) | 配置初值 | `.can_init_config.can_handle = &hcan1,` |
| [42](../application/chassis/chassis_init.c#L42) | 配置初值 | `.controller_param_init_config = {` |
| [43](../application/chassis/chassis_init.c#L43) | 配置初值 | `.speed_PID = pid_chassis_speed_config,` |
| [44](../application/chassis/chassis_init.c#L44) | 配置初值 | `},` |
| [45](../application/chassis/chassis_init.c#L45) | 配置初值 | `.controller_setting_init_config = {` |
| [46](../application/chassis/chassis_init.c#L46) | 配置初值 | `.angle_feedback_source = MOTOR_FEED,` |
| [47](../application/chassis/chassis_init.c#L47) | 配置初值 | `.speed_feedback_source = MOTOR_FEED,` |
| [48](../application/chassis/chassis_init.c#L48) | 配置初值 | `.outer_loop_type = SPEED_LOOP,` |
| [49](../application/chassis/chassis_init.c#L49) | 配置初值 | `.close_loop_type = SPEED_LOOP,` |
| [50](../application/chassis/chassis_init.c#L50) | 配置初值 | `},` |
| [51](../application/chassis/chassis_init.c#L51) | 配置初值 | `.motor_type = M3508,` |
| [74](../application/chassis/chassis_init.c#L74) | 配置初值 | `const DJIPowerAdapterConfig_s adapter_config={.motors={motor_lf,motor_rf,motor_lb,motor_rb},` |
| [75](../application/chassis/chassis_init.c#L75) | 配置初值 | `.count=4,.max_current_command=power_config.algorithm.max_current_command};` |
| [86](../application/chassis/chassis_init.c#L86) | 预处理 | `#if REFEREE_ENABLE` |
| [88](../application/chassis/chassis_init.c#L88) | 预处理 | `#endif` |
| [90](../application/chassis/chassis_init.c#L90) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE == CHASSIS_POWER_SOURCE_SUPERCAP` |
| [91](../application/chassis/chassis_init.c#L91) | 配置初值 | `SuperCap_Init_Config_s cap_conf = {` |
| [92](../application/chassis/chassis_init.c#L92) | 配置初值 | `.can_config = {` |
| [93](../application/chassis/chassis_init.c#L93) | 配置初值 | `.can_handle = BOARD_SUPERCAP_CAN,` |
| [94](../application/chassis/chassis_init.c#L94) | 配置初值 | `.tx_id = SUPERCAP_COMMAND_CAN_ID,` |
| [95](../application/chassis/chassis_init.c#L95) | 配置初值 | `.rx_id = SUPERCAP_STATUS_CAN_ID,` |
| [96](../application/chassis/chassis_init.c#L96) | 配置初值 | `},` |
| [97](../application/chassis/chassis_init.c#L97) | 配置初值 | `.offline_reload_count = 20U,` |
| [98](../application/chassis/chassis_init.c#L98) | 配置初值 | `.input_power_feedback_enabled = CHASSIS_BUFFER_POWER_ENABLE != 0,` |
| [101](../application/chassis/chassis_init.c#L101) | 预处理 | `#endif` |
| [102](../application/chassis/chassis_init.c#L102) | 预处理 | `#if POWER_METER_ENABLE` |
| [103](../application/chassis/chassis_init.c#L103) | 配置初值 | `const PowerMeter_Init_Config_s power_meter_config = {` |
| [104](../application/chassis/chassis_init.c#L104) | 配置初值 | `.can_config = {` |
| [105](../application/chassis/chassis_init.c#L105) | 配置初值 | `.can_handle = BOARD_POWER_METER_CAN,` |
| [106](../application/chassis/chassis_init.c#L106) | 配置初值 | `.tx_id = POWER_METER_STATUS_CAN_ID,` |
| [107](../application/chassis/chassis_init.c#L107) | 配置初值 | `.rx_id = POWER_METER_STATUS_CAN_ID,` |
| [108](../application/chassis/chassis_init.c#L108) | 配置初值 | `},` |
| [109](../application/chassis/chassis_init.c#L109) | 配置初值 | `.offline_reload_count = 10U,` |
| [112](../application/chassis/chassis_init.c#L112) | 预处理 | `#endif` |
| [116](../application/chassis/chassis_init.c#L116) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [119](../application/chassis/chassis_init.c#L119) | 配置初值 | `CANComm_Init_Config_s comm_conf = {` |
| [120](../application/chassis/chassis_init.c#L120) | 配置初值 | `.can_config = {` |
| [121](../application/chassis/chassis_init.c#L121) | 配置初值 | `.can_handle = &hcan2,` |
| [122](../application/chassis/chassis_init.c#L122) | 配置初值 | `.tx_id = 0x311,` |
| [123](../application/chassis/chassis_init.c#L123) | 配置初值 | `.rx_id = 0x312,` |
| [124](../application/chassis/chassis_init.c#L124) | 配置初值 | `},` |
| [125](../application/chassis/chassis_init.c#L125) | 配置初值 | `.recv_data_len = sizeof(Chassis_Ctrl_Cmd_s),` |
| [126](../application/chassis/chassis_init.c#L126) | 配置初值 | `.send_data_len = sizeof(Chassis_Upload_Data_s),` |
| [130](../application/chassis/chassis_init.c#L130) | 预处理 | `#endif                                          // CHASSIS_BOARD` |
| [132](../application/chassis/chassis_init.c#L132) | 预处理 | `#ifdef ONE_BOARD // 单板控制整车,则通过pubsub来传递消息` |
| [135](../application/chassis/chassis_init.c#L135) | 预处理 | `#endif // ONE_BOARD` |

## application/chassis/chassis_internal.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/chassis/chassis_internal.h#L1) | 预处理 | `#ifndef CHASSIS_INTERNAL_H` |
| [2](../application/chassis/chassis_internal.h#L2) | 预处理 | `#define CHASSIS_INTERNAL_H` |
| [20](../application/chassis/chassis_internal.h#L20) | 预处理 | `#ifdef ONE_BOARD` |
| [23](../application/chassis/chassis_internal.h#L23) | 预处理 | `#endif` |
| [24](../application/chassis/chassis_internal.h#L24) | 预处理 | `#ifdef CHASSIS_BOARD` |
| [27](../application/chassis/chassis_internal.h#L27) | 预处理 | `#endif` |
| [32](../application/chassis/chassis_internal.h#L32) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [34](../application/chassis/chassis_internal.h#L34) | 预处理 | `#endif` |
| [42](../application/chassis/chassis_internal.h#L42) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [44](../application/chassis/chassis_internal.h#L44) | 预处理 | `#endif` |
| [48](../application/chassis/chassis_internal.h#L48) | 预处理 | `#endif` |

## application/chassis/chassis_motion.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/chassis/chassis_motion.h#L1) | 预处理 | `#ifndef CHASSIS_MOTION_H` |
| [2](../application/chassis/chassis_motion.h#L2) | 预处理 | `#define CHASSIS_MOTION_H` |
| [19](../application/chassis/chassis_motion.h#L19) | 预处理 | `#if CHASSIS_FOLLOW_MODE == 0` |
| [22](../application/chassis/chassis_motion.h#L22) | 预处理 | `#else` |
| [25](../application/chassis/chassis_motion.h#L25) | 预处理 | `#endif` |
| [41](../application/chassis/chassis_motion.h#L41) | 预处理 | `#endif` |

## application/chassis/chassis_tuning.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [35](../application/chassis/chassis_tuning.c#L35) | 配置初值 | `ChassisPowerOuterLoopConfig_s outer = {` |
| [36](../application/chassis/chassis_tuning.c#L36) | 配置初值 | `.kp=v[0], .ki=v[1], .measurement_lpf_tau_s=v[2],` |
| [37](../application/chassis/chassis_tuning.c#L37) | 配置初值 | `.activation_margin_w=v[3], .error_deadband_w=v[4],` |
| [38](../application/chassis/chassis_tuning.c#L38) | 配置初值 | `.max_positive_correction_w=v[5], .positive_slew_rate_w_per_s=v[6],` |
| [39](../application/chassis/chassis_tuning.c#L39) | 配置初值 | `.budget_drop_reset_w=v[7], .antiwindup_gain_per_s=v[8],` |
| [40](../application/chassis/chassis_tuning.c#L40) | 配置初值 | `.low_demand_delay_s=v[9], .low_demand_decay_w_per_s=v[10]};` |
| [67](../application/chassis/chassis_tuning.c#L67) | 配置初值 | `ChassisSpinConfig_s spin = {` |
| [68](../application/chassis/chassis_tuning.c#L68) | 配置初值 | `.fixed_wz=v[0], .min_wz=v[1], .max_wz=v[2], .gain_wz_per_w_s=v[3],` |
| [69](../application/chassis/chassis_tuning.c#L69) | 配置初值 | `.rise_wz_per_s=v[4], .fall_wz_per_s=v[5], .power_lpf_tau_s=v[6],` |
| [70](../application/chassis/chassis_tuning.c#L70) | 配置初值 | `.power_deadband_w=v[7], .settle_time_s=v[8], .budget_drop_reset_w=v[9],` |
| [71](../application/chassis/chassis_tuning.c#L71) | 配置初值 | `.sine_amplitude_wz=v[10], .sine_period_s=v[11], .sine_ramp_s=v[12],` |
| [72](../application/chassis/chassis_tuning.c#L72) | 配置初值 | `.sine_rise_wz_per_s=v[13], .sine_fall_wz_per_s=v[14],` |
| [73](../application/chassis/chassis_tuning.c#L73) | 配置初值 | `.sine_direction=(int8_t)v[15]};` |
| [82](../application/chassis/chassis_tuning.c#L82) | 预处理 | `#if CHASSIS_SPIN_DYNAMIC_ENABLED` |
| [87](../application/chassis/chassis_tuning.c#L87) | 预处理 | `#endif` |

## application/chassis/chassis_velocity_observer.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../application/chassis/chassis_velocity_observer.c#L3) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE != 0 && CHASSIS_VELOCITY_FUSION_ENABLE != 1` |
| [5](../application/chassis/chassis_velocity_observer.c#L5) | 预处理 | `#endif` |
| [6](../application/chassis/chassis_velocity_observer.c#L6) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED != 0 && CHASSIS_VELOCITY_FUSION_CALIBRATED != 1` |
| [8](../application/chassis/chassis_velocity_observer.c#L8) | 预处理 | `#endif` |
| [9](../application/chassis/chassis_velocity_observer.c#L9) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED && !CHASSIS_VELOCITY_FUSION_ENABLE` |
| [11](../application/chassis/chassis_velocity_observer.c#L11) | 预处理 | `#endif` |
| [13](../application/chassis/chassis_velocity_observer.c#L13) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [22](../application/chassis/chassis_velocity_observer.c#L22) | 预处理 | `#if !defined(ONE_BOARD) &#124;&#124; CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED` |
| [24](../application/chassis/chassis_velocity_observer.c#L24) | 预处理 | `#endif` |
| [45](../application/chassis/chassis_velocity_observer.c#L45) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED` |
| [53](../application/chassis/chassis_velocity_observer.c#L53) | 预处理 | `#define TWO_PI 6.2831853071795864769f` |
| [201](../application/chassis/chassis_velocity_observer.c#L201) | 预处理 | `#endif /* CHASSIS_VELOCITY_FUSION_CALIBRATED */` |
| [203](../application/chassis/chassis_velocity_observer.c#L203) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [241](../application/chassis/chassis_velocity_observer.c#L241) | 预处理 | `#endif` |
| [250](../application/chassis/chassis_velocity_observer.c#L250) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [252](../application/chassis/chassis_velocity_observer.c#L252) | 预处理 | `#endif` |
| [253](../application/chassis/chassis_velocity_observer.c#L253) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED` |
| [255](../application/chassis/chassis_velocity_observer.c#L255) | 预处理 | `#endif` |
| [263](../application/chassis/chassis_velocity_observer.c#L263) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED` |
| [265](../application/chassis/chassis_velocity_observer.c#L265) | 预处理 | `#endif` |
| [267](../application/chassis/chassis_velocity_observer.c#L267) | 预处理 | `#if !CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [269](../application/chassis/chassis_velocity_observer.c#L269) | 预处理 | `#endif` |
| [271](../application/chassis/chassis_velocity_observer.c#L271) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED` |
| [273](../application/chassis/chassis_velocity_observer.c#L273) | 预处理 | `#endif` |
| [275](../application/chassis/chassis_velocity_observer.c#L275) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [279](../application/chassis/chassis_velocity_observer.c#L279) | 预处理 | `#endif` |
| [282](../application/chassis/chassis_velocity_observer.c#L282) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [291](../application/chassis/chassis_velocity_observer.c#L291) | 预处理 | `#endif` |
| [320](../application/chassis/chassis_velocity_observer.c#L320) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_CALIBRATED` |
| [327](../application/chassis/chassis_velocity_observer.c#L327) | 预处理 | `#endif` |
| [328](../application/chassis/chassis_velocity_observer.c#L328) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [330](../application/chassis/chassis_velocity_observer.c#L330) | 预处理 | `#endif` |
| [335](../application/chassis/chassis_velocity_observer.c#L335) | 预处理 | `#endif /* CHASSIS_VELOCITY_FUSION_ENABLE */` |

## application/chassis/chassis_velocity_observer.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/chassis/chassis_velocity_observer.h#L1) | 预处理 | `#ifndef CHASSIS_VELOCITY_OBSERVER_H` |
| [2](../application/chassis/chassis_velocity_observer.h#L2) | 预处理 | `#define CHASSIS_VELOCITY_OBSERVER_H` |
| [52](../application/chassis/chassis_velocity_observer.h#L52) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [56](../application/chassis/chassis_velocity_observer.h#L56) | 预处理 | `#endif` |
| [58](../application/chassis/chassis_velocity_observer.h#L58) | 预处理 | `#endif` |

## application/cmd/robot_cmd.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../application/cmd/robot_cmd.c#L5) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [9](../application/cmd/robot_cmd.c#L9) | 预处理 | `#endif` |
| [30](../application/cmd/robot_cmd.c#L30) | 预处理 | `#define YAW_ALIGN_ANGLE (YAW_CHASSIS_ALIGN_ECD * ECD_ANGLE_COEF_DJI) // 对齐时的角度,0-360` |
| [31](../application/cmd/robot_cmd.c#L31) | 预处理 | `#define PTICH_HORIZON_ANGLE (PITCH_HORIZON_ECD * ECD_ANGLE_COEF_DJI) // pitch水平时电机的角度,0-360` |
| [34](../application/cmd/robot_cmd.c#L34) | 预处理 | `#ifdef GIMBAL_BOARD // 对双板的兼容,条件编译` |
| [38](../application/cmd/robot_cmd.c#L38) | 预处理 | `#endif` |
| [39](../application/cmd/robot_cmd.c#L39) | 预处理 | `#ifdef ONE_BOARD` |
| [42](../application/cmd/robot_cmd.c#L42) | 预处理 | `#endif                                 // ONE_BOARD` |
| [78](../application/cmd/robot_cmd.c#L78) | 配置初值 | `HeatControl_Init_Config_s heat_config = {` |
| [79](../application/cmd/robot_cmd.c#L79) | 配置初值 | `.bullet_heat = HEAT_CONTROL_BULLET_HEAT,` |
| [80](../application/cmd/robot_cmd.c#L80) | 配置初值 | `.max_shoot_rate = HEAT_CONTROL_DEFAULT_MAX_RATE,` |
| [84](../application/cmd/robot_cmd.c#L84) | 预处理 | `#ifdef ONE_BOARD // 双板兼容` |
| [85](../application/cmd/robot_cmd.c#L85) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [87](../application/cmd/robot_cmd.c#L87) | 预处理 | `#else` |
| [89](../application/cmd/robot_cmd.c#L89) | 预处理 | `#endif` |
| [91](../application/cmd/robot_cmd.c#L91) | 预处理 | `#endif // ONE_BOARD` |
| [92](../application/cmd/robot_cmd.c#L92) | 预处理 | `#ifdef GIMBAL_BOARD` |
| [93](../application/cmd/robot_cmd.c#L93) | 配置初值 | `CANComm_Init_Config_s comm_conf = {` |
| [94](../application/cmd/robot_cmd.c#L94) | 配置初值 | `.can_config = {` |
| [95](../application/cmd/robot_cmd.c#L95) | 配置初值 | `.can_handle = &hcan2,` |
| [96](../application/cmd/robot_cmd.c#L96) | 配置初值 | `.tx_id = 0x312,` |
| [97](../application/cmd/robot_cmd.c#L97) | 配置初值 | `.rx_id = 0x311,` |
| [98](../application/cmd/robot_cmd.c#L98) | 配置初值 | `},` |
| [99](../application/cmd/robot_cmd.c#L99) | 配置初值 | `.recv_data_len = sizeof(Chassis_Upload_Data_s),` |
| [100](../application/cmd/robot_cmd.c#L100) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [100](../application/cmd/robot_cmd.c#L100) | 配置初值 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [101](../application/cmd/robot_cmd.c#L101) | 配置初值 | `.send_data_len = CHASSIS_FORCE_WIRE_SIZE,` |
| [102](../application/cmd/robot_cmd.c#L102) | 预处理 | `#else` |
| [102](../application/cmd/robot_cmd.c#L102) | 配置初值 | `#else` |
| [103](../application/cmd/robot_cmd.c#L103) | 配置初值 | `.send_data_len = sizeof(Chassis_Ctrl_Cmd_s),` |
| [104](../application/cmd/robot_cmd.c#L104) | 预处理 | `#endif` |
| [104](../application/cmd/robot_cmd.c#L104) | 配置初值 | `#endif` |
| [109](../application/cmd/robot_cmd.c#L109) | 预处理 | `#endif // GIMBAL_BOARD` |
| [170](../application/cmd/robot_cmd.c#L170) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [173](../application/cmd/robot_cmd.c#L173) | 预处理 | `#else` |
| [176](../application/cmd/robot_cmd.c#L176) | 预处理 | `#endif` |
| [202](../application/cmd/robot_cmd.c#L202) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [206](../application/cmd/robot_cmd.c#L206) | 预处理 | `#else` |
| [209](../application/cmd/robot_cmd.c#L209) | 预处理 | `#endif` |
| [274](../application/cmd/robot_cmd.c#L274) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [276](../application/cmd/robot_cmd.c#L276) | 预处理 | `#endif` |
| [298](../application/cmd/robot_cmd.c#L298) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [302](../application/cmd/robot_cmd.c#L302) | 预处理 | `#else` |
| [305](../application/cmd/robot_cmd.c#L305) | 预处理 | `#endif` |
| [373](../application/cmd/robot_cmd.c#L373) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [375](../application/cmd/robot_cmd.c#L375) | 预处理 | `#else` |
| [379](../application/cmd/robot_cmd.c#L379) | 预处理 | `#endif` |
| [484](../application/cmd/robot_cmd.c#L484) | 预处理 | `#ifdef ONE_BOARD` |
| [486](../application/cmd/robot_cmd.c#L486) | 预处理 | `#endif // ONE_BOARD` |
| [487](../application/cmd/robot_cmd.c#L487) | 预处理 | `#ifdef GIMBAL_BOARD` |
| [498](../application/cmd/robot_cmd.c#L498) | 预处理 | `#endif // GIMBAL_BOARD` |
| [526](../application/cmd/robot_cmd.c#L526) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [538](../application/cmd/robot_cmd.c#L538) | 预处理 | `#endif` |
| [540](../application/cmd/robot_cmd.c#L540) | 预处理 | `#ifdef ONE_BOARD` |
| [541](../application/cmd/robot_cmd.c#L541) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [544](../application/cmd/robot_cmd.c#L544) | 预处理 | `#else` |
| [546](../application/cmd/robot_cmd.c#L546) | 预处理 | `#endif` |
| [547](../application/cmd/robot_cmd.c#L547) | 预处理 | `#endif // ONE_BOARD` |
| [548](../application/cmd/robot_cmd.c#L548) | 预处理 | `#ifdef GIMBAL_BOARD` |
| [552](../application/cmd/robot_cmd.c#L552) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [556](../application/cmd/robot_cmd.c#L556) | 预处理 | `#else` |
| [558](../application/cmd/robot_cmd.c#L558) | 预处理 | `#endif` |
| [559](../application/cmd/robot_cmd.c#L559) | 预处理 | `#endif // GIMBAL_BOARD` |

## application/cmd/robot_cmd.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/cmd/robot_cmd.h#L1) | 预处理 | `#ifndef ROBOT_CMD_H` |
| [2](../application/cmd/robot_cmd.h#L2) | 预处理 | `#define ROBOT_CMD_H` |
| [17](../application/cmd/robot_cmd.h#L17) | 预处理 | `#endif // !ROBOT_CMD_H` |

## application/gimbal/gimbal.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [22](../application/gimbal/gimbal.c#L22) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [38](../application/gimbal/gimbal.c#L38) | 预处理 | `#endif` |
| [44](../application/gimbal/gimbal.c#L44) | 配置初值 | `Motor_Init_Config_s yaw_config = {` |
| [45](../application/gimbal/gimbal.c#L45) | 配置初值 | `.can_init_config = {` |
| [46](../application/gimbal/gimbal.c#L46) | 配置初值 | `.can_handle = &hcan2,` |
| [47](../application/gimbal/gimbal.c#L47) | 配置初值 | `.tx_id = 1,` |
| [48](../application/gimbal/gimbal.c#L48) | 配置初值 | `},` |
| [49](../application/gimbal/gimbal.c#L49) | 配置初值 | `.controller_param_init_config = {` |
| [50](../application/gimbal/gimbal.c#L50) | 配置初值 | `.angle_PID = pid_gimbal_yaw_angle_config,` |
| [51](../application/gimbal/gimbal.c#L51) | 配置初值 | `.speed_PID = pid_gimbal_yaw_speed_config,` |
| [52](../application/gimbal/gimbal.c#L52) | 配置初值 | `.other_angle_feedback_ptr = &gimbal_IMU_data->YawTotalAngle,` |
| [53](../application/gimbal/gimbal.c#L53) | 配置初值 | `// 还需要增加角速度额外反馈指针,注意方向,ins_task.md中有c板的bodyframe坐标系说明` |
| [54](../application/gimbal/gimbal.c#L54) | 配置初值 | `.other_speed_feedback_ptr = &gimbal_IMU_data->Gyro[2],` |
| [55](../application/gimbal/gimbal.c#L55) | 配置初值 | `},` |
| [56](../application/gimbal/gimbal.c#L56) | 配置初值 | `.controller_setting_init_config = {` |
| [57](../application/gimbal/gimbal.c#L57) | 配置初值 | `.angle_feedback_source = OTHER_FEED,` |
| [58](../application/gimbal/gimbal.c#L58) | 配置初值 | `.speed_feedback_source = OTHER_FEED,` |
| [59](../application/gimbal/gimbal.c#L59) | 配置初值 | `.outer_loop_type = ANGLE_LOOP,` |
| [60](../application/gimbal/gimbal.c#L60) | 配置初值 | `.close_loop_type = ANGLE_LOOP &#124; SPEED_LOOP,` |
| [61](../application/gimbal/gimbal.c#L61) | 配置初值 | `.motor_reverse_flag = MOTOR_DIRECTION_NORMAL,` |
| [62](../application/gimbal/gimbal.c#L62) | 配置初值 | `},` |
| [63](../application/gimbal/gimbal.c#L63) | 配置初值 | `.motor_type = GM6020};` |
| [65](../application/gimbal/gimbal.c#L65) | 配置初值 | `Motor_Init_Config_s pitch_config = {` |
| [66](../application/gimbal/gimbal.c#L66) | 配置初值 | `.can_init_config = {` |
| [67](../application/gimbal/gimbal.c#L67) | 配置初值 | `.can_handle = &hcan2,` |
| [68](../application/gimbal/gimbal.c#L68) | 配置初值 | `.tx_id = 2,` |
| [69](../application/gimbal/gimbal.c#L69) | 配置初值 | `},` |
| [70](../application/gimbal/gimbal.c#L70) | 配置初值 | `.controller_param_init_config = {` |
| [71](../application/gimbal/gimbal.c#L71) | 配置初值 | `.angle_PID = pid_gimbal_pitch_angle_config,` |
| [72](../application/gimbal/gimbal.c#L72) | 配置初值 | `.speed_PID = pid_gimbal_pitch_speed_config,` |
| [73](../application/gimbal/gimbal.c#L73) | 配置初值 | `.other_angle_feedback_ptr = &gimbal_IMU_data->Pitch,` |
| [74](../application/gimbal/gimbal.c#L74) | 配置初值 | `// 还需要增加角速度额外反馈指针,注意方向,ins_task.md中有c板的bodyframe坐标系说明` |
| [75](../application/gimbal/gimbal.c#L75) | 配置初值 | `.other_speed_feedback_ptr = (&gimbal_IMU_data->Gyro[1]),` |
| [76](../application/gimbal/gimbal.c#L76) | 配置初值 | `},` |
| [77](../application/gimbal/gimbal.c#L77) | 配置初值 | `.controller_setting_init_config = {` |
| [78](../application/gimbal/gimbal.c#L78) | 配置初值 | `.angle_feedback_source = OTHER_FEED,` |
| [79](../application/gimbal/gimbal.c#L79) | 配置初值 | `.speed_feedback_source = OTHER_FEED,` |
| [80](../application/gimbal/gimbal.c#L80) | 配置初值 | `.outer_loop_type = ANGLE_LOOP,` |
| [81](../application/gimbal/gimbal.c#L81) | 配置初值 | `.close_loop_type = SPEED_LOOP &#124; ANGLE_LOOP,` |
| [82](../application/gimbal/gimbal.c#L82) | 配置初值 | `.motor_reverse_flag = MOTOR_DIRECTION_NORMAL,` |
| [83](../application/gimbal/gimbal.c#L83) | 配置初值 | `.feedback_reverse_flag = FEEDBACK_DIRECTION_REVERSE,` |
| [84](../application/gimbal/gimbal.c#L84) | 配置初值 | `},` |
| [85](../application/gimbal/gimbal.c#L85) | 配置初值 | `.motor_type = GM6020,` |

## application/gimbal/gimbal.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/gimbal/gimbal.h#L1) | 预处理 | `#ifndef GIMBAL_H` |
| [2](../application/gimbal/gimbal.h#L2) | 预处理 | `#define GIMBAL_H` |
| [4](../application/gimbal/gimbal.h#L4) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [16](../application/gimbal/gimbal.h#L16) | 预处理 | `#endif` |
| [30](../application/gimbal/gimbal.h#L30) | 预处理 | `#endif // GIMBAL_H` |

## application/robot.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [6](../application/robot.c#L6) | 预处理 | `#if !IMU_CAPTURE_ONLY` |
| [10](../application/robot.c#L10) | 预处理 | `#endif` |
| [12](../application/robot.c#L12) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE &#124;&#124; CHASSIS_IMU_CAN_RX_ENABLE` |
| [14](../application/robot.c#L14) | 预处理 | `#endif` |
| [17](../application/robot.c#L17) | 预处理 | `#ifndef ROBOT_DEF_PARAM_WARNING` |
| [18](../application/robot.c#L18) | 预处理 | `#define ROBOT_DEF_PARAM_WARNING` |
| [20](../application/robot.c#L20) | 预处理 | `#endif // !ROBOT_DEF_PARAM_WARNING` |
| [22](../application/robot.c#L22) | 预处理 | `#if !IMU_CAPTURE_ONLY && (defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD))` |
| [24](../application/robot.c#L24) | 预处理 | `#endif` |
| [26](../application/robot.c#L26) | 预处理 | `#if !IMU_CAPTURE_ONLY && (defined(ONE_BOARD) &#124;&#124; defined(GIMBAL_BOARD))` |
| [30](../application/robot.c#L30) | 预处理 | `#endif` |
| [42](../application/robot.c#L42) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE &#124;&#124; CHASSIS_IMU_CAN_RX_ENABLE` |
| [44](../application/robot.c#L44) | 预处理 | `#endif` |
| [45](../application/robot.c#L45) | 预处理 | `#if !IMU_CAPTURE_ONLY` |
| [50](../application/robot.c#L50) | 预处理 | `#if defined(ONE_BOARD) &#124;&#124; defined(GIMBAL_BOARD)` |
| [54](../application/robot.c#L54) | 预处理 | `#endif` |
| [56](../application/robot.c#L56) | 预处理 | `#if defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD)` |
| [58](../application/robot.c#L58) | 预处理 | `#endif` |
| [59](../application/robot.c#L59) | 预处理 | `#endif /* !IMU_CAPTURE_ONLY */` |
| [69](../application/robot.c#L69) | 预处理 | `#if !IMU_CAPTURE_ONLY` |
| [70](../application/robot.c#L70) | 预处理 | `#if defined(ONE_BOARD) &#124;&#124; defined(GIMBAL_BOARD)` |
| [74](../application/robot.c#L74) | 预处理 | `#endif` |
| [76](../application/robot.c#L76) | 预处理 | `#if defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD)` |
| [78](../application/robot.c#L78) | 预处理 | `#endif` |
| [79](../application/robot.c#L79) | 预处理 | `#endif /* !IMU_CAPTURE_ONLY */` |

## application/robot.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/robot.h#L1) | 预处理 | `#ifndef ROBOT_H` |
| [2](../application/robot.h#L2) | 预处理 | `#define ROBOT_H` |
| [18](../application/robot.h#L18) | 预处理 | `#endif` |

## application/robot_def.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../application/robot_def.h#L12) | 预处理 | `#ifndef ROBOT_DEF_H` |
| [13](../application/robot_def.h#L13) | 预处理 | `#define ROBOT_DEF_H` |
| [160](../application/robot_def.h#L160) | 预处理 | `#if defined(CHASSIS_BOARD) &#124;&#124; defined(GIMBAL_BOARD) // 非单板的时候底盘还将imu数据回传(若有必要)` |
| [162](../application/robot_def.h#L162) | 预处理 | `#endif` |
| [191](../application/robot_def.h#L191) | 预处理 | `#endif // !ROBOT_DEF_H` |

## application/runtime/chassis_imu_can.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../application/runtime/chassis_imu_can.c#L3) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE &#124;&#124; CHASSIS_IMU_CAN_RX_ENABLE` |
| [11](../application/runtime/chassis_imu_can.c#L11) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE && (!defined(ONE_BOARD) &#124;&#124; CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED &#124;&#124; !CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; CHASSIS_VELOCITY_FUSION_CALIBRATED &#124;&#124; !VOFA_TELEMETRY_ENABLE &#124;&#124; (VOFA_TELEMETRY_MODE != 1 && VOFA_TELEMETRY_MODE != 2))` |
| [13](../application/runtime/chassis_imu_can.c#L13) | 预处理 | `#endif` |
| [17](../application/runtime/chassis_imu_can.c#L17) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [37](../application/runtime/chassis_imu_can.c#L37) | 预处理 | `#endif` |
| [42](../application/runtime/chassis_imu_can.c#L42) | 配置初值 | `CAN_Init_Config_s config={.can_handle=BOARD_CHASSIS_IMU_CAN,.tx_id=CHASSIS_IMU_CAN_DATA_ID,` |
| [43](../application/runtime/chassis_imu_can.c#L43) | 配置初值 | `.rx_id=CHASSIS_IMU_CAN_DATA_ID};` |
| [44](../application/runtime/chassis_imu_can.c#L44) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [47](../application/runtime/chassis_imu_can.c#L47) | 预处理 | `#endif` |
| [53](../application/runtime/chassis_imu_can.c#L53) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE` |
| [119](../application/runtime/chassis_imu_can.c#L119) | 预处理 | `#endif` |
| [120](../application/runtime/chassis_imu_can.c#L120) | 预处理 | `#endif` |

## application/runtime/chassis_imu_can.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/chassis_imu_can.h#L1) | 预处理 | `#ifndef CHASSIS_IMU_CAN_H` |
| [2](../application/runtime/chassis_imu_can.h#L2) | 预处理 | `#define CHASSIS_IMU_CAN_H` |
| [7](../application/runtime/chassis_imu_can.h#L7) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE &#124;&#124; CHASSIS_IMU_CAN_RX_ENABLE` |
| [10](../application/runtime/chassis_imu_can.h#L10) | 预处理 | `#endif` |
| [11](../application/runtime/chassis_imu_can.h#L11) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE` |
| [16](../application/runtime/chassis_imu_can.h#L16) | 预处理 | `#endif` |
| [17](../application/runtime/chassis_imu_can.h#L17) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [22](../application/runtime/chassis_imu_can.h#L22) | 预处理 | `#endif` |
| [24](../application/runtime/chassis_imu_can.h#L24) | 预处理 | `#endif` |

## application/runtime/chassis_speed_observation.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../application/runtime/chassis_speed_observation.c#L5) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [14](../application/runtime/chassis_speed_observation.c#L14) | 预处理 | `#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` |
| [16](../application/runtime/chassis_speed_observation.c#L16) | 预处理 | `#endif` |
| [17](../application/runtime/chassis_speed_observation.c#L17) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [24](../application/runtime/chassis_speed_observation.c#L24) | 预处理 | `#endif` |
| [48](../application/runtime/chassis_speed_observation.c#L48) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [51](../application/runtime/chassis_speed_observation.c#L51) | 预处理 | `#endif` |
| [52](../application/runtime/chassis_speed_observation.c#L52) | 预处理 | `#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` |
| [60](../application/runtime/chassis_speed_observation.c#L60) | 预处理 | `#endif` |
| [64](../application/runtime/chassis_speed_observation.c#L64) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [72](../application/runtime/chassis_speed_observation.c#L72) | 预处理 | `#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` |
| [75](../application/runtime/chassis_speed_observation.c#L75) | 预处理 | `#endif` |
| [82](../application/runtime/chassis_speed_observation.c#L82) | 预处理 | `#else` |
| [84](../application/runtime/chassis_speed_observation.c#L84) | 预处理 | `#endif` |
| [88](../application/runtime/chassis_speed_observation.c#L88) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [90](../application/runtime/chassis_speed_observation.c#L90) | 预处理 | `#endif` |
| [123](../application/runtime/chassis_speed_observation.c#L123) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [131](../application/runtime/chassis_speed_observation.c#L131) | 预处理 | `#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` |
| [133](../application/runtime/chassis_speed_observation.c#L133) | 预处理 | `#endif` |
| [148](../application/runtime/chassis_speed_observation.c#L148) | 预处理 | `#endif` |
| [160](../application/runtime/chassis_speed_observation.c#L160) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [179](../application/runtime/chassis_speed_observation.c#L179) | 预处理 | `#endif` |
| [181](../application/runtime/chassis_speed_observation.c#L181) | 预处理 | `#else` |
| [190](../application/runtime/chassis_speed_observation.c#L190) | 预处理 | `#endif` |

## application/runtime/chassis_speed_observation.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/chassis_speed_observation.h#L1) | 预处理 | `#ifndef CHASSIS_SPEED_OBSERVATION_H` |
| [2](../application/runtime/chassis_speed_observation.h#L2) | 预处理 | `#define CHASSIS_SPEED_OBSERVATION_H` |
| [25](../application/runtime/chassis_speed_observation.h#L25) | 预处理 | `#endif` |

## application/runtime/force_calibration_app.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/force_calibration_app.h#L1) | 预处理 | `#ifndef FORCE_CALIBRATION_APP_H` |
| [2](../application/runtime/force_calibration_app.h#L2) | 预处理 | `#define FORCE_CALIBRATION_APP_H` |
| [9](../application/runtime/force_calibration_app.h#L9) | 预处理 | `#endif` |

## application/runtime/ins_bridge.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [9](../application/runtime/ins_bridge.c#L9) | 预处理 | `#if defined(ONE_BOARD) &#124;&#124; defined(GIMBAL_BOARD)` |
| [13](../application/runtime/ins_bridge.c#L13) | 预处理 | `#endif` |

## application/runtime/ins_bridge.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/ins_bridge.h#L1) | 预处理 | `#ifndef INS_BRIDGE_H` |
| [2](../application/runtime/ins_bridge.h#L2) | 预处理 | `#define INS_BRIDGE_H` |
| [4](../application/runtime/ins_bridge.h#L4) | 预处理 | `#endif` |

## application/runtime/robot_task.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [4](../application/runtime/robot_task.c#L4) | 预处理 | `#if IMU_CAPTURE_ONLY` |
| [14](../application/runtime/robot_task.c#L14) | 预处理 | `#if USB_TELEMETRY_ENABLE &#124;&#124; (!IMU_CAPTURE_CAN_TX_ENABLE && !VOFA_TELEMETRY_ENABLE) &#124;&#124; (VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE != 1 && VOFA_TELEMETRY_MODE != 2)` |
| [16](../application/runtime/robot_task.c#L16) | 预处理 | `#endif` |
| [17](../application/runtime/robot_task.c#L17) | 预处理 | `#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED &#124;&#124; CHASSIS_VELOCITY_FUSION_ENABLE` |
| [19](../application/runtime/robot_task.c#L19) | 预处理 | `#endif` |
| [51](../application/runtime/robot_task.c#L51) | 预处理 | `#else /* Normal robot application tasks. */` |
| [84](../application/runtime/robot_task.c#L84) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [88](../application/runtime/robot_task.c#L88) | 预处理 | `#endif` |
| [91](../application/runtime/robot_task.c#L91) | 预处理 | `#if REFEREE_ENABLE && (defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD))` |
| [93](../application/runtime/robot_task.c#L93) | 预处理 | `#endif` |
| [94](../application/runtime/robot_task.c#L94) | 预处理 | `#if USB_TELEMETRY_ENABLE &#124;&#124; VOFA_TELEMETRY_ENABLE` |
| [102](../application/runtime/robot_task.c#L102) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [108](../application/runtime/robot_task.c#L108) | 预处理 | `#endif` |
| [113](../application/runtime/robot_task.c#L113) | 预处理 | `#endif` |
| [114](../application/runtime/robot_task.c#L114) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED` |
| [132](../application/runtime/robot_task.c#L132) | 预处理 | `#endif` |
| [134](../application/runtime/robot_task.c#L134) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE &#124;&#124; CHASSIS_SHADOW_JSCOPE_ENABLE` |
| [153](../application/runtime/robot_task.c#L153) | 预处理 | `#endif` |
| [158](../application/runtime/robot_task.c#L158) | 预处理 | `#if REFEREE_ENABLE && (defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD))` |
| [160](../application/runtime/robot_task.c#L160) | 预处理 | `#endif` |
| [168](../application/runtime/robot_task.c#L168) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [170](../application/runtime/robot_task.c#L170) | 预处理 | `#endif` |
| [176](../application/runtime/robot_task.c#L176) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [179](../application/runtime/robot_task.c#L179) | 预处理 | `#else` |
| [181](../application/runtime/robot_task.c#L181) | 预处理 | `#endif` |
| [191](../application/runtime/robot_task.c#L191) | 预处理 | `#if USB_TELEMETRY_ENABLE &#124;&#124; VOFA_TELEMETRY_ENABLE` |
| [195](../application/runtime/robot_task.c#L195) | 预处理 | `#endif` |
| [197](../application/runtime/robot_task.c#L197) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED` |
| [200](../application/runtime/robot_task.c#L200) | 预处理 | `#endif` |
| [202](../application/runtime/robot_task.c#L202) | 预处理 | `#if REFEREE_ENABLE && (defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD))` |
| [205](../application/runtime/robot_task.c#L205) | 预处理 | `#endif` |
| [207](../application/runtime/robot_task.c#L207) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE &#124;&#124; CHASSIS_SHADOW_JSCOPE_ENABLE` |
| [210](../application/runtime/robot_task.c#L210) | 预处理 | `#endif` |
| [241](../application/runtime/robot_task.c#L241) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [243](../application/runtime/robot_task.c#L243) | 预处理 | `#endif` |
| [245](../application/runtime/robot_task.c#L245) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [247](../application/runtime/robot_task.c#L247) | 预处理 | `#endif` |
| [249](../application/runtime/robot_task.c#L249) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [252](../application/runtime/robot_task.c#L252) | 预处理 | `#endif` |
| [253](../application/runtime/robot_task.c#L253) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED` |
| [255](../application/runtime/robot_task.c#L255) | 预处理 | `#endif` |
| [299](../application/runtime/robot_task.c#L299) | 预处理 | `#if REFEREE_ENABLE && (defined(ONE_BOARD) &#124;&#124; defined(CHASSIS_BOARD))` |
| [312](../application/runtime/robot_task.c#L312) | 预处理 | `#endif` |
| [313](../application/runtime/robot_task.c#L313) | 预处理 | `#endif /* IMU_CAPTURE_ONLY */` |

## application/runtime/robot_task.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/robot_task.h#L1) | 预处理 | `#ifndef ROBOT_TASK_H` |
| [2](../application/runtime/robot_task.h#L2) | 预处理 | `#define ROBOT_TASK_H` |
| [7](../application/runtime/robot_task.h#L7) | 预处理 | `#endif` |

## application/runtime/robot_telemetry.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [4](../application/runtime/robot_telemetry.c#L4) | 预处理 | `#if IMU_CAPTURE_ONLY` |
| [5](../application/runtime/robot_telemetry.c#L5) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE` |
| [12](../application/runtime/robot_telemetry.c#L12) | 预处理 | `#else` |
| [14](../application/runtime/robot_telemetry.c#L14) | 预处理 | `#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0` |
| [16](../application/runtime/robot_telemetry.c#L16) | 预处理 | `#endif` |
| [39](../application/runtime/robot_telemetry.c#L39) | 预处理 | `#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0` |
| [41](../application/runtime/robot_telemetry.c#L41) | 预处理 | `#else` |
| [43](../application/runtime/robot_telemetry.c#L43) | 预处理 | `#endif` |
| [45](../application/runtime/robot_telemetry.c#L45) | 预处理 | `#endif /* IMU_CAPTURE_CAN_TX_ENABLE */` |
| [47](../application/runtime/robot_telemetry.c#L47) | 预处理 | `#else` |
| [54](../application/runtime/robot_telemetry.c#L54) | 预处理 | `#if VOFA_TELEMETRY_ENABLE` |
| [56](../application/runtime/robot_telemetry.c#L56) | 预处理 | `#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0` |
| [58](../application/runtime/robot_telemetry.c#L58) | 预处理 | `#endif` |
| [59](../application/runtime/robot_telemetry.c#L59) | 预处理 | `#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED` |
| [61](../application/runtime/robot_telemetry.c#L61) | 预处理 | `#endif` |
| [62](../application/runtime/robot_telemetry.c#L62) | 预处理 | `#endif` |
| [64](../application/runtime/robot_telemetry.c#L64) | 预处理 | `#if USB_TELEMETRY_ENABLE` |
| [66](../application/runtime/robot_telemetry.c#L66) | 预处理 | `#endif` |
| [68](../application/runtime/robot_telemetry.c#L68) | 预处理 | `#if USB_TELEMETRY_ENABLE &#124;&#124; VOFA_TELEMETRY_ENABLE` |
| [83](../application/runtime/robot_telemetry.c#L83) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED` |
| [252](../application/runtime/robot_telemetry.c#L252) | 预处理 | `#endif` |
| [253](../application/runtime/robot_telemetry.c#L253) | 预处理 | `#endif` |
| [257](../application/runtime/robot_telemetry.c#L257) | 预处理 | `#if USB_TELEMETRY_ENABLE` |
| [259](../application/runtime/robot_telemetry.c#L259) | 预处理 | `#endif` |
| [264](../application/runtime/robot_telemetry.c#L264) | 预处理 | `#if USB_TELEMETRY_ENABLE` |
| [267](../application/runtime/robot_telemetry.c#L267) | 预处理 | `#endif` |
| [268](../application/runtime/robot_telemetry.c#L268) | 预处理 | `#if (USB_TELEMETRY_ENABLE &#124;&#124; VOFA_TELEMETRY_ENABLE) && CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_SPEED` |
| [270](../application/runtime/robot_telemetry.c#L270) | 预处理 | `#endif` |
| [271](../application/runtime/robot_telemetry.c#L271) | 预处理 | `#if VOFA_TELEMETRY_ENABLE` |
| [272](../application/runtime/robot_telemetry.c#L272) | 预处理 | `#if VOFA_TELEMETRY_MODE == 1 &#124;&#124; VOFA_TELEMETRY_MODE == 2` |
| [273](../application/runtime/robot_telemetry.c#L273) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [278](../application/runtime/robot_telemetry.c#L278) | 预处理 | `#else` |
| [280](../application/runtime/robot_telemetry.c#L280) | 预处理 | `#endif` |
| [281](../application/runtime/robot_telemetry.c#L281) | 预处理 | `#if VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0` |
| [284](../application/runtime/robot_telemetry.c#L284) | 预处理 | `#else` |
| [287](../application/runtime/robot_telemetry.c#L287) | 预处理 | `#endif` |
| [288](../application/runtime/robot_telemetry.c#L288) | 预处理 | `#else` |
| [290](../application/runtime/robot_telemetry.c#L290) | 预处理 | `#endif` |
| [291](../application/runtime/robot_telemetry.c#L291) | 预处理 | `#endif` |
| [292](../application/runtime/robot_telemetry.c#L292) | 预处理 | `#if USB_TELEMETRY_ENABLE` |
| [294](../application/runtime/robot_telemetry.c#L294) | 预处理 | `#endif` |
| [296](../application/runtime/robot_telemetry.c#L296) | 预处理 | `#endif /* IMU_CAPTURE_ONLY */` |

## application/runtime/robot_telemetry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/robot_telemetry.h#L1) | 预处理 | `#ifndef ROBOT_TELEMETRY_H` |
| [2](../application/runtime/robot_telemetry.h#L2) | 预处理 | `#define ROBOT_TELEMETRY_H` |
| [8](../application/runtime/robot_telemetry.h#L8) | 预处理 | `#endif` |

## application/runtime/shadow_jscope.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../application/runtime/shadow_jscope.c#L3) | 预处理 | `#if CHASSIS_SHADOW_JSCOPE_ENABLE` |
| [11](../application/runtime/shadow_jscope.c#L11) | 预处理 | `#ifdef STM32F427xx` |
| [13](../application/runtime/shadow_jscope.c#L13) | 预处理 | `#endif` |
| [14](../application/runtime/shadow_jscope.c#L14) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE != CHASSIS_PREDICTIVE_SHADOW` |
| [16](../application/runtime/shadow_jscope.c#L16) | 预处理 | `#endif` |
| [17](../application/runtime/shadow_jscope.c#L17) | 预处理 | `#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED &#124;&#124; !BOARD_LOG_RTT_ENABLED` |
| [19](../application/runtime/shadow_jscope.c#L19) | 预处理 | `#endif` |
| [56](../application/runtime/shadow_jscope.c#L56) | 预处理 | `#ifdef STM32F427xx` |
| [58](../application/runtime/shadow_jscope.c#L58) | 预处理 | `#endif` |
| [91](../application/runtime/shadow_jscope.c#L91) | 预处理 | `#ifdef STM32F427xx` |
| [95](../application/runtime/shadow_jscope.c#L95) | 预处理 | `#endif` |
| [97](../application/runtime/shadow_jscope.c#L97) | 预处理 | `#else` |
| [100](../application/runtime/shadow_jscope.c#L100) | 预处理 | `#endif` |

## application/runtime/shadow_jscope.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/shadow_jscope.h#L1) | 预处理 | `#ifndef SHADOW_JSCOPE_H` |
| [2](../application/runtime/shadow_jscope.h#L2) | 预处理 | `#define SHADOW_JSCOPE_H` |
| [13](../application/runtime/shadow_jscope.h#L13) | 预处理 | `#endif` |

## application/runtime/tuning_policy.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [16](../application/runtime/tuning_policy.c#L16) | 预处理 | `#if defined(CHASSIS_BOARD)` |
| [18](../application/runtime/tuning_policy.c#L18) | 预处理 | `#endif` |

## application/runtime/tuning_policy.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/tuning_policy.h#L1) | 预处理 | `#ifndef TUNING_POLICY_H` |
| [2](../application/runtime/tuning_policy.h#L2) | 预处理 | `#define TUNING_POLICY_H` |
| [4](../application/runtime/tuning_policy.h#L4) | 预处理 | `#endif` |

## application/runtime/vofa_observation.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../application/runtime/vofa_observation.c#L5) | 预处理 | `#if VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2 && VOFA_OBSERVATION_PROFILE != 0` |
| [11](../application/runtime/vofa_observation.c#L11) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [13](../application/runtime/vofa_observation.c#L13) | 预处理 | `#endif` |
| [41](../application/runtime/vofa_observation.c#L41) | 预处理 | `#if VOFA_OBSERVATION_PROFILE == 1` |
| [42](../application/runtime/vofa_observation.c#L42) | 预处理 | `#define P(name) f[VOFA_POWER_##name]` |
| [172](../application/runtime/vofa_observation.c#L172) | 预处理 | `#else` |
| [173](../application/runtime/vofa_observation.c#L173) | 预处理 | `#define K(name) f[VOFA_KALMAN_##name]` |
| [219](../application/runtime/vofa_observation.c#L219) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [229](../application/runtime/vofa_observation.c#L229) | 预处理 | `#endif` |
| [295](../application/runtime/vofa_observation.c#L295) | 预处理 | `#endif` |
| [313](../application/runtime/vofa_observation.c#L313) | 预处理 | `#if VOFA_OBSERVATION_PROFILE == 1` |
| [316](../application/runtime/vofa_observation.c#L316) | 预处理 | `#else` |
| [319](../application/runtime/vofa_observation.c#L319) | 预处理 | `#endif` |
| [330](../application/runtime/vofa_observation.c#L330) | 预处理 | `#endif` |

## application/runtime/vofa_observation.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/vofa_observation.h#L1) | 预处理 | `#ifndef VOFA_OBSERVATION_H` |
| [2](../application/runtime/vofa_observation.h#L2) | 预处理 | `#define VOFA_OBSERVATION_H` |
| [15](../application/runtime/vofa_observation.h#L15) | 预处理 | `#endif` |

## application/runtime/vofa_schema.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [2](../application/runtime/vofa_schema.h#L2) | 预处理 | `#ifndef VOFA_SCHEMA_H` |
| [3](../application/runtime/vofa_schema.h#L3) | 预处理 | `#define VOFA_SCHEMA_H` |
| [5](../application/runtime/vofa_schema.h#L5) | 预处理 | `#define VOFA_POWER_SCHEMA_ID 4U` |
| [106](../application/runtime/vofa_schema.h#L106) | 预处理 | `#define VOFA_KALMAN_SCHEMA_ID 5U` |
| [207](../application/runtime/vofa_schema.h#L207) | 预处理 | `#endif` |

## application/runtime/vofa_telemetry.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../application/runtime/vofa_telemetry.c#L3) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [5](../application/runtime/vofa_telemetry.c#L5) | 预处理 | `#endif` |
| [6](../application/runtime/vofa_telemetry.c#L6) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [8](../application/runtime/vofa_telemetry.c#L8) | 预处理 | `#endif` |
| [10](../application/runtime/vofa_telemetry.c#L10) | 预处理 | `#if VOFA_TELEMETRY_ENABLE` |
| [12](../application/runtime/vofa_telemetry.c#L12) | 预处理 | `#if VOFA_TELEMETRY_MODE == 0` |
| [66](../application/runtime/vofa_telemetry.c#L66) | 预处理 | `#else` |
| [68](../application/runtime/vofa_telemetry.c#L68) | 预处理 | `#if VOFA_TELEMETRY_MODE == 1` |
| [70](../application/runtime/vofa_telemetry.c#L70) | 预处理 | `#else` |
| [72](../application/runtime/vofa_telemetry.c#L72) | 预处理 | `#endif` |
| [84](../application/runtime/vofa_telemetry.c#L84) | 预处理 | `#if VOFA_TELEMETRY_MODE == 1` |
| [88](../application/runtime/vofa_telemetry.c#L88) | 预处理 | `#else` |
| [98](../application/runtime/vofa_telemetry.c#L98) | 预处理 | `#endif` |
| [119](../application/runtime/vofa_telemetry.c#L119) | 预处理 | `#if VOFA_TELEMETRY_MODE == 2` |
| [126](../application/runtime/vofa_telemetry.c#L126) | 预处理 | `#else` |
| [130](../application/runtime/vofa_telemetry.c#L130) | 预处理 | `#endif` |
| [146](../application/runtime/vofa_telemetry.c#L146) | 预处理 | `#if !IMU_CAPTURE_ONLY` |
| [148](../application/runtime/vofa_telemetry.c#L148) | 预处理 | `#endif` |
| [177](../application/runtime/vofa_telemetry.c#L177) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [192](../application/runtime/vofa_telemetry.c#L192) | 预处理 | `#else` |
| [196](../application/runtime/vofa_telemetry.c#L196) | 预处理 | `#endif` |
| [211](../application/runtime/vofa_telemetry.c#L211) | 预处理 | `#if IMU_CAPTURE_ONLY` |
| [213](../application/runtime/vofa_telemetry.c#L213) | 预处理 | `#endif` |
| [214](../application/runtime/vofa_telemetry.c#L214) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [216](../application/runtime/vofa_telemetry.c#L216) | 预处理 | `#endif` |
| [217](../application/runtime/vofa_telemetry.c#L217) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [220](../application/runtime/vofa_telemetry.c#L220) | 预处理 | `#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` |
| [222](../application/runtime/vofa_telemetry.c#L222) | 预处理 | `#endif` |
| [223](../application/runtime/vofa_telemetry.c#L223) | 预处理 | `#endif` |
| [231](../application/runtime/vofa_telemetry.c#L231) | 预处理 | `#if IMU_CAPTURE_ONLY` |
| [233](../application/runtime/vofa_telemetry.c#L233) | 预处理 | `#else` |
| [235](../application/runtime/vofa_telemetry.c#L235) | 预处理 | `#endif` |
| [284](../application/runtime/vofa_telemetry.c#L284) | 预处理 | `#if IMU_CAPTURE_ONLY` |
| [286](../application/runtime/vofa_telemetry.c#L286) | 预处理 | `#else` |
| [288](../application/runtime/vofa_telemetry.c#L288) | 预处理 | `#endif` |
| [289](../application/runtime/vofa_telemetry.c#L289) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE` |
| [296](../application/runtime/vofa_telemetry.c#L296) | 预处理 | `#endif` |
| [301](../application/runtime/vofa_telemetry.c#L301) | 预处理 | `#if VOFA_TELEMETRY_MODE == 1` |
| [310](../application/runtime/vofa_telemetry.c#L310) | 预处理 | `#else` |
| [324](../application/runtime/vofa_telemetry.c#L324) | 预处理 | `#endif` |
| [326](../application/runtime/vofa_telemetry.c#L326) | 预处理 | `#endif` |
| [327](../application/runtime/vofa_telemetry.c#L327) | 预处理 | `#endif` |

## application/runtime/vofa_telemetry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/runtime/vofa_telemetry.h#L1) | 预处理 | `#ifndef VOFA_TELEMETRY_H` |
| [2](../application/runtime/vofa_telemetry.h#L2) | 预处理 | `#define VOFA_TELEMETRY_H` |
| [8](../application/runtime/vofa_telemetry.h#L8) | 预处理 | `#if VOFA_TELEMETRY_MODE == 1 &#124;&#124; VOFA_TELEMETRY_MODE == 2` |
| [14](../application/runtime/vofa_telemetry.h#L14) | 预处理 | `#else` |
| [16](../application/runtime/vofa_telemetry.h#L16) | 预处理 | `#endif` |
| [18](../application/runtime/vofa_telemetry.h#L18) | 预处理 | `#endif` |

## application/shoot/shoot.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [30](../application/shoot/shoot.c#L30) | 配置初值 | `Motor_Init_Config_s friction_config = {` |
| [31](../application/shoot/shoot.c#L31) | 配置初值 | `.can_init_config = {` |
| [32](../application/shoot/shoot.c#L32) | 配置初值 | `.can_handle = &hcan2,` |
| [33](../application/shoot/shoot.c#L33) | 配置初值 | `},` |
| [34](../application/shoot/shoot.c#L34) | 配置初值 | `.controller_param_init_config = {` |
| [35](../application/shoot/shoot.c#L35) | 配置初值 | `.speed_PID = pid_shoot_friction_speed_config,` |
| [36](../application/shoot/shoot.c#L36) | 配置初值 | `.current_PID = pid_shoot_friction_current_config,` |
| [37](../application/shoot/shoot.c#L37) | 配置初值 | `},` |
| [38](../application/shoot/shoot.c#L38) | 配置初值 | `.controller_setting_init_config = {` |
| [39](../application/shoot/shoot.c#L39) | 配置初值 | `.angle_feedback_source = MOTOR_FEED,` |
| [40](../application/shoot/shoot.c#L40) | 配置初值 | `.speed_feedback_source = MOTOR_FEED,` |
| [41](../application/shoot/shoot.c#L41) | 配置初值 | `.outer_loop_type = SPEED_LOOP,` |
| [42](../application/shoot/shoot.c#L42) | 配置初值 | `.close_loop_type = SPEED_LOOP &#124; CURRENT_LOOP,` |
| [43](../application/shoot/shoot.c#L43) | 配置初值 | `.motor_reverse_flag = MOTOR_DIRECTION_REVERSE,` |
| [44](../application/shoot/shoot.c#L44) | 配置初值 | `},` |
| [45](../application/shoot/shoot.c#L45) | 配置初值 | `.motor_type = M3508};` |
| [54](../application/shoot/shoot.c#L54) | 配置初值 | `Motor_Init_Config_s loader_config = {` |
| [55](../application/shoot/shoot.c#L55) | 配置初值 | `.can_init_config = {` |
| [56](../application/shoot/shoot.c#L56) | 配置初值 | `.can_handle = &hcan2,` |
| [57](../application/shoot/shoot.c#L57) | 配置初值 | `.tx_id = 7,` |
| [58](../application/shoot/shoot.c#L58) | 配置初值 | `},` |
| [59](../application/shoot/shoot.c#L59) | 配置初值 | `.controller_param_init_config = {` |
| [60](../application/shoot/shoot.c#L60) | 配置初值 | `.angle_PID = pid_shoot_loader_angle_config,` |
| [61](../application/shoot/shoot.c#L61) | 配置初值 | `.speed_PID = pid_shoot_loader_speed_config,` |
| [62](../application/shoot/shoot.c#L62) | 配置初值 | `.current_PID = pid_shoot_loader_current_config,` |
| [63](../application/shoot/shoot.c#L63) | 配置初值 | `},` |
| [64](../application/shoot/shoot.c#L64) | 配置初值 | `.controller_setting_init_config = {` |
| [65](../application/shoot/shoot.c#L65) | 配置初值 | `.angle_feedback_source = MOTOR_FEED,` |
| [66](../application/shoot/shoot.c#L66) | 配置初值 | `.speed_feedback_source = MOTOR_FEED,` |
| [67](../application/shoot/shoot.c#L67) | 配置初值 | `.outer_loop_type = SPEED_LOOP, // 初始化成SPEED_LOOP,让拨盘停在原地,防止拨盘上电时乱转` |
| [68](../application/shoot/shoot.c#L68) | 配置初值 | `.close_loop_type = CURRENT_LOOP &#124; SPEED_LOOP &#124; ANGLE_LOOP,` |
| [69](../application/shoot/shoot.c#L69) | 配置初值 | `.motor_reverse_flag = MOTOR_DIRECTION_NORMAL, // 注意方向设置为拨盘的拨出的击发方向` |
| [70](../application/shoot/shoot.c#L70) | 配置初值 | `},` |
| [71](../application/shoot/shoot.c#L71) | 配置初值 | `.motor_type = M2006 // 英雄使用m3508` |

## application/shoot/shoot.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../application/shoot/shoot.h#L1) | 预处理 | `#ifndef SHOOT_H` |
| [2](../application/shoot/shoot.h#L2) | 预处理 | `#define SHOOT_H` |
| [16](../application/shoot/shoot.h#L16) | 预处理 | `#endif // SHOOT_H` |

## bsp/bsp_init.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/bsp_init.h#L1) | 预处理 | `#ifndef BSP_INIT_h` |
| [2](../bsp/bsp_init.h#L2) | 预处理 | `#define BSP_INIT_h` |
| [23](../bsp/bsp_init.h#L23) | 预处理 | `#endif // !BSP_INIT_h` |

## bsp/bsp_tools.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [7](../bsp/bsp_tools.c#L7) | 预处理 | `#define MX_SIG_LIST_SIZE 32 // 不可修改,最大信号量数量` |

## bsp/can/bsp_can.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/can/bsp_can.h#L1) | 预处理 | `#ifndef BSP_CAN_H` |
| [2](../bsp/can/bsp_can.h#L2) | 预处理 | `#define BSP_CAN_H` |
| [8](../bsp/can/bsp_can.h#L8) | 预处理 | `#define CAN_MX_REGISTER_CNT 16     // 这个数量取决于CAN总线的负载` |
| [9](../bsp/can/bsp_can.h#L9) | 预处理 | `#define MX_CAN_FILTER_CNT (2 * 14) // 最多可以使用的CAN过滤器数量,目前远不会用到这么多` |
| [10](../bsp/can/bsp_can.h#L10) | 预处理 | `#define DEVICE_CAN_CNT 2           // 根据板子设定,F407IG有CAN1,CAN2,因此为2;F334只有一个,则设为1` |
| [77](../bsp/can/bsp_can.h#L77) | 预处理 | `#endif` |

## bsp/dwt/bsp_dwt.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [14](../bsp/dwt/bsp_dwt.h#L14) | 预处理 | `#ifndef _BSP_DWT_H` |
| [15](../bsp/dwt/bsp_dwt.h#L15) | 预处理 | `#define _BSP_DWT_H` |
| [33](../bsp/dwt/bsp_dwt.h#L33) | 预处理 | `#define TIME_ELAPSE(dt, code)                    \` |
| [106](../bsp/dwt/bsp_dwt.h#L106) | 预处理 | `#endif /* BSP_DWT_H_ */` |

## bsp/flash/bsp_flash.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/flash/bsp_flash.h#L1) | 预处理 | `#ifndef _BSP_FLASH_H` |
| [2](../bsp/flash/bsp_flash.h#L2) | 预处理 | `#define _BSP_FLASH_H` |
| [6](../bsp/flash/bsp_flash.h#L6) | 预处理 | `#define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000)  /* Base address of Sector 0, 16 Kbytes   */` |
| [7](../bsp/flash/bsp_flash.h#L7) | 预处理 | `#define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000)  /* Base address of Sector 1, 16 Kbytes   */` |
| [8](../bsp/flash/bsp_flash.h#L8) | 预处理 | `#define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000)  /* Base address of Sector 2, 16 Kbytes   */` |
| [9](../bsp/flash/bsp_flash.h#L9) | 预处理 | `#define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000)  /* Base address of Sector 3, 16 Kbytes   */` |
| [10](../bsp/flash/bsp_flash.h#L10) | 预处理 | `#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)  /* Base address of Sector 4, 64 Kbytes   */` |
| [11](../bsp/flash/bsp_flash.h#L11) | 预处理 | `#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)  /* Base address of Sector 5, 128 Kbytes  */` |
| [12](../bsp/flash/bsp_flash.h#L12) | 预处理 | `#define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000)  /* Base address of Sector 6, 128 Kbytes  */` |
| [13](../bsp/flash/bsp_flash.h#L13) | 预处理 | `#define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000)  /* Base address of Sector 7, 128 Kbytes  */` |
| [14](../bsp/flash/bsp_flash.h#L14) | 预处理 | `#define ADDR_FLASH_SECTOR_8 ((uint32_t)0x08080000)  /* Base address of Sector 8, 128 Kbytes  */` |
| [15](../bsp/flash/bsp_flash.h#L15) | 预处理 | `#define ADDR_FLASH_SECTOR_9 ((uint32_t)0x080A0000)  /* Base address of Sector 9, 128 Kbytes  */` |
| [16](../bsp/flash/bsp_flash.h#L16) | 预处理 | `#define ADDR_FLASH_SECTOR_10 ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */` |
| [17](../bsp/flash/bsp_flash.h#L17) | 预处理 | `#define ADDR_FLASH_SECTOR_11 ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */` |
| [19](../bsp/flash/bsp_flash.h#L19) | 预处理 | `#define FLASH_END_ADDR ((uint32_t)0x08200000)       /* Base address of Sector 23, 128 Kbytes */` |
| [22](../bsp/flash/bsp_flash.h#L22) | 预处理 | `#define ADDR_FLASH_SECTOR_12 ((uint32_t)0x08100000) /* Base address of Sector 12, 16 Kbytes  */` |
| [23](../bsp/flash/bsp_flash.h#L23) | 预处理 | `#define ADDR_FLASH_SECTOR_13 ((uint32_t)0x08104000) /* Base address of Sector 13, 16 Kbytes  */` |
| [24](../bsp/flash/bsp_flash.h#L24) | 预处理 | `#define ADDR_FLASH_SECTOR_14 ((uint32_t)0x08108000) /* Base address of Sector 14, 16 Kbytes  */` |
| [25](../bsp/flash/bsp_flash.h#L25) | 预处理 | `#define ADDR_FLASH_SECTOR_15 ((uint32_t)0x0810C000) /* Base address of Sector 15, 16 Kbytes  */` |
| [26](../bsp/flash/bsp_flash.h#L26) | 预处理 | `#define ADDR_FLASH_SECTOR_16 ((uint32_t)0x08110000) /* Base address of Sector 16, 64 Kbytes  */` |
| [27](../bsp/flash/bsp_flash.h#L27) | 预处理 | `#define ADDR_FLASH_SECTOR_17 ((uint32_t)0x08120000) /* Base address of Sector 17, 128 Kbytes */` |
| [28](../bsp/flash/bsp_flash.h#L28) | 预处理 | `#define ADDR_FLASH_SECTOR_18 ((uint32_t)0x08140000) /* Base address of Sector 18, 128 Kbytes */` |
| [29](../bsp/flash/bsp_flash.h#L29) | 预处理 | `#define ADDR_FLASH_SECTOR_19 ((uint32_t)0x08160000) /* Base address of Sector 19, 128 Kbytes */` |
| [30](../bsp/flash/bsp_flash.h#L30) | 预处理 | `#define ADDR_FLASH_SECTOR_20 ((uint32_t)0x08180000) /* Base address of Sector 20, 128 Kbytes */` |
| [31](../bsp/flash/bsp_flash.h#L31) | 预处理 | `#define ADDR_FLASH_SECTOR_21 ((uint32_t)0x081A0000) /* Base address of Sector 21, 128 Kbytes */` |
| [32](../bsp/flash/bsp_flash.h#L32) | 预处理 | `#define ADDR_FLASH_SECTOR_22 ((uint32_t)0x081C0000) /* Base address of Sector 22, 128 Kbytes */` |
| [33](../bsp/flash/bsp_flash.h#L33) | 预处理 | `#define ADDR_FLASH_SECTOR_23 ((uint32_t)0x081E0000) /* Base address of Sector 23, 128 Kbytes */` |
| [80](../bsp/flash/bsp_flash.h#L80) | 预处理 | `#endif` |

## bsp/flash/bsp_tuning_flash.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../bsp/flash/bsp_tuning_flash.c#L5) | 预处理 | `#define BANK_WORDS (128U*1024U/sizeof(uint32_t))` |

## bsp/flash/bsp_tuning_flash.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/flash/bsp_tuning_flash.h#L1) | 预处理 | `#ifndef BSP_TUNING_FLASH_H` |
| [2](../bsp/flash/bsp_tuning_flash.h#L2) | 预处理 | `#define BSP_TUNING_FLASH_H` |
| [11](../bsp/flash/bsp_tuning_flash.h#L11) | 预处理 | `#endif` |

## bsp/gpio/bsp_gpio.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/gpio/bsp_gpio.h#L1) | 预处理 | `#ifndef BSP_GPIO_H` |
| [2](../bsp/gpio/bsp_gpio.h#L2) | 预处理 | `#define BSP_GPIO_H` |
| [6](../bsp/gpio/bsp_gpio.h#L6) | 预处理 | `#define GPIO_MX_DEVICE_NUM 10` |
| [90](../bsp/gpio/bsp_gpio.h#L90) | 预处理 | `#endif` |

## bsp/iic/bsp_iic.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/iic/bsp_iic.h#L1) | 预处理 | `#ifndef BSP_IIC_H` |
| [2](../bsp/iic/bsp_iic.h#L2) | 预处理 | `#define BSP_IIC_H` |
| [7](../bsp/iic/bsp_iic.h#L7) | 预处理 | `#define IIC_DEVICE_CNT 2   // C板引出了I2C2和I2C3` |
| [8](../bsp/iic/bsp_iic.h#L8) | 预处理 | `#define MX_IIC_SLAVE_CNT 8 // 最大从机数目,根据需要修改` |
| [111](../bsp/iic/bsp_iic.h#L111) | 预处理 | `#endif` |

## bsp/log/bsp_log.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../bsp/log/bsp_log.c#L11) | 预处理 | `#define BSP_LOG_FORMAT_BUFFER_SIZE 384U // 384字节的格式化缓冲区,足够打印一行日志,超过则截断` |
| [16](../bsp/log/bsp_log.c#L16) | 预处理 | `#if BOARD_LOG_UART_ENABLED` |
| [40](../bsp/log/bsp_log.c#L40) | 预处理 | `#endif` |
| [46](../bsp/log/bsp_log.c#L46) | 预处理 | `#if BOARD_LOG_RTT_ENABLED` |
| [49](../bsp/log/bsp_log.c#L49) | 预处理 | `#endif` |
| [50](../bsp/log/bsp_log.c#L50) | 预处理 | `#if BOARD_LOG_UART_ENABLED` |
| [53](../bsp/log/bsp_log.c#L53) | 预处理 | `#endif` |
| [61](../bsp/log/bsp_log.c#L61) | 预处理 | `#if BOARD_LOG_RTT_ENABLED` |
| [64](../bsp/log/bsp_log.c#L64) | 预处理 | `#endif` |
| [65](../bsp/log/bsp_log.c#L65) | 预处理 | `#if BOARD_LOG_UART_ENABLED` |
| [69](../bsp/log/bsp_log.c#L69) | 预处理 | `#endif` |
| [75](../bsp/log/bsp_log.c#L75) | 预处理 | `#if BOARD_LOG_RTT_ENABLED` |
| [77](../bsp/log/bsp_log.c#L77) | 预处理 | `#endif` |
| [78](../bsp/log/bsp_log.c#L78) | 预处理 | `#if BOARD_LOG_UART_ENABLED` |
| [80](../bsp/log/bsp_log.c#L80) | 预处理 | `#endif` |

## bsp/log/bsp_log.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/log/bsp_log.h#L1) | 预处理 | `#ifndef _BSP_LOG_H` |
| [2](../bsp/log/bsp_log.h#L2) | 预处理 | `#define _BSP_LOG_H` |
| [10](../bsp/log/bsp_log.h#L10) | 预处理 | `#define BUFFER_INDEX 0` |
| [31](../bsp/log/bsp_log.h#L31) | 预处理 | `#define LOG_PROTO(type, color, format, ...)              \` |
| [38](../bsp/log/bsp_log.h#L38) | 预处理 | `#define LOG_CLEAR() BSPLogClear()` |
| [41](../bsp/log/bsp_log.h#L41) | 预处理 | `#define LOG(format, ...) LOG_PROTO("", "", format, ##__VA_ARGS__)` |
| [48](../bsp/log/bsp_log.h#L48) | 预处理 | `#if DISABLE_LOG_SYSTEM` |
| [49](../bsp/log/bsp_log.h#L49) | 预处理 | `#define LOGINFO(format, ...)` |
| [50](../bsp/log/bsp_log.h#L50) | 预处理 | `#define LOGWARNING(format, ...)` |
| [51](../bsp/log/bsp_log.h#L51) | 预处理 | `#define LOGERROR(format, ...)` |
| [52](../bsp/log/bsp_log.h#L52) | 预处理 | `#else` |
| [54](../bsp/log/bsp_log.h#L54) | 预处理 | `#define LOGINFO(format, ...) LOG_PROTO("I:", RTT_CTRL_TEXT_BRIGHT_GREEN, format, ##__VA_ARGS__)` |
| [56](../bsp/log/bsp_log.h#L56) | 预处理 | `#define LOGWARNING(format, ...) LOG_PROTO("W:", RTT_CTRL_TEXT_BRIGHT_YELLOW, format, ##__VA_ARGS__)` |
| [58](../bsp/log/bsp_log.h#L58) | 预处理 | `#define LOGERROR(format, ...) LOG_PROTO("E:", RTT_CTRL_TEXT_BRIGHT_RED, format, ##__VA_ARGS__)` |
| [59](../bsp/log/bsp_log.h#L59) | 预处理 | `#endif //  DISABLE_LOG_SYSTEM` |
| [80](../bsp/log/bsp_log.h#L80) | 预处理 | `#endif` |

## bsp/log/bsp_uart_log.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [15](../bsp/log/bsp_uart_log.c#L15) | 预处理 | `#define BSP_UART_LOG_RING_SIZE 4096U` |

## bsp/log/bsp_uart_log.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/log/bsp_uart_log.h#L1) | 预处理 | `#ifndef BSP_UART_LOG_H` |
| [2](../bsp/log/bsp_uart_log.h#L2) | 预处理 | `#define BSP_UART_LOG_H` |
| [33](../bsp/log/bsp_uart_log.h#L33) | 预处理 | `#endif` |

## bsp/pwm/bsp_pwm.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../bsp/pwm/bsp_pwm.h#L12) | 预处理 | `#ifndef BSP_PWM_H` |
| [13](../bsp/pwm/bsp_pwm.h#L13) | 预处理 | `#define BSP_PWM_H` |
| [19](../bsp/pwm/bsp_pwm.h#L19) | 预处理 | `#define PWM_DEVICE_CNT 16 // 最大支持的PWM实例数量` |
| [91](../bsp/pwm/bsp_pwm.h#L91) | 预处理 | `#endif // BSP_PWM_H` |

## bsp/spi/bsp_spi.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/spi/bsp_spi.h#L1) | 预处理 | `#ifndef BSP_SPI_H` |
| [2](../bsp/spi/bsp_spi.h#L2) | 预处理 | `#define BSP_SPI_H` |
| [9](../bsp/spi/bsp_spi.h#L9) | 预处理 | `#define SPI_DEVICE_CNT 2       // C型开发板引出两路spi,分别连接BMI088/作为扩展IO在8pin牛角座引出` |
| [10](../bsp/spi/bsp_spi.h#L10) | 预处理 | `#define MX_SPI_BUS_SLAVE_CNT 4 // 单个spi总线上挂载的从机数目` |
| [110](../bsp/spi/bsp_spi.h#L110) | 预处理 | `#endif` |

## bsp/usart/bsp_usart.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../bsp/usart/bsp_usart.h#L1) | 预处理 | `#ifndef BSP_RC_H` |
| [2](../bsp/usart/bsp_usart.h#L2) | 预处理 | `#define BSP_RC_H` |
| [8](../bsp/usart/bsp_usart.h#L8) | 预处理 | `#define DEVICE_USART_CNT 6     // C板至多分配3个串口` |
| [9](../bsp/usart/bsp_usart.h#L9) | 预处理 | `#define USART_RXBUFF_LIMIT 256 // 如果协议需要更大的buff,请修改这里` |
| [93](../bsp/usart/bsp_usart.h#L93) | 预处理 | `#endif` |

## config/00_flash/board_flash_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/00_flash/board_flash_config.h#L1) | 预处理 | `#ifndef BOARD_FLASH_CONFIG_H` |
| [2](../config/00_flash/board_flash_config.h#L2) | 预处理 | `#define BOARD_FLASH_CONFIG_H` |
| [11](../config/00_flash/board_flash_config.h#L11) | 预处理 | `#define BOARD_FLASH_TARGET 1 /* 烧录目标：1=整车主板，2=下方独立底盘IMU采集板。 */` |
| [12](../config/00_flash/board_flash_config.h#L12) | 预处理 | `#define BOARD_FLASH_PROGRAMMER 1 /* 下载器：1=DAPLink/OpenOCD，2=J-Link/J-Flash；仅Flash动作使用。 */` |
| [13](../config/00_flash/board_flash_config.h#L13) | 预处理 | `#define BOARD_FLASH_PROFILE "atype-vehicle" /* 两板共用编译profile，任务由各板采集开关区分。 */` |
| [18](../config/00_flash/board_flash_config.h#L18) | 预处理 | `#define BOARD_FLASH_1_NAME "第一块：整车主板" /* 命令执行前显示的板卡名称。 */` |
| [19](../config/00_flash/board_flash_config.h#L19) | 预处理 | `#define BOARD_FLASH_1_BUILD_DIR "build/board-1-main" /* 主板输出目录；build下使用英文字母/数字/下划线/连字符和斜杠。 */` |
| [20](../config/00_flash/board_flash_config.h#L20) | 预处理 | `#define BOARD_FLASH_1_ONE_BOARD 1 /* 单板整车角色：必须为1，不与CHASSIS_BOARD或GIMBAL_BOARD同时定义。 */` |
| [21](../config/00_flash/board_flash_config.h#L21) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_CONTROL_MODE 0 /* 控制后端：0=速度控制；当前远端IMU观测路径要求0。 */` |
| [22](../config/00_flash/board_flash_config.h#L22) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 力控标定工具：0=关闭；本方案不编译标定固件。 */` |
| [23](../config/00_flash/board_flash_config.h#L23) | 预处理 | `#define BOARD_FLASH_1_ATYPE_BENCH_MODE 0 /* 台架门控：0=正常整车应用；此项不代替遥控器的启停。 */` |
| [24](../config/00_flash/board_flash_config.h#L24) | 预处理 | `#define BOARD_FLASH_1_IMU_CAPTURE_ONLY 0 /* 0=完整整车任务；本入口不启用独立本地IMU诊断任务。 */` |
| [25](../config/00_flash/board_flash_config.h#L25) | 预处理 | `#define BOARD_FLASH_1_IMU_CAPTURE_CAN_TX_ENABLE 0 /* 0=主板不发送采集帧，避免与第二块重复发送。 */` |
| [26](../config/00_flash/board_flash_config.h#L26) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_IMU_CAN_RX_ENABLE 1 /* 1=接收所选底盘IMU；0=关闭外部接收。 */` |
| [27](../config/00_flash/board_flash_config.h#L27) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_ENABLE 1 /* 1=启用速度观测接口；当前远端接收路径要求1。 */` |
| [28](../config/00_flash/board_flash_config.h#L28) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_VELOCITY_FUSION_CALIBRATED 0 /* 0=不启用本地云台标定；远端固定底盘IMU使用独立的安装与噪声配置。 */` |
| [29](../config/00_flash/board_flash_config.h#L29) | 预处理 | `#define BOARD_FLASH_1_CHASSIS_REMOTE_IMU_FUSION_ENABLE 1 /* 1=用固定底盘IMU做只读卡尔曼观测；不表示安装与噪声参数已实测审核，也不接管控制。 */` |
| [30](../config/00_flash/board_flash_config.h#L30) | 预处理 | `#define BOARD_FLASH_1_VOFA_TELEMETRY_ENABLE 1 /* 1=开启UART7统一遥测，需保持UART7未被裁判/辅助设备占用。 */` |
| [31](../config/00_flash/board_flash_config.h#L31) | 预处理 | `#define BOARD_FLASH_1_VOFA_TELEMETRY_MODE 2 /* 2=JustFloat二进制观测；0/1保留旧格式，不使用用途选择。 */` |
| [33](../config/00_flash/board_flash_config.h#L33) | 预处理 | `#define BOARD_FLASH_1_USB_TELEMETRY_ENABLE 1 /* 1=保留USB网页遥测；视觉应使用UART，不能同时选USB视觉。 */` |
| [37](../config/00_flash/board_flash_config.h#L37) | 预处理 | `#define BOARD_FLASH_2_NAME "第二块：底盘IMU采集板" /* 命令执行前显示的板卡名称。 */` |
| [38](../config/00_flash/board_flash_config.h#L38) | 预处理 | `#define BOARD_FLASH_2_BUILD_DIR "build/board-2-imu" /* 独立输出目录，不能与第一块相同。 */` |
| [39](../config/00_flash/board_flash_config.h#L39) | 预处理 | `#define BOARD_FLASH_2_ONE_BOARD 1 /* 同工程默认角色；IMU_CAPTURE_ONLY选择独立采集任务。 */` |
| [40](../config/00_flash/board_flash_config.h#L40) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_CONTROL_MODE 0 /* 0=速度后端编译配置；采集模式不会启动底盘控制。 */` |
| [41](../config/00_flash/board_flash_config.h#L41) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 0=关闭力控标定工具。 */` |
| [42](../config/00_flash/board_flash_config.h#L42) | 预处理 | `#define BOARD_FLASH_2_ATYPE_BENCH_MODE 0 /* 0=不使用整车台架任务。 */` |
| [43](../config/00_flash/board_flash_config.h#L43) | 预处理 | `#define BOARD_FLASH_2_IMU_CAPTURE_ONLY 1 /* 1=仅IMU采集固件。 */` |
| [44](../config/00_flash/board_flash_config.h#L44) | 预处理 | `#define BOARD_FLASH_2_IMU_CAPTURE_CAN_TX_ENABLE 1 /* 1=经所选CAN发送IMU，主板对应开启接收。 */` |
| [45](../config/00_flash/board_flash_config.h#L45) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_IMU_CAN_RX_ENABLE 0 /* 0=不作为整车接收端。 */` |
| [46](../config/00_flash/board_flash_config.h#L46) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_VELOCITY_FUSION_ENABLE 0 /* 0=不运行整车速度融合。 */` |
| [47](../config/00_flash/board_flash_config.h#L47) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_VELOCITY_FUSION_CALIBRATED 0 /* 0=不加载整车融合标定。 */` |
| [48](../config/00_flash/board_flash_config.h#L48) | 预处理 | `#define BOARD_FLASH_2_CHASSIS_REMOTE_IMU_FUSION_ENABLE 0 /* 0=采集板只发送IMU。 */` |
| [49](../config/00_flash/board_flash_config.h#L49) | 预处理 | `#define BOARD_FLASH_2_VOFA_TELEMETRY_ENABLE 0 /* 0=关闭本板UART7遥测，由主板统一输出。 */` |
| [50](../config/00_flash/board_flash_config.h#L50) | 预处理 | `#define BOARD_FLASH_2_VOFA_TELEMETRY_MODE 1 /* 预留格式；遥测关闭时不产生串口输出。 */` |
| [51](../config/00_flash/board_flash_config.h#L51) | 预处理 | `#define BOARD_FLASH_2_USB_TELEMETRY_ENABLE 0 /* 0=关闭USB网页遥测，独立采集固件要求关闭。 */` |
| [53](../config/00_flash/board_flash_config.h#L53) | 预处理 | `#endif` |

## config/01_pid/pid_config.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [14](../config/01_pid/pid_config.c#L14) | 配置初值 | `const PID_Init_Config_s pid_chassis_speed_config = {` |
| [15](../config/01_pid/pid_config.c#L15) | 配置初值 | `.Kp = 0.75f, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [16](../config/01_pid/pid_config.c#L16) | 配置初值 | `.Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [17](../config/01_pid/pid_config.c#L17) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [18](../config/01_pid/pid_config.c#L18) | 配置初值 | `.IntegralLimit = 3000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [19](../config/01_pid/pid_config.c#L19) | 配置初值 | `.Improve = PID_Trapezoid_Intergral &#124; PID_Integral_Limit &#124; PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 &#124; 增删标志切换，0 关闭全部。 */` |
| [20](../config/01_pid/pid_config.c#L20) | 配置初值 | `.MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [21](../config/01_pid/pid_config.c#L21) | 配置初值 | `.Output_LPF_RC = 0.3, /* 输出低通时间常数，单位秒；需 Improve 含 PID_OutputFilter 才生效，当前默认未启用该标志。 */` |
| [26](../config/01_pid/pid_config.c#L26) | 配置初值 | `const PID_Init_Config_s pid_gimbal_yaw_angle_config = {` |
| [27](../config/01_pid/pid_config.c#L27) | 配置初值 | `.Kp = 8, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [28](../config/01_pid/pid_config.c#L28) | 配置初值 | `.Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [29](../config/01_pid/pid_config.c#L29) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [30](../config/01_pid/pid_config.c#L30) | 配置初值 | `.DeadBand = 0.1, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */` |
| [31](../config/01_pid/pid_config.c#L31) | 配置初值 | `.Improve = PID_Trapezoid_Intergral &#124; PID_Integral_Limit &#124; PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 &#124; 增删标志切换，0 关闭全部。 */` |
| [32](../config/01_pid/pid_config.c#L32) | 配置初值 | `.IntegralLimit = 100, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [33](../config/01_pid/pid_config.c#L33) | 配置初值 | `.MaxOut = 500, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [38](../config/01_pid/pid_config.c#L38) | 配置初值 | `const PID_Init_Config_s pid_gimbal_yaw_speed_config = {` |
| [39](../config/01_pid/pid_config.c#L39) | 配置初值 | `.Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [40](../config/01_pid/pid_config.c#L40) | 配置初值 | `.Ki = 200, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [41](../config/01_pid/pid_config.c#L41) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [42](../config/01_pid/pid_config.c#L42) | 配置初值 | `.Improve = PID_Trapezoid_Intergral &#124; PID_Integral_Limit &#124; PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 &#124; 增删标志切换，0 关闭全部。 */` |
| [43](../config/01_pid/pid_config.c#L43) | 配置初值 | `.IntegralLimit = 3000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [44](../config/01_pid/pid_config.c#L44) | 配置初值 | `.MaxOut = 20000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [49](../config/01_pid/pid_config.c#L49) | 配置初值 | `const PID_Init_Config_s pid_gimbal_pitch_angle_config = {` |
| [50](../config/01_pid/pid_config.c#L50) | 配置初值 | `.Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [51](../config/01_pid/pid_config.c#L51) | 配置初值 | `.Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [52](../config/01_pid/pid_config.c#L52) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [53](../config/01_pid/pid_config.c#L53) | 配置初值 | `.Improve = PID_Trapezoid_Intergral &#124; PID_Integral_Limit &#124; PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 &#124; 增删标志切换，0 关闭全部。 */` |
| [54](../config/01_pid/pid_config.c#L54) | 配置初值 | `.IntegralLimit = 100, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [55](../config/01_pid/pid_config.c#L55) | 配置初值 | `.MaxOut = 500, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [60](../config/01_pid/pid_config.c#L60) | 配置初值 | `const PID_Init_Config_s pid_gimbal_pitch_speed_config = {` |
| [61](../config/01_pid/pid_config.c#L61) | 配置初值 | `.Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [62](../config/01_pid/pid_config.c#L62) | 配置初值 | `.Ki = 350, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [63](../config/01_pid/pid_config.c#L63) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [64](../config/01_pid/pid_config.c#L64) | 配置初值 | `.Improve = PID_Trapezoid_Intergral &#124; PID_Integral_Limit &#124; PID_Derivative_On_Measurement, /* 默认 0x07：梯形积分、积分限幅、微分先行；用 &#124; 增删标志切换，0 关闭全部。 */` |
| [65](../config/01_pid/pid_config.c#L65) | 配置初值 | `.IntegralLimit = 2500, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [66](../config/01_pid/pid_config.c#L66) | 配置初值 | `.MaxOut = 20000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [71](../config/01_pid/pid_config.c#L71) | 配置初值 | `const PID_Init_Config_s pid_shoot_friction_speed_config = {` |
| [72](../config/01_pid/pid_config.c#L72) | 配置初值 | `.Kp = 20, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [73](../config/01_pid/pid_config.c#L73) | 配置初值 | `.Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [74](../config/01_pid/pid_config.c#L74) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [75](../config/01_pid/pid_config.c#L75) | 配置初值 | `.Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |
| [76](../config/01_pid/pid_config.c#L76) | 配置初值 | `.IntegralLimit = 10000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [77](../config/01_pid/pid_config.c#L77) | 配置初值 | `.MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [82](../config/01_pid/pid_config.c#L82) | 配置初值 | `const PID_Init_Config_s pid_shoot_friction_current_config = {` |
| [83](../config/01_pid/pid_config.c#L83) | 配置初值 | `.Kp = 0.7, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [84](../config/01_pid/pid_config.c#L84) | 配置初值 | `.Ki = 0.1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [85](../config/01_pid/pid_config.c#L85) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [86](../config/01_pid/pid_config.c#L86) | 配置初值 | `.Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |
| [87](../config/01_pid/pid_config.c#L87) | 配置初值 | `.IntegralLimit = 10000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [88](../config/01_pid/pid_config.c#L88) | 配置初值 | `.MaxOut = 15000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [94](../config/01_pid/pid_config.c#L94) | 配置初值 | `const PID_Init_Config_s pid_shoot_loader_angle_config = {` |
| [95](../config/01_pid/pid_config.c#L95) | 配置初值 | `.Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [96](../config/01_pid/pid_config.c#L96) | 配置初值 | `.Ki = 0, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [97](../config/01_pid/pid_config.c#L97) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [98](../config/01_pid/pid_config.c#L98) | 配置初值 | `.MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [103](../config/01_pid/pid_config.c#L103) | 配置初值 | `const PID_Init_Config_s pid_shoot_loader_speed_config = {` |
| [104](../config/01_pid/pid_config.c#L104) | 配置初值 | `.Kp = 10, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [105](../config/01_pid/pid_config.c#L105) | 配置初值 | `.Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [106](../config/01_pid/pid_config.c#L106) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [107](../config/01_pid/pid_config.c#L107) | 配置初值 | `.Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |
| [108](../config/01_pid/pid_config.c#L108) | 配置初值 | `.IntegralLimit = 5000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [109](../config/01_pid/pid_config.c#L109) | 配置初值 | `.MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [114](../config/01_pid/pid_config.c#L114) | 配置初值 | `const PID_Init_Config_s pid_shoot_loader_current_config = {` |
| [115](../config/01_pid/pid_config.c#L115) | 配置初值 | `.Kp = 0.7, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [116](../config/01_pid/pid_config.c#L116) | 配置初值 | `.Ki = 0.1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [117](../config/01_pid/pid_config.c#L117) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [118](../config/01_pid/pid_config.c#L118) | 配置初值 | `.Improve = PID_Integral_Limit, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |
| [119](../config/01_pid/pid_config.c#L119) | 配置初值 | `.IntegralLimit = 5000, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [120](../config/01_pid/pid_config.c#L120) | 配置初值 | `.MaxOut = 5000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [125](../config/01_pid/pid_config.c#L125) | 配置初值 | `const PID_Init_Config_s pid_imu_mpu6500_temperature_config = {` |
| [126](../config/01_pid/pid_config.c#L126) | 配置初值 | `.MaxOut = 100, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [127](../config/01_pid/pid_config.c#L127) | 配置初值 | `.IntegralLimit = 15, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [128](../config/01_pid/pid_config.c#L128) | 配置初值 | `.DeadBand = 0, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */` |
| [129](../config/01_pid/pid_config.c#L129) | 配置初值 | `.Kp = 50, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [130](../config/01_pid/pid_config.c#L130) | 配置初值 | `.Ki = 1, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [131](../config/01_pid/pid_config.c#L131) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [132](../config/01_pid/pid_config.c#L132) | 配置初值 | `.Improve = 0x01, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |
| [138](../config/01_pid/pid_config.c#L138) | 配置初值 | `const PID_Init_Config_s pid_imu_bmi088_temperature_config = {` |
| [139](../config/01_pid/pid_config.c#L139) | 配置初值 | `.MaxOut = 2000, /* 总输出绝对值上限，单位 U；控制器将输出限制在正负此值。 */` |
| [140](../config/01_pid/pid_config.c#L140) | 配置初值 | `.IntegralLimit = 300, /* 积分输出限幅参数，单位 U；需 Improve 含 PID_Integral_Limit 才参与限幅。 */` |
| [141](../config/01_pid/pid_config.c#L141) | 配置初值 | `.DeadBand = 0, /* 误差死区，单位 E；误差绝对值不超过此值时输出归零。 */` |
| [142](../config/01_pid/pid_config.c#L142) | 配置初值 | `.Kp = 1000, /* 比例增益，单位 U/E；乘以当前误差得到比例输出。 */` |
| [143](../config/01_pid/pid_config.c#L143) | 配置初值 | `.Ki = 20, /* 积分增益，单位 U/(E·s)；设为 0 时不累积新的积分输出。 */` |
| [144](../config/01_pid/pid_config.c#L144) | 配置初值 | `.Kd = 0, /* 微分增益，单位 U·s/E；当前默认 0，不产生微分输出。 */` |
| [145](../config/01_pid/pid_config.c#L145) | 配置初值 | `.Improve = 0x01, /* 默认 0x01：仅开启积分限幅；改为 0 关闭，或用 &#124; 加入其他改善标志。 */` |

## config/01_pid/pid_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/01_pid/pid_config.h#L1) | 预处理 | `#ifndef PID_CONFIG_H` |
| [2](../config/01_pid/pid_config.h#L2) | 预处理 | `#define PID_CONFIG_H` |
| [20](../config/01_pid/pid_config.h#L20) | 预处理 | `#endif` |

## config/02_chassis/chassis_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/02_chassis/chassis_config.h#L1) | 预处理 | `#ifndef CHASSIS_CONFIG_H` |
| [2](../config/02_chassis/chassis_config.h#L2) | 预处理 | `#define CHASSIS_CONFIG_H` |
| [15](../config/02_chassis/chassis_config.h#L15) | 预处理 | `#ifndef CHASSIS_POWER_BENCH_TEST` |
| [17](../config/02_chassis/chassis_config.h#L17) | 预处理 | `#define CHASSIS_POWER_BENCH_TEST 0` |
| [18](../config/02_chassis/chassis_config.h#L18) | 预处理 | `#endif` |
| [19](../config/02_chassis/chassis_config.h#L19) | 预处理 | `#ifndef CHASSIS_POWER_BENCH_LIMIT_W` |
| [21](../config/02_chassis/chassis_config.h#L21) | 预处理 | `#define CHASSIS_POWER_BENCH_LIMIT_W 50.0f // 底盘功率回退限制，单位 W` |
| [22](../config/02_chassis/chassis_config.h#L22) | 预处理 | `#endif` |
| [24](../config/02_chassis/chassis_config.h#L24) | 预处理 | `#if CHASSIS_POWER_BENCH_TEST != 0 && CHASSIS_POWER_BENCH_TEST != 1` |
| [26](../config/02_chassis/chassis_config.h#L26) | 预处理 | `#endif` |
| [33](../config/02_chassis/chassis_config.h#L33) | 预处理 | `#define POWER_METER_ENABLE_IS_VALID_0 1// 0 表示不启用功率计` |
| [34](../config/02_chassis/chassis_config.h#L34) | 预处理 | `#define POWER_METER_ENABLE_IS_VALID_1 1// 1 表示启用功率计` |
| [35](../config/02_chassis/chassis_config.h#L35) | 预处理 | `#define POWER_METER_ENABLE_IS_VALID_IMPL(value) POWER_METER_ENABLE_IS_VALID_##value` |
| [36](../config/02_chassis/chassis_config.h#L36) | 预处理 | `#define POWER_METER_ENABLE_IS_VALID(value) POWER_METER_ENABLE_IS_VALID_IMPL(value)` |
| [38](../config/02_chassis/chassis_config.h#L38) | 预处理 | `#if !POWER_METER_ENABLE_IS_VALID(POWER_METER_ENABLE)` |
| [40](../config/02_chassis/chassis_config.h#L40) | 预处理 | `#endif` |
| [50](../config/02_chassis/chassis_config.h#L50) | 预处理 | `#define CHASSIS_DRIVE_MECANUM 0` |
| [52](../config/02_chassis/chassis_config.h#L52) | 预处理 | `#define CHASSIS_DRIVE_OMNI_X 1` |
| [53](../config/02_chassis/chassis_config.h#L53) | 预处理 | `#ifndef CHASSIS_DRIVE_TYPE` |
| [55](../config/02_chassis/chassis_config.h#L55) | 预处理 | `#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_OMNI_X` |
| [56](../config/02_chassis/chassis_config.h#L56) | 预处理 | `#endif` |
| [58](../config/02_chassis/chassis_config.h#L58) | 预处理 | `#if CHASSIS_DRIVE_TYPE != CHASSIS_DRIVE_MECANUM && \` |
| [61](../config/02_chassis/chassis_config.h#L61) | 预处理 | `#endif` |
| [64](../config/02_chassis/chassis_config.h#L64) | 预处理 | `#ifndef CHASSIS_FOLLOW_MODE` |
| [66](../config/02_chassis/chassis_config.h#L66) | 预处理 | `#define CHASSIS_FOLLOW_MODE 0` |
| [67](../config/02_chassis/chassis_config.h#L67) | 预处理 | `#endif` |
| [68](../config/02_chassis/chassis_config.h#L68) | 预处理 | `#if CHASSIS_FOLLOW_MODE != 0 && CHASSIS_FOLLOW_MODE != 1` |
| [70](../config/02_chassis/chassis_config.h#L70) | 预处理 | `#endif` |
| [73](../config/02_chassis/chassis_config.h#L73) | 预处理 | `#ifndef WHEEL_BASE` |
| [75](../config/02_chassis/chassis_config.h#L75) | 预处理 | `#define WHEEL_BASE 350              // 纵向轴距(前进后退方向)` |
| [76](../config/02_chassis/chassis_config.h#L76) | 预处理 | `#endif` |
| [77](../config/02_chassis/chassis_config.h#L77) | 预处理 | `#ifndef TRACK_WIDTH` |
| [79](../config/02_chassis/chassis_config.h#L79) | 预处理 | `#define TRACK_WIDTH 300             // 横向轮距(左右平移方向)` |
| [80](../config/02_chassis/chassis_config.h#L80) | 预处理 | `#endif` |
| [81](../config/02_chassis/chassis_config.h#L81) | 预处理 | `#ifndef CENTER_GIMBAL_OFFSET_X` |
| [83](../config/02_chassis/chassis_config.h#L83) | 预处理 | `#define CENTER_GIMBAL_OFFSET_X 0    // 云台旋转中心距底盘几何中心的距离,前后方向,云台位于正中心时默认设为0` |
| [84](../config/02_chassis/chassis_config.h#L84) | 预处理 | `#endif` |
| [85](../config/02_chassis/chassis_config.h#L85) | 预处理 | `#ifndef CENTER_GIMBAL_OFFSET_Y` |
| [87](../config/02_chassis/chassis_config.h#L87) | 预处理 | `#define CENTER_GIMBAL_OFFSET_Y 0    // 云台旋转中心距底盘几何中心的距离,左右方向,云台位于正中心时默认设为0` |
| [88](../config/02_chassis/chassis_config.h#L88) | 预处理 | `#endif` |
| [89](../config/02_chassis/chassis_config.h#L89) | 预处理 | `#ifndef RADIUS_WHEEL` |
| [91](../config/02_chassis/chassis_config.h#L91) | 预处理 | `#define RADIUS_WHEEL 60             // 轮子半径` |
| [92](../config/02_chassis/chassis_config.h#L92) | 预处理 | `#endif` |
| [93](../config/02_chassis/chassis_config.h#L93) | 预处理 | `#ifndef REDUCTION_RATIO_WHEEL` |
| [95](../config/02_chassis/chassis_config.h#L95) | 预处理 | `#define REDUCTION_RATIO_WHEEL 19.0f // 电机减速比,因为编码器量测的是转子的速度而不是输出轴的速度故需进行转换` |
| [96](../config/02_chassis/chassis_config.h#L96) | 预处理 | `#endif` |
| [99](../config/02_chassis/chassis_config.h#L99) | 预处理 | `#ifndef OMNI_X_LF_DIRECTION` |
| [101](../config/02_chassis/chassis_config.h#L101) | 预处理 | `#define OMNI_X_LF_DIRECTION 1.0f` |
| [102](../config/02_chassis/chassis_config.h#L102) | 预处理 | `#endif` |
| [103](../config/02_chassis/chassis_config.h#L103) | 预处理 | `#ifndef OMNI_X_RF_DIRECTION` |
| [105](../config/02_chassis/chassis_config.h#L105) | 预处理 | `#define OMNI_X_RF_DIRECTION 1.0f` |
| [106](../config/02_chassis/chassis_config.h#L106) | 预处理 | `#endif` |
| [107](../config/02_chassis/chassis_config.h#L107) | 预处理 | `#ifndef OMNI_X_LB_DIRECTION` |
| [109](../config/02_chassis/chassis_config.h#L109) | 预处理 | `#define OMNI_X_LB_DIRECTION 1.0f` |
| [110](../config/02_chassis/chassis_config.h#L110) | 预处理 | `#endif` |
| [111](../config/02_chassis/chassis_config.h#L111) | 预处理 | `#ifndef OMNI_X_RB_DIRECTION` |
| [113](../config/02_chassis/chassis_config.h#L113) | 预处理 | `#define OMNI_X_RB_DIRECTION 1.0f` |
| [114](../config/02_chassis/chassis_config.h#L114) | 预处理 | `#endif` |
| [116](../config/02_chassis/chassis_config.h#L116) | 预处理 | `#endif` |

## config/03_gimbal/gimbal_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/03_gimbal/gimbal_config.h#L1) | 预处理 | `#ifndef GIMBAL_CONFIG_H` |
| [2](../config/03_gimbal/gimbal_config.h#L2) | 预处理 | `#define GIMBAL_CONFIG_H` |
| [11](../config/03_gimbal/gimbal_config.h#L11) | 预处理 | `#ifndef YAW_CHASSIS_ALIGN_ECD` |
| [13](../config/03_gimbal/gimbal_config.h#L13) | 预处理 | `#define YAW_CHASSIS_ALIGN_ECD 350// 云台和底盘对齐指向相同方向时的电机编码器值,若对云台有机械改动需要修改` |
| [14](../config/03_gimbal/gimbal_config.h#L14) | 预处理 | `#endif` |
| [15](../config/03_gimbal/gimbal_config.h#L15) | 预处理 | `#ifndef YAW_ECD_GREATER_THAN_4096` |
| [17](../config/03_gimbal/gimbal_config.h#L17) | 预处理 | `#define YAW_ECD_GREATER_THAN_4096 0 // ALIGN_ECD值是否大于4096,是为1,否为0;用于计算云台偏转角度` |
| [18](../config/03_gimbal/gimbal_config.h#L18) | 预处理 | `#endif` |
| [19](../config/03_gimbal/gimbal_config.h#L19) | 预处理 | `#ifndef PITCH_HORIZON_ECD` |
| [21](../config/03_gimbal/gimbal_config.h#L21) | 预处理 | `#define PITCH_HORIZON_ECD 3412      // 云台处于水平位置时编码器值,若对云台有机械改动需要修改` |
| [22](../config/03_gimbal/gimbal_config.h#L22) | 预处理 | `#endif` |
| [23](../config/03_gimbal/gimbal_config.h#L23) | 预处理 | `#ifndef PITCH_MAX_ANGLE` |
| [25](../config/03_gimbal/gimbal_config.h#L25) | 预处理 | `#define PITCH_MAX_ANGLE 30           // 云台竖直方向最大角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)` |
| [26](../config/03_gimbal/gimbal_config.h#L26) | 预处理 | `#endif` |
| [27](../config/03_gimbal/gimbal_config.h#L27) | 预处理 | `#ifndef PITCH_MIN_ANGLE` |
| [29](../config/03_gimbal/gimbal_config.h#L29) | 预处理 | `#define PITCH_MIN_ANGLE -30           // 云台竖直方向最小角度 (注意反馈如果是陀螺仪，则填写陀螺仪的角度)` |
| [30](../config/03_gimbal/gimbal_config.h#L30) | 预处理 | `#endif` |
| [31](../config/03_gimbal/gimbal_config.h#L31) | 预处理 | `#ifndef GYRO2GIMBAL_DIR_YAW` |
| [33](../config/03_gimbal/gimbal_config.h#L33) | 预处理 | `#define GYRO2GIMBAL_DIR_YAW 1   // 陀螺仪数据相较于云台的yaw的方向,1为相同,-1为相反` |
| [34](../config/03_gimbal/gimbal_config.h#L34) | 预处理 | `#endif` |
| [35](../config/03_gimbal/gimbal_config.h#L35) | 预处理 | `#ifndef GYRO2GIMBAL_DIR_PITCH` |
| [37](../config/03_gimbal/gimbal_config.h#L37) | 预处理 | `#define GYRO2GIMBAL_DIR_PITCH 1 // 陀螺仪数据相较于云台的pitch的方向,1为相同,-1为相反` |
| [38](../config/03_gimbal/gimbal_config.h#L38) | 预处理 | `#endif` |
| [39](../config/03_gimbal/gimbal_config.h#L39) | 预处理 | `#ifndef GYRO2GIMBAL_DIR_ROLL` |
| [41](../config/03_gimbal/gimbal_config.h#L41) | 预处理 | `#define GYRO2GIMBAL_DIR_ROLL 1  // 陀螺仪数据相较于云台的roll的方向,1为相同,-1为相反` |
| [42](../config/03_gimbal/gimbal_config.h#L42) | 预处理 | `#endif` |
| [46](../config/03_gimbal/gimbal_config.h#L46) | 预处理 | `#define YAW_MAX_TRACK_ERROR 60.0f` |
| [48](../config/03_gimbal/gimbal_config.h#L48) | 预处理 | `#endif` |

## config/04_shoot/heat_control_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/04_shoot/heat_control_config.h#L1) | 预处理 | `#ifndef HEAT_CONTROL_CONFIG_H` |
| [2](../config/04_shoot/heat_control_config.h#L2) | 预处理 | `#define HEAT_CONTROL_CONFIG_H` |
| [10](../config/04_shoot/heat_control_config.h#L10) | 预处理 | `#define HEAT_CONTROL_BULLET_HEAT 10.0f` |
| [12](../config/04_shoot/heat_control_config.h#L12) | 预处理 | `#define HEAT_CONTROL_DEFAULT_MAX_RATE 30.0f` |
| [14](../config/04_shoot/heat_control_config.h#L14) | 预处理 | `#define HEAT_CONTROL_RESERVE_SHOTS 7.0f /* Delay reserve; see integration validation report. */` |
| [16](../config/04_shoot/heat_control_config.h#L16) | 预处理 | `#define HEAT_CONTROL_SETTLE_PERIOD_S 0.1f /* 裁判系统按 10 Hz 结算冷却 */` |
| [18](../config/04_shoot/heat_control_config.h#L18) | 预处理 | `#define HEAT_CONTROL_PLAN_PERIOD_S 0.5   /* 热量规划窗口；越长越早降速，与冷却周期独立 */` |
| [20](../config/04_shoot/heat_control_config.h#L20) | 预处理 | `#endif` |

## config/04_shoot/shoot_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/04_shoot/shoot_config.h#L1) | 预处理 | `#ifndef SHOOT_CONFIG_H` |
| [2](../config/04_shoot/shoot_config.h#L2) | 预处理 | `#define SHOOT_CONFIG_H` |
| [11](../config/04_shoot/shoot_config.h#L11) | 预处理 | `#ifndef ONE_BULLET_DELTA_ANGLE` |
| [13](../config/04_shoot/shoot_config.h#L13) | 预处理 | `#define ONE_BULLET_DELTA_ANGLE 45    // 发射一发弹丸拨盘转动的距离,由机械设计图纸给出` |
| [14](../config/04_shoot/shoot_config.h#L14) | 预处理 | `#endif` |
| [15](../config/04_shoot/shoot_config.h#L15) | 预处理 | `#ifndef REDUCTION_RATIO_LOADER` |
| [17](../config/04_shoot/shoot_config.h#L17) | 预处理 | `#define REDUCTION_RATIO_LOADER 36.0f // 2006拨盘电机的减速比,英雄需要修改为3508的19.0f` |
| [18](../config/04_shoot/shoot_config.h#L18) | 预处理 | `#endif` |
| [19](../config/04_shoot/shoot_config.h#L19) | 预处理 | `#ifndef NUM_PER_CIRCLE` |
| [21](../config/04_shoot/shoot_config.h#L21) | 预处理 | `#define NUM_PER_CIRCLE 8            // 拨盘一圈的装载量` |
| [22](../config/04_shoot/shoot_config.h#L22) | 预处理 | `#endif` |
| [26](../config/04_shoot/shoot_config.h#L26) | 预处理 | `#define SHOOT_LOADER_ONE_BULLET_MOTOR_ANGLE (ONE_BULLET_DELTA_ANGLE * REDUCTION_RATIO_LOADER)` |
| [28](../config/04_shoot/shoot_config.h#L28) | 预处理 | `#define SHOOT_LOADER_ANGLE_TOLERANCE 5.0f` |
| [32](../config/04_shoot/shoot_config.h#L32) | 预处理 | `#define SHOOT_FRICTION_SPEED_15 4000` |
| [34](../config/04_shoot/shoot_config.h#L34) | 预处理 | `#define SHOOT_FRICTION_SPEED_18 32000` |
| [36](../config/04_shoot/shoot_config.h#L36) | 预处理 | `#define SHOOT_FRICTION_SPEED_30 8000` |
| [38](../config/04_shoot/shoot_config.h#L38) | 预处理 | `#define SHOOT_FRICTION_SPEED_DEFAULT 32000` |
| [40](../config/04_shoot/shoot_config.h#L40) | 预处理 | `#define SHOOT_FRICTION_SPEED_TOLERANCE_RATIO .1f` |
| [42](../config/04_shoot/shoot_config.h#L42) | 预处理 | `#define SHOOT_FRICTION_STABLE_TIME_MS 100U` |
| [46](../config/04_shoot/shoot_config.h#L46) | 预处理 | `#define SHOOT_MOTOR_FEEDBACK_MAX_AGE_MS 20U` |
| [48](../config/04_shoot/shoot_config.h#L48) | 预处理 | `#define SHOOT_HEAT_FEEDBACK_MAX_AGE_MS 300U` |
| [50](../config/04_shoot/shoot_config.h#L50) | 预处理 | `#define SHOOT_HEAT_SETTLE_DELAY_MS 300U` |
| [52](../config/04_shoot/shoot_config.h#L52) | 预处理 | `#endif` |

## config/05_remote/remote_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/05_remote/remote_config.h#L1) | 预处理 | `#ifndef REMOTE_CONFIG_H` |
| [2](../config/05_remote/remote_config.h#L2) | 预处理 | `#define REMOTE_CONFIG_H` |
| [11](../config/05_remote/remote_config.h#L11) | 预处理 | `#define REMOTE_STICK_YAW_GAIN 0.005f` |
| [13](../config/05_remote/remote_config.h#L13) | 预处理 | `#define REMOTE_STICK_PITCH_GAIN 0.001f` |
| [15](../config/05_remote/remote_config.h#L15) | 预处理 | `#define REMOTE_STICK_CHASSIS_GAIN 40.0f` |
| [17](../config/05_remote/remote_config.h#L17) | 预处理 | `#define REMOTE_STICK_FULL_SCALE 660` |
| [21](../config/05_remote/remote_config.h#L21) | 预处理 | `#define REMOTE_DIAL_FRICTION_THRESHOLD (-100)` |
| [23](../config/05_remote/remote_config.h#L23) | 预处理 | `#define REMOTE_DIAL_SHOOT_THRESHOLD (-200)` |
| [25](../config/05_remote/remote_config.h#L25) | 预处理 | `#define REMOTE_DIAL_EMERGENCY_THRESHOLD 300` |
| [27](../config/05_remote/remote_config.h#L27) | 预处理 | `#define REMOTE_NEUTRAL_THRESHOLD 30` |
| [31](../config/05_remote/remote_config.h#L31) | 预处理 | `#define REMOTE_KEY_CHASSIS_SPEED 300` |
| [33](../config/05_remote/remote_config.h#L33) | 预处理 | `#define REMOTE_VTX_KEY_CHASSIS_SPEED 3000` |
| [35](../config/05_remote/remote_config.h#L35) | 预处理 | `#define REMOTE_MOUSE_FULL_SCALE 660` |
| [37](../config/05_remote/remote_config.h#L37) | 预处理 | `#define REMOTE_MOUSE_YAW_GAIN 10` |
| [39](../config/05_remote/remote_config.h#L39) | 预处理 | `#define REMOTE_MOUSE_PITCH_GAIN 10` |
| [43](../config/05_remote/remote_config.h#L43) | 预处理 | `#define REMOTE_CHASSIS_SPEED_LEVEL_1 40` |
| [45](../config/05_remote/remote_config.h#L45) | 预处理 | `#define REMOTE_CHASSIS_SPEED_LEVEL_2 60` |
| [47](../config/05_remote/remote_config.h#L47) | 预处理 | `#define REMOTE_CHASSIS_SPEED_LEVEL_3 80` |
| [49](../config/05_remote/remote_config.h#L49) | 预处理 | `#define REMOTE_CHASSIS_SPEED_LEVEL_4 100` |
| [53](../config/05_remote/remote_config.h#L53) | 预处理 | `#define REMOTE_INPUT_MAX_AGE_MS 100U` |
| [55](../config/05_remote/remote_config.h#L55) | 预处理 | `#define REMOTE_YAW_FEEDBACK_MAX_AGE_MS 20U` |
| [57](../config/05_remote/remote_config.h#L57) | 预处理 | `#define REMOTE_BOARD_FEEDBACK_MAX_AGE_MS 10U` |
| [59](../config/05_remote/remote_config.h#L59) | 预处理 | `#define REMOTE_BOARD_OFFLINE_RELOAD_COUNT 10` |
| [61](../config/05_remote/remote_config.h#L61) | 预处理 | `#endif` |

## config/06_robot/robot_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/06_robot/robot_config.h#L1) | 预处理 | `#ifndef ROBOT_CONFIG_H` |
| [2](../config/06_robot/robot_config.h#L2) | 预处理 | `#define ROBOT_CONFIG_H` |
| [18](../config/06_robot/robot_config.h#L18) | 预处理 | `#if !defined(ONE_BOARD) && !defined(CHASSIS_BOARD) && !defined(GIMBAL_BOARD)` |
| [20](../config/06_robot/robot_config.h#L20) | 预处理 | `#define ONE_BOARD // 单板控制整车` |
| [21](../config/06_robot/robot_config.h#L21) | 预处理 | `#endif` |
| [25](../config/06_robot/robot_config.h#L25) | 预处理 | `#if !defined(VISION_USE_VCP) && !defined(VISION_USE_UART)` |
| [27](../config/06_robot/robot_config.h#L27) | 预处理 | `#define VISION_USE_UART` |
| [28](../config/06_robot/robot_config.h#L28) | 预处理 | `#endif` |
| [29](../config/06_robot/robot_config.h#L29) | 预处理 | `#if defined(VISION_USE_VCP) && defined(VISION_USE_UART)` |
| [31](../config/06_robot/robot_config.h#L31) | 预处理 | `#endif` |
| [33](../config/06_robot/robot_config.h#L33) | 预处理 | `#if defined(VISION_USE_VCP) && USB_TELEMETRY_ENABLE` |
| [35](../config/06_robot/robot_config.h#L35) | 预处理 | `#endif` |
| [39](../config/06_robot/robot_config.h#L39) | 预处理 | `#if (defined(ONE_BOARD) && defined(CHASSIS_BOARD)) &#124;&#124; \` |
| [43](../config/06_robot/robot_config.h#L43) | 预处理 | `#endif` |
| [45](../config/06_robot/robot_config.h#L45) | 预处理 | `#endif` |

## config/20_chassis_power/chassis_energy_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/20_chassis_power/chassis_energy_config.h#L1) | 预处理 | `#ifndef CHASSIS_ENERGY_CONFIG_H` |
| [2](../config/20_chassis_power/chassis_energy_config.h#L2) | 预处理 | `#define CHASSIS_ENERGY_CONFIG_H` |
| [9](../config/20_chassis_power/chassis_energy_config.h#L9) | 预处理 | `#ifndef CHASSIS_BUFFER_POWER_ENABLE` |
| [10](../config/20_chassis_power/chassis_energy_config.h#L10) | 预处理 | `#define CHASSIS_BUFFER_POWER_ENABLE 1 /* 缓冲增补开关：默认 1 启用，0 关闭；不改变裁判原始功率限额。 */` |
| [11](../config/20_chassis_power/chassis_energy_config.h#L11) | 预处理 | `#endif` |
| [13](../config/20_chassis_power/chassis_energy_config.h#L13) | 预处理 | `#define CHASSIS_POWER_SOURCE_DIRECT 0 /* 供电模式编号：0，底盘直接供电。 */` |
| [14](../config/20_chassis_power/chassis_energy_config.h#L14) | 预处理 | `#define CHASSIS_POWER_SOURCE_SUPERCAP 1 /* 供电模式编号：1，由匹配协议的超级电容供电。 */` |
| [17](../config/20_chassis_power/chassis_energy_config.h#L17) | 预处理 | `#ifndef CHASSIS_POWER_SOURCE_MODE` |
| [18](../config/20_chassis_power/chassis_energy_config.h#L18) | 预处理 | `#define CHASSIS_POWER_SOURCE_MODE CHASSIS_POWER_SOURCE_DIRECT /* 供电选择：默认 0 直供；1 超级电容，须与实际接线及 F334 固件一致。 */` |
| [19](../config/20_chassis_power/chassis_energy_config.h#L19) | 预处理 | `#endif` |
| [21](../config/20_chassis_power/chassis_energy_config.h#L21) | 预处理 | `#if CHASSIS_BUFFER_POWER_ENABLE != 0 && CHASSIS_BUFFER_POWER_ENABLE != 1` |
| [23](../config/20_chassis_power/chassis_energy_config.h#L23) | 预处理 | `#endif` |
| [24](../config/20_chassis_power/chassis_energy_config.h#L24) | 预处理 | `#if CHASSIS_POWER_SOURCE_MODE != CHASSIS_POWER_SOURCE_DIRECT && \` |
| [27](../config/20_chassis_power/chassis_energy_config.h#L27) | 预处理 | `#endif` |
| [31](../config/20_chassis_power/chassis_energy_config.h#L31) | 预处理 | `#ifndef CHASSIS_BUFFER_MAX_ENERGY_J` |
| [32](../config/20_chassis_power/chassis_energy_config.h#L32) | 预处理 | `#define CHASSIS_BUFFER_MAX_ENERGY_J 60.0f       // 缓冲能量满值(J):裁判原始读数超过它即判缓冲数据无效,不再参与增补` |
| [33](../config/20_chassis_power/chassis_energy_config.h#L33) | 预处理 | `#endif` |
| [34](../config/20_chassis_power/chassis_energy_config.h#L34) | 预处理 | `#ifndef CHASSIS_BUFFER_RECOVERY_ENTER_J` |
| [35](../config/20_chassis_power/chassis_energy_config.h#L35) | 预处理 | `#define CHASSIS_BUFFER_RECOVERY_ENTER_J 10.0f   // 进入回充(J):估算下限降到它或以下就转入恢复` |
| [36](../config/20_chassis_power/chassis_energy_config.h#L36) | 预处理 | `#endif` |
| [37](../config/20_chassis_power/chassis_energy_config.h#L37) | 预处理 | `#ifndef CHASSIS_BUFFER_RECOVERY_EXIT_J` |
| [38](../config/20_chassis_power/chassis_energy_config.h#L38) | 预处理 | `#define CHASSIS_BUFFER_RECOVERY_EXIT_J 20.0f    // 退出回充(J):估算下限回到它或以上并保持 RECOVERY_HOLD_S 才解除` |
| [39](../config/20_chassis_power/chassis_energy_config.h#L39) | 预处理 | `#endif` |
| [40](../config/20_chassis_power/chassis_energy_config.h#L40) | 预处理 | `#ifndef CHASSIS_BUFFER_HARD_FLOOR_J` |
| [41](../config/20_chassis_power/chassis_energy_config.h#L41) | 预处理 | `#define CHASSIS_BUFFER_HARD_FLOOR_J 5.0f        // 能量硬底座(J):预测值跌破它强制转入恢复,也是增补功率的保底基准` |
| [42](../config/20_chassis_power/chassis_energy_config.h#L42) | 预处理 | `#endif` |
| [43](../config/20_chassis_power/chassis_energy_config.h#L43) | 预处理 | `#ifndef CHASSIS_BUFFER_PREDICTION_WINDOW_S` |
| [44](../config/20_chassis_power/chassis_energy_config.h#L44) | 预处理 | `#define CHASSIS_BUFFER_PREDICTION_WINDOW_S 0.25f // 能量释放时间(s)` |
| [45](../config/20_chassis_power/chassis_energy_config.h#L45) | 预处理 | `#endif` |
| [46](../config/20_chassis_power/chassis_energy_config.h#L46) | 预处理 | `#ifndef CHASSIS_BUFFER_GUARD_WINDOW_S` |
| [47](../config/20_chassis_power/chassis_energy_config.h#L47) | 预处理 | `#define CHASSIS_BUFFER_GUARD_WINDOW_S 0.3f      // 保护窗口(s):把超限输入功率和硬底座余量换算成该时长内的能量预算` |
| [48](../config/20_chassis_power/chassis_energy_config.h#L48) | 预处理 | `#endif` |
| [49](../config/20_chassis_power/chassis_energy_config.h#L49) | 预处理 | `#ifndef CHASSIS_BUFFER_SETTLEMENT_DELAY_S` |
| [50](../config/20_chassis_power/chassis_energy_config.h#L50) | 预处理 | `#define CHASSIS_BUFFER_SETTLEMENT_DELAY_S 0.1f  // 结算延迟(s):裁判帧接收时刻不等于结算时刻,额外预留这段时间的消耗` |
| [51](../config/20_chassis_power/chassis_energy_config.h#L51) | 预处理 | `#endif` |
| [52](../config/20_chassis_power/chassis_energy_config.h#L52) | 预处理 | `#ifndef CHASSIS_BUFFER_ENERGY_MARGIN_J` |
| [53](../config/20_chassis_power/chassis_energy_config.h#L53) | 预处理 | `#define CHASSIS_BUFFER_ENERGY_MARGIN_J 3.0f     // 能量安全余量(J):计算估算下限时直接扣除` |
| [54](../config/20_chassis_power/chassis_energy_config.h#L54) | 预处理 | `#endif` |
| [55](../config/20_chassis_power/chassis_energy_config.h#L55) | 预处理 | `#ifndef CHASSIS_BUFFER_POWER_MARGIN_W` |
| [56](../config/20_chassis_power/chassis_energy_config.h#L56) | 预处理 | `#define CHASSIS_BUFFER_POWER_MARGIN_W 5.0f      // 功率安全余量(W):估算下限按它预留消耗,判输入超限和增补时也扣掉` |
| [57](../config/20_chassis_power/chassis_energy_config.h#L57) | 预处理 | `#endif` |
| [58](../config/20_chassis_power/chassis_energy_config.h#L58) | 预处理 | `#ifndef CHASSIS_BUFFER_BOOST_MAX_W` |
| [59](../config/20_chassis_power/chassis_energy_config.h#L59) | 预处理 | `#define CHASSIS_BUFFER_BOOST_MAX_W 60.0f        // 输入预算增补上限(W):注意与外层 max_boost_w(电容负载增补上限)不是同一个量` |
| [60](../config/20_chassis_power/chassis_energy_config.h#L60) | 预处理 | `#endif` |
| [61](../config/20_chassis_power/chassis_energy_config.h#L61) | 预处理 | `#ifndef CHASSIS_BUFFER_RECOVERY_POWER_W` |
| [62](../config/20_chassis_power/chassis_energy_config.h#L62) | 预处理 | `#define CHASSIS_BUFFER_RECOVERY_POWER_W 20.0f   // 最大回充减让(W):按能量缺口调节,逼近硬底座时使用全部减让` |
| [63](../config/20_chassis_power/chassis_energy_config.h#L63) | 预处理 | `#endif` |
| [64](../config/20_chassis_power/chassis_energy_config.h#L64) | 预处理 | `#ifndef CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S` |
| [65](../config/20_chassis_power/chassis_energy_config.h#L65) | 预处理 | `#define CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S 1.0f // 目标恢复时间(s):能量缺口除以该时间得到回充减让功率` |
| [66](../config/20_chassis_power/chassis_energy_config.h#L66) | 预处理 | `#endif` |
| [67](../config/20_chassis_power/chassis_energy_config.h#L67) | 预处理 | `#ifndef CHASSIS_BUFFER_OBSERVER_MAX_STEP_S` |
| [68](../config/20_chassis_power/chassis_energy_config.h#L68) | 预处理 | `#define CHASSIS_BUFFER_OBSERVER_MAX_STEP_S 0.020f // 最大连续积分间隔(s):超出时按最坏耗能扣减并撤销增补` |
| [69](../config/20_chassis_power/chassis_energy_config.h#L69) | 预处理 | `#endif` |
| [70](../config/20_chassis_power/chassis_energy_config.h#L70) | 预处理 | `#ifndef CHASSIS_BUFFER_RECOVERY_HOLD_S` |
| [71](../config/20_chassis_power/chassis_energy_config.h#L71) | 预处理 | `#define CHASSIS_BUFFER_RECOVERY_HOLD_S 0.1f     // 退出回充的确认时长(s):在该时长内持续高于退出门槛才解除` |
| [72](../config/20_chassis_power/chassis_energy_config.h#L72) | 预处理 | `#endif` |
| [73](../config/20_chassis_power/chassis_energy_config.h#L73) | 预处理 | `#ifndef CHASSIS_BUFFER_RISE_RATE_W_PER_S` |
| [74](../config/20_chassis_power/chassis_energy_config.h#L74) | 预处理 | `#define CHASSIS_BUFFER_RISE_RATE_W_PER_S 1200.0f // 输入预算上升斜率(W/s);电容负载侧使用独立斜率,下降立即生效` |
| [75](../config/20_chassis_power/chassis_energy_config.h#L75) | 预处理 | `#endif` |
| [76](../config/20_chassis_power/chassis_energy_config.h#L76) | 预处理 | `#ifndef CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S` |
| [77](../config/20_chassis_power/chassis_energy_config.h#L77) | 预处理 | `#define CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S 20.0f // 新裁判帧向上校正估算的上限(J/s);向下立即校正` |
| [78](../config/20_chassis_power/chassis_energy_config.h#L78) | 预处理 | `#endif` |
| [79](../config/20_chassis_power/chassis_energy_config.h#L79) | 预处理 | `#ifndef CHASSIS_POWER_DEMAND_MARGIN_W` |
| [80](../config/20_chassis_power/chassis_energy_config.h#L80) | 预处理 | `#define CHASSIS_POWER_DEMAND_MARGIN_W 5.0f     // 高于基础功率的真实驱动需求所允许的额外余量(W)` |
| [81](../config/20_chassis_power/chassis_energy_config.h#L81) | 预处理 | `#endif` |
| [84](../config/20_chassis_power/chassis_energy_config.h#L84) | 预处理 | `#define CHASSIS_SPIN_BASE_LPF_S 0.050f /* 自旋诊断功率低通时间常数，单位 s；不决定缓冲增补准入。 */` |
| [86](../config/20_chassis_power/chassis_energy_config.h#L86) | 预处理 | `#endif` |

## config/20_chassis_power/chassis_power_config.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [9](../config/20_chassis_power/chassis_power_config.c#L9) | 配置初值 | `ChassisPowerBudgetConfig_s chassis_budget_config = {` |
| [10](../config/20_chassis_power/chassis_power_config.c#L10) | 配置初值 | `.empty_energy_ratio = 0.10f, /* 电容增补能力开始开放的能量比例，无单位；0～1，低于它不开放额外负载功率。 */` |
| [11](../config/20_chassis_power/chassis_power_config.c#L11) | 配置初值 | `.full_energy_ratio = 0.30f, /* 电容增补完全开放的能量比例，无单位；应高于空电门槛。 */` |
| [12](../config/20_chassis_power/chassis_power_config.c#L12) | 配置初值 | `.max_boost_w = 100.0f, /* 增补功率上限，单位 W；外层限制电容负载侧，buffer 内层限制裁判缓冲的输入侧。 */` |
| [13](../config/20_chassis_power/chassis_power_config.c#L13) | 配置初值 | `.rise_rate_w_per_s = 200.0f, /* 预算上升斜率，单位 W/s；外层用于负载侧，buffer 内层用于输入侧，下降立即生效。 */` |
| [14](../config/20_chassis_power/chassis_power_config.c#L14) | 配置初值 | `.demand_margin_w = CHASSIS_POWER_DEMAND_MARGIN_W, /* 高于基础预算的有效驱动需求所允许的申请余量，单位 W。 */` |
| [15](../config/20_chassis_power/chassis_power_config.c#L15) | 配置初值 | `.buffer = { /* 裁判缓冲能量配置；具体宏值统一在 chassis_energy_config.h 修改。 */` |
| [16](../config/20_chassis_power/chassis_power_config.c#L16) | 配置初值 | `.max_energy_j = CHASSIS_BUFFER_MAX_ENERGY_J, /* 裁判缓冲原始读数的有效上限，单位 J。 */` |
| [17](../config/20_chassis_power/chassis_power_config.c#L17) | 配置初值 | `.recovery_enter_j = CHASSIS_BUFFER_RECOVERY_ENTER_J, /* 保守能量估计不高于此值时进入回充，单位 J。 */` |
| [18](../config/20_chassis_power/chassis_power_config.c#L18) | 配置初值 | `.recovery_exit_j = CHASSIS_BUFFER_RECOVERY_EXIT_J, /* 退出回充的保守能量门槛，单位 J；还须满足连续保持时间。 */` |
| [19](../config/20_chassis_power/chassis_power_config.c#L19) | 配置初值 | `.hard_floor_j = CHASSIS_BUFFER_HARD_FLOOR_J, /* 预测能量硬底座，单位 J；低于它强制回充。 */` |
| [20](../config/20_chassis_power/chassis_power_config.c#L20) | 配置初值 | `.prediction_window_s = CHASSIS_BUFFER_PREDICTION_WINDOW_S, /* 缓冲能量释放与输入增补预测窗口，单位 s。 */` |
| [21](../config/20_chassis_power/chassis_power_config.c#L21) | 配置初值 | `.guard_window_s = CHASSIS_BUFFER_GUARD_WINDOW_S, /* 超限输入功率换算能量时使用的保护窗口，单位 s。 */` |
| [22](../config/20_chassis_power/chassis_power_config.c#L22) | 配置初值 | `.settlement_delay_s = CHASSIS_BUFFER_SETTLEMENT_DELAY_S, /* 裁判结算时刻相对收帧时刻的延迟裕量，单位 s。 */` |
| [23](../config/20_chassis_power/chassis_power_config.c#L23) | 配置初值 | `.energy_margin_j = CHASSIS_BUFFER_ENERGY_MARGIN_J, /* 从缓冲估计值中扣除的能量余量，单位 J。 */` |
| [24](../config/20_chassis_power/chassis_power_config.c#L24) | 配置初值 | `.power_margin_w = CHASSIS_BUFFER_POWER_MARGIN_W, /* 输入功率观测与预算预留的功率余量，单位 W。 */` |
| [25](../config/20_chassis_power/chassis_power_config.c#L25) | 配置初值 | `.max_boost_w = CHASSIS_BUFFER_BOOST_MAX_W, /* 增补功率上限，单位 W；外层限制电容负载侧，buffer 内层限制裁判缓冲的输入侧。 */` |
| [26](../config/20_chassis_power/chassis_power_config.c#L26) | 配置初值 | `.recovery_power_w = CHASSIS_BUFFER_RECOVERY_POWER_W, /* 回充状态下最大功率减让量，单位 W，并非每周期固定减量。 */` |
| [27](../config/20_chassis_power/chassis_power_config.c#L27) | 配置初值 | `.recovery_target_time_s = CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S, /* 把能量缺口换算成减让功率的目标恢复时间，单位 s。 */` |
| [28](../config/20_chassis_power/chassis_power_config.c#L28) | 配置初值 | `.observer_max_step_s = CHASSIS_BUFFER_OBSERVER_MAX_STEP_S, /* 观测器允许连续积分的最大时间间隔，单位 s。 */` |
| [29](../config/20_chassis_power/chassis_power_config.c#L29) | 配置初值 | `.recovery_hold_s = CHASSIS_BUFFER_RECOVERY_HOLD_S, /* 满足退出回充门槛后需要连续保持的时间，单位 s。 */` |
| [30](../config/20_chassis_power/chassis_power_config.c#L30) | 配置初值 | `.rise_rate_w_per_s = CHASSIS_BUFFER_RISE_RATE_W_PER_S, /* 预算上升斜率，单位 W/s；外层用于负载侧，buffer 内层用于输入侧，下降立即生效。 */` |
| [31](../config/20_chassis_power/chassis_power_config.c#L31) | 配置初值 | `.safety_factor = CHASSIS_POWER_SAFETY_FACTOR, /* 功率与能量预算共用的安全系数，无单位。 */` |
| [32](../config/20_chassis_power/chassis_power_config.c#L32) | 配置初值 | `.correction_rise_j_per_s = CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S, /* 新裁判帧向上修正能量估计的速率上限，单位 J/s；向下修正立即生效。 */` |
| [33](../config/20_chassis_power/chassis_power_config.c#L33) | 配置初值 | `},` |
| [37](../config/20_chassis_power/chassis_power_config.c#L37) | 配置初值 | `ChassisPowerOuterLoopConfig_s chassis_outer_loop_config = {` |
| [38](../config/20_chassis_power/chassis_power_config.c#L38) | 配置初值 | `.kp = 1.0f, // 比例增益(无单位),误差 = 目标功率 - 实测功率(W),输出为预算修正量(W)` |
| [39](../config/20_chassis_power/chassis_power_config.c#L39) | 配置初值 | `.ki = 1.5f, // 积分增益(1/s),积分限幅为[-基准预算, max_positive_correction_w]` |
| [40](../config/20_chassis_power/chassis_power_config.c#L40) | 配置初值 | `.measurement_lpf_tau_s = 0.030f, // 功率计读数一阶低通时间常数(s),抑制采样噪声` |
| [41](../config/20_chassis_power/chassis_power_config.c#L41) | 配置初值 | `.activation_margin_w = 5.0f, // 介入阈值(W):需求或实测功率距目标不到此值时才进入ACTIVE` |
| [42](../config/20_chassis_power/chassis_power_config.c#L42) | 配置初值 | `.error_deadband_w = 0.3f, // 误差死区(W):&#124;误差&#124;不超过此值时按0处理,停止积分` |
| [43](../config/20_chassis_power/chassis_power_config.c#L43) | 配置初值 | `.max_positive_correction_w = 8.0f, // 修正量上限(W):最多只上调8W;下限为-基准预算,即最狠可压到0` |
| [44](../config/20_chassis_power/chassis_power_config.c#L44) | 配置初值 | `.positive_slew_rate_w_per_s = 60.0f, // 修正量上升/回零速率(W/s);下调不限速,立即生效` |
| [45](../config/20_chassis_power/chassis_power_config.c#L45) | 配置初值 | `.budget_drop_reset_w = 1.0f, // 基准预算单周期跌落超过此值(W)时,清零正向积分与修正量` |
| [46](../config/20_chassis_power/chassis_power_config.c#L46) | 配置初值 | `.antiwindup_gain_per_s = 10, /* 慢环积分回算的抗饱和增益，单位 1/s。 */` |
| [47](../config/20_chassis_power/chassis_power_config.c#L47) | 配置初值 | `.low_demand_delay_s = .5f, /* 低需求持续多久后衰减旧功率修正，单位 s。 */` |
| [48](../config/20_chassis_power/chassis_power_config.c#L48) | 配置初值 | `.low_demand_decay_w_per_s = 1, /* 低需求状态下旧功率修正回零的速度，单位 W/s。 */` |

## config/20_chassis_power/chassis_power_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/20_chassis_power/chassis_power_config.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_CONFIG_H` |
| [2](../config/20_chassis_power/chassis_power_config.h#L2) | 预处理 | `#define CHASSIS_POWER_CONFIG_H` |
| [10](../config/20_chassis_power/chassis_power_config.h#L10) | 预处理 | `#define CHASSIS_POWER_SAFETY_FACTOR 0.98f /* 无单位；用于功率预算与分配的共同安全折减系数。 */` |
| [13](../config/20_chassis_power/chassis_power_config.h#L13) | 预处理 | `#define CHASSIS_POWER_SMALL_ERROR_RPM 500.0f /* 四轮转子速度误差绝对值之和的阈值，单位 rpm；不超过时平均分配功率。 */` |
| [14](../config/20_chassis_power/chassis_power_config.h#L14) | 预处理 | `#define CHASSIS_POWER_RESERVED_THRESHOLD_W 54.0f /* 总有效预算阈值，单位 W；低于它按误差分配，不预留逐轮基础功率。 */` |
| [15](../config/20_chassis_power/chassis_power_config.h#L15) | 预处理 | `#define CHASSIS_POWER_PER_MOTOR_RESERVED_W 8.0f /* 达到预留门槛后每台电机先获得的基础功率，单位 W。 */` |
| [16](../config/20_chassis_power/chassis_power_config.h#L16) | 预处理 | `#define CHASSIS_POWER_MAX_CURRENT_COMMAND 15000.0f /* 电机输出电流 CAN 命令幅值上限；命令计数，不能直接当作 A。 */` |
| [23](../config/20_chassis_power/chassis_power_config.h#L23) | 预处理 | `#ifndef CHASSIS_POWER_CONTROL_STAGE` |
| [24](../config/20_chassis_power/chassis_power_config.h#L24) | 预处理 | `#define CHASSIS_POWER_CONTROL_STAGE 3 /* 功控阶段：默认 3；0 观测，1 快保护，2 启动保护，3 完整慢环，具体区别见上方说明。 */` |
| [25](../config/20_chassis_power/chassis_power_config.h#L25) | 预处理 | `#endif` |
| [26](../config/20_chassis_power/chassis_power_config.h#L26) | 预处理 | `#ifndef CHASSIS_POWER_CAPTURE_ENABLE` |
| [27](../config/20_chassis_power/chassis_power_config.h#L27) | 预处理 | `#define CHASSIS_POWER_CAPTURE_ENABLE 0 /* 功控采集开关：默认 0 关闭，1 开启；要求 LOG_OUTPUT=2 或 3 以使用 USART2。 */` |
| [28](../config/20_chassis_power/chassis_power_config.h#L28) | 预处理 | `#endif` |
| [29](../config/20_chassis_power/chassis_power_config.h#L29) | 预处理 | `#ifndef POWER_METER_ENABLE` |
| [30](../config/20_chassis_power/chassis_power_config.h#L30) | 预处理 | `#define POWER_METER_ENABLE 1 /* 负载功率计开关：默认 1 启用，0 不注册功率计；自适应直接力控要求为 1。 */` |
| [31](../config/20_chassis_power/chassis_power_config.h#L31) | 预处理 | `#endif` |
| [32](../config/20_chassis_power/chassis_power_config.h#L32) | 预处理 | `#if CHASSIS_POWER_CONTROL_STAGE < 0 &#124;&#124; CHASSIS_POWER_CONTROL_STAGE > 3` |
| [34](../config/20_chassis_power/chassis_power_config.h#L34) | 预处理 | `#endif` |
| [35](../config/20_chassis_power/chassis_power_config.h#L35) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE != 0 && CHASSIS_POWER_CAPTURE_ENABLE != 1` |
| [37](../config/20_chassis_power/chassis_power_config.h#L37) | 预处理 | `#endif` |
| [38](../config/20_chassis_power/chassis_power_config.h#L38) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE` |
| [40](../config/20_chassis_power/chassis_power_config.h#L40) | 预处理 | `#if !BOARD_LOG_UART_ENABLED` |
| [42](../config/20_chassis_power/chassis_power_config.h#L42) | 预处理 | `#endif` |
| [43](../config/20_chassis_power/chassis_power_config.h#L43) | 预处理 | `#endif` |
| [44](../config/20_chassis_power/chassis_power_config.h#L44) | 预处理 | `#endif` |

## config/20_chassis_power/chassis_power_defaults.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/20_chassis_power/chassis_power_defaults.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_DEFAULTS_H` |
| [2](../config/20_chassis_power/chassis_power_defaults.h#L2) | 预处理 | `#define CHASSIS_POWER_DEFAULTS_H` |
| [12](../config/20_chassis_power/chassis_power_defaults.h#L12) | 配置初值 | `ChassisPower_Init_Config_s config={` |
| [13](../config/20_chassis_power/chassis_power_defaults.h#L13) | 配置初值 | `.budget=chassis_budget_config,.outer=chassis_outer_loop_config,` |
| [14](../config/20_chassis_power/chassis_power_defaults.h#L14) | 配置初值 | `.transient=ChassisPowerTransientDefaultConfig(),.stage=CHASSIS_POWER_CONTROL_STAGE,` |
| [15](../config/20_chassis_power/chassis_power_defaults.h#L15) | 配置初值 | `.use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP,` |
| [16](../config/20_chassis_power/chassis_power_defaults.h#L16) | 配置初值 | `.meter_enabled=POWER_METER_ENABLE!=0,.buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0,` |
| [17](../config/20_chassis_power/chassis_power_defaults.h#L17) | 配置初值 | `.algorithm={.safety_factor=CHASSIS_POWER_SAFETY_FACTOR,` |
| [18](../config/20_chassis_power/chassis_power_defaults.h#L18) | 配置初值 | `.small_error_threshold_rpm=CHASSIS_POWER_SMALL_ERROR_RPM,` |
| [19](../config/20_chassis_power/chassis_power_defaults.h#L19) | 配置初值 | `.reserved_power_threshold_w=CHASSIS_POWER_RESERVED_THRESHOLD_W,` |
| [20](../config/20_chassis_power/chassis_power_defaults.h#L20) | 配置初值 | `.per_motor_reserved_power_w=CHASSIS_POWER_PER_MOTOR_RESERVED_W,` |
| [21](../config/20_chassis_power/chassis_power_defaults.h#L21) | 配置初值 | `.max_current_command=CHASSIS_POWER_MAX_CURRENT_COMMAND}};` |
| [26](../config/20_chassis_power/chassis_power_defaults.h#L26) | 预处理 | `#endif` |

## config/20_chassis_power/chassis_power_model_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/20_chassis_power/chassis_power_model_config.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_MODEL_CONFIG_H` |
| [2](../config/20_chassis_power/chassis_power_model_config.h#L2) | 预处理 | `#define CHASSIS_POWER_MODEL_CONFIG_H` |
| [10](../config/20_chassis_power/chassis_power_model_config.h#L10) | 预处理 | `#define CHASSIS_SPEED_POWER_MODEL_INITIALIZER { \` |
| [20](../config/20_chassis_power/chassis_power_model_config.h#L20) | 预处理 | `#endif` |

## config/21_chassis_spin/chassis_spin_config.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [7](../config/21_chassis_spin/chassis_spin_config.c#L7) | 配置初值 | `ChassisSpinConfig_s chassis_spin_config = {` |
| [8](../config/21_chassis_spin/chassis_spin_config.c#L8) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [8](../config/21_chassis_spin/chassis_spin_config.c#L8) | 配置初值 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [9](../config/21_chassis_spin/chassis_spin_config.c#L9) | 配置初值 | `.fixed_wz = CHASSIS_SPIN_SINE_CENTER_WZ, /* 正弦分支为轨迹中心，其余为固定/初始目标；单位 Wz 命令尺度。 */` |
| [10](../config/21_chassis_spin/chassis_spin_config.c#L10) | 配置初值 | `.min_wz = CHASSIS_SPIN_SINE_MIN_WZ, /* 对应模式的目标下限，单位 Wz 命令尺度。 */` |
| [11](../config/21_chassis_spin/chassis_spin_config.c#L11) | 预处理 | `#else` |
| [11](../config/21_chassis_spin/chassis_spin_config.c#L11) | 配置初值 | `#else` |
| [12](../config/21_chassis_spin/chassis_spin_config.c#L12) | 配置初值 | `.fixed_wz = CHASSIS_SPIN_FIXED_WZ, /* 正弦分支为轨迹中心，其余为固定/初始目标；单位 Wz 命令尺度。 */` |
| [13](../config/21_chassis_spin/chassis_spin_config.c#L13) | 配置初值 | `.min_wz = CHASSIS_SPIN_MIN_WZ, /* 对应模式的目标下限，单位 Wz 命令尺度。 */` |
| [14](../config/21_chassis_spin/chassis_spin_config.c#L14) | 预处理 | `#endif` |
| [14](../config/21_chassis_spin/chassis_spin_config.c#L14) | 配置初值 | `#endif` |
| [15](../config/21_chassis_spin/chassis_spin_config.c#L15) | 配置初值 | `.max_wz = CHASSIS_SPIN_MAX_WZ, /* 目标软上限，单位 Wz 命令尺度。 */` |
| [16](../config/21_chassis_spin/chassis_spin_config.c#L16) | 配置初值 | `.gain_wz_per_w_s = CHASSIS_SPIN_POWER_GAIN, /* 功率误差到目标变化率的增益，单位 Wz/(W*s)。 */` |
| [17](../config/21_chassis_spin/chassis_spin_config.c#L17) | 配置初值 | `.rise_wz_per_s = CHASSIS_SPIN_RISE_RATE, /* 普通目标增长速率上限，单位 Wz/s。 */` |
| [18](../config/21_chassis_spin/chassis_spin_config.c#L18) | 配置初值 | `.fall_wz_per_s = CHASSIS_SPIN_FALL_RATE, /* 普通目标下降速率上限，单位 Wz/s。 */` |
| [19](../config/21_chassis_spin/chassis_spin_config.c#L19) | 配置初值 | `.power_lpf_tau_s = CHASSIS_SPIN_POWER_LPF_TAU_S, /* 实测功率低通时间常数，单位 s。 */` |
| [20](../config/21_chassis_spin/chassis_spin_config.c#L20) | 配置初值 | `.power_deadband_w = CHASSIS_SPIN_POWER_DEADBAND_W, /* 功率误差死区，单位 W。 */` |
| [21](../config/21_chassis_spin/chassis_spin_config.c#L21) | 配置初值 | `.settle_time_s = CHASSIS_SPIN_SETTLE_TIME_S, /* 增长前稳定确认时间，单位 s。 */` |
| [22](../config/21_chassis_spin/chassis_spin_config.c#L22) | 配置初值 | `.budget_drop_reset_w = CHASSIS_SPIN_BUDGET_DROP_RESET_W, /* 自适应预算下降复位阈值，单位 W；正弦不使用该复位缩放。 */` |
| [23](../config/21_chassis_spin/chassis_spin_config.c#L23) | 配置初值 | `.sine_amplitude_wz = CHASSIS_SPIN_SINE_AMPLITUDE_WZ, /* 正弦振幅，单位 Wz 命令尺度。 */` |
| [24](../config/21_chassis_spin/chassis_spin_config.c#L24) | 配置初值 | `.sine_period_s = CHASSIS_SPIN_SINE_PERIOD_S, /* 正弦完整周期，单位 s。 */` |
| [25](../config/21_chassis_spin/chassis_spin_config.c#L25) | 配置初值 | `.sine_ramp_s = CHASSIS_SPIN_SINE_RAMP_S, /* 正弦振幅建立时间，单位 s。 */` |
| [26](../config/21_chassis_spin/chassis_spin_config.c#L26) | 配置初值 | `.sine_rise_wz_per_s = CHASSIS_SPIN_SINE_RISE_RATE, /* 正弦目标上升速率上限，单位 Wz/s。 */` |
| [27](../config/21_chassis_spin/chassis_spin_config.c#L27) | 配置初值 | `.sine_fall_wz_per_s = CHASSIS_SPIN_SINE_FALL_RATE, /* 正弦目标下降速率上限，单位 Wz/s。 */` |
| [28](../config/21_chassis_spin/chassis_spin_config.c#L28) | 配置初值 | `.sine_direction = CHASSIS_SPIN_SINE_DIRECTION, /* 正弦方向：+1 正向，-1 反向。 */` |
| [29](../config/21_chassis_spin/chassis_spin_config.c#L29) | 配置初值 | `.extension_enabled=CHASSIS_SPIN_EXTENSION_ENABLE!=0, /* 软上限扩展是否启用；0 关闭、非零开启，默认关闭。 */` |
| [30](../config/21_chassis_spin/chassis_spin_config.c#L30) | 配置初值 | `.hard_wz=CHASSIS_SPIN_HARD_WZ, /* 扩展目标硬上限，单位 Wz 命令尺度；0 跟随软上限。 */` |
| [31](../config/21_chassis_spin/chassis_spin_config.c#L31) | 配置初值 | `.body_hard_rps=CHASSIS_SPIN_BODY_HARD_RPS, /* 车体实测转速硬上限，单位转/秒。 */` |
| [32](../config/21_chassis_spin/chassis_spin_config.c#L32) | 配置初值 | `.wheel_hard_rpm=CHASSIS_SPIN_WHEEL_HARD_RPM, /* 电机转子实测转速硬上限，单位 rpm。 */` |
| [33](../config/21_chassis_spin/chassis_spin_config.c#L33) | 配置初值 | `.extension_rise_wz_per_s=CHASSIS_SPIN_EXTENSION_RISE_RATE, /* 扩展区间目标增长速率上限，单位 Wz/s。 */` |

## config/21_chassis_spin/chassis_spin_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/21_chassis_spin/chassis_spin_config.h#L1) | 预处理 | `#ifndef CHASSIS_SPIN_CONFIG_H` |
| [2](../config/21_chassis_spin/chassis_spin_config.h#L2) | 预处理 | `#define CHASSIS_SPIN_CONFIG_H` |
| [11](../config/21_chassis_spin/chassis_spin_config.h#L11) | 预处理 | `#ifndef CHASSIS_SPIN_ADAPTIVE_ENABLE` |
| [12](../config/21_chassis_spin/chassis_spin_config.h#L12) | 预处理 | `#define CHASSIS_SPIN_ADAPTIVE_ENABLE 1 /* 兼容开关：默认 1 自适应、0 固定；仅未显式指定 CHASSIS_SPIN_MODE 时决定模式。 */` |
| [13](../config/21_chassis_spin/chassis_spin_config.h#L13) | 预处理 | `#endif` |
| [14](../config/21_chassis_spin/chassis_spin_config.h#L14) | 预处理 | `#if CHASSIS_SPIN_ADAPTIVE_ENABLE != 0 && CHASSIS_SPIN_ADAPTIVE_ENABLE != 1` |
| [16](../config/21_chassis_spin/chassis_spin_config.h#L16) | 预处理 | `#endif` |
| [18](../config/21_chassis_spin/chassis_spin_config.h#L18) | 预处理 | `#define CHASSIS_SPIN_MODE_FIXED 0 /* 自旋模式编号：0，固定 Wz 目标。 */` |
| [19](../config/21_chassis_spin/chassis_spin_config.h#L19) | 预处理 | `#define CHASSIS_SPIN_MODE_ADAPTIVE 1 /* 自旋模式编号：1，功率余量允许时提高 Wz 目标。 */` |
| [20](../config/21_chassis_spin/chassis_spin_config.h#L20) | 预处理 | `#define CHASSIS_SPIN_MODE_SINE 2 /* 自旋模式编号：2，按正弦轨迹改变同向 Wz 目标。 */` |
| [21](../config/21_chassis_spin/chassis_spin_config.h#L21) | 预处理 | `#ifndef CHASSIS_SPIN_MODE` |
| [22](../config/21_chassis_spin/chassis_spin_config.h#L22) | 预处理 | `#if CHASSIS_SPIN_ADAPTIVE_ENABLE` |
| [23](../config/21_chassis_spin/chassis_spin_config.h#L23) | 预处理 | `#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_ADAPTIVE /* 模式选择：0 固定、1 自适应、2 正弦；未覆盖时由兼容开关选择，工程默认 1。 */` |
| [24](../config/21_chassis_spin/chassis_spin_config.h#L24) | 预处理 | `#else` |
| [25](../config/21_chassis_spin/chassis_spin_config.h#L25) | 预处理 | `#define CHASSIS_SPIN_MODE CHASSIS_SPIN_MODE_FIXED /* 模式选择：0 固定、1 自适应、2 正弦；未覆盖时由兼容开关选择，工程默认 1。 */` |
| [26](../config/21_chassis_spin/chassis_spin_config.h#L26) | 预处理 | `#endif` |
| [27](../config/21_chassis_spin/chassis_spin_config.h#L27) | 预处理 | `#endif` |
| [28](../config/21_chassis_spin/chassis_spin_config.h#L28) | 预处理 | `#if CHASSIS_SPIN_MODE < CHASSIS_SPIN_MODE_FIXED &#124;&#124; CHASSIS_SPIN_MODE > CHASSIS_SPIN_MODE_SINE` |
| [30](../config/21_chassis_spin/chassis_spin_config.h#L30) | 预处理 | `#endif` |
| [31](../config/21_chassis_spin/chassis_spin_config.h#L31) | 预处理 | `#define CHASSIS_SPIN_DYNAMIC_ENABLED (CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_FIXED) /* 派生开关：固定模式为 0，其余模式为 1；由模式自动计算，不单独修改。 */` |
| [34](../config/21_chassis_spin/chassis_spin_config.h#L34) | 预处理 | `#ifndef CHASSIS_SPIN_FIXED_WZ` |
| [35](../config/21_chassis_spin/chassis_spin_config.h#L35) | 预处理 | `#define CHASSIS_SPIN_FIXED_WZ 8000.0f /* 固定模式目标/自适应初始目标，单位为本工程 Wz 命令尺度。 */` |
| [36](../config/21_chassis_spin/chassis_spin_config.h#L36) | 预处理 | `#endif` |
| [37](../config/21_chassis_spin/chassis_spin_config.h#L37) | 预处理 | `#ifndef CHASSIS_SPIN_MIN_WZ` |
| [38](../config/21_chassis_spin/chassis_spin_config.h#L38) | 预处理 | `#define CHASSIS_SPIN_MIN_WZ 3000.0f /* 自适应模式的 Wz 目标下限，单位 Wz 命令尺度，须为正且不大于初始目标。 */` |
| [39](../config/21_chassis_spin/chassis_spin_config.h#L39) | 预处理 | `#endif` |
| [40](../config/21_chassis_spin/chassis_spin_config.h#L40) | 预处理 | `#ifndef CHASSIS_SPIN_MAX_WZ` |
| [41](../config/21_chassis_spin/chassis_spin_config.h#L41) | 预处理 | `#define CHASSIS_SPIN_MAX_WZ 9000.0f /* 动态自旋目标软上限，单位 Wz 命令尺度，须不小于初始或正弦峰值。 */` |
| [42](../config/21_chassis_spin/chassis_spin_config.h#L42) | 预处理 | `#endif` |
| [43](../config/21_chassis_spin/chassis_spin_config.h#L43) | 预处理 | `#ifndef CHASSIS_SPIN_POWER_GAIN` |
| [44](../config/21_chassis_spin/chassis_spin_config.h#L44) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [45](../config/21_chassis_spin/chassis_spin_config.h#L45) | 预处理 | `#define CHASSIS_SPIN_POWER_GAIN 80.0f /* 功率误差到目标变化速率的增益，单位 Wz/(W*s)；自适应默认 80，其余默认 40。 */` |
| [46](../config/21_chassis_spin/chassis_spin_config.h#L46) | 预处理 | `#else` |
| [47](../config/21_chassis_spin/chassis_spin_config.h#L47) | 预处理 | `#define CHASSIS_SPIN_POWER_GAIN 40.0f /* 功率误差到目标变化速率的增益，单位 Wz/(W*s)；自适应默认 80，其余默认 40。 */` |
| [48](../config/21_chassis_spin/chassis_spin_config.h#L48) | 预处理 | `#endif` |
| [49](../config/21_chassis_spin/chassis_spin_config.h#L49) | 预处理 | `#endif` |
| [50](../config/21_chassis_spin/chassis_spin_config.h#L50) | 预处理 | `#ifndef CHASSIS_SPIN_RISE_RATE` |
| [51](../config/21_chassis_spin/chassis_spin_config.h#L51) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [52](../config/21_chassis_spin/chassis_spin_config.h#L52) | 预处理 | `#define CHASSIS_SPIN_RISE_RATE 3600.0f /* 自适应目标上升速率上限，单位 Wz/s；自适应默认 3600，其余默认 1200。 */` |
| [53](../config/21_chassis_spin/chassis_spin_config.h#L53) | 预处理 | `#else` |
| [54](../config/21_chassis_spin/chassis_spin_config.h#L54) | 预处理 | `#define CHASSIS_SPIN_RISE_RATE 1200.0f /* 自适应目标上升速率上限，单位 Wz/s；自适应默认 3600，其余默认 1200。 */` |
| [55](../config/21_chassis_spin/chassis_spin_config.h#L55) | 预处理 | `#endif` |
| [56](../config/21_chassis_spin/chassis_spin_config.h#L56) | 预处理 | `#endif` |
| [57](../config/21_chassis_spin/chassis_spin_config.h#L57) | 预处理 | `#ifndef CHASSIS_SPIN_FALL_RATE` |
| [58](../config/21_chassis_spin/chassis_spin_config.h#L58) | 预处理 | `#define CHASSIS_SPIN_FALL_RATE 2400.0f /* 非正弦目标下降速率上限，单位 Wz/s；正弦使用独立下降速率。 */` |
| [59](../config/21_chassis_spin/chassis_spin_config.h#L59) | 预处理 | `#endif` |
| [60](../config/21_chassis_spin/chassis_spin_config.h#L60) | 预处理 | `#ifndef CHASSIS_SPIN_POWER_LPF_TAU_S` |
| [61](../config/21_chassis_spin/chassis_spin_config.h#L61) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [62](../config/21_chassis_spin/chassis_spin_config.h#L62) | 预处理 | `#define CHASSIS_SPIN_POWER_LPF_TAU_S 0.05f /* 实测功率低通时间常数，单位 s；自适应默认 0.05，其余默认 0.1。 */` |
| [63](../config/21_chassis_spin/chassis_spin_config.h#L63) | 预处理 | `#else` |
| [64](../config/21_chassis_spin/chassis_spin_config.h#L64) | 预处理 | `#define CHASSIS_SPIN_POWER_LPF_TAU_S 0.1f /* 实测功率低通时间常数，单位 s；自适应默认 0.05，其余默认 0.1。 */` |
| [65](../config/21_chassis_spin/chassis_spin_config.h#L65) | 预处理 | `#endif` |
| [66](../config/21_chassis_spin/chassis_spin_config.h#L66) | 预处理 | `#endif` |
| [67](../config/21_chassis_spin/chassis_spin_config.h#L67) | 预处理 | `#ifndef CHASSIS_SPIN_POWER_DEADBAND_W` |
| [68](../config/21_chassis_spin/chassis_spin_config.h#L68) | 预处理 | `#define CHASSIS_SPIN_POWER_DEADBAND_W 1.0f /* 功率误差死区，单位 W；死区内不继续按误差增加或降低目标。 */` |
| [69](../config/21_chassis_spin/chassis_spin_config.h#L69) | 预处理 | `#endif` |
| [70](../config/21_chassis_spin/chassis_spin_config.h#L70) | 预处理 | `#ifndef CHASSIS_SPIN_SETTLE_TIME_S` |
| [71](../config/21_chassis_spin/chassis_spin_config.h#L71) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [72](../config/21_chassis_spin/chassis_spin_config.h#L72) | 预处理 | `#define CHASSIS_SPIN_SETTLE_TIME_S 0.01f /* 允许增长前的稳定确认时间，单位 s；自适应默认 0.01，其余默认 0.2。 */` |
| [73](../config/21_chassis_spin/chassis_spin_config.h#L73) | 预处理 | `#else` |
| [74](../config/21_chassis_spin/chassis_spin_config.h#L74) | 预处理 | `#define CHASSIS_SPIN_SETTLE_TIME_S 0.2f /* 允许增长前的稳定确认时间，单位 s；自适应默认 0.01，其余默认 0.2。 */` |
| [75](../config/21_chassis_spin/chassis_spin_config.h#L75) | 预处理 | `#endif` |
| [76](../config/21_chassis_spin/chassis_spin_config.h#L76) | 预处理 | `#endif` |
| [77](../config/21_chassis_spin/chassis_spin_config.h#L77) | 预处理 | `#ifndef CHASSIS_SPIN_TRACKING_ERROR_RPM` |
| [78](../config/21_chassis_spin/chassis_spin_config.h#L78) | 预处理 | `#define CHASSIS_SPIN_TRACKING_ERROR_RPM 500.0f /* 轮速跟踪误差门槛，单位转子 rpm；跟踪不足时限制目标增长。 */` |
| [79](../config/21_chassis_spin/chassis_spin_config.h#L79) | 预处理 | `#endif` |
| [80](../config/21_chassis_spin/chassis_spin_config.h#L80) | 预处理 | `#ifndef CHASSIS_SPIN_BUDGET_DROP_RESET_W` |
| [81](../config/21_chassis_spin/chassis_spin_config.h#L81) | 预处理 | `#define CHASSIS_SPIN_BUDGET_DROP_RESET_W 1.0f /* 自适应预算下降复位门槛，单位 W；正弦模式不据此缩放轨迹。 */` |
| [82](../config/21_chassis_spin/chassis_spin_config.h#L82) | 预处理 | `#endif` |
| [85](../config/21_chassis_spin/chassis_spin_config.h#L85) | 预处理 | `#ifndef CHASSIS_SPIN_EXTENSION_ENABLE` |
| [86](../config/21_chassis_spin/chassis_spin_config.h#L86) | 预处理 | `#define CHASSIS_SPIN_EXTENSION_ENABLE 0 /* 软上限扩展开关：默认 0 关闭，1 开启；仅支持自适应且必须完成负载标定。 */` |
| [87](../config/21_chassis_spin/chassis_spin_config.h#L87) | 预处理 | `#endif` |
| [88](../config/21_chassis_spin/chassis_spin_config.h#L88) | 预处理 | `#ifndef CHASSIS_SPIN_HARD_WZ` |
| [89](../config/21_chassis_spin/chassis_spin_config.h#L89) | 预处理 | `#define CHASSIS_SPIN_HARD_WZ 0.0f /* 扩展后的 Wz 硬上限；0 随当前软上限，启用扩展时须不低于软上限。 */` |
| [90](../config/21_chassis_spin/chassis_spin_config.h#L90) | 预处理 | `#endif` |
| [91](../config/21_chassis_spin/chassis_spin_config.h#L91) | 预处理 | `#ifndef CHASSIS_SPIN_BODY_HARD_RPS` |
| [92](../config/21_chassis_spin/chassis_spin_config.h#L92) | 预处理 | `#define CHASSIS_SPIN_BODY_HARD_RPS 0.0f /* 实测车体转速硬上限，单位转/秒（不是 rad/s）；默认 0 未标定，扩展要求正值。 */` |
| [93](../config/21_chassis_spin/chassis_spin_config.h#L93) | 预处理 | `#endif` |
| [94](../config/21_chassis_spin/chassis_spin_config.h#L94) | 预处理 | `#ifndef CHASSIS_SPIN_WHEEL_HARD_RPM` |
| [95](../config/21_chassis_spin/chassis_spin_config.h#L95) | 预处理 | `#define CHASSIS_SPIN_WHEEL_HARD_RPM 0.0f /* 实测电机转子转速硬上限，单位 rpm；默认 0 未标定，扩展要求正值。 */` |
| [96](../config/21_chassis_spin/chassis_spin_config.h#L96) | 预处理 | `#endif` |
| [97](../config/21_chassis_spin/chassis_spin_config.h#L97) | 预处理 | `#ifndef CHASSIS_SPIN_EXTENSION_RISE_RATE` |
| [98](../config/21_chassis_spin/chassis_spin_config.h#L98) | 预处理 | `#define CHASSIS_SPIN_EXTENSION_RISE_RATE 600.0f /* 超出软上限后的增长速率，单位 Wz/s；启用时须为正且不高于普通上升速率。 */` |
| [99](../config/21_chassis_spin/chassis_spin_config.h#L99) | 预处理 | `#endif` |
| [100](../config/21_chassis_spin/chassis_spin_config.h#L100) | 预处理 | `#if CHASSIS_SPIN_EXTENSION_ENABLE != 0 && CHASSIS_SPIN_EXTENSION_ENABLE != 1` |
| [102](../config/21_chassis_spin/chassis_spin_config.h#L102) | 预处理 | `#endif` |
| [103](../config/21_chassis_spin/chassis_spin_config.h#L103) | 预处理 | `#if CHASSIS_SPIN_EXTENSION_ENABLE && CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_ADAPTIVE` |
| [105](../config/21_chassis_spin/chassis_spin_config.h#L105) | 预处理 | `#endif` |
| [109](../config/21_chassis_spin/chassis_spin_config.h#L109) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_CENTER_WZ` |
| [110](../config/21_chassis_spin/chassis_spin_config.h#L110) | 预处理 | `#define CHASSIS_SPIN_SINE_CENTER_WZ 800.0f /* 正弦轨迹中心，单位 Wz 命令尺度；中心减振幅不得低于正弦最小值。 */` |
| [111](../config/21_chassis_spin/chassis_spin_config.h#L111) | 预处理 | `#endif` |
| [112](../config/21_chassis_spin/chassis_spin_config.h#L112) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_MIN_WZ` |
| [113](../config/21_chassis_spin/chassis_spin_config.h#L113) | 预处理 | `#define CHASSIS_SPIN_SINE_MIN_WZ 200.0f /* 正弦同向目标下限，单位 Wz 命令尺度；须为正，避免跨零换向。 */` |
| [114](../config/21_chassis_spin/chassis_spin_config.h#L114) | 预处理 | `#endif` |
| [115](../config/21_chassis_spin/chassis_spin_config.h#L115) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_AMPLITUDE_WZ` |
| [116](../config/21_chassis_spin/chassis_spin_config.h#L116) | 预处理 | `#define CHASSIS_SPIN_SINE_AMPLITUDE_WZ 200.0f /* 正弦振幅，单位 Wz 命令尺度；须非负，中心加振幅不得超过软上限。 */` |
| [117](../config/21_chassis_spin/chassis_spin_config.h#L117) | 预处理 | `#endif` |
| [118](../config/21_chassis_spin/chassis_spin_config.h#L118) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_PERIOD_S` |
| [119](../config/21_chassis_spin/chassis_spin_config.h#L119) | 预处理 | `#define CHASSIS_SPIN_SINE_PERIOD_S 4.0f /* 正弦完整周期，单位 s；须大于 0。 */` |
| [120](../config/21_chassis_spin/chassis_spin_config.h#L120) | 预处理 | `#endif` |
| [121](../config/21_chassis_spin/chassis_spin_config.h#L121) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_RAMP_S` |
| [122](../config/21_chassis_spin/chassis_spin_config.h#L122) | 预处理 | `#define CHASSIS_SPIN_SINE_RAMP_S 1.0f /* 建立正弦振幅的时间，单位 s；须大于 0。 */` |
| [123](../config/21_chassis_spin/chassis_spin_config.h#L123) | 预处理 | `#endif` |
| [124](../config/21_chassis_spin/chassis_spin_config.h#L124) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_RISE_RATE` |
| [125](../config/21_chassis_spin/chassis_spin_config.h#L125) | 预处理 | `#define CHASSIS_SPIN_SINE_RISE_RATE 400.0f /* 正弦目标上升速率上限，单位 Wz/s；须大于 0。 */` |
| [126](../config/21_chassis_spin/chassis_spin_config.h#L126) | 预处理 | `#endif` |
| [127](../config/21_chassis_spin/chassis_spin_config.h#L127) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_FALL_RATE` |
| [128](../config/21_chassis_spin/chassis_spin_config.h#L128) | 预处理 | `#define CHASSIS_SPIN_SINE_FALL_RATE 2000.0f /* 正弦目标下降及超功率退让速率上限，单位 Wz/s；须大于 0。 */` |
| [129](../config/21_chassis_spin/chassis_spin_config.h#L129) | 预处理 | `#endif` |
| [130](../config/21_chassis_spin/chassis_spin_config.h#L130) | 预处理 | `#ifndef CHASSIS_SPIN_SINE_DIRECTION` |
| [131](../config/21_chassis_spin/chassis_spin_config.h#L131) | 预处理 | `#define CHASSIS_SPIN_SINE_DIRECTION 1 /* 正弦旋转方向：默认 +1 正向，-1 反向；只接受这两个值。 */` |
| [132](../config/21_chassis_spin/chassis_spin_config.h#L132) | 预处理 | `#endif` |
| [134](../config/21_chassis_spin/chassis_spin_config.h#L134) | 预处理 | `#endif` |

## config/22_chassis_force/chassis_control_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/22_chassis_force/chassis_control_config.h#L1) | 预处理 | `#ifndef CHASSIS_CONTROL_CONFIG_H` |
| [2](../config/22_chassis_force/chassis_control_config.h#L2) | 预处理 | `#define CHASSIS_CONTROL_CONFIG_H` |
| [9](../config/22_chassis_force/chassis_control_config.h#L9) | 预处理 | `#define CHASSIS_CONTROL_SPEED 0 /* 速度环后端编号：0，默认选择。 */` |
| [10](../config/22_chassis_force/chassis_control_config.h#L10) | 预处理 | `#define CHASSIS_CONTROL_FORCE 1 /* 直接力控后端编号：1，需要完成车辆力控标定。 */` |
| [11](../config/22_chassis_force/chassis_control_config.h#L11) | 预处理 | `#ifndef CHASSIS_CONTROL_MODE` |
| [12](../config/22_chassis_force/chassis_control_config.h#L12) | 预处理 | `#define CHASSIS_CONTROL_MODE CHASSIS_CONTROL_SPEED /* 后端选择：0 速度环，1 直接力控；默认 0，必须通过上方构建参数切换。 */` |
| [13](../config/22_chassis_force/chassis_control_config.h#L13) | 预处理 | `#endif` |
| [14](../config/22_chassis_force/chassis_control_config.h#L14) | 预处理 | `#define CHASSIS_FORCE_EXIT_COAST 0 /* 力控退出方式编号：0，撤销驱动力后滑行。 */` |
| [15](../config/22_chassis_force/chassis_control_config.h#L15) | 预处理 | `#define CHASSIS_FORCE_EXIT_BRAKE 1 /* 力控退出方式编号：1，按制动转矩、停止死区和超时限值制动。 */` |
| [16](../config/22_chassis_force/chassis_control_config.h#L16) | 预处理 | `#ifndef CHASSIS_FORCE_EXIT_MODE` |
| [17](../config/22_chassis_force/chassis_control_config.h#L17) | 预处理 | `#define CHASSIS_FORCE_EXIT_MODE CHASSIS_FORCE_EXIT_COAST /* 退出策略：默认 0 滑行；改为 1 前须填写并验证车辆 brake_* 参数。 */` |
| [18](../config/22_chassis_force/chassis_control_config.h#L18) | 预处理 | `#endif` |
| [19](../config/22_chassis_force/chassis_control_config.h#L19) | 预处理 | `#ifndef CHASSIS_FORCE_CALIBRATION_VALID` |
| [20](../config/22_chassis_force/chassis_control_config.h#L20) | 预处理 | `#define CHASSIS_FORCE_CALIBRATION_VALID 0 /* 标定验收开关：默认 0 未验收、禁止驱动；1 已验收，仍须通过所有物理参数校验。 */` |
| [21](../config/22_chassis_force/chassis_control_config.h#L21) | 预处理 | `#endif` |
| [22](../config/22_chassis_force/chassis_control_config.h#L22) | 预处理 | `#define CHASSIS_FORCE_SPIN_FIXED_TORQUE 0 /* 力控自旋策略编号：0，固定请求转矩。 */` |
| [23](../config/22_chassis_force/chassis_control_config.h#L23) | 预处理 | `#define CHASSIS_FORCE_SPIN_POWER_ADAPTIVE 1 /* 力控自旋策略编号：1，根据负载功率调整请求转矩。 */` |
| [24](../config/22_chassis_force/chassis_control_config.h#L24) | 预处理 | `#ifndef CHASSIS_FORCE_SPIN_MODE` |
| [25](../config/22_chassis_force/chassis_control_config.h#L25) | 预处理 | `#define CHASSIS_FORCE_SPIN_MODE CHASSIS_FORCE_SPIN_POWER_ADAPTIVE /* 自旋策略：默认 1 功率自适应；0 固定转矩；模式 1 要求启用负载功率计。 */` |
| [26](../config/22_chassis_force/chassis_control_config.h#L26) | 预处理 | `#endif` |
| [27](../config/22_chassis_force/chassis_control_config.h#L27) | 预处理 | `#if CHASSIS_CONTROL_MODE != 0 && CHASSIS_CONTROL_MODE != 1` |
| [29](../config/22_chassis_force/chassis_control_config.h#L29) | 预处理 | `#endif` |
| [30](../config/22_chassis_force/chassis_control_config.h#L30) | 预处理 | `#if CHASSIS_FORCE_EXIT_MODE != 0 && CHASSIS_FORCE_EXIT_MODE != 1` |
| [32](../config/22_chassis_force/chassis_control_config.h#L32) | 预处理 | `#endif` |
| [33](../config/22_chassis_force/chassis_control_config.h#L33) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_VALID != 0 && CHASSIS_FORCE_CALIBRATION_VALID != 1` |
| [35](../config/22_chassis_force/chassis_control_config.h#L35) | 预处理 | `#endif` |
| [36](../config/22_chassis_force/chassis_control_config.h#L36) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE != 0 && CHASSIS_FORCE_SPIN_MODE != 1` |
| [38](../config/22_chassis_force/chassis_control_config.h#L38) | 预处理 | `#endif` |
| [39](../config/22_chassis_force/chassis_control_config.h#L39) | 预处理 | `#endif` |

## config/22_chassis_force/chassis_force_config.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [7](../config/22_chassis_force/chassis_force_config.c#L7) | 配置初值 | `ChassisForceConfig_s c={` |
| [8](../config/22_chassis_force/chassis_force_config.c#L8) | 配置初值 | `.backend=1, /* 力控后端协议编号，固定为 1；不是选择速度/力控的构建开关。 */` |
| [9](../config/22_chassis_force/chassis_force_config.c#L9) | 配置初值 | `.schema_version=1, /* 参数结构版本，当前校验只接受 1。 */` |
| [10](../config/22_chassis_force/chassis_force_config.c#L10) | 配置初值 | `.calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0, /* 标定验收状态；默认宏为 0，未验收。 */` |
| [11](../config/22_chassis_force/chassis_force_config.c#L11) | 配置初值 | `.resume_ratio=.9f, /* 速度保护解除回滞比例，无单位；低于上限的 90% 后可恢复。 */` |
| [12](../config/22_chassis_force/chassis_force_config.c#L12) | 配置初值 | `.input_deadzone=.05f, /* 归一化摇杆径向死区，无单位；默认 5%。 */` |
| [13](../config/22_chassis_force/chassis_force_config.c#L13) | 配置初值 | `.input_gamma=1.5f, /* 输入力曲线的幂指数，无单位；控制小输入的细腻程度。 */` |
| [14](../config/22_chassis_force/chassis_force_config.c#L14) | 配置初值 | `.power_gain_nm_per_w_s=.01f, /* 功率误差到转矩变化率的增益，单位 N*m/(W*s)。 */` |
| [15](../config/22_chassis_force/chassis_force_config.c#L15) | 配置初值 | `.power_deadband_w=2, /* 负载功率跟踪死区，单位 W。 */` |
| [16](../config/22_chassis_force/chassis_force_config.c#L16) | 配置初值 | `.power_lpf_tau_s=.02f, /* 负载功率低通时间常数，单位 s。 */` |
| [17](../config/22_chassis_force/chassis_force_config.c#L17) | 配置初值 | `.power_recovery_samples=3, /* 恢复增长前所需连续新鲜功率样本数。 */` |
| [18](../config/22_chassis_force/chassis_force_config.c#L18) | 配置初值 | `.torque_probe_step_nm=.05f, /* 单次候选转矩增量，单位 N*m。 */` |
| [19](../config/22_chassis_force/chassis_force_config.c#L19) | 配置初值 | `.power_request_step_w=5, /* 单次能源申请的功率步长，单位 W。 */` |
| [20](../config/22_chassis_force/chassis_force_config.c#L20) | 配置初值 | `.grant_timeout_ms=50 /* 等待能源批准的超时时间，单位 ms。 */` |

## config/22_chassis_force/chassis_force_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/22_chassis_force/chassis_force_config.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_CONFIG_H` |
| [2](../config/22_chassis_force/chassis_force_config.h#L2) | 预处理 | `#define CHASSIS_FORCE_CONFIG_H` |
| [68](../config/22_chassis_force/chassis_force_config.h#L68) | 预处理 | `#endif` |

## config/22_chassis_force/chassis_force_vehicle_config.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../config/22_chassis_force/chassis_force_vehicle_config.c#L5) | 预处理 | `#define BOARD_CHASSIS_LEGACY_CURRENT_LIMIT 15000.0f` |
| [33](../config/22_chassis_force/chassis_force_vehicle_config.c#L33) | 配置初值 | `ChassisForceConfig_s c={` |
| [34](../config/22_chassis_force/chassis_force_vehicle_config.c#L34) | 配置初值 | `.backend=CHASSIS_CONTROL_FORCE, /* 协议后端标识，无单位；固定为 FORCE，不是调参项。 */` |
| [35](../config/22_chassis_force/chassis_force_vehicle_config.c#L35) | 配置初值 | `.schema_version=1U,             /* 参数结构版本，无单位；修改结构时单独升级。 */` |
| [36](../config/22_chassis_force/chassis_force_vehicle_config.c#L36) | 配置初值 | `.calibration_valid=CHASSIS_FORCE_CALIBRATION_VALID!=0, /* 标定验收标志；默认宏值为 0。 */` |
| [38](../config/22_chassis_force/chassis_force_vehicle_config.c#L38) | 配置初值 | `/* 四轮逐轮填写。共同标定方法：` |
| [39](../config/22_chassis_force/chassis_force_vehicle_config.c#L39) | 配置初值 | `* k_out_nm_a：输出轴接已知力臂 L(m)，测力 F(N)，tau=F*L；多个低电流正/反向` |
| [40](../config/22_chassis_force/chassis_force_vehicle_config.c#L40) | 配置初值 | `*             点拟合 tau-I 的斜率，单位 N*m/A。它是减速器输出端常数，不再乘减速比。` |
| [41](../config/22_chassis_force/chassis_force_vehicle_config.c#L41) | 配置初值 | `*             电源输入电流不能代替电机转矩电流；检查摩擦、偏置及正反向拟合误差。` |
| [42](../config/22_chassis_force/chassis_force_vehicle_config.c#L42) | 配置初值 | `* current_limit：沿用原速控/功率配置的 15000；更换硬件或调整持续负载时复核电流/温升，` |
| [43](../config/22_chassis_force/chassis_force_vehicle_config.c#L43) | 配置初值 | `*                不得超过 16384；起步、堵转和温度保护仍需单独填写。` |
| [44](../config/22_chassis_force/chassis_force_vehicle_config.c#L44) | 配置初值 | `* force_limit_n：用测力装置和低力加载验证可重复传递的轮缘力，考虑接地滑移后留裕量；` |
| [45](../config/22_chassis_force/chassis_force_vehicle_config.c#L45) | 配置初值 | `*                架空轮速或输出轴转矩不能证明真实接地力。` |
| [46](../config/22_chassis_force/chassis_force_vehicle_config.c#L46) | 配置初值 | `* current_slew：初值按原电流幅值上限除以 1 s，表示 0 到旧上限至少约 1 s；` |
| [47](../config/22_chassis_force/chassis_force_vehicle_config.c#L47) | 配置初值 | `*               单轮低电流斜坡记录功率/冲击后调整，不是已测得的电流响应斜率。` |
| [48](../config/22_chassis_force/chassis_force_vehicle_config.c#L48) | 配置初值 | `*/` |
| [49](../config/22_chassis_force/chassis_force_vehicle_config.c#L49) | 配置初值 | `.wheel={` |
| [50](../config/22_chassis_force/chassis_force_vehicle_config.c#L50) | 配置初值 | `[0]={ /* LF 左前，CAN1 ID 1。几何在下方从原车配置装配。 */` |
| [51](../config/22_chassis_force/chassis_force_vehicle_config.c#L51) | 配置初值 | `.k_out_nm_a=0.0f,   /* [待测] N*m/A，输出轴转矩常数，按上述力臂法拟合。 */` |
| [52](../config/22_chassis_force/chassis_force_vehicle_config.c#L52) | 配置初值 | `.current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */` |
| [53](../config/22_chassis_force/chassis_force_vehicle_config.c#L53) | 配置初值 | `.force_limit_n=0.0f, /* [待测] N，保守可传递轮缘力，低力加载验证。 */` |
| [54](../config/22_chassis_force/chassis_force_vehicle_config.c#L54) | 配置初值 | `.current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */` |
| [55](../config/22_chassis_force/chassis_force_vehicle_config.c#L55) | 配置初值 | `},` |
| [56](../config/22_chassis_force/chassis_force_vehicle_config.c#L56) | 配置初值 | `[1]={ /* RF 右前，CAN1 ID 4。 */` |
| [57](../config/22_chassis_force/chassis_force_vehicle_config.c#L57) | 配置初值 | `.k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右前轮组。 */` |
| [58](../config/22_chassis_force/chassis_force_vehicle_config.c#L58) | 配置初值 | `.current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */` |
| [59](../config/22_chassis_force/chassis_force_vehicle_config.c#L59) | 配置初值 | `.force_limit_n=0.0f, /* [待测] N。 */` |
| [60](../config/22_chassis_force/chassis_force_vehicle_config.c#L60) | 配置初值 | `.current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */` |
| [61](../config/22_chassis_force/chassis_force_vehicle_config.c#L61) | 配置初值 | `},` |
| [62](../config/22_chassis_force/chassis_force_vehicle_config.c#L62) | 配置初值 | `[2]={ /* LB 左后，CAN1 ID 2。 */` |
| [63](../config/22_chassis_force/chassis_force_vehicle_config.c#L63) | 配置初值 | `.k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量左后轮组。 */` |
| [64](../config/22_chassis_force/chassis_force_vehicle_config.c#L64) | 配置初值 | `.current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */` |
| [65](../config/22_chassis_force/chassis_force_vehicle_config.c#L65) | 配置初值 | `.force_limit_n=0.0f, /* [待测] N。 */` |
| [66](../config/22_chassis_force/chassis_force_vehicle_config.c#L66) | 配置初值 | `.current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */` |
| [67](../config/22_chassis_force/chassis_force_vehicle_config.c#L67) | 配置初值 | `},` |
| [68](../config/22_chassis_force/chassis_force_vehicle_config.c#L68) | 配置初值 | `[3]={ /* RB 右后，CAN1 ID 3。 */` |
| [69](../config/22_chassis_force/chassis_force_vehicle_config.c#L69) | 配置初值 | `.k_out_nm_a=0.0f,    /* [待测] N*m/A，单独测量右后轮组。 */` |
| [70](../config/22_chassis_force/chassis_force_vehicle_config.c#L70) | 配置初值 | `.current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，原速控/功率上限。 */` |
| [71](../config/22_chassis_force/chassis_force_vehicle_config.c#L71) | 配置初值 | `.force_limit_n=0.0f, /* [待测] N。 */` |
| [72](../config/22_chassis_force/chassis_force_vehicle_config.c#L72) | 配置初值 | `.current_slew=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT/1.0f, /* [初值] CAN 命令/s，旧上限的 1 s 斜坡。 */` |
| [73](../config/22_chassis_force/chassis_force_vehicle_config.c#L73) | 配置初值 | `},` |
| [74](../config/22_chassis_force/chassis_force_vehicle_config.c#L74) | 配置初值 | `},` |
| [76](../config/22_chassis_force/chassis_force_vehicle_config.c#L76) | 配置初值 | `/* 车体力/转矩是可选择的操作幅度，不是电机或轮地能力的标定常数。` |
| [77](../config/22_chassis_force/chassis_force_vehicle_config.c#L77) | 配置初值 | `* 以下采用低幅起点；物理输出仍受逐轮力/电流、速度及功率限制，不从旧目标轮速换算。 */` |
| [78](../config/22_chassis_force/chassis_force_vehicle_config.c#L78) | 配置初值 | `.force_max_n=10.0f,            /* [初值] N，满输入合力；低力加载/加速试验后调整。 */` |
| [79](../config/22_chassis_force/chassis_force_vehicle_config.c#L79) | 配置初值 | `.spin_torque_nm=0.3f,          /* [初值] N*m，固定旋转请求；用电流/负载功率日志复核。 */` |
| [80](../config/22_chassis_force/chassis_force_vehicle_config.c#L80) | 配置初值 | `.spin_start_torque_nm=0.1f,    /* [初值] N*m，自适应起步请求；不保证起转，也不强制最小输出。 */` |
| [81](../config/22_chassis_force/chassis_force_vehicle_config.c#L81) | 配置初值 | `.spin_torque_max_nm=1.0f,      /* [调试限值] N*m，自适应请求上限；不是最大物理转矩的测量值。 */` |
| [82](../config/22_chassis_force/chassis_force_vehicle_config.c#L82) | 配置初值 | `.force_rise_n_s=20.0f,         /* [初值] N/s，0 到 10 N 约 0.5 s；阶跃试验核对冲击。 */` |
| [83](../config/22_chassis_force/chassis_force_vehicle_config.c#L83) | 配置初值 | `.force_fall_n_s=40.0f,         /* [初值] N/s，10 N 撤至 0 约 0.25 s；松杆试验调整。 */` |
| [84](../config/22_chassis_force/chassis_force_vehicle_config.c#L84) | 配置初值 | `.torque_rise_nm_s=0.5f,        /* [初值] N*m/s，0 到固定 0.3 N*m 约 0.6 s。 */` |
| [85](../config/22_chassis_force/chassis_force_vehicle_config.c#L85) | 配置初值 | `.torque_fall_nm_s=1.0f,        /* [初值] N*m/s，固定 0.3 N*m 撤至 0 约 0.3 s。 */` |
| [87](../config/22_chassis_force/chassis_force_vehicle_config.c#L87) | 配置初值 | `/* 速度只作保护，不是正常驱动力的目标值。轮级 rpm 一律指电机转子。` |
| [88](../config/22_chassis_force/chassis_force_vehicle_config.c#L88) | 配置初值 | `* 原工程未配置经过验证的机械硬上限；此处人为限制低速调试，仍需确认估计方向和实际峰值。 */` |
| [89](../config/22_chassis_force/chassis_force_vehicle_config.c#L89) | 配置初值 | `.velocity_limit_mps=0.3f,      /* [调试限值] m/s，平移保护；外部测速核对，不是车体最高速度。 */` |
| [90](../config/22_chassis_force/chassis_force_vehicle_config.c#L90) | 配置初值 | `.omega_limit_rad_s=1.0f,       /* [调试限值] rad/s，旋转保护；独立测速核对撤力延迟。 */` |
| [91](../config/22_chassis_force/chassis_force_vehicle_config.c#L91) | 配置初值 | `.wheel_limit_rpm=1000.0f,      /* [调试限值] 转子 rpm，轮级保护；不是轮子输出轴 rpm。 */` |
| [92](../config/22_chassis_force/chassis_force_vehicle_config.c#L92) | 配置初值 | `.resume_ratio=0.9f,            /* [初值] 无单位，(0,1)；降至上限乘此比例后恢复，检查有无抖动。 */` |
| [93](../config/22_chassis_force/chassis_force_vehicle_config.c#L93) | 配置初值 | `.startup_current_limit=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT*0.1f, /* [调试限值] CAN 命令幅值，旧上限的 10%；验证起步。 */` |
| [94](../config/22_chassis_force/chassis_force_vehicle_config.c#L94) | 配置初值 | `.speed_prediction_rpm=0.0f,    /* [待测] 转子 rpm，功率预测增长裕量；取实测预测窗口内最大增长并留裕量。 */` |
| [96](../config/22_chassis_force/chassis_force_vehicle_config.c#L96) | 配置初值 | `/* 堵转/温度：不以长时间硬堵转测试阈值；使用受限短脉冲或负载台架。 */` |
| [97](../config/22_chassis_force/chassis_force_vehicle_config.c#L97) | 配置初值 | `.stall_current_command=1000.0f, /* [调试限值] CAN 命令幅值；低于起步上限 1500，使近零速堵转检测可触发。 */` |
| [98](../config/22_chassis_force/chassis_force_vehicle_config.c#L98) | 配置初值 | `.stall_rpm=20.0f,              /* [初值] 转子 rpm，低速判据；记录静止噪声后复核，可能误触发。 */` |
| [99](../config/22_chassis_force/chassis_force_vehicle_config.c#L99) | 配置初值 | `.stall_timeout_ms=100U,        /* [调试限值] ms，达到上述电流且低速时的连续确认时间；不是允许堵转时长的实测结果。 */` |
| [100](../config/22_chassis_force/chassis_force_vehicle_config.c#L100) | 配置初值 | `.temp_derate_c=50.0f,          /* [调试限值] 摄氏度，提前降额；先核对温度反馈与环境温度。 */` |
| [101](../config/22_chassis_force/chassis_force_vehicle_config.c#L101) | 配置初值 | `.temp_stop_c=60.0f,            /* [调试限值] 摄氏度，停机；不是电机额定耐温，按温升试验复核。 */` |
| [103](../config/22_chassis_force/chassis_force_vehicle_config.c#L103) | 配置初值 | `/* 退出制动：只预填参数，不改变默认 EXIT_MODE=0（滑行）。启用制动后须验证不反向持续驱动。 */` |
| [104](../config/22_chassis_force/chassis_force_vehicle_config.c#L104) | 配置初值 | `.brake_torque_nm=0.2f,         /* [初值] N*m，单次反向请求；低转速核对制动距离、电流与功率。 */` |
| [105](../config/22_chassis_force/chassis_force_vehicle_config.c#L105) | 配置初值 | `.brake_stop_rad_s=0.1f,        /* [初值] rad/s，停止死区；静止噪声与撤销制动试验后调整。 */` |
| [106](../config/22_chassis_force/chassis_force_vehicle_config.c#L106) | 配置初值 | `.brake_timeout_ms=300U,        /* [调试限值] ms，单次制动最多持续时间；不承诺在此时间内停稳。 */` |
| [108](../config/22_chassis_force/chassis_force_vehicle_config.c#L108) | 配置初值 | `/* 输入整形：摇杆径向死区和力曲线；键盘同样经过单位圆限幅与力斜率。 */` |
| [109](../config/22_chassis_force/chassis_force_vehicle_config.c#L109) | 配置初值 | `.input_deadzone=0.05f,         /* [初值] 归一化幅值，[0,1)；记录松杆噪声，设置为噪声包络以上。 */` |
| [110](../config/22_chassis_force/chassis_force_vehicle_config.c#L110) | 配置初值 | `.input_gamma=1.5f,             /* [初值] 无单位，>0；低输入驾驶试验调节细腻程度。 */` |
| [112](../config/22_chassis_force/chassis_force_vehicle_config.c#L112) | 配置初值 | `/* 自适应功率环：软件初值，不是已完成的功率计/整车标定结果。` |
| [113](../config/22_chassis_force/chassis_force_vehicle_config.c#L113) | 配置初值 | `* 即使当前使用固定转矩模式，这些字段也需要保持合法正值以通过统一参数校验。` |
| [114](../config/22_chassis_force/chassis_force_vehicle_config.c#L114) | 配置初值 | `* 功率计必须位于受控底盘负载侧，先核对零点/方向/采样周期，再做低转矩功率阶跃。` |
| [115](../config/22_chassis_force/chassis_force_vehicle_config.c#L115) | 配置初值 | `*/` |
| [116](../config/22_chassis_force/chassis_force_vehicle_config.c#L116) | 配置初值 | `.power_gain_nm_per_w_s=0.01f,  /* [初值] N*m/(W*s)，功率误差增益；低转矩阶跃整定，观察超调与恢复。 */` |
| [117](../config/22_chassis_force/chassis_force_vehicle_config.c#L117) | 配置初值 | `.power_deadband_w=2.0f,        /* [初值] W，功率死区；记录零点和稳态功率噪声后复核。 */` |
| [118](../config/22_chassis_force/chassis_force_vehicle_config.c#L118) | 配置初值 | `.power_lpf_tau_s=0.02f,        /* [初值] s，功率跟踪低通；按采样间隔/波动整定，不能代替快保护。 */` |
| [119](../config/22_chassis_force/chassis_force_vehicle_config.c#L119) | 配置初值 | `.power_recovery_samples=3U,    /* [初值] 样本数，3~20；连续新鲜功率样本确认，重复样本不计数。 */` |
| [120](../config/22_chassis_force/chassis_force_vehicle_config.c#L120) | 配置初值 | `.torque_probe_step_nm=0.05f,   /* [初值] N*m，单次候选转矩增量；低转矩申请试验确认，不能大于转矩上限。 */` |
| [121](../config/22_chassis_force/chassis_force_vehicle_config.c#L121) | 配置初值 | `.power_request_step_w=5.0f,    /* [初值] W，申请功率步长；观察能源批准/拒绝，要求 >0 且 <=20。 */` |
| [122](../config/22_chassis_force/chassis_force_vehicle_config.c#L122) | 配置初值 | `.grant_timeout_ms=50U,         /* [初值] ms，等待批准超时；测慢任务及输入确认延迟，允许范围 20~1000。 */` |
| [151](../config/22_chassis_force/chassis_force_vehicle_config.c#L151) | 配置初值 | `ChassisPower_Init_Config_s c={` |
| [152](../config/22_chassis_force/chassis_force_vehicle_config.c#L152) | 配置初值 | `.stage=3, /* 功控阶段：固定为 3 完整慢环；直接力控不支持较早阶段。 */` |
| [153](../config/22_chassis_force/chassis_force_vehicle_config.c#L153) | 配置初值 | `.force_control=true, /* 后端标志：true 使用直接力控快路径，不是构建后端选择开关。 */` |
| [154](../config/22_chassis_force/chassis_force_vehicle_config.c#L154) | 配置初值 | `.meter_enabled=POWER_METER_ENABLE!=0, /* 负载功率计是否启用；由宏控制，默认启用。 */` |
| [155](../config/22_chassis_force/chassis_force_vehicle_config.c#L155) | 配置初值 | `.buffer_enabled=CHASSIS_BUFFER_POWER_ENABLE!=0, /* 裁判缓冲增补开关：0 关闭、1 启用，默认 1。 */` |
| [156](../config/22_chassis_force/chassis_force_vehicle_config.c#L156) | 配置初值 | `.use_supercap=CHASSIS_POWER_SOURCE_MODE==CHASSIS_POWER_SOURCE_SUPERCAP, /* 是否按超级电容供电策略处理；默认直供为 false。 */` |
| [157](../config/22_chassis_force/chassis_force_vehicle_config.c#L157) | 配置初值 | `.algorithm={ /* 共用功率分配接口的数值配置。 */` |
| [158](../config/22_chassis_force/chassis_force_vehicle_config.c#L158) | 配置初值 | `.safety_factor=.98f,                 /* [沿用] 无单位，物理功率安全系数；按模型误差复核裕量。 */` |
| [159](../config/22_chassis_force/chassis_force_vehicle_config.c#L159) | 配置初值 | `.small_error_threshold_rpm=500,      /* [沿用] rpm，旧速控接口参数，力控不以它生成速度误差。 */` |
| [160](../config/22_chassis_force/chassis_force_vehicle_config.c#L160) | 配置初值 | `.reserved_power_threshold_w=54,      /* [沿用] W，旧分配器参数，力控不走旧轮速分配路径。 */` |
| [161](../config/22_chassis_force/chassis_force_vehicle_config.c#L161) | 配置初值 | `.per_motor_reserved_power_w=8,       /* [沿用] W，同上，保留共用配置结构兼容性。 */` |
| [162](../config/22_chassis_force/chassis_force_vehicle_config.c#L162) | 配置初值 | `.max_current_command=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* [沿用] CAN 命令幅值，与四轮原限流保持一致。 */` |
| [163](../config/22_chassis_force/chassis_force_vehicle_config.c#L163) | 配置初值 | `},` |
| [164](../config/22_chassis_force/chassis_force_vehicle_config.c#L164) | 配置初值 | `.transient={ /* 快速超功率保护及旧速度后端兼容参数。 */` |
| [165](../config/22_chassis_force/chassis_force_vehicle_config.c#L165) | 配置初值 | `/* [沿用] 与 ChassisPowerTransientDefaultConfig 相同；力控快路径实际读取以下四项。` |
| [166](../config/22_chassis_force/chassis_force_vehicle_config.c#L166) | 配置初值 | `* 低功率阶跃验证：超限迅速收紧，恢复需连续新鲜样本，恢复斜率不引入反冲。 */` |
| [167](../config/22_chassis_force/chassis_force_vehicle_config.c#L167) | 配置初值 | `.fast_gain=.5f,                      /* 无单位，实测功率超限量到负向快修正的增益。 */` |
| [168](../config/22_chassis_force/chassis_force_vehicle_config.c#L168) | 配置初值 | `.release_w_per_s=60,                 /* W/s，负向快修正释放速度。 */` |
| [169](../config/22_chassis_force/chassis_force_vehicle_config.c#L169) | 配置初值 | `.release_confirm_ms=10U,             /* ms，解除超功率前低于门槛的连续确认时间。 */` |
| [170](../config/22_chassis_force/chassis_force_vehicle_config.c#L170) | 配置初值 | `.stable_run_ms=100U,                 /* ms，驱动稳定/持续超限后允许慢模型学习的时间。 */` |
| [172](../config/22_chassis_force/chassis_force_vehicle_config.c#L172) | 配置初值 | `/* [沿用] 下列为共用旧快保护结构的兼容字段，力控不走旧启动峰值或轮速分配器。` |
| [173](../config/22_chassis_force/chassis_force_vehicle_config.c#L173) | 配置初值 | `* 力控实际电流、速度及转速预测限制由上方车辆参数决定，不通过此处放宽。 */` |
| [174](../config/22_chassis_force/chassis_force_vehicle_config.c#L174) | 配置初值 | `.eta=.95f,                          /* 无单位，旧接口初值；校验时用 algorithm.safety_factor 替换。 */` |
| [175](../config/22_chassis_force/chassis_force_vehicle_config.c#L175) | 配置初值 | `.stale_release_w_per_s=20,           /* W/s，旧失联恢复速度；力控失联时不释放负向快修正。 */` |
| [176](../config/22_chassis_force/chassis_force_vehicle_config.c#L176) | 配置初值 | `.meter_fresh_ms=20U,                 /* ms，旧功率计新鲜门槛；力控快路径固定检查 <=20 ms。 */` |
| [177](../config/22_chassis_force/chassis_force_vehicle_config.c#L177) | 配置初值 | `.feedback_fresh_ms=5U,               /* ms，旧轮反馈门槛；力控快路径固定检查 <=5 ms。 */` |
| [178](../config/22_chassis_force/chassis_force_vehicle_config.c#L178) | 配置初值 | `.max_current_command=BOARD_CHASSIS_LEGACY_CURRENT_LIMIT, /* CAN 命令幅值，旧限流；校验时使用 algorithm 值。 */` |
| [179](../config/22_chassis_force/chassis_force_vehicle_config.c#L179) | 配置初值 | `.acceleration_tau_s=.010f,           /* s，旧转速增长滤波常数，力控使用实测 speed_prediction_rpm。 */` |
| [180](../config/22_chassis_force/chassis_force_vehicle_config.c#L180) | 配置初值 | `.prediction_s=.008f,                 /* s，旧转速预测窗口，复核增长裕量时可参考其时长。 */` |
| [181](../config/22_chassis_force/chassis_force_vehicle_config.c#L181) | 配置初值 | `.growth_time_s=.005f,                /* s，旧增长兜底时间，力控不使用。 */` |
| [182](../config/22_chassis_force/chassis_force_vehicle_config.c#L182) | 配置初值 | `.peak_ratio=1.30f,                   /* 无单位，旧启动峰值比例，力控不开放该额外额度。 */` |
| [183](../config/22_chassis_force/chassis_force_vehicle_config.c#L183) | 配置初值 | `.low_rpm=100,                        /* 转子 rpm，旧低速判据，力控不使用。 */` |
| [184](../config/22_chassis_force/chassis_force_vehicle_config.c#L184) | 配置初值 | `.low_current=500,                    /* CAN 命令幅值，旧低需求判据，力控不使用。 */` |
| [185](../config/22_chassis_force/chassis_force_vehicle_config.c#L185) | 配置初值 | `.target_step_rpm=300,                /* 转子 rpm，旧目标阶跃门槛，力控没有目标轮速。 */` |
| [186](../config/22_chassis_force/chassis_force_vehicle_config.c#L186) | 配置初值 | `.zero_enter_rpm=50,                  /* 转子 rpm，旧近零速进入门槛，力控不使用。 */` |
| [187](../config/22_chassis_force/chassis_force_vehicle_config.c#L187) | 配置初值 | `.zero_exit_rpm=100,                  /* 转子 rpm，旧近零速退出门槛，力控不使用。 */` |
| [188](../config/22_chassis_force/chassis_force_vehicle_config.c#L188) | 配置初值 | `.demand_enter_current=200,           /* CAN 命令幅值，旧需求进入门槛，力控不使用。 */` |
| [189](../config/22_chassis_force/chassis_force_vehicle_config.c#L189) | 配置初值 | `.demand_exit_current=100,            /* CAN 命令幅值，旧需求退出门槛，力控不使用。 */` |
| [190](../config/22_chassis_force/chassis_force_vehicle_config.c#L190) | 配置初值 | `.low_hold_ms=100U,                   /* ms，旧低需求保持时间，力控不使用。 */` |
| [191](../config/22_chassis_force/chassis_force_vehicle_config.c#L191) | 配置初值 | `.startup_window_ms=30U,              /* ms，旧启动峰值窗口，力控不使用。 */` |
| [192](../config/22_chassis_force/chassis_force_vehicle_config.c#L192) | 配置初值 | `.startup_cooldown_ms=500U,           /* ms，旧启动峰值冷却时间，力控不使用。 */` |
| [193](../config/22_chassis_force/chassis_force_vehicle_config.c#L193) | 配置初值 | `.guard_search_nodes=256U,            /* 搜索节点数，旧 guard 工作量上限，力控使用自己的有界分配器。 */` |
| [194](../config/22_chassis_force/chassis_force_vehicle_config.c#L194) | 配置初值 | `},` |
| [195](../config/22_chassis_force/chassis_force_vehicle_config.c#L195) | 配置初值 | `.outer={ /* 模型误差的慢速预算修正参数。 */` |
| [196](../config/22_chassis_force/chassis_force_vehicle_config.c#L196) | 配置初值 | `.kp=1,                              /* [沿用] 无单位，功率模型慢修正比例增益；功率阶跃整定。 */` |
| [197](../config/22_chassis_force/chassis_force_vehicle_config.c#L197) | 配置初值 | `.ki=1.5f,                           /* [沿用] 1/s，慢修正积分增益；观察稳态误差与恢复。 */` |
| [198](../config/22_chassis_force/chassis_force_vehicle_config.c#L198) | 配置初值 | `.measurement_lpf_tau_s=.030f,        /* [沿用] s，慢环功率滤波时间常数；依据采样噪声复核。 */` |
| [199](../config/22_chassis_force/chassis_force_vehicle_config.c#L199) | 配置初值 | `.activation_margin_w=5,              /* [沿用] W，接近目标功率时的激活裕量。 */` |
| [200](../config/22_chassis_force/chassis_force_vehicle_config.c#L200) | 配置初值 | `.error_deadband_w=.3f,               /* [沿用] W，慢修正误差死区；与负载功率计噪声比较。 */` |
| [201](../config/22_chassis_force/chassis_force_vehicle_config.c#L201) | 配置初值 | `.max_positive_correction_w=8,        /* [沿用] W，模型正修正上限，不是额外物理能源额度。 */` |
| [202](../config/22_chassis_force/chassis_force_vehicle_config.c#L202) | 配置初值 | `.positive_slew_rate_w_per_s=60,      /* [沿用] W/s，正修正恢复斜率；预算阶跃时检查冲击。 */` |
| [203](../config/22_chassis_force/chassis_force_vehicle_config.c#L203) | 配置初值 | `.budget_drop_reset_w=1,              /* [沿用] W，预算下降复位阈值；验证下降时不释放旧积分。 */` |
| [204](../config/22_chassis_force/chassis_force_vehicle_config.c#L204) | 配置初值 | `.antiwindup_gain_per_s=10,           /* [沿用] 1/s，慢修正抗饱和增益；检查限幅解除后的恢复。 */` |
| [205](../config/22_chassis_force/chassis_force_vehicle_config.c#L205) | 配置初值 | `.low_demand_delay_s=.5f,             /* [沿用] s，低需求保持时间，超时后衰减历史修正。 */` |
| [206](../config/22_chassis_force/chassis_force_vehicle_config.c#L206) | 配置初值 | `.low_demand_decay_w_per_s=1,         /* [沿用] W/s，低需求修正衰减速度。 */` |
| [207](../config/22_chassis_force/chassis_force_vehicle_config.c#L207) | 配置初值 | `},` |
| [208](../config/22_chassis_force/chassis_force_vehicle_config.c#L208) | 配置初值 | `.budget={ /* 负载预算与输入侧缓冲预算；两层功率分别限幅。 */` |
| [209](../config/22_chassis_force/chassis_force_vehicle_config.c#L209) | 配置初值 | `.empty_energy_ratio=.1f,             /* [沿用] 无单位，0~1，电容额外负载能力的空电门槛。 */` |
| [210](../config/22_chassis_force/chassis_force_vehicle_config.c#L210) | 配置初值 | `.full_energy_ratio=.3f,              /* [沿用] 无单位，0~1，电容增补全部开放门槛；用充放电日志复核。 */` |
| [211](../config/22_chassis_force/chassis_force_vehicle_config.c#L211) | 配置初值 | `.max_boost_w=100,                    /* [沿用] W，电容负载侧增补上限；始终受 051 总能力封顶。 */` |
| [212](../config/22_chassis_force/chassis_force_vehicle_config.c#L212) | 配置初值 | `.rise_rate_w_per_s=200,              /* [沿用] W/s，负载额度上升斜率，下降不等待斜率。 */` |
| [213](../config/22_chassis_force/chassis_force_vehicle_config.c#L213) | 配置初值 | `.demand_margin_w=CHASSIS_POWER_DEMAND_MARGIN_W, /* [沿用] W，真实需求申请裕量，验证零需求不申请。 */` |
| [214](../config/22_chassis_force/chassis_force_vehicle_config.c#L214) | 配置初值 | `.buffer={ /* 裁判缓冲能量观测和恢复策略。 */` |
| [215](../config/22_chassis_force/chassis_force_vehicle_config.c#L215) | 配置初值 | `.max_energy_j=CHASSIS_BUFFER_MAX_ENERGY_J,                  /* [沿用] J，裁判缓冲最大有效值。 */` |
| [216](../config/22_chassis_force/chassis_force_vehicle_config.c#L216) | 配置初值 | `.recovery_enter_j=CHASSIS_BUFFER_RECOVERY_ENTER_J,          /* [沿用] J，进入恢复阈值；低缓冲试验验证。 */` |
| [217](../config/22_chassis_force/chassis_force_vehicle_config.c#L217) | 配置初值 | `.recovery_exit_j=CHASSIS_BUFFER_RECOVERY_EXIT_J,            /* [沿用] J，退出恢复阈值，应高于进入值。 */` |
| [218](../config/22_chassis_force/chassis_force_vehicle_config.c#L218) | 配置初值 | `.hard_floor_j=CHASSIS_BUFFER_HARD_FLOOR_J,                  /* [沿用] J，预测缓冲硬底座。 */` |
| [219](../config/22_chassis_force/chassis_force_vehicle_config.c#L219) | 配置初值 | `.prediction_window_s=CHASSIS_BUFFER_PREDICTION_WINDOW_S,    /* [沿用] s，输入增补能量预测窗口。 */` |
| [220](../config/22_chassis_force/chassis_force_vehicle_config.c#L220) | 配置初值 | `.guard_window_s=CHASSIS_BUFFER_GUARD_WINDOW_S,              /* [沿用] s，输入超功率预测保护窗口。 */` |
| [221](../config/22_chassis_force/chassis_force_vehicle_config.c#L221) | 配置初值 | `.settlement_delay_s=CHASSIS_BUFFER_SETTLEMENT_DELAY_S,      /* [沿用] s，裁判结算延迟裕量，按实测消息时序复核。 */` |
| [222](../config/22_chassis_force/chassis_force_vehicle_config.c#L222) | 配置初值 | `.energy_margin_j=CHASSIS_BUFFER_ENERGY_MARGIN_J,           /* [沿用] J，缓冲估计能量裕量。 */` |
| [223](../config/22_chassis_force/chassis_force_vehicle_config.c#L223) | 配置初值 | `.power_margin_w=CHASSIS_BUFFER_POWER_MARGIN_W,             /* [沿用] W，输入功率测量/调度裕量。 */` |
| [224](../config/22_chassis_force/chassis_force_vehicle_config.c#L224) | 配置初值 | `.max_boost_w=CHASSIS_BUFFER_BOOST_MAX_W,                    /* [沿用] W，裁判缓冲允许的输入侧增补上限。 */` |
| [225](../config/22_chassis_force/chassis_force_vehicle_config.c#L225) | 配置初值 | `.recovery_power_w=CHASSIS_BUFFER_RECOVERY_POWER_W,          /* [沿用] W，恢复时最大输入功率削减量。 */` |
| [226](../config/22_chassis_force/chassis_force_vehicle_config.c#L226) | 配置初值 | `.recovery_target_time_s=CHASSIS_BUFFER_RECOVERY_TARGET_TIME_S, /* [沿用] s，缓冲恢复目标时间。 */` |
| [227](../config/22_chassis_force/chassis_force_vehicle_config.c#L227) | 配置初值 | `.observer_max_step_s=CHASSIS_BUFFER_OBSERVER_MAX_STEP_S,    /* [沿用] s，输入侧能量观测最大有效步长。 */` |
| [228](../config/22_chassis_force/chassis_force_vehicle_config.c#L228) | 配置初值 | `.recovery_hold_s=CHASSIS_BUFFER_RECOVERY_HOLD_S,            /* [沿用] s，退出恢复前连续保持时间。 */` |
| [229](../config/22_chassis_force/chassis_force_vehicle_config.c#L229) | 配置初值 | `.rise_rate_w_per_s=CHASSIS_BUFFER_RISE_RATE_W_PER_S,        /* [沿用] W/s，输入额度上升斜率。 */` |
| [230](../config/22_chassis_force/chassis_force_vehicle_config.c#L230) | 配置初值 | `.safety_factor=.98f,                                       /* [沿用] 无单位，与功率算法使用同一换算系数。 */` |
| [231](../config/22_chassis_force/chassis_force_vehicle_config.c#L231) | 配置初值 | `.correction_rise_j_per_s=CHASSIS_BUFFER_CORRECTION_RISE_J_PER_S, /* [沿用] J/s，缓冲估计向上修正速率。 */` |
| [232](../config/22_chassis_force/chassis_force_vehicle_config.c#L232) | 配置初值 | `},` |
| [233](../config/22_chassis_force/chassis_force_vehicle_config.c#L233) | 配置初值 | `},` |
| [240](../config/22_chassis_force/chassis_force_vehicle_config.c#L240) | 配置初值 | `const MotorPowerModelConfig_s m={` |
| [241](../config/22_chassis_force/chassis_force_vehicle_config.c#L241) | 配置初值 | `.k0=1.3123816f,       /* [沿用] W，常数损耗项。 */` |
| [242](../config/22_chassis_force/chassis_force_vehicle_config.c#L242) | 配置初值 | `.k1=-.25836727f,      /* [沿用] W/缩放电流单位，一次电流项。 */` |
| [243](../config/22_chassis_force/chassis_force_vehicle_config.c#L243) | 配置初值 | `.k2=.0007424524f,     /* [沿用] W/rpm，一次转速项。 */` |
| [244](../config/22_chassis_force/chassis_force_vehicle_config.c#L244) | 配置初值 | `.k3=.0024271158f,     /* [沿用] W/(缩放电流单位*rpm)，电流转速交叉项。 */` |
| [245](../config/22_chassis_force/chassis_force_vehicle_config.c#L245) | 配置初值 | `.k4=.081198552f,      /* [沿用] W/(缩放电流单位^2)，二次电流项。 */` |
| [246](../config/22_chassis_force/chassis_force_vehicle_config.c#L246) | 配置初值 | `.k5=3.890596e-8f,     /* [沿用] W/rpm^2，二次转速项。 */` |
| [247](../config/22_chassis_force/chassis_force_vehicle_config.c#L247) | 配置初值 | `.current_conversion=1000.0f, /* [沿用] 每个缩放电流单位对应的 CAN 命令值，不能单改而保留系数。 */` |

## config/22_chassis_force/chassis_force_vehicle_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/22_chassis_force/chassis_force_vehicle_config.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_VEHICLE_CONFIG_H` |
| [2](../config/22_chassis_force/chassis_force_vehicle_config.h#L2) | 预处理 | `#define CHASSIS_FORCE_VEHICLE_CONFIG_H` |
| [15](../config/22_chassis_force/chassis_force_vehicle_config.h#L15) | 预处理 | `#endif` |

## config/22_chassis_force/force_calibration_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/22_chassis_force/force_calibration_config.h#L1) | 预处理 | `#ifndef FORCE_CALIBRATION_CONFIG_H` |
| [2](../config/22_chassis_force/force_calibration_config.h#L2) | 预处理 | `#define FORCE_CALIBRATION_CONFIG_H` |
| [12](../config/22_chassis_force/force_calibration_config.h#L12) | 预处理 | `#ifndef CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [13](../config/22_chassis_force/force_calibration_config.h#L13) | 预处理 | `#define CHASSIS_FORCE_CALIBRATION_ENABLE 0 /* 标定固件开关：默认 0 不编译标定模块，1 编译；必须使用上述构建参数切换。 */` |
| [14](../config/22_chassis_force/force_calibration_config.h#L14) | 预处理 | `#endif` |
| [15](../config/22_chassis_force/force_calibration_config.h#L15) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE != 0 && CHASSIS_FORCE_CALIBRATION_ENABLE != 1` |
| [17](../config/22_chassis_force/force_calibration_config.h#L17) | 预处理 | `#endif` |
| [18](../config/22_chassis_force/force_calibration_config.h#L18) | 预处理 | `#define FC_RECORD_BYTES 48U /* 每条标定记录的线格式长度，单位字节；固定协议为 48，不能单独改此宏扩展记录。 */` |
| [19](../config/22_chassis_force/force_calibration_config.h#L19) | 预处理 | `#define FC_RECORD_CAPACITY 1280U /* 最多缓存的记录条数；默认 1280，记录区与控制状态合计受 64 KiB 静态断言约束。 */` |
| [20](../config/22_chassis_force/force_calibration_config.h#L20) | 预处理 | `#define FC_RESERVED_RECORDS 8U /* 采集接近容量时用于提前停止的预留记录数；默认8，直接影响容量截止点。 */` |
| [21](../config/22_chassis_force/force_calibration_config.h#L21) | 预处理 | `#define FC_MAX_CYCLE_US 5000U /* 标定控制周期最大允许间隔，单位 us；超过时停止标定或标记采集质量不合格。 */` |
| [22](../config/22_chassis_force/force_calibration_config.h#L22) | 预处理 | `#define FC_MAX_FEEDBACK_US 5000U /* 电机反馈最大允许年龄/相邻反馈间隔，单位 us；超过时停止标定或标记质量不合格。 */` |
| [23](../config/22_chassis_force/force_calibration_config.h#L23) | 预处理 | `#endif` |

## config/30_velocity_observation/chassis_remote_imu_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/30_velocity_observation/chassis_remote_imu_config.h#L1) | 预处理 | `#ifndef CHASSIS_REMOTE_IMU_CONFIG_H` |
| [2](../config/30_velocity_observation/chassis_remote_imu_config.h#L2) | 预处理 | `#define CHASSIS_REMOTE_IMU_CONFIG_H` |
| [12](../config/30_velocity_observation/chassis_remote_imu_config.h#L12) | 预处理 | `#ifndef CHASSIS_REMOTE_IMU_CONFIG_VERIFIED` |
| [13](../config/30_velocity_observation/chassis_remote_imu_config.h#L13) | 预处理 | `#define CHASSIS_REMOTE_IMU_CONFIG_VERIFIED 0 /* 0=未实测初值；完成安装/参数审核后才设1；只作记录标识，不放行控制。 */` |
| [14](../config/30_velocity_observation/chassis_remote_imu_config.h#L14) | 预处理 | `#endif` |
| [15](../config/30_velocity_observation/chassis_remote_imu_config.h#L15) | 预处理 | `#if CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 0 && CHASSIS_REMOTE_IMU_CONFIG_VERIFIED != 1` |
| [17](../config/30_velocity_observation/chassis_remote_imu_config.h#L17) | 预处理 | `#endif` |
| [20](../config/30_velocity_observation/chassis_remote_imu_config.h#L20) | 预处理 | `#define CHASSIS_REMOTE_IMU_SENSOR_TO_BODY {1.0f, 0.0f, 0.0f, 0.0f}` |
| [24](../config/30_velocity_observation/chassis_remote_imu_config.h#L24) | 预处理 | `#define CHASSIS_REMOTE_IMU_OFFSET_BODY_M {0.0f, 0.116f, 0.0f}` |
| [28](../config/30_velocity_observation/chassis_remote_imu_config.h#L28) | 预处理 | `#define CHASSIS_REMOTE_IMU_WHEEL_VARIANCE 0.01f` |
| [29](../config/30_velocity_observation/chassis_remote_imu_config.h#L29) | 预处理 | `#define CHASSIS_REMOTE_IMU_ACCEL_VARIANCE 0.25f` |
| [30](../config/30_velocity_observation/chassis_remote_imu_config.h#L30) | 预处理 | `#define CHASSIS_REMOTE_IMU_JERK_DENSITY 1.0f` |
| [31](../config/30_velocity_observation/chassis_remote_imu_config.h#L31) | 预处理 | `#define CHASSIS_REMOTE_IMU_MAX_RESIDUAL_MPS 1.5f` |
| [37](../config/30_velocity_observation/chassis_remote_imu_config.h#L37) | 预处理 | `#define CHASSIS_REMOTE_IMU_MAX_PAIR_SPAN_MS 15U` |
| [38](../config/30_velocity_observation/chassis_remote_imu_config.h#L38) | 预处理 | `#define CHASSIS_REMOTE_IMU_MAX_WHEEL_SKEW_MS 3U` |
| [39](../config/30_velocity_observation/chassis_remote_imu_config.h#L39) | 预处理 | `#define CHASSIS_REMOTE_IMU_MAX_TIMING_JITTER_MS 10U` |
| [42](../config/30_velocity_observation/chassis_remote_imu_config.h#L42) | 预处理 | `#define CHASSIS_REMOTE_IMU_CONFIG_INITIALIZER { \` |
| [54](../config/30_velocity_observation/chassis_remote_imu_config.h#L54) | 预处理 | `#endif` |

## config/30_velocity_observation/chassis_velocity_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/30_velocity_observation/chassis_velocity_config.h#L1) | 预处理 | `#ifndef CHASSIS_VELOCITY_CONFIG_H` |
| [2](../config/30_velocity_observation/chassis_velocity_config.h#L2) | 预处理 | `#define CHASSIS_VELOCITY_CONFIG_H` |
| [5](../config/30_velocity_observation/chassis_velocity_config.h#L5) | 预处理 | `#define CHASSIS_VELOCITY_MAX_AGE_MS 20U /* 轮速及普通速度快照最大允许年龄，单位毫秒；超过该值判无效。 */` |
| [6](../config/30_velocity_observation/chassis_velocity_config.h#L6) | 预处理 | `#define CHASSIS_FUSED_VELOCITY_MAX_AGE_MS 60U /* 融合速度快照最大允许年龄，单位毫秒；轮速自身仍须满足上面的独立阈值。 */` |
| [7](../config/30_velocity_observation/chassis_velocity_config.h#L7) | 预处理 | `#define CHASSIS_FUSION_RECEIVE_MAX_AGE_US 10000U /* 转换接收时间戳时允许的最大样本年龄，单位微秒。 */` |
| [8](../config/30_velocity_observation/chassis_velocity_config.h#L8) | 预处理 | `#define CHASSIS_FUSION_RECEIVE_MAX_SKEW_US 3000U /* IMU、关节编码器和各轮样本间允许的最大时间偏差，单位微秒。 */` |
| [18](../config/30_velocity_observation/chassis_velocity_config.h#L18) | 预处理 | `#endif` |

## config/30_velocity_observation/predictive_power_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/30_velocity_observation/predictive_power_config.h#L1) | 预处理 | `#ifndef PREDICTIVE_POWER_CONFIG_H` |
| [2](../config/30_velocity_observation/predictive_power_config.h#L2) | 预处理 | `#define PREDICTIVE_POWER_CONFIG_H` |
| [3](../config/30_velocity_observation/predictive_power_config.h#L3) | 预处理 | `#define CHASSIS_PREDICTIVE_LEGACY 0 /* 模式编号 0：使用现有功率控制；编号本身不改，切换下方 MODE。 */` |
| [4](../config/30_velocity_observation/predictive_power_config.h#L4) | 预处理 | `#define CHASSIS_PREDICTIVE_SHADOW 1 /* 模式编号 1：预测算法旁路计算供观察，实际控制继续使用现有路径。 */` |
| [5](../config/30_velocity_observation/predictive_power_config.h#L5) | 预处理 | `#define CHASSIS_PREDICTIVE_CONTROL 2 /* 保留的接管模式编号 2；当前尚未开放，选用会被下面的编译检查拒绝。 */` |
| [9](../config/30_velocity_observation/predictive_power_config.h#L9) | 预处理 | `#ifndef CHASSIS_PREDICTIVE_POWER_MODE` |
| [10](../config/30_velocity_observation/predictive_power_config.h#L10) | 预处理 | `#define CHASSIS_PREDICTIVE_POWER_MODE CHASSIS_PREDICTIVE_LEGACY /* 当前默认选择 0：现有功率控制。 */` |
| [11](../config/30_velocity_observation/predictive_power_config.h#L11) | 预处理 | `#endif` |
| [12](../config/30_velocity_observation/predictive_power_config.h#L12) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE != 0 && CHASSIS_PREDICTIVE_POWER_MODE != 1` |
| [14](../config/30_velocity_observation/predictive_power_config.h#L14) | 预处理 | `#endif` |
| [15](../config/30_velocity_observation/predictive_power_config.h#L15) | 预处理 | `#endif` |

## config/30_velocity_observation/speed_observation_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/30_velocity_observation/speed_observation_config.h#L1) | 预处理 | `#ifndef SPEED_OBSERVATION_CONFIG_H` |
| [2](../config/30_velocity_observation/speed_observation_config.h#L2) | 预处理 | `#define SPEED_OBSERVATION_CONFIG_H` |
| [10](../config/30_velocity_observation/speed_observation_config.h#L10) | 预处理 | `#ifndef CHASSIS_SPEED_CAPTURE_ENABLE` |
| [11](../config/30_velocity_observation/speed_observation_config.h#L11) | 预处理 | `#define CHASSIS_SPEED_CAPTURE_ENABLE 0 /* 0 关闭、1 启用轮速采集缓存与导出；默认关闭，导出要求 LOG_OUTPUT=2 或 3（USART2）。 */` |
| [12](../config/30_velocity_observation/speed_observation_config.h#L12) | 预处理 | `#endif` |
| [13](../config/30_velocity_observation/speed_observation_config.h#L13) | 预处理 | `#ifndef CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [14](../config/30_velocity_observation/speed_observation_config.h#L14) | 预处理 | `#define CHASSIS_SPEED_KF_SHADOW_ENABLE 0 /* 0 关闭、1 启用轮速卡尔曼旁路观察；默认关闭，必须同时选择预测功率 SHADOW 模式。 */` |
| [15](../config/30_velocity_observation/speed_observation_config.h#L15) | 预处理 | `#endif` |
| [16](../config/30_velocity_observation/speed_observation_config.h#L16) | 预处理 | `#ifndef CHASSIS_SPEED_KF_EXPERIMENT_CONFIG` |
| [17](../config/30_velocity_observation/speed_observation_config.h#L17) | 预处理 | `#define CHASSIS_SPEED_KF_EXPERIMENT_CONFIG 0 /* 0 使用默认 KF 参数；1 从 chassis_speed_kf_experiment.h 加载实验参数；默认 0，必须同时启用 KF_SHADOW。 */` |
| [18](../config/30_velocity_observation/speed_observation_config.h#L18) | 预处理 | `#endif` |
| [19](../config/30_velocity_observation/speed_observation_config.h#L19) | 预处理 | `#if (CHASSIS_SPEED_CAPTURE_ENABLE != 0 && CHASSIS_SPEED_CAPTURE_ENABLE != 1) &#124;&#124; \` |
| [23](../config/30_velocity_observation/speed_observation_config.h#L23) | 预处理 | `#endif` |
| [26](../config/30_velocity_observation/speed_observation_config.h#L26) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [27](../config/30_velocity_observation/speed_observation_config.h#L27) | 预处理 | `#if CHASSIS_CONTROL_MODE != CHASSIS_CONTROL_SPEED &#124;&#124; CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [29](../config/30_velocity_observation/speed_observation_config.h#L29) | 预处理 | `#endif` |
| [30](../config/30_velocity_observation/speed_observation_config.h#L30) | 预处理 | `#endif` |
| [31](../config/30_velocity_observation/speed_observation_config.h#L31) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE && (!defined(LOG_OUTPUT) &#124;&#124; (LOG_OUTPUT != 2 && LOG_OUTPUT != 3))` |
| [33](../config/30_velocity_observation/speed_observation_config.h#L33) | 预处理 | `#endif` |
| [34](../config/30_velocity_observation/speed_observation_config.h#L34) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE && CHASSIS_PREDICTIVE_POWER_MODE != CHASSIS_PREDICTIVE_SHADOW` |
| [36](../config/30_velocity_observation/speed_observation_config.h#L36) | 预处理 | `#endif` |
| [38](../config/30_velocity_observation/speed_observation_config.h#L38) | 预处理 | `#if CHASSIS_SPEED_KF_EXPERIMENT_CONFIG && !CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [40](../config/30_velocity_observation/speed_observation_config.h#L40) | 预处理 | `#endif` |
| [41](../config/30_velocity_observation/speed_observation_config.h#L41) | 预处理 | `#endif` |

## config/31_telemetry/rtt_telemetry_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/31_telemetry/rtt_telemetry_config.h#L1) | 预处理 | `#ifndef RTT_TELEMETRY_CONFIG_H` |
| [2](../config/31_telemetry/rtt_telemetry_config.h#L2) | 预处理 | `#define RTT_TELEMETRY_CONFIG_H` |
| [6](../config/31_telemetry/rtt_telemetry_config.h#L6) | 预处理 | `#ifndef CHASSIS_POWER_CALIBRATION_ENABLE` |
| [7](../config/31_telemetry/rtt_telemetry_config.h#L7) | 预处理 | `#define CHASSIS_POWER_CALIBRATION_ENABLE 0 /* 0=普通遥测，1=附加标定列。 */` |
| [8](../config/31_telemetry/rtt_telemetry_config.h#L8) | 预处理 | `#endif` |
| [10](../config/31_telemetry/rtt_telemetry_config.h#L10) | 预处理 | `#if CHASSIS_POWER_CALIBRATION_ENABLE != 0 && CHASSIS_POWER_CALIBRATION_ENABLE != 1` |
| [12](../config/31_telemetry/rtt_telemetry_config.h#L12) | 预处理 | `#endif` |
| [14](../config/31_telemetry/rtt_telemetry_config.h#L14) | 预处理 | `#define RTT_TELEMETRY_CHASSIS_POWER_CSV_MAX_LENGTH 320U /* 功率CSV格式化缓冲区容量，单位字节；默认320。 */` |
| [16](../config/31_telemetry/rtt_telemetry_config.h#L16) | 预处理 | `#endif` |

## config/31_telemetry/tuning_overrides.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/31_telemetry/tuning_overrides.h#L1) | 预处理 | `#ifndef TUNING_OVERRIDES_H` |
| [2](../config/31_telemetry/tuning_overrides.h#L2) | 预处理 | `#define TUNING_OVERRIDES_H` |
| [8](../config/31_telemetry/tuning_overrides.h#L8) | 预处理 | `#define TUNING_PID_OVERRIDE 0` |
| [10](../config/31_telemetry/tuning_overrides.h#L10) | 预处理 | `#define TUNING_PID_MODE 0` |
| [13](../config/31_telemetry/tuning_overrides.h#L13) | 预处理 | `#define TUNING_PID_VALUES {0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f, 0.75f, 0.0f, 0.0f, 15000.0f, 3000.0f, 0.0f}` |
| [15](../config/31_telemetry/tuning_overrides.h#L15) | 预处理 | `#define TUNING_POWER_OVERRIDE 0` |
| [17](../config/31_telemetry/tuning_overrides.h#L17) | 预处理 | `#define TUNING_POWER_MODE 3` |
| [21](../config/31_telemetry/tuning_overrides.h#L21) | 预处理 | `#define TUNING_POWER_VALUES {0}` |
| [23](../config/31_telemetry/tuning_overrides.h#L23) | 预处理 | `#define TUNING_SPIN_OVERRIDE 0` |
| [25](../config/31_telemetry/tuning_overrides.h#L25) | 预处理 | `#define TUNING_SPIN_MODE 2` |
| [29](../config/31_telemetry/tuning_overrides.h#L29) | 预处理 | `#define TUNING_SPIN_VALUES {0}` |
| [30](../config/31_telemetry/tuning_overrides.h#L30) | 预处理 | `#endif` |

## config/31_telemetry/usb_telemetry_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/31_telemetry/usb_telemetry_config.h#L1) | 预处理 | `#ifndef USB_TELEMETRY_CONFIG_H` |
| [2](../config/31_telemetry/usb_telemetry_config.h#L2) | 预处理 | `#define USB_TELEMETRY_CONFIG_H` |
| [7](../config/31_telemetry/usb_telemetry_config.h#L7) | 预处理 | `#ifndef USB_TELEMETRY_ENABLE` |
| [8](../config/31_telemetry/usb_telemetry_config.h#L8) | 预处理 | `#define USB_TELEMETRY_ENABLE 1 /* USB遥测开关：0=关闭，1=开启；与USB视觉互斥。 */` |
| [9](../config/31_telemetry/usb_telemetry_config.h#L9) | 预处理 | `#endif` |
| [10](../config/31_telemetry/usb_telemetry_config.h#L10) | 预处理 | `#if USB_TELEMETRY_ENABLE != 0 && USB_TELEMETRY_ENABLE != 1` |
| [12](../config/31_telemetry/usb_telemetry_config.h#L12) | 预处理 | `#endif` |
| [13](../config/31_telemetry/usb_telemetry_config.h#L13) | 预处理 | `#ifndef USB_TELEMETRY_PERIOD_MS` |
| [14](../config/31_telemetry/usb_telemetry_config.h#L14) | 预处理 | `#define USB_TELEMETRY_PERIOD_MS 20U /* 遥测数据帧发送周期，单位ms；默认20（50Hz）。 */` |
| [15](../config/31_telemetry/usb_telemetry_config.h#L15) | 预处理 | `#endif` |
| [16](../config/31_telemetry/usb_telemetry_config.h#L16) | 预处理 | `#ifndef USB_TELEMETRY_HEADER_PERIOD_MS` |
| [17](../config/31_telemetry/usb_telemetry_config.h#L17) | 预处理 | `#define USB_TELEMETRY_HEADER_PERIOD_MS 1000U /* CSV列名表头重发周期，单位ms；默认1000。 */` |
| [18](../config/31_telemetry/usb_telemetry_config.h#L18) | 预处理 | `#endif` |
| [19](../config/31_telemetry/usb_telemetry_config.h#L19) | 预处理 | `#ifndef USB_TELEMETRY_BUFFER_SIZE` |
| [20](../config/31_telemetry/usb_telemetry_config.h#L20) | 预处理 | `#define USB_TELEMETRY_BUFFER_SIZE 3072U /* USB遥测发送缓冲区容量，单位字节；默认3072。 */` |
| [21](../config/31_telemetry/usb_telemetry_config.h#L21) | 预处理 | `#endif` |
| [23](../config/31_telemetry/usb_telemetry_config.h#L23) | 预处理 | `#endif` |

## config/32_capture/imu_capture_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/32_capture/imu_capture_config.h#L1) | 预处理 | `#ifndef IMU_CAPTURE_CONFIG_H` |
| [2](../config/32_capture/imu_capture_config.h#L2) | 预处理 | `#define IMU_CAPTURE_CONFIG_H` |
| [9](../config/32_capture/imu_capture_config.h#L9) | 预处理 | `#ifndef IMU_CAPTURE_ONLY` |
| [10](../config/32_capture/imu_capture_config.h#L10) | 预处理 | `#define IMU_CAPTURE_ONLY 0 /* 默认 0：正常整车任务；1：仅采集/温控/诊断，需速度后端且 CHASSIS_VELOCITY_FUSION_ENABLE=0。 */` |
| [11](../config/32_capture/imu_capture_config.h#L11) | 预处理 | `#endif` |
| [12](../config/32_capture/imu_capture_config.h#L12) | 预处理 | `#if IMU_CAPTURE_ONLY != 0 && IMU_CAPTURE_ONLY != 1` |
| [14](../config/32_capture/imu_capture_config.h#L14) | 预处理 | `#endif` |
| [15](../config/32_capture/imu_capture_config.h#L15) | 预处理 | `#ifndef IMU_CAPTURE_CAN_TX_ENABLE` |
| [16](../config/32_capture/imu_capture_config.h#L16) | 预处理 | `#define IMU_CAPTURE_CAN_TX_ENABLE 0 /* 默认 0：不发送 IMU CAN 帧；1：启用采集板发送，必须同时 IMU_CAPTURE_ONLY=1。 */` |
| [17](../config/32_capture/imu_capture_config.h#L17) | 预处理 | `#endif` |
| [18](../config/32_capture/imu_capture_config.h#L18) | 预处理 | `#ifndef CHASSIS_IMU_CAN_RX_ENABLE` |
| [19](../config/32_capture/imu_capture_config.h#L19) | 预处理 | `#define CHASSIS_IMU_CAN_RX_ENABLE 0 /* 默认 0：不接收外部 IMU CAN 帧；1：启用整车端接收，须满足下方接收约束。 */` |
| [20](../config/32_capture/imu_capture_config.h#L20) | 预处理 | `#endif` |
| [21](../config/32_capture/imu_capture_config.h#L21) | 预处理 | `#ifndef CHASSIS_REMOTE_IMU_FUSION_ENABLE` |
| [22](../config/32_capture/imu_capture_config.h#L22) | 预处理 | `#define CHASSIS_REMOTE_IMU_FUSION_ENABLE 0 /* 0=仅记录远端IMU；1=固定底盘IMU融合观测，仍不进入控制。须同时传给构建系统。 */` |
| [23](../config/32_capture/imu_capture_config.h#L23) | 预处理 | `#endif` |
| [24](../config/32_capture/imu_capture_config.h#L24) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE != 0 && CHASSIS_REMOTE_IMU_FUSION_ENABLE != 1` |
| [26](../config/32_capture/imu_capture_config.h#L26) | 预处理 | `#endif` |
| [28](../config/32_capture/imu_capture_config.h#L28) | 预处理 | `#if CHASSIS_REMOTE_IMU_FUSION_ENABLE && (!CHASSIS_IMU_CAN_RX_ENABLE &#124;&#124; !CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; CHASSIS_VELOCITY_FUSION_CALIBRATED &#124;&#124; IMU_CAPTURE_ONLY)` |
| [30](../config/32_capture/imu_capture_config.h#L30) | 预处理 | `#endif` |
| [31](../config/32_capture/imu_capture_config.h#L31) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE != 0 && IMU_CAPTURE_CAN_TX_ENABLE != 1` |
| [33](../config/32_capture/imu_capture_config.h#L33) | 预处理 | `#endif` |
| [34](../config/32_capture/imu_capture_config.h#L34) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE != 0 && CHASSIS_IMU_CAN_RX_ENABLE != 1` |
| [36](../config/32_capture/imu_capture_config.h#L36) | 预处理 | `#endif` |
| [37](../config/32_capture/imu_capture_config.h#L37) | 预处理 | `#if IMU_CAPTURE_CAN_TX_ENABLE && !IMU_CAPTURE_ONLY` |
| [39](../config/32_capture/imu_capture_config.h#L39) | 预处理 | `#endif` |
| [43](../config/32_capture/imu_capture_config.h#L43) | 预处理 | `#if CHASSIS_IMU_CAN_RX_ENABLE && IMU_CAPTURE_ONLY` |
| [45](../config/32_capture/imu_capture_config.h#L45) | 预处理 | `#endif` |
| [47](../config/32_capture/imu_capture_config.h#L47) | 预处理 | `#endif` |

## config/32_capture/shadow_scope_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/32_capture/shadow_scope_config.h#L1) | 预处理 | `#ifndef SHADOW_SCOPE_CONFIG_H` |
| [2](../config/32_capture/shadow_scope_config.h#L2) | 预处理 | `#define SHADOW_SCOPE_CONFIG_H` |
| [6](../config/32_capture/shadow_scope_config.h#L6) | 预处理 | `#ifndef CHASSIS_SHADOW_JSCOPE_ENABLE` |
| [7](../config/32_capture/shadow_scope_config.h#L7) | 预处理 | `#define CHASSIS_SHADOW_JSCOPE_ENABLE 0 /* J-Scope影子观测：0=关闭（默认），1=开启；仅输出诊断，不接管控制。 */` |
| [8](../config/32_capture/shadow_scope_config.h#L8) | 预处理 | `#endif` |
| [9](../config/32_capture/shadow_scope_config.h#L9) | 预处理 | `#ifndef CHASSIS_SHADOW_JSCOPE_HZ` |
| [10](../config/32_capture/shadow_scope_config.h#L10) | 预处理 | `#define CHASSIS_SHADOW_JSCOPE_HZ 50 /* J-Scope采样频率，单位Hz；允许1～100，默认50。 */` |
| [11](../config/32_capture/shadow_scope_config.h#L11) | 预处理 | `#endif` |
| [12](../config/32_capture/shadow_scope_config.h#L12) | 预处理 | `#ifndef CHASSIS_SHADOW_JSCOPE_WHEEL` |
| [13](../config/32_capture/shadow_scope_config.h#L13) | 预处理 | `#define CHASSIS_SHADOW_JSCOPE_WHEEL 0 /* 观测轮号：0=左前，1=右前，2=左后，3=右后；默认左前。 */` |
| [14](../config/32_capture/shadow_scope_config.h#L14) | 预处理 | `#endif` |
| [15](../config/32_capture/shadow_scope_config.h#L15) | 预处理 | `#if CHASSIS_SHADOW_JSCOPE_ENABLE != 0 && CHASSIS_SHADOW_JSCOPE_ENABLE != 1` |
| [17](../config/32_capture/shadow_scope_config.h#L17) | 预处理 | `#endif` |
| [18](../config/32_capture/shadow_scope_config.h#L18) | 预处理 | `#if CHASSIS_SHADOW_JSCOPE_HZ < 1 &#124;&#124; CHASSIS_SHADOW_JSCOPE_HZ > 100` |
| [20](../config/32_capture/shadow_scope_config.h#L20) | 预处理 | `#endif` |
| [21](../config/32_capture/shadow_scope_config.h#L21) | 预处理 | `#if CHASSIS_SHADOW_JSCOPE_WHEEL < 0 &#124;&#124; CHASSIS_SHADOW_JSCOPE_WHEEL > 3` |
| [23](../config/32_capture/shadow_scope_config.h#L23) | 预处理 | `#endif` |
| [24](../config/32_capture/shadow_scope_config.h#L24) | 预处理 | `#endif` |

## config/90_board/board_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/90_board/board_config.h#L1) | 预处理 | `#ifndef BOARD_CONFIG_H` |
| [2](../config/90_board/board_config.h#L2) | 预处理 | `#define BOARD_CONFIG_H` |
| [8](../config/90_board/board_config.h#L8) | 预处理 | `#ifdef UART1_USE` |
| [10](../config/90_board/board_config.h#L10) | 预处理 | `#endif` |
| [11](../config/90_board/board_config.h#L11) | 预处理 | `#ifndef REFEREE_ENABLE` |
| [12](../config/90_board/board_config.h#L12) | 预处理 | `#define REFEREE_ENABLE 1 /* 裁判系统开关：0=关闭，1=开启；默认开启。 */` |
| [13](../config/90_board/board_config.h#L13) | 预处理 | `#endif` |
| [14](../config/90_board/board_config.h#L14) | 预处理 | `#ifndef REFEREE_UART_PORT` |
| [15](../config/90_board/board_config.h#L15) | 预处理 | `#define REFEREE_UART_PORT 6 /* 裁判串口：6=USART6（A板编号16、最左侧丝印6），7=UART7；默认6。 */` |
| [16](../config/90_board/board_config.h#L16) | 预处理 | `#endif` |
| [17](../config/90_board/board_config.h#L17) | 预处理 | `#ifndef LOG_OUTPUT` |
| [18](../config/90_board/board_config.h#L18) | 预处理 | `#define LOG_OUTPUT 1 /* 日志输出：0=关闭，1=RTT，2=USART2，3=RTT与USART2同时输出。 */` |
| [19](../config/90_board/board_config.h#L19) | 预处理 | `#endif` |
| [20](../config/90_board/board_config.h#L20) | 预处理 | `#ifndef CHASSIS_IMU_CAN_PORT` |
| [21](../config/90_board/board_config.h#L21) | 预处理 | `#define CHASSIS_IMU_CAN_PORT 2 /* 主板与IMU采集板通信：1=CAN1，2=CAN2（默认）；两板共用，修改后分别重新编译烧录。 */` |
| [22](../config/90_board/board_config.h#L22) | 预处理 | `#endif` |
| [23](../config/90_board/board_config.h#L23) | 预处理 | `#ifndef VOFA_TELEMETRY_MODE` |
| [24](../config/90_board/board_config.h#L24) | 预处理 | `#define VOFA_TELEMETRY_MODE 2 /* VOFA传输：0=旧19路JustFloat，1=旧FireWater，2=JustFloat用途观测。 */` |
| [25](../config/90_board/board_config.h#L25) | 预处理 | `#endif` |
| [27](../config/90_board/board_config.h#L27) | 预处理 | `#ifndef ATYPE_BENCH_MODE` |
| [28](../config/90_board/board_config.h#L28) | 预处理 | `#define ATYPE_BENCH_MODE 0 /* 台架模式：0=整车正常控制，1=台架门控（云台/发射锁停）；atype-bench构建默认设1。 */` |
| [29](../config/90_board/board_config.h#L29) | 预处理 | `#endif` |
| [30](../config/90_board/board_config.h#L30) | 预处理 | `#if REFEREE_ENABLE != 0 && REFEREE_ENABLE != 1` |
| [32](../config/90_board/board_config.h#L32) | 预处理 | `#endif` |
| [33](../config/90_board/board_config.h#L33) | 预处理 | `#if REFEREE_UART_PORT != 6 && REFEREE_UART_PORT != 7` |
| [35](../config/90_board/board_config.h#L35) | 预处理 | `#endif` |
| [36](../config/90_board/board_config.h#L36) | 预处理 | `#if LOG_OUTPUT < 0 &#124;&#124; LOG_OUTPUT > 3` |
| [38](../config/90_board/board_config.h#L38) | 预处理 | `#endif` |
| [39](../config/90_board/board_config.h#L39) | 预处理 | `#if CHASSIS_IMU_CAN_PORT != 1 && CHASSIS_IMU_CAN_PORT != 2` |
| [41](../config/90_board/board_config.h#L41) | 预处理 | `#endif` |
| [42](../config/90_board/board_config.h#L42) | 预处理 | `#if VOFA_TELEMETRY_MODE != 0 && VOFA_TELEMETRY_MODE != 1 && VOFA_TELEMETRY_MODE != 2` |
| [44](../config/90_board/board_config.h#L44) | 预处理 | `#endif` |
| [45](../config/90_board/board_config.h#L45) | 预处理 | `#if VOFA_OBSERVATION_PROFILE != 0 && VOFA_OBSERVATION_PROFILE != 1 && VOFA_OBSERVATION_PROFILE != 2` |
| [47](../config/90_board/board_config.h#L47) | 预处理 | `#endif` |
| [48](../config/90_board/board_config.h#L48) | 预处理 | `#if ATYPE_BENCH_MODE != 0 && ATYPE_BENCH_MODE != 1` |
| [50](../config/90_board/board_config.h#L50) | 预处理 | `#endif` |
| [51](../config/90_board/board_config.h#L51) | 预处理 | `#define BOARD_LOG_UART_ENABLED (LOG_OUTPUT == 2 &#124;&#124; LOG_OUTPUT == 3) /* 按LOG_OUTPUT自动判断是否启用串口日志，不单独修改。 */` |
| [52](../config/90_board/board_config.h#L52) | 预处理 | `#define BOARD_LOG_RTT_ENABLED (LOG_OUTPUT == 1 &#124;&#124; LOG_OUTPUT == 3) /* 按LOG_OUTPUT自动判断是否启用RTT日志，不单独修改。 */` |
| [54](../config/90_board/board_config.h#L54) | 预处理 | `#define BOARD_REMOTE_UART (&huart1) /* DBUS遥控器固定连接USART1；更改句柄前须同步硬件与串口初始化。 */` |
| [55](../config/90_board/board_config.h#L55) | 预处理 | `#define BOARD_VISION_UART (&huart3) /* 视觉通信绑定USART3；启用方式见robot_config.h。 */` |
| [56](../config/90_board/board_config.h#L56) | 预处理 | `#define BOARD_VTX_UART (&huart8) /* 图传键鼠输入绑定UART8。 */` |
| [57](../config/90_board/board_config.h#L57) | 预处理 | `#if REFEREE_UART_PORT == 7` |
| [58](../config/90_board/board_config.h#L58) | 预处理 | `#define BOARD_REFEREE_UART (&huart7) /* 裁判串口句柄，由REFEREE_UART_PORT自动选择，不单独修改。 */` |
| [59](../config/90_board/board_config.h#L59) | 预处理 | `#else` |
| [60](../config/90_board/board_config.h#L60) | 预处理 | `#define BOARD_REFEREE_UART (&huart6) /* 裁判串口句柄，由REFEREE_UART_PORT自动选择，不单独修改。 */` |
| [61](../config/90_board/board_config.h#L61) | 预处理 | `#endif` |
| [62](../config/90_board/board_config.h#L62) | 预处理 | `#define BOARD_LOG_UART (&huart2) /* 串口日志绑定USART2；由LOG_OUTPUT决定是否启用。 */` |
| [63](../config/90_board/board_config.h#L63) | 预处理 | `#define BOARD_VOFA_UART (&huart7) /* VOFA遥测绑定UART7，不能与裁判或UART7辅助设备同时占用。 */` |
| [64](../config/90_board/board_config.h#L64) | 预处理 | `#define BOARD_SUPERCAP_CAN (&hcan1) /* 超级电容通信绑定CAN1；协议CAN ID仍由协议模块定义。 */` |
| [65](../config/90_board/board_config.h#L65) | 预处理 | `#define BOARD_POWER_METER_CAN (&hcan1) /* 功率计通信绑定CAN1；启用开关POWER_METER_ENABLE见chassis_power_config.h。 */` |
| [66](../config/90_board/board_config.h#L66) | 预处理 | `#if CHASSIS_IMU_CAN_PORT == 1` |
| [67](../config/90_board/board_config.h#L67) | 预处理 | `#define BOARD_CHASSIS_IMU_CAN (&hcan1) /* 板间IMU通信句柄，由CHASSIS_IMU_CAN_PORT自动选择，不单独修改。 */` |
| [68](../config/90_board/board_config.h#L68) | 预处理 | `#else` |
| [69](../config/90_board/board_config.h#L69) | 预处理 | `#define BOARD_CHASSIS_IMU_CAN (&hcan2) /* 板间IMU通信句柄，由CHASSIS_IMU_CAN_PORT自动选择，不单独修改。 */` |
| [70](../config/90_board/board_config.h#L70) | 预处理 | `#endif` |
| [72](../config/90_board/board_config.h#L72) | 预处理 | `#ifndef OLED_USE_I2C` |
| [73](../config/90_board/board_config.h#L73) | 预处理 | `#define OLED_USE_I2C 0 /* OLED后端：0=SPI1（默认），1=I2C2；还需BOARD_OLED_ENABLE=1注册显示屏。 */` |
| [74](../config/90_board/board_config.h#L74) | 预处理 | `#endif` |
| [75](../config/90_board/board_config.h#L75) | 预处理 | `#if OLED_USE_I2C != 0 && OLED_USE_I2C != 1` |
| [77](../config/90_board/board_config.h#L77) | 预处理 | `#endif` |
| [81](../config/90_board/board_config.h#L81) | 预处理 | `#ifndef BOARD_AUX_UART2_DEVICE` |
| [82](../config/90_board/board_config.h#L82) | 预处理 | `#define BOARD_AUX_UART2_DEVICE 0 /* USART2辅助设备：0=无，1=TFmini，2=HC05，3=总线舵机；非0时LOG_OUTPUT不能为2或3。 */` |
| [83](../config/90_board/board_config.h#L83) | 预处理 | `#endif` |
| [84](../config/90_board/board_config.h#L84) | 预处理 | `#ifndef BOARD_AUX_UART7_DEVICE` |
| [85](../config/90_board/board_config.h#L85) | 预处理 | `#define BOARD_AUX_UART7_DEVICE 0 /* UART7辅助设备：0=无，1=TFmini，2=HC05，3=总线舵机；非0时不能同时接UART7裁判或启用VOFA。 */` |
| [86](../config/90_board/board_config.h#L86) | 预处理 | `#endif` |
| [87](../config/90_board/board_config.h#L87) | 预处理 | `#ifndef BOARD_OLED_ENABLE` |
| [88](../config/90_board/board_config.h#L88) | 预处理 | `#define BOARD_OLED_ENABLE 0 /* OLED开关：0=不注册（默认），1=注册并初始化；后端由OLED_USE_I2C选择。 */` |
| [89](../config/90_board/board_config.h#L89) | 预处理 | `#endif` |
| [90](../config/90_board/board_config.h#L90) | 预处理 | `#ifndef BOARD_KEY_ENABLE` |
| [91](../config/90_board/board_config.h#L91) | 预处理 | `#define BOARD_KEY_ENABLE 0 /* 板载按键开关：0=不注册（默认），1=注册按键模块。 */` |
| [92](../config/90_board/board_config.h#L92) | 预处理 | `#endif` |
| [93](../config/90_board/board_config.h#L93) | 预处理 | `#if BOARD_AUX_UART2_DEVICE < 0 &#124;&#124; BOARD_AUX_UART2_DEVICE > 3 &#124;&#124; BOARD_AUX_UART7_DEVICE < 0 &#124;&#124; BOARD_AUX_UART7_DEVICE > 3` |
| [95](../config/90_board/board_config.h#L95) | 预处理 | `#endif` |
| [96](../config/90_board/board_config.h#L96) | 预处理 | `#if BOARD_AUX_UART2_DEVICE && BOARD_LOG_UART_ENABLED` |
| [98](../config/90_board/board_config.h#L98) | 预处理 | `#endif` |
| [99](../config/90_board/board_config.h#L99) | 预处理 | `#if REFEREE_ENABLE && REFEREE_UART_PORT == 7 && BOARD_AUX_UART7_DEVICE` |
| [101](../config/90_board/board_config.h#L101) | 预处理 | `#endif` |
| [102](../config/90_board/board_config.h#L102) | 预处理 | `#define BOARD_UART7_RESERVED_FOR_DEVICE ((REFEREE_ENABLE && REFEREE_UART_PORT == 7) &#124;&#124; BOARD_AUX_UART7_DEVICE) /* 按裁判和辅助设备选择自动判断UART7是否被占用，不单独修改。 */` |
| [105](../config/90_board/board_config.h#L105) | 预处理 | `#ifndef VOFA_TELEMETRY_ENABLE` |
| [106](../config/90_board/board_config.h#L106) | 预处理 | `#if (!defined(CHASSIS_CONTROL_MODE) &#124;&#124; CHASSIS_CONTROL_MODE == 0) && !BOARD_UART7_RESERVED_FOR_DEVICE` |
| [107](../config/90_board/board_config.h#L107) | 预处理 | `#define VOFA_TELEMETRY_ENABLE 1 /* VOFA发送开关：1=发送，0=关闭；默认速度后端且UART7空闲时为1，其余情况为0。 */` |
| [108](../config/90_board/board_config.h#L108) | 预处理 | `#else` |
| [109](../config/90_board/board_config.h#L109) | 预处理 | `#define VOFA_TELEMETRY_ENABLE 0 /* VOFA发送开关：1=发送，0=关闭；默认速度后端且UART7空闲时为1，其余情况为0。 */` |
| [110](../config/90_board/board_config.h#L110) | 预处理 | `#endif` |
| [111](../config/90_board/board_config.h#L111) | 预处理 | `#endif` |
| [112](../config/90_board/board_config.h#L112) | 预处理 | `#if VOFA_TELEMETRY_ENABLE != 0 && VOFA_TELEMETRY_ENABLE != 1` |
| [114](../config/90_board/board_config.h#L114) | 预处理 | `#endif` |
| [115](../config/90_board/board_config.h#L115) | 预处理 | `#if VOFA_TELEMETRY_ENABLE && BOARD_UART7_RESERVED_FOR_DEVICE` |
| [117](../config/90_board/board_config.h#L117) | 预处理 | `#endif` |
| [119](../config/90_board/board_config.h#L119) | 预处理 | `#if VOFA_TELEMETRY_ENABLE && VOFA_TELEMETRY_MODE == 2` |
| [120](../config/90_board/board_config.h#L120) | 预处理 | `#define BOARD_UART7_BAUDRATE 230400U /* 模式2配合20ms输出；VOFA和串口接收设备须使用相同波特率。 */` |
| [121](../config/90_board/board_config.h#L121) | 预处理 | `#else` |
| [122](../config/90_board/board_config.h#L122) | 预处理 | `#define BOARD_UART7_BAUDRATE 115200U /* 模式0/1、VOFA关闭、裁判或辅助设备使用UART7时保留原值。 */` |
| [123](../config/90_board/board_config.h#L123) | 预处理 | `#endif` |
| [124](../config/90_board/board_config.h#L124) | 预处理 | `#if BOARD_AUX_UART2_DEVICE && BOARD_AUX_UART7_DEVICE && BOARD_AUX_UART2_DEVICE == BOARD_AUX_UART7_DEVICE` |
| [126](../config/90_board/board_config.h#L126) | 预处理 | `#endif` |
| [127](../config/90_board/board_config.h#L127) | 预处理 | `#if BOARD_OLED_ENABLE < 0 &#124;&#124; BOARD_OLED_ENABLE > 1 &#124;&#124; BOARD_KEY_ENABLE < 0 &#124;&#124; BOARD_KEY_ENABLE > 1` |
| [129](../config/90_board/board_config.h#L129) | 预处理 | `#endif` |
| [131](../config/90_board/board_config.h#L131) | 预处理 | `#endif` |

## config/90_board/log_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/90_board/log_config.h#L1) | 预处理 | `#ifndef LOG_CONFIG_H` |
| [2](../config/90_board/log_config.h#L2) | 预处理 | `#define LOG_CONFIG_H` |
| [7](../config/90_board/log_config.h#L7) | 预处理 | `#ifndef DISABLE_LOG_SYSTEM` |
| [8](../config/90_board/log_config.h#L8) | 预处理 | `#define DISABLE_LOG_SYSTEM 0 /* 0=保留分级日志，1=禁用分级日志。 */` |
| [9](../config/90_board/log_config.h#L9) | 预处理 | `#endif` |
| [11](../config/90_board/log_config.h#L11) | 预处理 | `#endif` |

## config/90_board/referee_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/90_board/referee_config.h#L1) | 预处理 | `#ifndef REFEREE_CONFIG_H` |
| [2](../config/90_board/referee_config.h#L2) | 预处理 | `#define REFEREE_CONFIG_H` |
| [4](../config/90_board/referee_config.h#L4) | 预处理 | `#define REFEREE_POWER_DATA_TIMEOUT_MS 300U /* 裁判功率/热量数据有效期，单位ms；超过后按离线处理。 */` |
| [9](../config/90_board/referee_config.h#L9) | 预处理 | `#ifndef REFEREE_LEGACY_POWER_FIELDS_ENABLED` |
| [10](../config/90_board/referee_config.h#L10) | 预处理 | `#define REFEREE_LEGACY_POWER_FIELDS_ENABLED 0 /* 0=禁用旧字段，1=启用已验证的旧字段。 */` |
| [11](../config/90_board/referee_config.h#L11) | 预处理 | `#endif` |
| [13](../config/90_board/referee_config.h#L13) | 预处理 | `#if REFEREE_LEGACY_POWER_FIELDS_ENABLED != 0 && REFEREE_LEGACY_POWER_FIELDS_ENABLED != 1` |
| [15](../config/90_board/referee_config.h#L15) | 预处理 | `#endif` |
| [17](../config/90_board/referee_config.h#L17) | 预处理 | `#endif` |

## config/91_motor/dji_motor_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/91_motor/dji_motor_config.h#L1) | 预处理 | `#ifndef DJI_MOTOR_CONFIG_H` |
| [2](../config/91_motor/dji_motor_config.h#L2) | 预处理 | `#define DJI_MOTOR_CONFIG_H` |
| [6](../config/91_motor/dji_motor_config.h#L6) | 预处理 | `#define DJI_MOTOR_CNT 12 /* 可注册 DJI 电机实例上限，单位个；默认 12，同时决定实例/采样缓存容量。 */` |
| [9](../config/91_motor/dji_motor_config.h#L9) | 预处理 | `#define SPEED_SMOOTH_COEF 0.85f /* 速度新样本权重，无量纲；默认 0.85，旧值权重为 1−本值，改为 1 关闭速度平滑。 */` |
| [10](../config/91_motor/dji_motor_config.h#L10) | 预处理 | `#define CURRENT_SMOOTH_COEF 0.9f /* 电流新样本权重，无量纲；默认 0.9，旧值权重为 1−本值，改为 1 关闭电流平滑。 */` |
| [12](../config/91_motor/dji_motor_config.h#L12) | 预处理 | `#endif` |

## config/92_imu/imu_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/92_imu/imu_config.h#L1) | 预处理 | `#ifndef IMU_CONFIG_H` |
| [2](../config/92_imu/imu_config.h#L2) | 预处理 | `#define IMU_CONFIG_H` |
| [7](../config/92_imu/imu_config.h#L7) | 预处理 | `#define IMU_MPU6500_BOARD_DOWN (1) /* 保留的安装方向标记，默认 1；当前未接入逻辑，不支持用 0/1 切换实际朝向。 */` |
| [8](../config/92_imu/imu_config.h#L8) | 预处理 | `#define IMU_MPU6500_USE_IST8310 /* 保留的磁力计标记，默认已定义；当前未接入逻辑，删除定义也不会实际关闭磁力计。 */` |
| [11](../config/92_imu/imu_config.h#L11) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_LIMIT_X 0.04f /* X 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */` |
| [12](../config/92_imu/imu_config.h#L12) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_LIMIT_Y 0.04f /* Y 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */` |
| [13](../config/92_imu/imu_config.h#L13) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_LIMIT_Z 0.04f /* Z 轴零偏标定允许的绝对值门槛，单位 rad/s；超出时重试标定。 */` |
| [14](../config/92_imu/imu_config.h#L14) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_X .037452f /* 标定超时回退的 X 轴陀螺仪零偏，单位 rad/s。 */` |
| [15](../config/92_imu/imu_config.h#L15) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_Y .003360f /* 标定超时回退的 Y 轴陀螺仪零偏，单位 rad/s。 */` |
| [16](../config/92_imu/imu_config.h#L16) | 预处理 | `#define IMU_MPU6500_GYRO_OFFSET_Z .008532f /* 标定超时回退的 Z 轴陀螺仪零偏，单位 rad/s。 */` |
| [17](../config/92_imu/imu_config.h#L17) | 预处理 | `#define IMU_MPU6500_GRAVITY_NORM 9.605101f /* 回退标定的重力模长，单位 m/s²，用于计算加速度比例补偿。 */` |
| [21](../config/92_imu/imu_config.h#L21) | 预处理 | `#define IMU_MPU6500_REF_TEMP_C 40 /* MPU6500 恒温目标，单位 ℃；温控增益在 pid_config.c 中调整。 */` |
| [22](../config/92_imu/imu_config.h#L22) | 预处理 | `#define IMU_MPU6500_EKF_PROCESS_NOISE_1 10 /* 四元数状态过程噪声系数 Q1；算法以 Q1×dt 写入协方差，按 EKF 内部尺度调参。 */` |
| [23](../config/92_imu/imu_config.h#L23) | 预处理 | `#define IMU_MPU6500_EKF_PROCESS_NOISE_2 0.001 /* 陀螺仪零偏过程噪声系数 Q2；算法以 Q2×dt 写入协方差，按 EKF 内部尺度调参。 */` |
| [24](../config/92_imu/imu_config.h#L24) | 预处理 | `#define IMU_MPU6500_EKF_MEASUREMENT_NOISE 1000000 /* 归一化加速度观测的噪声协方差 R，按 EKF 内部尺度调参，不是原始加速度幅值。 */` |
| [25](../config/92_imu/imu_config.h#L25) | 预处理 | `#define IMU_MPU6500_EKF_LAMBDA 1 /* 零偏协方差渐消因子，无量纲；默认 1 不膨胀协方差，算法用协方差除以此值。 */` |
| [26](../config/92_imu/imu_config.h#L26) | 预处理 | `#define IMU_MPU6500_EKF_ACCEL_LPF 0 /* EKF 输入加速度低通时间常数，单位秒；默认 0 直通，改为正时间常数启用低通。 */` |
| [27](../config/92_imu/imu_config.h#L27) | 预处理 | `#define IMU_MPU6500_MOTION_ACCEL_LPF_S 0.0085 /* 去重力后的运动加速度低通时间常数，单位秒；改值后影响平滑程度与响应滞后。 */` |
| [30](../config/92_imu/imu_config.h#L30) | 预处理 | `#define IMU_BMI088_REF_TEMP_C 40 /* BMI088 恒温目标，单位 ℃；温控增益在 pid_config.c 中调整。 */` |
| [31](../config/92_imu/imu_config.h#L31) | 预处理 | `#define IMU_BMI088_EKF_PROCESS_NOISE_1 10 /* 四元数状态过程噪声系数 Q1；算法以 Q1×dt 写入协方差，按 EKF 内部尺度调参。 */` |
| [32](../config/92_imu/imu_config.h#L32) | 预处理 | `#define IMU_BMI088_EKF_PROCESS_NOISE_2 0.001 /* 陀螺仪零偏过程噪声系数 Q2；算法以 Q2×dt 写入协方差，按 EKF 内部尺度调参。 */` |
| [33](../config/92_imu/imu_config.h#L33) | 预处理 | `#define IMU_BMI088_EKF_MEASUREMENT_NOISE 1000000 /* 归一化加速度观测的噪声协方差 R，按 EKF 内部尺度调参，不是原始加速度幅值。 */` |
| [34](../config/92_imu/imu_config.h#L34) | 预处理 | `#define IMU_BMI088_EKF_LAMBDA 1 /* 零偏协方差渐消因子，无量纲；默认 1 不膨胀协方差，算法用协方差除以此值。 */` |
| [35](../config/92_imu/imu_config.h#L35) | 预处理 | `#define IMU_BMI088_EKF_ACCEL_LPF 0 /* EKF 输入加速度低通时间常数，单位秒；默认 0 直通，改为正时间常数启用低通。 */` |
| [36](../config/92_imu/imu_config.h#L36) | 预处理 | `#define IMU_BMI088_MOTION_ACCEL_LPF_S 0.0085 /* 去重力后的运动加速度低通时间常数，单位秒；改值后影响平滑程度与响应滞后。 */` |
| [38](../config/92_imu/imu_config.h#L38) | 预处理 | `#endif` |

## config/_vofa/vofa_config.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../config/_vofa/vofa_config.h#L1) | 预处理 | `#ifndef VOFA_CONFIG_H` |
| [2](../config/_vofa/vofa_config.h#L2) | 预处理 | `#define VOFA_CONFIG_H` |
| [9](../config/_vofa/vofa_config.h#L9) | 预处理 | `#ifndef VOFA_OBSERVATION_PROFILE` |
| [10](../config/_vofa/vofa_config.h#L10) | 预处理 | `#define VOFA_OBSERVATION_PROFILE 2` |
| [11](../config/_vofa/vofa_config.h#L11) | 预处理 | `#endif` |
| [13](../config/_vofa/vofa_config.h#L13) | 预处理 | `#endif` |

## Inc/adc.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/adc.h#L21) | 预处理 | `#ifndef __ADC_H__` |
| [22](../Inc/adc.h#L22) | 预处理 | `#define __ADC_H__` |
| [24](../Inc/adc.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/adc.h#L26) | 预处理 | `#endif` |
| [47](../Inc/adc.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/adc.h#L49) | 预处理 | `#endif` |
| [51](../Inc/adc.h#L51) | 预处理 | `#endif /* __ADC_H__ */` |

## Inc/can.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/can.h#L21) | 预处理 | `#ifndef __CAN_H__` |
| [22](../Inc/can.h#L22) | 预处理 | `#define __CAN_H__` |
| [24](../Inc/can.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/can.h#L26) | 预处理 | `#endif` |
| [50](../Inc/can.h#L50) | 预处理 | `#ifdef __cplusplus` |
| [52](../Inc/can.h#L52) | 预处理 | `#endif` |
| [54](../Inc/can.h#L54) | 预处理 | `#endif /* __CAN_H__ */` |

## Inc/crc.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/crc.h#L21) | 预处理 | `#ifndef __CRC_H__` |
| [22](../Inc/crc.h#L22) | 预处理 | `#define __CRC_H__` |
| [24](../Inc/crc.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/crc.h#L26) | 预处理 | `#endif` |
| [47](../Inc/crc.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/crc.h#L49) | 预处理 | `#endif` |
| [51](../Inc/crc.h#L51) | 预处理 | `#endif /* __CRC_H__ */` |

## Inc/dac.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/dac.h#L21) | 预处理 | `#ifndef __DAC_H__` |
| [22](../Inc/dac.h#L22) | 预处理 | `#define __DAC_H__` |
| [24](../Inc/dac.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/dac.h#L26) | 预处理 | `#endif` |
| [47](../Inc/dac.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/dac.h#L49) | 预处理 | `#endif` |
| [51](../Inc/dac.h#L51) | 预处理 | `#endif /* __DAC_H__ */` |

## Inc/dma.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/dma.h#L21) | 预处理 | `#ifndef __DMA_H__` |
| [22](../Inc/dma.h#L22) | 预处理 | `#define __DMA_H__` |
| [24](../Inc/dma.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/dma.h#L26) | 预处理 | `#endif` |
| [47](../Inc/dma.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/dma.h#L49) | 预处理 | `#endif` |
| [51](../Inc/dma.h#L51) | 预处理 | `#endif /* __DMA_H__ */` |

## Inc/FreeRTOSConfig.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [31](../Inc/FreeRTOSConfig.h#L31) | 预处理 | `#ifndef FREERTOS_CONFIG_H` |
| [32](../Inc/FreeRTOSConfig.h#L32) | 预处理 | `#define FREERTOS_CONFIG_H` |
| [51](../Inc/FreeRTOSConfig.h#L51) | 预处理 | `#if defined(__ICCARM__) &#124;&#124; defined(__CC_ARM) &#124;&#124; defined(__GNUC__)` |
| [54](../Inc/FreeRTOSConfig.h#L54) | 预处理 | `#endif` |
| [55](../Inc/FreeRTOSConfig.h#L55) | 预处理 | `#define configENABLE_FPU                         1` |
| [56](../Inc/FreeRTOSConfig.h#L56) | 预处理 | `#define configENABLE_MPU                         0` |
| [58](../Inc/FreeRTOSConfig.h#L58) | 预处理 | `#define configUSE_PREEMPTION                     1` |
| [59](../Inc/FreeRTOSConfig.h#L59) | 预处理 | `#define configSUPPORT_STATIC_ALLOCATION          1` |
| [60](../Inc/FreeRTOSConfig.h#L60) | 预处理 | `#define configSUPPORT_DYNAMIC_ALLOCATION         1` |
| [61](../Inc/FreeRTOSConfig.h#L61) | 预处理 | `#define configUSE_IDLE_HOOK                      0` |
| [62](../Inc/FreeRTOSConfig.h#L62) | 预处理 | `#define configUSE_TICK_HOOK                      0` |
| [63](../Inc/FreeRTOSConfig.h#L63) | 预处理 | `#define configCPU_CLOCK_HZ                       ( SystemCoreClock )` |
| [64](../Inc/FreeRTOSConfig.h#L64) | 预处理 | `#define configTICK_RATE_HZ                       ((TickType_t)1000)` |
| [65](../Inc/FreeRTOSConfig.h#L65) | 预处理 | `#define configMAX_PRIORITIES                     ( 7 )` |
| [66](../Inc/FreeRTOSConfig.h#L66) | 预处理 | `#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)` |
| [67](../Inc/FreeRTOSConfig.h#L67) | 预处理 | `#define configTOTAL_HEAP_SIZE                    ((size_t)(64U*1024U))` |
| [68](../Inc/FreeRTOSConfig.h#L68) | 预处理 | `#define configMAX_TASK_NAME_LEN                  ( 32 )` |
| [69](../Inc/FreeRTOSConfig.h#L69) | 预处理 | `#define configUSE_16_BIT_TICKS                   0` |
| [70](../Inc/FreeRTOSConfig.h#L70) | 预处理 | `#define configUSE_MUTEXES                        1` |
| [71](../Inc/FreeRTOSConfig.h#L71) | 预处理 | `#define configQUEUE_REGISTRY_SIZE                8` |
| [72](../Inc/FreeRTOSConfig.h#L72) | 预处理 | `#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1` |
| [76](../Inc/FreeRTOSConfig.h#L76) | 预处理 | `#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t` |
| [80](../Inc/FreeRTOSConfig.h#L80) | 预处理 | `#define configUSE_CO_ROUTINES                    0` |
| [81](../Inc/FreeRTOSConfig.h#L81) | 预处理 | `#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )` |
| [85](../Inc/FreeRTOSConfig.h#L85) | 预处理 | `#define INCLUDE_vTaskPrioritySet             1` |
| [86](../Inc/FreeRTOSConfig.h#L86) | 预处理 | `#define INCLUDE_uxTaskPriorityGet            1` |
| [87](../Inc/FreeRTOSConfig.h#L87) | 预处理 | `#define INCLUDE_uxTaskGetStackHighWaterMark   1` |
| [88](../Inc/FreeRTOSConfig.h#L88) | 预处理 | `#define INCLUDE_vTaskDelete                  1` |
| [89](../Inc/FreeRTOSConfig.h#L89) | 预处理 | `#define INCLUDE_vTaskCleanUpResources        0` |
| [90](../Inc/FreeRTOSConfig.h#L90) | 预处理 | `#define INCLUDE_vTaskSuspend                 1` |
| [91](../Inc/FreeRTOSConfig.h#L91) | 预处理 | `#define INCLUDE_vTaskDelayUntil              1` |
| [92](../Inc/FreeRTOSConfig.h#L92) | 预处理 | `#define INCLUDE_vTaskDelay                   1` |
| [93](../Inc/FreeRTOSConfig.h#L93) | 预处理 | `#define INCLUDE_xTaskGetSchedulerState       1` |
| [96](../Inc/FreeRTOSConfig.h#L96) | 预处理 | `#ifdef __NVIC_PRIO_BITS` |
| [98](../Inc/FreeRTOSConfig.h#L98) | 预处理 | `#define configPRIO_BITS         __NVIC_PRIO_BITS` |
| [99](../Inc/FreeRTOSConfig.h#L99) | 预处理 | `#else` |
| [100](../Inc/FreeRTOSConfig.h#L100) | 预处理 | `#define configPRIO_BITS         4` |
| [101](../Inc/FreeRTOSConfig.h#L101) | 预处理 | `#endif` |
| [105](../Inc/FreeRTOSConfig.h#L105) | 预处理 | `#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15` |
| [111](../Inc/FreeRTOSConfig.h#L111) | 预处理 | `#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5` |
| [115](../Inc/FreeRTOSConfig.h#L115) | 预处理 | `#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )` |
| [118](../Inc/FreeRTOSConfig.h#L118) | 预处理 | `#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )` |
| [123](../Inc/FreeRTOSConfig.h#L123) | 预处理 | `#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}` |
| [128](../Inc/FreeRTOSConfig.h#L128) | 预处理 | `#define vPortSVCHandler    SVC_Handler` |
| [129](../Inc/FreeRTOSConfig.h#L129) | 预处理 | `#define xPortPendSVHandler PendSV_Handler` |
| [134](../Inc/FreeRTOSConfig.h#L134) | 预处理 | `#define xPortSysTickHandler SysTick_Handler` |
| [140](../Inc/FreeRTOSConfig.h#L140) | 预处理 | `#endif /* FREERTOS_CONFIG_H */` |

## Inc/gpio.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/gpio.h#L21) | 预处理 | `#ifndef __GPIO_H__` |
| [22](../Inc/gpio.h#L22) | 预处理 | `#define __GPIO_H__` |
| [24](../Inc/gpio.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/gpio.h#L26) | 预处理 | `#endif` |
| [45](../Inc/gpio.h#L45) | 预处理 | `#ifdef __cplusplus` |
| [47](../Inc/gpio.h#L47) | 预处理 | `#endif` |
| [48](../Inc/gpio.h#L48) | 预处理 | `#endif /*__ GPIO_H__ */` |

## Inc/i2c.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/i2c.h#L21) | 预处理 | `#ifndef __I2C_H__` |
| [22](../Inc/i2c.h#L22) | 预处理 | `#define __I2C_H__` |
| [24](../Inc/i2c.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/i2c.h#L26) | 预处理 | `#endif` |
| [47](../Inc/i2c.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/i2c.h#L49) | 预处理 | `#endif` |
| [51](../Inc/i2c.h#L51) | 预处理 | `#endif /* __I2C_H__ */` |

## Inc/main.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [22](../Inc/main.h#L22) | 预处理 | `#ifndef __MAIN_H` |
| [23](../Inc/main.h#L23) | 预处理 | `#define __MAIN_H` |
| [25](../Inc/main.h#L25) | 预处理 | `#ifdef __cplusplus` |
| [27](../Inc/main.h#L27) | 预处理 | `#endif` |
| [60](../Inc/main.h#L60) | 预处理 | `#define MAG_INT_Pin GPIO_PIN_3` |
| [61](../Inc/main.h#L61) | 预处理 | `#define MAG_INT_GPIO_Port GPIOE` |
| [62](../Inc/main.h#L62) | 预处理 | `#define MAG_RST_Pin GPIO_PIN_2` |
| [63](../Inc/main.h#L63) | 预处理 | `#define MAG_RST_GPIO_Port GPIOE` |
| [64](../Inc/main.h#L64) | 预处理 | `#define IMU_INT_Pin GPIO_PIN_8` |
| [65](../Inc/main.h#L65) | 预处理 | `#define IMU_INT_GPIO_Port GPIOB` |
| [66](../Inc/main.h#L66) | 预处理 | `#define Laser_Pin GPIO_PIN_13` |
| [67](../Inc/main.h#L67) | 预处理 | `#define Laser_GPIO_Port GPIOG` |
| [68](../Inc/main.h#L68) | 预处理 | `#define OLED_DC_Pin GPIO_PIN_9` |
| [69](../Inc/main.h#L69) | 预处理 | `#define OLED_DC_GPIO_Port GPIOB` |
| [70](../Inc/main.h#L70) | 预处理 | `#define Power1_Pin GPIO_PIN_2` |
| [71](../Inc/main.h#L71) | 预处理 | `#define Power1_GPIO_Port GPIOH` |
| [72](../Inc/main.h#L72) | 预处理 | `#define Power2_Pin GPIO_PIN_3` |
| [73](../Inc/main.h#L73) | 预处理 | `#define Power2_GPIO_Port GPIOH` |
| [74](../Inc/main.h#L74) | 预处理 | `#define Power3_Pin GPIO_PIN_4` |
| [75](../Inc/main.h#L75) | 预处理 | `#define Power3_GPIO_Port GPIOH` |
| [76](../Inc/main.h#L76) | 预处理 | `#define D8_Pin GPIO_PIN_8` |
| [77](../Inc/main.h#L77) | 预处理 | `#define D8_GPIO_Port GPIOG` |
| [78](../Inc/main.h#L78) | 预处理 | `#define Power4_Pin GPIO_PIN_5` |
| [79](../Inc/main.h#L79) | 预处理 | `#define Power4_GPIO_Port GPIOH` |
| [80](../Inc/main.h#L80) | 预处理 | `#define D7_Pin GPIO_PIN_7` |
| [81](../Inc/main.h#L81) | 预处理 | `#define D7_GPIO_Port GPIOG` |
| [82](../Inc/main.h#L82) | 预处理 | `#define D6_Pin GPIO_PIN_6` |
| [83](../Inc/main.h#L83) | 预处理 | `#define D6_GPIO_Port GPIOG` |
| [84](../Inc/main.h#L84) | 预处理 | `#define D5_Pin GPIO_PIN_5` |
| [85](../Inc/main.h#L85) | 预处理 | `#define D5_GPIO_Port GPIOG` |
| [86](../Inc/main.h#L86) | 预处理 | `#define D4_Pin GPIO_PIN_4` |
| [87](../Inc/main.h#L87) | 预处理 | `#define D4_GPIO_Port GPIOG` |
| [88](../Inc/main.h#L88) | 预处理 | `#define D3_Pin GPIO_PIN_3` |
| [89](../Inc/main.h#L89) | 预处理 | `#define D3_GPIO_Port GPIOG` |
| [90](../Inc/main.h#L90) | 预处理 | `#define D2_Pin GPIO_PIN_2` |
| [91](../Inc/main.h#L91) | 预处理 | `#define D2_GPIO_Port GPIOG` |
| [92](../Inc/main.h#L92) | 预处理 | `#define HJ_L1_Pin GPIO_PIN_0` |
| [93](../Inc/main.h#L93) | 预处理 | `#define HJ_L1_GPIO_Port GPIOC` |
| [94](../Inc/main.h#L94) | 预处理 | `#define HJ_L2_Pin GPIO_PIN_1` |
| [95](../Inc/main.h#L95) | 预处理 | `#define HJ_L2_GPIO_Port GPIOC` |
| [96](../Inc/main.h#L96) | 预处理 | `#define KEY_Pin GPIO_PIN_2` |
| [97](../Inc/main.h#L97) | 预处理 | `#define KEY_GPIO_Port GPIOB` |
| [98](../Inc/main.h#L98) | 预处理 | `#define KEY_EXTI_IRQn EXTI2_IRQn` |
| [99](../Inc/main.h#L99) | 预处理 | `#define D1_Pin GPIO_PIN_1` |
| [100](../Inc/main.h#L100) | 预处理 | `#define D1_GPIO_Port GPIOG` |
| [101](../Inc/main.h#L101) | 预处理 | `#define BUZZER_Pin GPIO_PIN_6` |
| [102](../Inc/main.h#L102) | 预处理 | `#define BUZZER_GPIO_Port GPIOH` |
| [103](../Inc/main.h#L103) | 预处理 | `#define HJ_M_Pin GPIO_PIN_4` |
| [104](../Inc/main.h#L104) | 预处理 | `#define HJ_M_GPIO_Port GPIOA` |
| [105](../Inc/main.h#L105) | 预处理 | `#define LED_R_Pin GPIO_PIN_11` |
| [106](../Inc/main.h#L106) | 预处理 | `#define LED_R_GPIO_Port GPIOE` |
| [107](../Inc/main.h#L107) | 预处理 | `#define HJ_R1_Pin GPIO_PIN_1` |
| [108](../Inc/main.h#L108) | 预处理 | `#define HJ_R1_GPIO_Port GPIOB` |
| [109](../Inc/main.h#L109) | 预处理 | `#define HJ_R2_Pin GPIO_PIN_0` |
| [110](../Inc/main.h#L110) | 预处理 | `#define HJ_R2_GPIO_Port GPIOB` |
| [111](../Inc/main.h#L111) | 预处理 | `#define LED_G_Pin GPIO_PIN_14` |
| [112](../Inc/main.h#L112) | 预处理 | `#define LED_G_GPIO_Port GPIOF` |
| [113](../Inc/main.h#L113) | 预处理 | `#define OLED_RST_Pin GPIO_PIN_10` |
| [114](../Inc/main.h#L114) | 预处理 | `#define OLED_RST_GPIO_Port GPIOB` |
| [120](../Inc/main.h#L120) | 预处理 | `#ifdef __cplusplus` |
| [122](../Inc/main.h#L122) | 预处理 | `#endif` |
| [124](../Inc/main.h#L124) | 预处理 | `#endif /* __MAIN_H */` |

## Inc/rng.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/rng.h#L21) | 预处理 | `#ifndef __RNG_H__` |
| [22](../Inc/rng.h#L22) | 预处理 | `#define __RNG_H__` |
| [24](../Inc/rng.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/rng.h#L26) | 预处理 | `#endif` |
| [47](../Inc/rng.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/rng.h#L49) | 预处理 | `#endif` |
| [51](../Inc/rng.h#L51) | 预处理 | `#endif /* __RNG_H__ */` |

## Inc/rtc.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/rtc.h#L21) | 预处理 | `#ifndef __RTC_H__` |
| [22](../Inc/rtc.h#L22) | 预处理 | `#define __RTC_H__` |
| [24](../Inc/rtc.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/rtc.h#L26) | 预处理 | `#endif` |
| [47](../Inc/rtc.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../Inc/rtc.h#L49) | 预处理 | `#endif` |
| [51](../Inc/rtc.h#L51) | 预处理 | `#endif /* __RTC_H__ */` |

## Inc/spi.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/spi.h#L21) | 预处理 | `#ifndef __SPI_H__` |
| [22](../Inc/spi.h#L22) | 预处理 | `#define __SPI_H__` |
| [24](../Inc/spi.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/spi.h#L26) | 预处理 | `#endif` |
| [50](../Inc/spi.h#L50) | 预处理 | `#ifdef __cplusplus` |
| [52](../Inc/spi.h#L52) | 预处理 | `#endif` |
| [54](../Inc/spi.h#L54) | 预处理 | `#endif /* __SPI_H__ */` |

## Inc/stm32f4xx_hal_conf.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [24](../Inc/stm32f4xx_hal_conf.h#L24) | 预处理 | `#ifndef __STM32F4xx_HAL_CONF_H` |
| [25](../Inc/stm32f4xx_hal_conf.h#L25) | 预处理 | `#define __STM32F4xx_HAL_CONF_H` |
| [27](../Inc/stm32f4xx_hal_conf.h#L27) | 预处理 | `#ifdef __cplusplus` |
| [29](../Inc/stm32f4xx_hal_conf.h#L29) | 预处理 | `#endif` |
| [38](../Inc/stm32f4xx_hal_conf.h#L38) | 预处理 | `#define HAL_MODULE_ENABLED` |
| [41](../Inc/stm32f4xx_hal_conf.h#L41) | 预处理 | `#define HAL_ADC_MODULE_ENABLED` |
| [42](../Inc/stm32f4xx_hal_conf.h#L42) | 预处理 | `#define HAL_CAN_MODULE_ENABLED` |
| [43](../Inc/stm32f4xx_hal_conf.h#L43) | 预处理 | `#define HAL_CRC_MODULE_ENABLED` |
| [45](../Inc/stm32f4xx_hal_conf.h#L45) | 预处理 | `#define HAL_DAC_MODULE_ENABLED` |
| [56](../Inc/stm32f4xx_hal_conf.h#L56) | 预处理 | `#define HAL_I2C_MODULE_ENABLED` |
| [60](../Inc/stm32f4xx_hal_conf.h#L60) | 预处理 | `#define HAL_RNG_MODULE_ENABLED` |
| [61](../Inc/stm32f4xx_hal_conf.h#L61) | 预处理 | `#define HAL_RTC_MODULE_ENABLED` |
| [65](../Inc/stm32f4xx_hal_conf.h#L65) | 预处理 | `#define HAL_SPI_MODULE_ENABLED` |
| [66](../Inc/stm32f4xx_hal_conf.h#L66) | 预处理 | `#define HAL_TIM_MODULE_ENABLED` |
| [67](../Inc/stm32f4xx_hal_conf.h#L67) | 预处理 | `#define HAL_UART_MODULE_ENABLED` |
| [73](../Inc/stm32f4xx_hal_conf.h#L73) | 预处理 | `#define HAL_PCD_MODULE_ENABLED` |
| [84](../Inc/stm32f4xx_hal_conf.h#L84) | 预处理 | `#define HAL_GPIO_MODULE_ENABLED` |
| [85](../Inc/stm32f4xx_hal_conf.h#L85) | 预处理 | `#define HAL_EXTI_MODULE_ENABLED` |
| [86](../Inc/stm32f4xx_hal_conf.h#L86) | 预处理 | `#define HAL_DMA_MODULE_ENABLED` |
| [87](../Inc/stm32f4xx_hal_conf.h#L87) | 预处理 | `#define HAL_RCC_MODULE_ENABLED` |
| [88](../Inc/stm32f4xx_hal_conf.h#L88) | 预处理 | `#define HAL_FLASH_MODULE_ENABLED` |
| [89](../Inc/stm32f4xx_hal_conf.h#L89) | 预处理 | `#define HAL_PWR_MODULE_ENABLED` |
| [90](../Inc/stm32f4xx_hal_conf.h#L90) | 预处理 | `#define HAL_CORTEX_MODULE_ENABLED` |
| [98](../Inc/stm32f4xx_hal_conf.h#L98) | 预处理 | `#if !defined  (HSE_VALUE)` |
| [99](../Inc/stm32f4xx_hal_conf.h#L99) | 预处理 | `#define HSE_VALUE    12000000U /*!< Value of the External oscillator in Hz */` |
| [100](../Inc/stm32f4xx_hal_conf.h#L100) | 预处理 | `#endif /* HSE_VALUE */` |
| [102](../Inc/stm32f4xx_hal_conf.h#L102) | 预处理 | `#if !defined  (HSE_STARTUP_TIMEOUT)` |
| [103](../Inc/stm32f4xx_hal_conf.h#L103) | 预处理 | `#define HSE_STARTUP_TIMEOUT    100U   /*!< Time out for HSE start up, in ms */` |
| [104](../Inc/stm32f4xx_hal_conf.h#L104) | 预处理 | `#endif /* HSE_STARTUP_TIMEOUT */` |
| [111](../Inc/stm32f4xx_hal_conf.h#L111) | 预处理 | `#if !defined  (HSI_VALUE)` |
| [112](../Inc/stm32f4xx_hal_conf.h#L112) | 预处理 | `#define HSI_VALUE    ((uint32_t)16000000U) /*!< Value of the Internal oscillator in Hz*/` |
| [113](../Inc/stm32f4xx_hal_conf.h#L113) | 预处理 | `#endif /* HSI_VALUE */` |
| [118](../Inc/stm32f4xx_hal_conf.h#L118) | 预处理 | `#if !defined  (LSI_VALUE)` |
| [119](../Inc/stm32f4xx_hal_conf.h#L119) | 预处理 | `#define LSI_VALUE  32000U       /*!< LSI Typical Value in Hz*/` |
| [120](../Inc/stm32f4xx_hal_conf.h#L120) | 预处理 | `#endif /* LSI_VALUE */                      /*!< Value of the Internal Low Speed oscillator in Hz` |
| [126](../Inc/stm32f4xx_hal_conf.h#L126) | 预处理 | `#if !defined  (LSE_VALUE)` |
| [127](../Inc/stm32f4xx_hal_conf.h#L127) | 预处理 | `#define LSE_VALUE  32768U    /*!< Value of the External Low Speed oscillator in Hz */` |
| [128](../Inc/stm32f4xx_hal_conf.h#L128) | 预处理 | `#endif /* LSE_VALUE */` |
| [130](../Inc/stm32f4xx_hal_conf.h#L130) | 预处理 | `#if !defined  (LSE_STARTUP_TIMEOUT)` |
| [131](../Inc/stm32f4xx_hal_conf.h#L131) | 预处理 | `#define LSE_STARTUP_TIMEOUT    5000U   /*!< Time out for LSE start up, in ms */` |
| [132](../Inc/stm32f4xx_hal_conf.h#L132) | 预处理 | `#endif /* LSE_STARTUP_TIMEOUT */` |
| [139](../Inc/stm32f4xx_hal_conf.h#L139) | 预处理 | `#if !defined  (EXTERNAL_CLOCK_VALUE)` |
| [140](../Inc/stm32f4xx_hal_conf.h#L140) | 预处理 | `#define EXTERNAL_CLOCK_VALUE    12288000U /*!< Value of the External audio frequency in Hz*/` |
| [141](../Inc/stm32f4xx_hal_conf.h#L141) | 预处理 | `#endif /* EXTERNAL_CLOCK_VALUE */` |
| [150](../Inc/stm32f4xx_hal_conf.h#L150) | 预处理 | `#define  VDD_VALUE		      3300U /*!< Value of VDD in mv */` |
| [151](../Inc/stm32f4xx_hal_conf.h#L151) | 预处理 | `#define  TICK_INT_PRIORITY            15U   /*!< tick interrupt priority */` |
| [152](../Inc/stm32f4xx_hal_conf.h#L152) | 预处理 | `#define  USE_RTOS                     0U` |
| [153](../Inc/stm32f4xx_hal_conf.h#L153) | 预处理 | `#define  PREFETCH_ENABLE              1U` |
| [154](../Inc/stm32f4xx_hal_conf.h#L154) | 预处理 | `#define  INSTRUCTION_CACHE_ENABLE     1U` |
| [155](../Inc/stm32f4xx_hal_conf.h#L155) | 预处理 | `#define  DATA_CACHE_ENABLE            1U` |
| [157](../Inc/stm32f4xx_hal_conf.h#L157) | 预处理 | `#define  USE_HAL_ADC_REGISTER_CALLBACKS         0U /* ADC register callback disabled       */` |
| [158](../Inc/stm32f4xx_hal_conf.h#L158) | 预处理 | `#define  USE_HAL_CAN_REGISTER_CALLBACKS         0U /* CAN register callback disabled       */` |
| [159](../Inc/stm32f4xx_hal_conf.h#L159) | 预处理 | `#define  USE_HAL_CEC_REGISTER_CALLBACKS         0U /* CEC register callback disabled       */` |
| [160](../Inc/stm32f4xx_hal_conf.h#L160) | 预处理 | `#define  USE_HAL_CRYP_REGISTER_CALLBACKS        0U /* CRYP register callback disabled      */` |
| [161](../Inc/stm32f4xx_hal_conf.h#L161) | 预处理 | `#define  USE_HAL_DAC_REGISTER_CALLBACKS         0U /* DAC register callback disabled       */` |
| [162](../Inc/stm32f4xx_hal_conf.h#L162) | 预处理 | `#define  USE_HAL_DCMI_REGISTER_CALLBACKS        0U /* DCMI register callback disabled      */` |
| [163](../Inc/stm32f4xx_hal_conf.h#L163) | 预处理 | `#define  USE_HAL_DFSDM_REGISTER_CALLBACKS       0U /* DFSDM register callback disabled     */` |
| [164](../Inc/stm32f4xx_hal_conf.h#L164) | 预处理 | `#define  USE_HAL_DMA2D_REGISTER_CALLBACKS       0U /* DMA2D register callback disabled     */` |
| [165](../Inc/stm32f4xx_hal_conf.h#L165) | 预处理 | `#define  USE_HAL_DSI_REGISTER_CALLBACKS         0U /* DSI register callback disabled       */` |
| [166](../Inc/stm32f4xx_hal_conf.h#L166) | 预处理 | `#define  USE_HAL_ETH_REGISTER_CALLBACKS         0U /* ETH register callback disabled       */` |
| [167](../Inc/stm32f4xx_hal_conf.h#L167) | 预处理 | `#define  USE_HAL_HASH_REGISTER_CALLBACKS        0U /* HASH register callback disabled      */` |
| [168](../Inc/stm32f4xx_hal_conf.h#L168) | 预处理 | `#define  USE_HAL_HCD_REGISTER_CALLBACKS         0U /* HCD register callback disabled       */` |
| [169](../Inc/stm32f4xx_hal_conf.h#L169) | 预处理 | `#define  USE_HAL_I2C_REGISTER_CALLBACKS         0U /* I2C register callback disabled       */` |
| [170](../Inc/stm32f4xx_hal_conf.h#L170) | 预处理 | `#define  USE_HAL_FMPI2C_REGISTER_CALLBACKS      0U /* FMPI2C register callback disabled    */` |
| [171](../Inc/stm32f4xx_hal_conf.h#L171) | 预处理 | `#define  USE_HAL_FMPSMBUS_REGISTER_CALLBACKS    0U /* FMPSMBUS register callback disabled  */` |
| [172](../Inc/stm32f4xx_hal_conf.h#L172) | 预处理 | `#define  USE_HAL_I2S_REGISTER_CALLBACKS         0U /* I2S register callback disabled       */` |
| [173](../Inc/stm32f4xx_hal_conf.h#L173) | 预处理 | `#define  USE_HAL_IRDA_REGISTER_CALLBACKS        0U /* IRDA register callback disabled      */` |
| [174](../Inc/stm32f4xx_hal_conf.h#L174) | 预处理 | `#define  USE_HAL_LPTIM_REGISTER_CALLBACKS       0U /* LPTIM register callback disabled     */` |
| [175](../Inc/stm32f4xx_hal_conf.h#L175) | 预处理 | `#define  USE_HAL_LTDC_REGISTER_CALLBACKS        0U /* LTDC register callback disabled      */` |
| [176](../Inc/stm32f4xx_hal_conf.h#L176) | 预处理 | `#define  USE_HAL_MMC_REGISTER_CALLBACKS         0U /* MMC register callback disabled       */` |
| [177](../Inc/stm32f4xx_hal_conf.h#L177) | 预处理 | `#define  USE_HAL_NAND_REGISTER_CALLBACKS        0U /* NAND register callback disabled      */` |
| [178](../Inc/stm32f4xx_hal_conf.h#L178) | 预处理 | `#define  USE_HAL_NOR_REGISTER_CALLBACKS         0U /* NOR register callback disabled       */` |
| [179](../Inc/stm32f4xx_hal_conf.h#L179) | 预处理 | `#define  USE_HAL_PCCARD_REGISTER_CALLBACKS      0U /* PCCARD register callback disabled    */` |
| [180](../Inc/stm32f4xx_hal_conf.h#L180) | 预处理 | `#define  USE_HAL_PCD_REGISTER_CALLBACKS         0U /* PCD register callback disabled       */` |
| [181](../Inc/stm32f4xx_hal_conf.h#L181) | 预处理 | `#define  USE_HAL_QSPI_REGISTER_CALLBACKS        0U /* QSPI register callback disabled      */` |
| [182](../Inc/stm32f4xx_hal_conf.h#L182) | 预处理 | `#define  USE_HAL_RNG_REGISTER_CALLBACKS         0U /* RNG register callback disabled       */` |
| [183](../Inc/stm32f4xx_hal_conf.h#L183) | 预处理 | `#define  USE_HAL_RTC_REGISTER_CALLBACKS         0U /* RTC register callback disabled       */` |
| [184](../Inc/stm32f4xx_hal_conf.h#L184) | 预处理 | `#define  USE_HAL_SAI_REGISTER_CALLBACKS         0U /* SAI register callback disabled       */` |
| [185](../Inc/stm32f4xx_hal_conf.h#L185) | 预处理 | `#define  USE_HAL_SD_REGISTER_CALLBACKS          0U /* SD register callback disabled        */` |
| [186](../Inc/stm32f4xx_hal_conf.h#L186) | 预处理 | `#define  USE_HAL_SMARTCARD_REGISTER_CALLBACKS   0U /* SMARTCARD register callback disabled */` |
| [187](../Inc/stm32f4xx_hal_conf.h#L187) | 预处理 | `#define  USE_HAL_SDRAM_REGISTER_CALLBACKS       0U /* SDRAM register callback disabled     */` |
| [188](../Inc/stm32f4xx_hal_conf.h#L188) | 预处理 | `#define  USE_HAL_SRAM_REGISTER_CALLBACKS        0U /* SRAM register callback disabled      */` |
| [189](../Inc/stm32f4xx_hal_conf.h#L189) | 预处理 | `#define  USE_HAL_SPDIFRX_REGISTER_CALLBACKS     0U /* SPDIFRX register callback disabled   */` |
| [190](../Inc/stm32f4xx_hal_conf.h#L190) | 预处理 | `#define  USE_HAL_SMBUS_REGISTER_CALLBACKS       0U /* SMBUS register callback disabled     */` |
| [191](../Inc/stm32f4xx_hal_conf.h#L191) | 预处理 | `#define  USE_HAL_SPI_REGISTER_CALLBACKS         0U /* SPI register callback disabled       */` |
| [192](../Inc/stm32f4xx_hal_conf.h#L192) | 预处理 | `#define  USE_HAL_TIM_REGISTER_CALLBACKS         0U /* TIM register callback disabled       */` |
| [193](../Inc/stm32f4xx_hal_conf.h#L193) | 预处理 | `#define  USE_HAL_UART_REGISTER_CALLBACKS        0U /* UART register callback disabled      */` |
| [194](../Inc/stm32f4xx_hal_conf.h#L194) | 预处理 | `#define  USE_HAL_USART_REGISTER_CALLBACKS       0U /* USART register callback disabled     */` |
| [195](../Inc/stm32f4xx_hal_conf.h#L195) | 预处理 | `#define  USE_HAL_WWDG_REGISTER_CALLBACKS        0U /* WWDG register callback disabled      */` |
| [209](../Inc/stm32f4xx_hal_conf.h#L209) | 预处理 | `#define MAC_ADDR0   2U` |
| [210](../Inc/stm32f4xx_hal_conf.h#L210) | 预处理 | `#define MAC_ADDR1   0U` |
| [211](../Inc/stm32f4xx_hal_conf.h#L211) | 预处理 | `#define MAC_ADDR2   0U` |
| [212](../Inc/stm32f4xx_hal_conf.h#L212) | 预处理 | `#define MAC_ADDR3   0U` |
| [213](../Inc/stm32f4xx_hal_conf.h#L213) | 预处理 | `#define MAC_ADDR4   0U` |
| [214](../Inc/stm32f4xx_hal_conf.h#L214) | 预处理 | `#define MAC_ADDR5   0U` |
| [217](../Inc/stm32f4xx_hal_conf.h#L217) | 预处理 | `#define ETH_RX_BUF_SIZE                 /* buffer size for receive               */` |
| [218](../Inc/stm32f4xx_hal_conf.h#L218) | 预处理 | `#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for transmit              */` |
| [219](../Inc/stm32f4xx_hal_conf.h#L219) | 预处理 | `#define ETH_RXBUFNB                    4U       /* 4 Rx buffers of size ETH_RX_BUF_SIZE  */` |
| [220](../Inc/stm32f4xx_hal_conf.h#L220) | 预处理 | `#define ETH_TXBUFNB                    4U       /* 4 Tx buffers of size ETH_TX_BUF_SIZE  */` |
| [225](../Inc/stm32f4xx_hal_conf.h#L225) | 预处理 | `#define DP83848_PHY_ADDRESS` |
| [227](../Inc/stm32f4xx_hal_conf.h#L227) | 预处理 | `#define PHY_RESET_DELAY                 0x000000FFU` |
| [229](../Inc/stm32f4xx_hal_conf.h#L229) | 预处理 | `#define PHY_CONFIG_DELAY                0x00000FFFU` |
| [231](../Inc/stm32f4xx_hal_conf.h#L231) | 预处理 | `#define PHY_READ_TO                     0x0000FFFFU` |
| [232](../Inc/stm32f4xx_hal_conf.h#L232) | 预处理 | `#define PHY_WRITE_TO                    0x0000FFFFU` |
| [236](../Inc/stm32f4xx_hal_conf.h#L236) | 预处理 | `#define PHY_BCR                         ((uint16_t)0x0000U)    /*!< Transceiver Basic Control Register   */` |
| [237](../Inc/stm32f4xx_hal_conf.h#L237) | 预处理 | `#define PHY_BSR                         ((uint16_t)0x0001U)    /*!< Transceiver Basic Status Register    */` |
| [239](../Inc/stm32f4xx_hal_conf.h#L239) | 预处理 | `#define PHY_RESET                       ((uint16_t)0x8000U)  /*!< PHY Reset */` |
| [240](../Inc/stm32f4xx_hal_conf.h#L240) | 预处理 | `#define PHY_LOOPBACK                    ((uint16_t)0x4000U)  /*!< Select loop-back mode */` |
| [241](../Inc/stm32f4xx_hal_conf.h#L241) | 预处理 | `#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100U)  /*!< Set the full-duplex mode at 100 Mb/s */` |
| [242](../Inc/stm32f4xx_hal_conf.h#L242) | 预处理 | `#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000U)  /*!< Set the half-duplex mode at 100 Mb/s */` |
| [243](../Inc/stm32f4xx_hal_conf.h#L243) | 预处理 | `#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100U)  /*!< Set the full-duplex mode at 10 Mb/s  */` |
| [244](../Inc/stm32f4xx_hal_conf.h#L244) | 预处理 | `#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000U)  /*!< Set the half-duplex mode at 10 Mb/s  */` |
| [245](../Inc/stm32f4xx_hal_conf.h#L245) | 预处理 | `#define PHY_AUTONEGOTIATION             ((uint16_t)0x1000U)  /*!< Enable auto-negotiation function     */` |
| [246](../Inc/stm32f4xx_hal_conf.h#L246) | 预处理 | `#define PHY_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200U)  /*!< Restart auto-negotiation function    */` |
| [247](../Inc/stm32f4xx_hal_conf.h#L247) | 预处理 | `#define PHY_POWERDOWN                   ((uint16_t)0x0800U)  /*!< Select the power down mode           */` |
| [248](../Inc/stm32f4xx_hal_conf.h#L248) | 预处理 | `#define PHY_ISOLATE                     ((uint16_t)0x0400U)  /*!< Isolate PHY from MII                 */` |
| [250](../Inc/stm32f4xx_hal_conf.h#L250) | 预处理 | `#define PHY_AUTONEGO_COMPLETE           ((uint16_t)0x0020U)  /*!< Auto-Negotiation process completed   */` |
| [251](../Inc/stm32f4xx_hal_conf.h#L251) | 预处理 | `#define PHY_LINKED_STATUS               ((uint16_t)0x0004U)  /*!< Valid link established               */` |
| [252](../Inc/stm32f4xx_hal_conf.h#L252) | 预处理 | `#define PHY_JABBER_DETECTION            ((uint16_t)0x0002U)  /*!< Jabber condition detected            */` |
| [255](../Inc/stm32f4xx_hal_conf.h#L255) | 预处理 | `#define PHY_SR                          ((uint16_t))    /*!< PHY status register Offset                      */` |
| [257](../Inc/stm32f4xx_hal_conf.h#L257) | 预处理 | `#define PHY_SPEED_STATUS                ((uint16_t))  /*!< PHY Speed mask                                  */` |
| [258](../Inc/stm32f4xx_hal_conf.h#L258) | 预处理 | `#define PHY_DUPLEX_STATUS               ((uint16_t))  /*!< PHY Duplex mask                                 */` |
| [267](../Inc/stm32f4xx_hal_conf.h#L267) | 预处理 | `#define USE_SPI_CRC                     0U` |
| [274](../Inc/stm32f4xx_hal_conf.h#L274) | 预处理 | `#ifdef HAL_RCC_MODULE_ENABLED` |
| [276](../Inc/stm32f4xx_hal_conf.h#L276) | 预处理 | `#endif /* HAL_RCC_MODULE_ENABLED */` |
| [278](../Inc/stm32f4xx_hal_conf.h#L278) | 预处理 | `#ifdef HAL_GPIO_MODULE_ENABLED` |
| [280](../Inc/stm32f4xx_hal_conf.h#L280) | 预处理 | `#endif /* HAL_GPIO_MODULE_ENABLED */` |
| [282](../Inc/stm32f4xx_hal_conf.h#L282) | 预处理 | `#ifdef HAL_EXTI_MODULE_ENABLED` |
| [284](../Inc/stm32f4xx_hal_conf.h#L284) | 预处理 | `#endif /* HAL_EXTI_MODULE_ENABLED */` |
| [286](../Inc/stm32f4xx_hal_conf.h#L286) | 预处理 | `#ifdef HAL_DMA_MODULE_ENABLED` |
| [288](../Inc/stm32f4xx_hal_conf.h#L288) | 预处理 | `#endif /* HAL_DMA_MODULE_ENABLED */` |
| [290](../Inc/stm32f4xx_hal_conf.h#L290) | 预处理 | `#ifdef HAL_CORTEX_MODULE_ENABLED` |
| [292](../Inc/stm32f4xx_hal_conf.h#L292) | 预处理 | `#endif /* HAL_CORTEX_MODULE_ENABLED */` |
| [294](../Inc/stm32f4xx_hal_conf.h#L294) | 预处理 | `#ifdef HAL_ADC_MODULE_ENABLED` |
| [296](../Inc/stm32f4xx_hal_conf.h#L296) | 预处理 | `#endif /* HAL_ADC_MODULE_ENABLED */` |
| [298](../Inc/stm32f4xx_hal_conf.h#L298) | 预处理 | `#ifdef HAL_CAN_MODULE_ENABLED` |
| [300](../Inc/stm32f4xx_hal_conf.h#L300) | 预处理 | `#endif /* HAL_CAN_MODULE_ENABLED */` |
| [302](../Inc/stm32f4xx_hal_conf.h#L302) | 预处理 | `#ifdef HAL_CAN_LEGACY_MODULE_ENABLED` |
| [304](../Inc/stm32f4xx_hal_conf.h#L304) | 预处理 | `#endif /* HAL_CAN_LEGACY_MODULE_ENABLED */` |
| [306](../Inc/stm32f4xx_hal_conf.h#L306) | 预处理 | `#ifdef HAL_CRC_MODULE_ENABLED` |
| [308](../Inc/stm32f4xx_hal_conf.h#L308) | 预处理 | `#endif /* HAL_CRC_MODULE_ENABLED */` |
| [310](../Inc/stm32f4xx_hal_conf.h#L310) | 预处理 | `#ifdef HAL_CRYP_MODULE_ENABLED` |
| [312](../Inc/stm32f4xx_hal_conf.h#L312) | 预处理 | `#endif /* HAL_CRYP_MODULE_ENABLED */` |
| [314](../Inc/stm32f4xx_hal_conf.h#L314) | 预处理 | `#ifdef HAL_DMA2D_MODULE_ENABLED` |
| [316](../Inc/stm32f4xx_hal_conf.h#L316) | 预处理 | `#endif /* HAL_DMA2D_MODULE_ENABLED */` |
| [318](../Inc/stm32f4xx_hal_conf.h#L318) | 预处理 | `#ifdef HAL_DAC_MODULE_ENABLED` |
| [320](../Inc/stm32f4xx_hal_conf.h#L320) | 预处理 | `#endif /* HAL_DAC_MODULE_ENABLED */` |
| [322](../Inc/stm32f4xx_hal_conf.h#L322) | 预处理 | `#ifdef HAL_DCMI_MODULE_ENABLED` |
| [324](../Inc/stm32f4xx_hal_conf.h#L324) | 预处理 | `#endif /* HAL_DCMI_MODULE_ENABLED */` |
| [326](../Inc/stm32f4xx_hal_conf.h#L326) | 预处理 | `#ifdef HAL_ETH_MODULE_ENABLED` |
| [328](../Inc/stm32f4xx_hal_conf.h#L328) | 预处理 | `#endif /* HAL_ETH_MODULE_ENABLED */` |
| [330](../Inc/stm32f4xx_hal_conf.h#L330) | 预处理 | `#ifdef HAL_ETH_LEGACY_MODULE_ENABLED` |
| [332](../Inc/stm32f4xx_hal_conf.h#L332) | 预处理 | `#endif /* HAL_ETH_LEGACY_MODULE_ENABLED */` |
| [334](../Inc/stm32f4xx_hal_conf.h#L334) | 预处理 | `#ifdef HAL_FLASH_MODULE_ENABLED` |
| [336](../Inc/stm32f4xx_hal_conf.h#L336) | 预处理 | `#endif /* HAL_FLASH_MODULE_ENABLED */` |
| [338](../Inc/stm32f4xx_hal_conf.h#L338) | 预处理 | `#ifdef HAL_SRAM_MODULE_ENABLED` |
| [340](../Inc/stm32f4xx_hal_conf.h#L340) | 预处理 | `#endif /* HAL_SRAM_MODULE_ENABLED */` |
| [342](../Inc/stm32f4xx_hal_conf.h#L342) | 预处理 | `#ifdef HAL_NOR_MODULE_ENABLED` |
| [344](../Inc/stm32f4xx_hal_conf.h#L344) | 预处理 | `#endif /* HAL_NOR_MODULE_ENABLED */` |
| [346](../Inc/stm32f4xx_hal_conf.h#L346) | 预处理 | `#ifdef HAL_NAND_MODULE_ENABLED` |
| [348](../Inc/stm32f4xx_hal_conf.h#L348) | 预处理 | `#endif /* HAL_NAND_MODULE_ENABLED */` |
| [350](../Inc/stm32f4xx_hal_conf.h#L350) | 预处理 | `#ifdef HAL_PCCARD_MODULE_ENABLED` |
| [352](../Inc/stm32f4xx_hal_conf.h#L352) | 预处理 | `#endif /* HAL_PCCARD_MODULE_ENABLED */` |
| [354](../Inc/stm32f4xx_hal_conf.h#L354) | 预处理 | `#ifdef HAL_SDRAM_MODULE_ENABLED` |
| [356](../Inc/stm32f4xx_hal_conf.h#L356) | 预处理 | `#endif /* HAL_SDRAM_MODULE_ENABLED */` |
| [358](../Inc/stm32f4xx_hal_conf.h#L358) | 预处理 | `#ifdef HAL_HASH_MODULE_ENABLED` |
| [360](../Inc/stm32f4xx_hal_conf.h#L360) | 预处理 | `#endif /* HAL_HASH_MODULE_ENABLED */` |
| [362](../Inc/stm32f4xx_hal_conf.h#L362) | 预处理 | `#ifdef HAL_I2C_MODULE_ENABLED` |
| [364](../Inc/stm32f4xx_hal_conf.h#L364) | 预处理 | `#endif /* HAL_I2C_MODULE_ENABLED */` |
| [366](../Inc/stm32f4xx_hal_conf.h#L366) | 预处理 | `#ifdef HAL_SMBUS_MODULE_ENABLED` |
| [368](../Inc/stm32f4xx_hal_conf.h#L368) | 预处理 | `#endif /* HAL_SMBUS_MODULE_ENABLED */` |
| [370](../Inc/stm32f4xx_hal_conf.h#L370) | 预处理 | `#ifdef HAL_I2S_MODULE_ENABLED` |
| [372](../Inc/stm32f4xx_hal_conf.h#L372) | 预处理 | `#endif /* HAL_I2S_MODULE_ENABLED */` |
| [374](../Inc/stm32f4xx_hal_conf.h#L374) | 预处理 | `#ifdef HAL_IWDG_MODULE_ENABLED` |
| [376](../Inc/stm32f4xx_hal_conf.h#L376) | 预处理 | `#endif /* HAL_IWDG_MODULE_ENABLED */` |
| [378](../Inc/stm32f4xx_hal_conf.h#L378) | 预处理 | `#ifdef HAL_LTDC_MODULE_ENABLED` |
| [380](../Inc/stm32f4xx_hal_conf.h#L380) | 预处理 | `#endif /* HAL_LTDC_MODULE_ENABLED */` |
| [382](../Inc/stm32f4xx_hal_conf.h#L382) | 预处理 | `#ifdef HAL_PWR_MODULE_ENABLED` |
| [384](../Inc/stm32f4xx_hal_conf.h#L384) | 预处理 | `#endif /* HAL_PWR_MODULE_ENABLED */` |
| [386](../Inc/stm32f4xx_hal_conf.h#L386) | 预处理 | `#ifdef HAL_RNG_MODULE_ENABLED` |
| [388](../Inc/stm32f4xx_hal_conf.h#L388) | 预处理 | `#endif /* HAL_RNG_MODULE_ENABLED */` |
| [390](../Inc/stm32f4xx_hal_conf.h#L390) | 预处理 | `#ifdef HAL_RTC_MODULE_ENABLED` |
| [392](../Inc/stm32f4xx_hal_conf.h#L392) | 预处理 | `#endif /* HAL_RTC_MODULE_ENABLED */` |
| [394](../Inc/stm32f4xx_hal_conf.h#L394) | 预处理 | `#ifdef HAL_SAI_MODULE_ENABLED` |
| [396](../Inc/stm32f4xx_hal_conf.h#L396) | 预处理 | `#endif /* HAL_SAI_MODULE_ENABLED */` |
| [398](../Inc/stm32f4xx_hal_conf.h#L398) | 预处理 | `#ifdef HAL_SD_MODULE_ENABLED` |
| [400](../Inc/stm32f4xx_hal_conf.h#L400) | 预处理 | `#endif /* HAL_SD_MODULE_ENABLED */` |
| [402](../Inc/stm32f4xx_hal_conf.h#L402) | 预处理 | `#ifdef HAL_SPI_MODULE_ENABLED` |
| [404](../Inc/stm32f4xx_hal_conf.h#L404) | 预处理 | `#endif /* HAL_SPI_MODULE_ENABLED */` |
| [406](../Inc/stm32f4xx_hal_conf.h#L406) | 预处理 | `#ifdef HAL_TIM_MODULE_ENABLED` |
| [408](../Inc/stm32f4xx_hal_conf.h#L408) | 预处理 | `#endif /* HAL_TIM_MODULE_ENABLED */` |
| [410](../Inc/stm32f4xx_hal_conf.h#L410) | 预处理 | `#ifdef HAL_UART_MODULE_ENABLED` |
| [412](../Inc/stm32f4xx_hal_conf.h#L412) | 预处理 | `#endif /* HAL_UART_MODULE_ENABLED */` |
| [414](../Inc/stm32f4xx_hal_conf.h#L414) | 预处理 | `#ifdef HAL_USART_MODULE_ENABLED` |
| [416](../Inc/stm32f4xx_hal_conf.h#L416) | 预处理 | `#endif /* HAL_USART_MODULE_ENABLED */` |
| [418](../Inc/stm32f4xx_hal_conf.h#L418) | 预处理 | `#ifdef HAL_IRDA_MODULE_ENABLED` |
| [420](../Inc/stm32f4xx_hal_conf.h#L420) | 预处理 | `#endif /* HAL_IRDA_MODULE_ENABLED */` |
| [422](../Inc/stm32f4xx_hal_conf.h#L422) | 预处理 | `#ifdef HAL_SMARTCARD_MODULE_ENABLED` |
| [424](../Inc/stm32f4xx_hal_conf.h#L424) | 预处理 | `#endif /* HAL_SMARTCARD_MODULE_ENABLED */` |
| [426](../Inc/stm32f4xx_hal_conf.h#L426) | 预处理 | `#ifdef HAL_WWDG_MODULE_ENABLED` |
| [428](../Inc/stm32f4xx_hal_conf.h#L428) | 预处理 | `#endif /* HAL_WWDG_MODULE_ENABLED */` |
| [430](../Inc/stm32f4xx_hal_conf.h#L430) | 预处理 | `#ifdef HAL_PCD_MODULE_ENABLED` |
| [432](../Inc/stm32f4xx_hal_conf.h#L432) | 预处理 | `#endif /* HAL_PCD_MODULE_ENABLED */` |
| [434](../Inc/stm32f4xx_hal_conf.h#L434) | 预处理 | `#ifdef HAL_HCD_MODULE_ENABLED` |
| [436](../Inc/stm32f4xx_hal_conf.h#L436) | 预处理 | `#endif /* HAL_HCD_MODULE_ENABLED */` |
| [438](../Inc/stm32f4xx_hal_conf.h#L438) | 预处理 | `#ifdef HAL_DSI_MODULE_ENABLED` |
| [440](../Inc/stm32f4xx_hal_conf.h#L440) | 预处理 | `#endif /* HAL_DSI_MODULE_ENABLED */` |
| [442](../Inc/stm32f4xx_hal_conf.h#L442) | 预处理 | `#ifdef HAL_QSPI_MODULE_ENABLED` |
| [444](../Inc/stm32f4xx_hal_conf.h#L444) | 预处理 | `#endif /* HAL_QSPI_MODULE_ENABLED */` |
| [446](../Inc/stm32f4xx_hal_conf.h#L446) | 预处理 | `#ifdef HAL_CEC_MODULE_ENABLED` |
| [448](../Inc/stm32f4xx_hal_conf.h#L448) | 预处理 | `#endif /* HAL_CEC_MODULE_ENABLED */` |
| [450](../Inc/stm32f4xx_hal_conf.h#L450) | 预处理 | `#ifdef HAL_FMPI2C_MODULE_ENABLED` |
| [452](../Inc/stm32f4xx_hal_conf.h#L452) | 预处理 | `#endif /* HAL_FMPI2C_MODULE_ENABLED */` |
| [454](../Inc/stm32f4xx_hal_conf.h#L454) | 预处理 | `#ifdef HAL_FMPSMBUS_MODULE_ENABLED` |
| [456](../Inc/stm32f4xx_hal_conf.h#L456) | 预处理 | `#endif /* HAL_FMPSMBUS_MODULE_ENABLED */` |
| [458](../Inc/stm32f4xx_hal_conf.h#L458) | 预处理 | `#ifdef HAL_SPDIFRX_MODULE_ENABLED` |
| [460](../Inc/stm32f4xx_hal_conf.h#L460) | 预处理 | `#endif /* HAL_SPDIFRX_MODULE_ENABLED */` |
| [462](../Inc/stm32f4xx_hal_conf.h#L462) | 预处理 | `#ifdef HAL_DFSDM_MODULE_ENABLED` |
| [464](../Inc/stm32f4xx_hal_conf.h#L464) | 预处理 | `#endif /* HAL_DFSDM_MODULE_ENABLED */` |
| [466](../Inc/stm32f4xx_hal_conf.h#L466) | 预处理 | `#ifdef HAL_LPTIM_MODULE_ENABLED` |
| [468](../Inc/stm32f4xx_hal_conf.h#L468) | 预处理 | `#endif /* HAL_LPTIM_MODULE_ENABLED */` |
| [470](../Inc/stm32f4xx_hal_conf.h#L470) | 预处理 | `#ifdef HAL_MMC_MODULE_ENABLED` |
| [472](../Inc/stm32f4xx_hal_conf.h#L472) | 预处理 | `#endif /* HAL_MMC_MODULE_ENABLED */` |
| [475](../Inc/stm32f4xx_hal_conf.h#L475) | 预处理 | `#ifdef  USE_FULL_ASSERT` |
| [484](../Inc/stm32f4xx_hal_conf.h#L484) | 预处理 | `#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))` |
| [487](../Inc/stm32f4xx_hal_conf.h#L487) | 预处理 | `#else` |
| [488](../Inc/stm32f4xx_hal_conf.h#L488) | 预处理 | `#define assert_param(expr) ((void)0U)` |
| [489](../Inc/stm32f4xx_hal_conf.h#L489) | 预处理 | `#endif /* USE_FULL_ASSERT */` |
| [491](../Inc/stm32f4xx_hal_conf.h#L491) | 预处理 | `#ifdef __cplusplus` |
| [493](../Inc/stm32f4xx_hal_conf.h#L493) | 预处理 | `#endif` |
| [495](../Inc/stm32f4xx_hal_conf.h#L495) | 预处理 | `#endif /* __STM32F4xx_HAL_CONF_H */` |

## Inc/stm32f4xx_it.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/stm32f4xx_it.h#L21) | 预处理 | `#ifndef __STM32F4xx_IT_H` |
| [22](../Inc/stm32f4xx_it.h#L22) | 预处理 | `#define __STM32F4xx_IT_H` |
| [24](../Inc/stm32f4xx_it.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/stm32f4xx_it.h#L26) | 预处理 | `#endif` |
| [82](../Inc/stm32f4xx_it.h#L82) | 预处理 | `#ifdef __cplusplus` |
| [84](../Inc/stm32f4xx_it.h#L84) | 预处理 | `#endif` |
| [86](../Inc/stm32f4xx_it.h#L86) | 预处理 | `#endif /* __STM32F4xx_IT_H */` |

## Inc/tim.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/tim.h#L21) | 预处理 | `#ifndef __TIM_H__` |
| [22](../Inc/tim.h#L22) | 预处理 | `#define __TIM_H__` |
| [24](../Inc/tim.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/tim.h#L26) | 预处理 | `#endif` |
| [67](../Inc/tim.h#L67) | 预处理 | `#ifdef __cplusplus` |
| [69](../Inc/tim.h#L69) | 预处理 | `#endif` |
| [71](../Inc/tim.h#L71) | 预处理 | `#endif /* __TIM_H__ */` |

## Inc/usart.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/usart.h#L21) | 预处理 | `#ifndef __USART_H__` |
| [22](../Inc/usart.h#L22) | 预处理 | `#define __USART_H__` |
| [24](../Inc/usart.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/usart.h#L26) | 预处理 | `#endif` |
| [62](../Inc/usart.h#L62) | 预处理 | `#ifdef __cplusplus` |
| [64](../Inc/usart.h#L64) | 预处理 | `#endif` |
| [66](../Inc/usart.h#L66) | 预处理 | `#endif /* __USART_H__ */` |

## Inc/usb_device.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [22](../Inc/usb_device.h#L22) | 预处理 | `#ifndef __USB_DEVICE__H__` |
| [23](../Inc/usb_device.h#L23) | 预处理 | `#define __USB_DEVICE__H__` |
| [25](../Inc/usb_device.h#L25) | 预处理 | `#ifdef __cplusplus` |
| [27](../Inc/usb_device.h#L27) | 预处理 | `#endif` |
| [98](../Inc/usb_device.h#L98) | 预处理 | `#ifdef __cplusplus` |
| [100](../Inc/usb_device.h#L100) | 预处理 | `#endif` |
| [102](../Inc/usb_device.h#L102) | 预处理 | `#endif /* __USB_DEVICE__H__ */` |

## Inc/usbd_cdc_if.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [22](../Inc/usbd_cdc_if.h#L22) | 预处理 | `#ifndef __USBD_CDC_IF_H__` |
| [23](../Inc/usbd_cdc_if.h#L23) | 预处理 | `#define __USBD_CDC_IF_H__` |
| [25](../Inc/usbd_cdc_if.h#L25) | 预处理 | `#ifdef __cplusplus` |
| [27](../Inc/usbd_cdc_if.h#L27) | 预处理 | `#endif` |
| [51](../Inc/usbd_cdc_if.h#L51) | 预处理 | `#define APP_RX_DATA_SIZE  2048` |
| [52](../Inc/usbd_cdc_if.h#L52) | 预处理 | `#define APP_TX_DATA_SIZE  2048` |
| [129](../Inc/usbd_cdc_if.h#L129) | 预处理 | `#ifdef __cplusplus` |
| [131](../Inc/usbd_cdc_if.h#L131) | 预处理 | `#endif` |
| [133](../Inc/usbd_cdc_if.h#L133) | 预处理 | `#endif /* __USBD_CDC_IF_H__ */` |

## Inc/usbd_conf.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [22](../Inc/usbd_conf.h#L22) | 预处理 | `#ifndef __USBD_CONF__H__` |
| [23](../Inc/usbd_conf.h#L23) | 预处理 | `#define __USBD_CONF__H__` |
| [25](../Inc/usbd_conf.h#L25) | 预处理 | `#ifdef __cplusplus` |
| [27](../Inc/usbd_conf.h#L27) | 预处理 | `#endif` |
| [66](../Inc/usbd_conf.h#L66) | 预处理 | `#define USBD_MAX_NUM_INTERFACES     1U` |
| [68](../Inc/usbd_conf.h#L68) | 预处理 | `#define USBD_MAX_NUM_CONFIGURATION     1U` |
| [70](../Inc/usbd_conf.h#L70) | 预处理 | `#define USBD_MAX_STR_DESC_SIZ     512U` |
| [72](../Inc/usbd_conf.h#L72) | 预处理 | `#define USBD_DEBUG_LEVEL     0U` |
| [74](../Inc/usbd_conf.h#L74) | 预处理 | `#define USBD_LPM_ENABLED     0U` |
| [76](../Inc/usbd_conf.h#L76) | 预处理 | `#define USBD_SELF_POWERED     1U` |
| [80](../Inc/usbd_conf.h#L80) | 预处理 | `#define DEVICE_FS 		0` |
| [81](../Inc/usbd_conf.h#L81) | 预处理 | `#define DEVICE_HS 		1` |
| [94](../Inc/usbd_conf.h#L94) | 预处理 | `#define USBD_malloc         (void *)USBD_static_malloc` |
| [97](../Inc/usbd_conf.h#L97) | 预处理 | `#define USBD_free           USBD_static_free` |
| [100](../Inc/usbd_conf.h#L100) | 预处理 | `#define USBD_memset         memset` |
| [103](../Inc/usbd_conf.h#L103) | 预处理 | `#define USBD_memcpy         memcpy` |
| [106](../Inc/usbd_conf.h#L106) | 预处理 | `#define USBD_Delay          HAL_Delay` |
| [110](../Inc/usbd_conf.h#L110) | 预处理 | `#if (USBD_DEBUG_LEVEL > 0)` |
| [111](../Inc/usbd_conf.h#L111) | 预处理 | `#define USBD_UsrLog(...)    printf(__VA_ARGS__);\` |
| [113](../Inc/usbd_conf.h#L113) | 预处理 | `#else` |
| [114](../Inc/usbd_conf.h#L114) | 预处理 | `#define USBD_UsrLog(...)` |
| [115](../Inc/usbd_conf.h#L115) | 预处理 | `#endif /* (USBD_DEBUG_LEVEL > 0U) */` |
| [117](../Inc/usbd_conf.h#L117) | 预处理 | `#if (USBD_DEBUG_LEVEL > 1)` |
| [119](../Inc/usbd_conf.h#L119) | 预处理 | `#define USBD_ErrLog(...)    printf("ERROR: ");\` |
| [122](../Inc/usbd_conf.h#L122) | 预处理 | `#else` |
| [123](../Inc/usbd_conf.h#L123) | 预处理 | `#define USBD_ErrLog(...)` |
| [124](../Inc/usbd_conf.h#L124) | 预处理 | `#endif /* (USBD_DEBUG_LEVEL > 1U) */` |
| [126](../Inc/usbd_conf.h#L126) | 预处理 | `#if (USBD_DEBUG_LEVEL > 2)` |
| [127](../Inc/usbd_conf.h#L127) | 预处理 | `#define USBD_DbgLog(...)    printf("DEBUG : ");\` |
| [130](../Inc/usbd_conf.h#L130) | 预处理 | `#else` |
| [131](../Inc/usbd_conf.h#L131) | 预处理 | `#define USBD_DbgLog(...)` |
| [132](../Inc/usbd_conf.h#L132) | 预处理 | `#endif /* (USBD_DEBUG_LEVEL > 2U) */` |
| [168](../Inc/usbd_conf.h#L168) | 预处理 | `#ifdef __cplusplus` |
| [170](../Inc/usbd_conf.h#L170) | 预处理 | `#endif` |
| [172](../Inc/usbd_conf.h#L172) | 预处理 | `#endif /* __USBD_CONF__H__ */` |

## Inc/usbd_desc.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../Inc/usbd_desc.h#L21) | 预处理 | `#ifndef __USBD_DESC__C__` |
| [22](../Inc/usbd_desc.h#L22) | 预处理 | `#define __USBD_DESC__C__` |
| [24](../Inc/usbd_desc.h#L24) | 预处理 | `#ifdef __cplusplus` |
| [26](../Inc/usbd_desc.h#L26) | 预处理 | `#endif` |
| [48](../Inc/usbd_desc.h#L48) | 预处理 | `#define         DEVICE_ID1          (UID_BASE)` |
| [49](../Inc/usbd_desc.h#L49) | 预处理 | `#define         DEVICE_ID2          (UID_BASE + 0x4)` |
| [50](../Inc/usbd_desc.h#L50) | 预处理 | `#define         DEVICE_ID3          (UID_BASE + 0x8)` |
| [52](../Inc/usbd_desc.h#L52) | 预处理 | `#define  USB_SIZ_STRING_SERIAL       0x1A` |
| [138](../Inc/usbd_desc.h#L138) | 预处理 | `#ifdef __cplusplus` |
| [140](../Inc/usbd_desc.h#L140) | 预处理 | `#endif` |
| [142](../Inc/usbd_desc.h#L142) | 预处理 | `#endif /* __USBD_DESC__C__ */` |

## Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [58](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L58) | 预处理 | `#ifndef SEGGER_RTT_CONF_H` |
| [59](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L59) | 预处理 | `#define SEGGER_RTT_CONF_H` |
| [61](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L61) | 预处理 | `#ifdef __IAR_SYSTEMS_ICC__` |
| [63](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L63) | 预处理 | `#endif` |
| [82](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L82) | 预处理 | `#ifndef   SEGGER_RTT_MAX_NUM_UP_BUFFERS` |
| [83](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L83) | 预处理 | `#define SEGGER_RTT_MAX_NUM_UP_BUFFERS             (3)     // Max. number of up-buffers (T->H) available on this target    (Default: 3)` |
| [84](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L84) | 预处理 | `#endif` |
| [90](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L90) | 预处理 | `#ifndef   SEGGER_RTT_MAX_NUM_DOWN_BUFFERS` |
| [91](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L91) | 预处理 | `#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS           (3)     // Max. number of down-buffers (H->T) available on this target  (Default: 3)` |
| [92](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L92) | 预处理 | `#endif` |
| [94](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L94) | 预处理 | `#ifndef   BUFFER_SIZE_UP` |
| [95](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L95) | 预处理 | `#define BUFFER_SIZE_UP                            (1024)  // Size of the buffer for terminal output of target, up to host (Default: 1k)` |
| [96](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L96) | 预处理 | `#endif` |
| [98](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L98) | 预处理 | `#ifndef   BUFFER_SIZE_DOWN` |
| [99](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L99) | 预处理 | `#define BUFFER_SIZE_DOWN                          (16)    // Size of the buffer for terminal input to target from host (Usually keyboard input) (Default: 16)` |
| [100](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L100) | 预处理 | `#endif` |
| [102](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L102) | 预处理 | `#ifndef   SEGGER_RTT_PRINTF_BUFFER_SIZE` |
| [103](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L103) | 预处理 | `#define SEGGER_RTT_PRINTF_BUFFER_SIZE             (64u)    // Size of buffer for RTT printf to bulk-send chars via RTT     (Default: 64)` |
| [104](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L104) | 预处理 | `#endif` |
| [106](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L106) | 预处理 | `#ifndef   SEGGER_RTT_MODE_DEFAULT` |
| [107](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L107) | 预处理 | `#define SEGGER_RTT_MODE_DEFAULT                   SEGGER_RTT_MODE_NO_BLOCK_SKIP // Mode for pre-initialized terminal channel (buffer 0)` |
| [108](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L108) | 预处理 | `#endif` |
| [122](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L122) | 预处理 | `#ifndef   SEGGER_RTT_MEMCPY_USE_BYTELOOP` |
| [123](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L123) | 预处理 | `#define SEGGER_RTT_MEMCPY_USE_BYTELOOP              0 // 0: Use memcpy/SEGGER_RTT_MEMCPY, 1: Use a simple byte-loop` |
| [124](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L124) | 预处理 | `#endif` |
| [146](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L146) | 预处理 | `#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY` |
| [147](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L147) | 预处理 | `#define SEGGER_RTT_MAX_INTERRUPT_PRIORITY         (0x20)   // Interrupt priority to lock on SEGGER_RTT_LOCK on Cortex-M3/4 (Default: 0x20)` |
| [148](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L148) | 预处理 | `#endif` |
| [155](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L155) | 预处理 | `#if ((defined(__SES_ARM) &#124;&#124; defined(__SES_RISCV) &#124;&#124; defined(__CROSSWORKS_ARM) &#124;&#124; defined(__GNUC__) &#124;&#124; defined(__clang__)) && !defined (__CC_ARM) && !defined(WIN32))` |
| [156](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L156) | 预处理 | `#if (defined(__ARM_ARCH_6M__) &#124;&#124; defined(__ARM_ARCH_8M_BASE__))` |
| [157](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L157) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [167](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L167) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __asm volatile ("msr   primask, %0  \n\t"                         \` |
| [173](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L173) | 预处理 | `#elif (defined(__ARM_ARCH_7M__) &#124;&#124; defined(__ARM_ARCH_7EM__) &#124;&#124; defined(__ARM_ARCH_8M_MAIN__))` |
| [174](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L174) | 预处理 | `#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY` |
| [175](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L175) | 预处理 | `#define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)` |
| [176](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L176) | 预处理 | `#endif` |
| [177](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L177) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [187](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L187) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __asm volatile ("msr   basepri, %0  \n\t"                         \` |
| [194](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L194) | 预处理 | `#elif (defined(__ARM_ARCH_7A__) &#124;&#124; defined(__ARM_ARCH_7R__))` |
| [195](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L195) | 预处理 | `#define SEGGER_RTT_LOCK() {                                                \` |
| [206](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L206) | 预处理 | `#define SEGGER_RTT_UNLOCK() __asm volatile ("mov r0, %0 \n\t"              \` |
| [217](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L217) | 预处理 | `#elif defined(__riscv) &#124;&#124; defined(__riscv_xlen)` |
| [218](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L218) | 预处理 | `#define SEGGER_RTT_LOCK()  {                                               \` |
| [228](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L228) | 预处理 | `#define SEGGER_RTT_UNLOCK()    __asm volatile ("csrr  a1, mstatus  \n\t"     \` |
| [236](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L236) | 预处理 | `#else` |
| [237](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L237) | 预处理 | `#define SEGGER_RTT_LOCK()` |
| [238](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L238) | 预处理 | `#define SEGGER_RTT_UNLOCK()` |
| [239](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L239) | 预处理 | `#endif` |
| [240](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L240) | 预处理 | `#endif` |
| [246](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L246) | 预处理 | `#ifdef __ICCARM__` |
| [247](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L247) | 预处理 | `#if (defined (__ARM6M__)          && (__CORE__ == __ARM6M__))             &#124;&#124;                      \` |
| [249](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L249) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [254](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L254) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __set_PRIMASK(_SEGGER_RTT__LockState);                                         \` |
| [256](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L256) | 预处理 | `#elif (defined (__ARM7EM__)         && (__CORE__ == __ARM7EM__))          &#124;&#124;                      \` |
| [260](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L260) | 预处理 | `#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY` |
| [261](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L261) | 预处理 | `#define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)` |
| [262](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L262) | 预处理 | `#endif` |
| [263](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L263) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [268](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L268) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __set_BASEPRI(_SEGGER_RTT__LockState);                                         \` |
| [270](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L270) | 预处理 | `#elif (defined (__ARM7A__) && (__CORE__ == __ARM7A__))                    &#124;&#124;                      \` |
| [272](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L272) | 预处理 | `#define SEGGER_RTT_LOCK() {                                                                     \` |
| [283](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L283) | 预处理 | `#define SEGGER_RTT_UNLOCK() __asm volatile ("mov r0, %0 \n\t"                                   \` |
| [294](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L294) | 预处理 | `#endif` |
| [295](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L295) | 预处理 | `#endif` |
| [301](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L301) | 预处理 | `#ifdef __ICCRX__` |
| [302](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L302) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                     \` |
| [307](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L307) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __set_interrupt_state(_SEGGER_RTT__LockState);                                   \` |
| [309](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L309) | 预处理 | `#endif` |
| [315](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L315) | 预处理 | `#ifdef __ICCRL78__` |
| [316](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L316) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                     \` |
| [321](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L321) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __set_interrupt_state(_SEGGER_RTT__LockState);                                   \` |
| [323](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L323) | 预处理 | `#endif` |
| [329](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L329) | 预处理 | `#ifdef __CC_ARM` |
| [330](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L330) | 预处理 | `#if (defined __TARGET_ARCH_6S_M)` |
| [331](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L331) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [338](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L338) | 预处理 | `#define SEGGER_RTT_UNLOCK()   _SEGGER_RTT__PRIMASK = _SEGGER_RTT__LockState;                                              \` |
| [341](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L341) | 预处理 | `#elif (defined(__TARGET_ARCH_7_M) &#124;&#124; defined(__TARGET_ARCH_7E_M))` |
| [342](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L342) | 预处理 | `#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY` |
| [343](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L343) | 预处理 | `#define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)` |
| [344](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L344) | 预处理 | `#endif` |
| [345](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L345) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [352](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L352) | 预处理 | `#define SEGGER_RTT_UNLOCK()   BASEPRI = _SEGGER_RTT__LockState;                                              \` |
| [355](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L355) | 预处理 | `#endif` |
| [356](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L356) | 预处理 | `#endif` |
| [362](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L362) | 预处理 | `#ifdef __TI_ARM__` |
| [363](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L363) | 预处理 | `#if defined (__TI_ARM_V6M0__)` |
| [364](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L364) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [369](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L369) | 预处理 | `#define SEGGER_RTT_UNLOCK()   __set_PRIMASK(_SEGGER_RTT__LockState);                                         \` |
| [371](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L371) | 预处理 | `#elif (defined (__TI_ARM_V7M3__) &#124;&#124; defined (__TI_ARM_V7M4__))` |
| [372](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L372) | 预处理 | `#ifndef   SEGGER_RTT_MAX_INTERRUPT_PRIORITY` |
| [373](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L373) | 预处理 | `#define SEGGER_RTT_MAX_INTERRUPT_PRIORITY   (0x20)` |
| [374](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L374) | 预处理 | `#endif` |
| [375](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L375) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                   \` |
| [379](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L379) | 预处理 | `#define SEGGER_RTT_UNLOCK()   _set_interrupt_priority(_SEGGER_RTT__LockState);                               \` |
| [381](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L381) | 预处理 | `#endif` |
| [382](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L382) | 预处理 | `#endif` |
| [388](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L388) | 预处理 | `#ifdef __RX` |
| [390](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L390) | 预处理 | `#define SEGGER_RTT_LOCK()   {                                                                     \` |
| [395](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L395) | 预处理 | `#define SEGGER_RTT_UNLOCK()   set_psw(get_psw() &#124; _SEGGER_RTT__LockState);                                     \` |
| [397](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L397) | 预处理 | `#endif` |
| [404](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L404) | 预处理 | `#if defined(WIN32) &#124;&#124; defined(SEGGER_RTT_LOCK_EMBOS)` |
| [409](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L409) | 预处理 | `#define SEGGER_RTT_LOCK()       {                                                                   \` |
| [412](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L412) | 预处理 | `#define SEGGER_RTT_UNLOCK()       OS_SIM_LeaveCriticalSection();                                    \` |
| [414](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L414) | 预处理 | `#endif` |
| [420](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L420) | 预处理 | `#ifndef   SEGGER_RTT_LOCK` |
| [421](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L421) | 预处理 | `#define SEGGER_RTT_LOCK()                // Lock RTT (nestable)   (i.e. disable interrupts)` |
| [422](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L422) | 预处理 | `#endif` |
| [424](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L424) | 预处理 | `#ifndef   SEGGER_RTT_UNLOCK` |
| [425](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L425) | 预处理 | `#define SEGGER_RTT_UNLOCK()              // Unlock RTT (nestable) (i.e. enable previous interrupt lock state)` |
| [426](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L426) | 预处理 | `#endif` |
| [428](../Middlewares/Third_Party/SEGGER/Config/SEGGER_RTT_Conf.h#L428) | 预处理 | `#endif` |

## modules/alarm/buzzer.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [16](../modules/alarm/buzzer.c#L16) | 配置初值 | `PWM_Init_Config_s buzzer_config = {` |
| [17](../modules/alarm/buzzer.c#L17) | 配置初值 | `.htim = &htim12,` |
| [18](../modules/alarm/buzzer.c#L18) | 配置初值 | `.channel = TIM_CHANNEL_1,` |
| [19](../modules/alarm/buzzer.c#L19) | 配置初值 | `.dutyratio = 0,` |
| [20](../modules/alarm/buzzer.c#L20) | 配置初值 | `.period = 0.001,` |

## modules/alarm/buzzer.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/alarm/buzzer.h#L1) | 预处理 | `#ifndef BUZZER_H` |
| [2](../modules/alarm/buzzer.h#L2) | 预处理 | `#define BUZZER_H` |
| [4](../modules/alarm/buzzer.h#L4) | 预处理 | `#define BUZZER_DEVICE_CNT 5` |
| [6](../modules/alarm/buzzer.h#L6) | 预处理 | `#define  DoFreq  523` |
| [7](../modules/alarm/buzzer.h#L7) | 预处理 | `#define  ReFreq  587` |
| [8](../modules/alarm/buzzer.h#L8) | 预处理 | `#define  MiFreq  659` |
| [9](../modules/alarm/buzzer.h#L9) | 预处理 | `#define  FaFreq  698` |
| [10](../modules/alarm/buzzer.h#L10) | 预处理 | `#define  SoFreq  784` |
| [11](../modules/alarm/buzzer.h#L11) | 预处理 | `#define  LaFreq  880` |
| [12](../modules/alarm/buzzer.h#L12) | 预处理 | `#define  SiFreq  988` |
| [60](../modules/alarm/buzzer.h#L60) | 预处理 | `#endif // !BUZZER_H` |

## modules/algorithm/chassis_gimbal_imu.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_gimbal_imu.h#L1) | 预处理 | `#ifndef CHASSIS_GIMBAL_IMU_H` |
| [2](../modules/algorithm/chassis_gimbal_imu.h#L2) | 预处理 | `#define CHASSIS_GIMBAL_IMU_H` |
| [30](../modules/algorithm/chassis_gimbal_imu.h#L30) | 预处理 | `#endif` |

## modules/algorithm/chassis_kinematics.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../modules/algorithm/chassis_kinematics.c#L5) | 预处理 | `#define OMNI_X_PROJECTION 0.70710678118654752440f` |

## modules/algorithm/chassis_kinematics.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_kinematics.h#L1) | 预处理 | `#ifndef CHASSIS_KINEMATICS_H` |
| [2](../modules/algorithm/chassis_kinematics.h#L2) | 预处理 | `#define CHASSIS_KINEMATICS_H` |
| [35](../modules/algorithm/chassis_kinematics.h#L35) | 预处理 | `#endif // CHASSIS_KINEMATICS_H` |

## modules/algorithm/chassis_power_budget.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_power_budget.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_BUDGET_H` |
| [2](../modules/algorithm/chassis_power_budget.h#L2) | 预处理 | `#define CHASSIS_POWER_BUDGET_H` |
| [7](../modules/algorithm/chassis_power_budget.h#L7) | 预处理 | `#define CHASSIS_BUFFER_HISTORY_LENGTH 96U` |
| [149](../modules/algorithm/chassis_power_budget.h#L149) | 预处理 | `#endif` |

## modules/algorithm/chassis_power_outer_loop.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_power_outer_loop.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_OUTER_LOOP_H` |
| [2](../modules/algorithm/chassis_power_outer_loop.h#L2) | 预处理 | `#define CHASSIS_POWER_OUTER_LOOP_H` |
| [81](../modules/algorithm/chassis_power_outer_loop.h#L81) | 预处理 | `#endif` |

## modules/algorithm/chassis_power_transient.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_power_transient.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_TRANSIENT_H` |
| [2](../modules/algorithm/chassis_power_transient.h#L2) | 预处理 | `#define CHASSIS_POWER_TRANSIENT_H` |
| [74](../modules/algorithm/chassis_power_transient.h#L74) | 预处理 | `#endif` |

## modules/algorithm/chassis_remote_fusion.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../modules/algorithm/chassis_remote_fusion.c#L5) | 预处理 | `#define HALF_RANGE UINT32_C(0x80000000)` |
| [6](../modules/algorithm/chassis_remote_fusion.c#L6) | 预处理 | `#define REMOTE_MAX_SOURCE_AGE_MS 20U` |
| [7](../modules/algorithm/chassis_remote_fusion.c#L7) | 预处理 | `#define REMOTE_MAX_KNOWN_AGE_MS 60U` |

## modules/algorithm/chassis_remote_fusion.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_remote_fusion.h#L1) | 预处理 | `#ifndef CHASSIS_REMOTE_FUSION_H` |
| [2](../modules/algorithm/chassis_remote_fusion.h#L2) | 预处理 | `#define CHASSIS_REMOTE_FUSION_H` |
| [9](../modules/algorithm/chassis_remote_fusion.h#L9) | 预处理 | `#define CHASSIS_REMOTE_WHEEL_HISTORY_CAPACITY 32U` |
| [89](../modules/algorithm/chassis_remote_fusion.h#L89) | 预处理 | `#endif` |

## modules/algorithm/chassis_spin.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [15](../modules/algorithm/chassis_spin.c#L15) | 预处理 | `#if CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_FIXED` |
| [23](../modules/algorithm/chassis_spin.c#L23) | 预处理 | `#else` |
| [25](../modules/algorithm/chassis_spin.c#L25) | 预处理 | `#endif` |
| [28](../modules/algorithm/chassis_spin.c#L28) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [248](../modules/algorithm/chassis_spin.c#L248) | 预处理 | `#endif` |
| [270](../modules/algorithm/chassis_spin.c#L270) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [280](../modules/algorithm/chassis_spin.c#L280) | 预处理 | `#else` |
| [282](../modules/algorithm/chassis_spin.c#L282) | 预处理 | `#endif` |
| [285](../modules/algorithm/chassis_spin.c#L285) | 预处理 | `#if CHASSIS_SPIN_MODE != CHASSIS_SPIN_MODE_SINE` |
| [291](../modules/algorithm/chassis_spin.c#L291) | 预处理 | `#else` |
| [368](../modules/algorithm/chassis_spin.c#L368) | 预处理 | `#endif` |
| [385](../modules/algorithm/chassis_spin.c#L385) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_FIXED` |
| [389](../modules/algorithm/chassis_spin.c#L389) | 预处理 | `#elif CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [391](../modules/algorithm/chassis_spin.c#L391) | 预处理 | `#else` |
| [480](../modules/algorithm/chassis_spin.c#L480) | 预处理 | `#endif` |
| [486](../modules/algorithm/chassis_spin.c#L486) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [489](../modules/algorithm/chassis_spin.c#L489) | 预处理 | `#endif` |
| [506](../modules/algorithm/chassis_spin.c#L506) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [514](../modules/algorithm/chassis_spin.c#L514) | 预处理 | `#endif` |
| [521](../modules/algorithm/chassis_spin.c#L521) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_SINE` |
| [524](../modules/algorithm/chassis_spin.c#L524) | 预处理 | `#else` |
| [527](../modules/algorithm/chassis_spin.c#L527) | 预处理 | `#endif` |
| [538](../modules/algorithm/chassis_spin.c#L538) | 预处理 | `#if CHASSIS_SPIN_MODE == CHASSIS_SPIN_MODE_ADAPTIVE` |
| [540](../modules/algorithm/chassis_spin.c#L540) | 预处理 | `#endif` |

## modules/algorithm/chassis_spin.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_spin.h#L1) | 预处理 | `#ifndef CHASSIS_SPIN_H` |
| [2](../modules/algorithm/chassis_spin.h#L2) | 预处理 | `#define CHASSIS_SPIN_H` |
| [102](../modules/algorithm/chassis_spin.h#L102) | 预处理 | `#endif` |

## modules/algorithm/chassis_velocity.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../modules/algorithm/chassis_velocity.c#L5) | 预处理 | `#define TWO_PI 6.2831853071795864769f` |
| [6](../modules/algorithm/chassis_velocity.c#L6) | 预处理 | `#define PROJECTION_45 0.7071067811865475244f` |

## modules/algorithm/chassis_velocity.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_velocity.h#L1) | 预处理 | `#ifndef CHASSIS_VELOCITY_H` |
| [2](../modules/algorithm/chassis_velocity.h#L2) | 预处理 | `#define CHASSIS_VELOCITY_H` |
| [38](../modules/algorithm/chassis_velocity.h#L38) | 预处理 | `#endif` |

## modules/algorithm/chassis_velocity_center.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../modules/algorithm/chassis_velocity_center.c#L5) | 预处理 | `#define CENTER_MIN_STEP_US 15000U` |
| [6](../modules/algorithm/chassis_velocity_center.c#L6) | 预处理 | `#define CENTER_MAX_STEP_US 30000U` |

## modules/algorithm/chassis_velocity_center.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_velocity_center.h#L1) | 预处理 | `#ifndef CHASSIS_VELOCITY_CENTER_H` |
| [2](../modules/algorithm/chassis_velocity_center.h#L2) | 预处理 | `#define CHASSIS_VELOCITY_CENTER_H` |
| [30](../modules/algorithm/chassis_velocity_center.h#L30) | 预处理 | `#endif` |

## modules/algorithm/chassis_velocity_fusion.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [5](../modules/algorithm/chassis_velocity_fusion.c#L5) | 预处理 | `#define HALF_CLOCK_RANGE (UINT32_C(1) << 31)` |

## modules/algorithm/chassis_velocity_fusion.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/chassis_velocity_fusion.h#L1) | 预处理 | `#ifndef CHASSIS_VELOCITY_FUSION_H` |
| [2](../modules/algorithm/chassis_velocity_fusion.h#L2) | 预处理 | `#define CHASSIS_VELOCITY_FUSION_H` |
| [52](../modules/algorithm/chassis_velocity_fusion.h#L52) | 预处理 | `#endif` |

## modules/algorithm/controller.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../modules/algorithm/controller.h#L13) | 预处理 | `#ifndef _CONTROLLER_H` |
| [14](../modules/algorithm/controller.h#L14) | 预处理 | `#define _CONTROLLER_H` |
| [22](../modules/algorithm/controller.h#L22) | 预处理 | `#ifndef abs` |
| [23](../modules/algorithm/controller.h#L23) | 预处理 | `#define abs(x) ((x > 0) ? x : -x)` |
| [24](../modules/algorithm/controller.h#L24) | 预处理 | `#endif` |
| [134](../modules/algorithm/controller.h#L134) | 预处理 | `#endif` |

## modules/algorithm/crc16.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/crc16.h#L1) | 预处理 | `#ifndef __CRC16_H` |
| [2](../modules/algorithm/crc16.h#L2) | 预处理 | `#define __CRC16_H` |
| [5](../modules/algorithm/crc16.h#L5) | 预处理 | `#define CRC_START_16 0xFFFF` |
| [6](../modules/algorithm/crc16.h#L6) | 预处理 | `#define CRC_START_MODBUS 0xFFFF` |
| [7](../modules/algorithm/crc16.h#L7) | 预处理 | `#define CRC_POLY_16 0xA001` |
| [14](../modules/algorithm/crc16.h#L14) | 预处理 | `#endif` |

## modules/algorithm/crc8.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/crc8.h#L1) | 预处理 | `#ifndef __CRC8_H` |
| [2](../modules/algorithm/crc8.h#L2) | 预处理 | `#define __CRC8_H` |
| [6](../modules/algorithm/crc8.h#L6) | 预处理 | `#define CRC_START_8 0x00` |
| [11](../modules/algorithm/crc8.h#L11) | 预处理 | `#endif` |

## modules/algorithm/kalman_filter.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../modules/algorithm/kalman_filter.h#L13) | 预处理 | `#ifndef __KALMAN_FILTER_H` |
| [14](../modules/algorithm/kalman_filter.h#L14) | 预处理 | `#define __KALMAN_FILTER_H` |
| [18](../modules/algorithm/kalman_filter.h#L18) | 预处理 | `#define __CC_ARM    // Keil` |
| [19](../modules/algorithm/kalman_filter.h#L19) | 预处理 | `#define ARM_MATH_CM4` |
| [20](../modules/algorithm/kalman_filter.h#L20) | 预处理 | `#define ARM_MATH_MATRIX_CHECK` |
| [21](../modules/algorithm/kalman_filter.h#L21) | 预处理 | `#define ARM_MATH_ROUNDING` |
| [22](../modules/algorithm/kalman_filter.h#L22) | 预处理 | `#define ARM_MATH_DSP    // define in arm_math.h` |
| [31](../modules/algorithm/kalman_filter.h#L31) | 预处理 | `#ifndef user_malloc` |
| [32](../modules/algorithm/kalman_filter.h#L32) | 预处理 | `#define user_malloc malloc` |
| [33](../modules/algorithm/kalman_filter.h#L33) | 预处理 | `#endif` |
| [36](../modules/algorithm/kalman_filter.h#L36) | 预处理 | `#define mat arm_matrix_instance_f32` |
| [37](../modules/algorithm/kalman_filter.h#L37) | 预处理 | `#define Matrix_Init arm_mat_init_f32` |
| [38](../modules/algorithm/kalman_filter.h#L38) | 预处理 | `#define Matrix_Add arm_mat_add_f32` |
| [39](../modules/algorithm/kalman_filter.h#L39) | 预处理 | `#define Matrix_Subtract arm_mat_sub_f32` |
| [40](../modules/algorithm/kalman_filter.h#L40) | 预处理 | `#define Matrix_Multiply arm_mat_mult_f32` |
| [41](../modules/algorithm/kalman_filter.h#L41) | 预处理 | `#define Matrix_Transpose arm_mat_trans_f32` |
| [42](../modules/algorithm/kalman_filter.h#L42) | 预处理 | `#define Matrix_Inverse arm_mat_inverse_f32` |
| [117](../modules/algorithm/kalman_filter.h#L117) | 预处理 | `#endif //__KALMAN_FILTER_H` |

## modules/algorithm/power_model.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/algorithm/power_model.h#L1) | 预处理 | `#ifndef POWER_MODEL_H` |
| [2](../modules/algorithm/power_model.h#L2) | 预处理 | `#define POWER_MODEL_H` |
| [7](../modules/algorithm/power_model.h#L7) | 预处理 | `#define POWER_MODEL_MOTOR_COUNT 4U` |
| [98](../modules/algorithm/power_model.h#L98) | 预处理 | `#endif // POWER_MODEL_H` |

## modules/algorithm/QuaternionEKF.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../modules/algorithm/QuaternionEKF.h#L13) | 预处理 | `#ifndef _QUAT_EKF_H` |
| [14](../modules/algorithm/QuaternionEKF.h#L14) | 预处理 | `#define _QUAT_EKF_H` |
| [18](../modules/algorithm/QuaternionEKF.h#L18) | 预处理 | `#ifndef TRUE` |
| [19](../modules/algorithm/QuaternionEKF.h#L19) | 预处理 | `#define TRUE 1 /**< boolean true  */` |
| [20](../modules/algorithm/QuaternionEKF.h#L20) | 预处理 | `#endif` |
| [22](../modules/algorithm/QuaternionEKF.h#L22) | 预处理 | `#ifndef FALSE` |
| [23](../modules/algorithm/QuaternionEKF.h#L23) | 预处理 | `#define FALSE 0 /**< boolean fails */` |
| [24](../modules/algorithm/QuaternionEKF.h#L24) | 预处理 | `#endif` |
| [75](../modules/algorithm/QuaternionEKF.h#L75) | 预处理 | `#endif` |

## modules/algorithm/user_lib.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../modules/algorithm/user_lib.h#L13) | 预处理 | `#ifndef _USER_LIB_H` |
| [14](../modules/algorithm/user_lib.h#L14) | 预处理 | `#define _USER_LIB_H` |
| [21](../modules/algorithm/user_lib.h#L21) | 预处理 | `#ifndef user_malloc` |
| [22](../modules/algorithm/user_lib.h#L22) | 预处理 | `#define user_malloc malloc` |
| [23](../modules/algorithm/user_lib.h#L23) | 预处理 | `#endif` |
| [25](../modules/algorithm/user_lib.h#L25) | 预处理 | `#define msin(x) (arm_sin_f32(x))` |
| [26](../modules/algorithm/user_lib.h#L26) | 预处理 | `#define mcos(x) (arm_cos_f32(x))` |
| [30](../modules/algorithm/user_lib.h#L30) | 预处理 | `#define MatAdd arm_mat_add_f32` |
| [31](../modules/algorithm/user_lib.h#L31) | 预处理 | `#define MatSubtract arm_mat_sub_f32` |
| [32](../modules/algorithm/user_lib.h#L32) | 预处理 | `#define MatMultiply arm_mat_mult_f32` |
| [33](../modules/algorithm/user_lib.h#L33) | 预处理 | `#define MatTranspose arm_mat_trans_f32` |
| [34](../modules/algorithm/user_lib.h#L34) | 预处理 | `#define MatInverse arm_mat_inverse_f32` |
| [38](../modules/algorithm/user_lib.h#L38) | 预处理 | `#ifndef TRUE` |
| [39](../modules/algorithm/user_lib.h#L39) | 预处理 | `#define TRUE 1 /**< boolean true  */` |
| [40](../modules/algorithm/user_lib.h#L40) | 预处理 | `#endif` |
| [42](../modules/algorithm/user_lib.h#L42) | 预处理 | `#ifndef FALSE` |
| [43](../modules/algorithm/user_lib.h#L43) | 预处理 | `#define FALSE 0 /**< boolean fails */` |
| [44](../modules/algorithm/user_lib.h#L44) | 预处理 | `#endif` |
| [47](../modules/algorithm/user_lib.h#L47) | 预处理 | `#ifndef PI` |
| [48](../modules/algorithm/user_lib.h#L48) | 预处理 | `#define PI 3.14159265354f` |
| [49](../modules/algorithm/user_lib.h#L49) | 预处理 | `#endif` |
| [51](../modules/algorithm/user_lib.h#L51) | 预处理 | `#define VAL_LIMIT(val, min, max) \` |
| [64](../modules/algorithm/user_lib.h#L64) | 预处理 | `#define ANGLE_LIMIT_360(val, angle)     \` |
| [71](../modules/algorithm/user_lib.h#L71) | 预处理 | `#define ANGLE_LIMIT_360_TO_180(val) \` |
| [78](../modules/algorithm/user_lib.h#L78) | 预处理 | `#define VAL_MIN(a, b) ((a) < (b) ? (a) : (b))` |
| [79](../modules/algorithm/user_lib.h#L79) | 预处理 | `#define VAL_MAX(a, b) ((a) > (b) ? (a) : (b))` |
| [118](../modules/algorithm/user_lib.h#L118) | 预处理 | `#define rad_format(Ang) loop_float_constrain((Ang), -PI, PI)` |
| [120](../modules/algorithm/user_lib.h#L120) | 预处理 | `#endif` |

## modules/bluetooth/HC05.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [4](../modules/bluetooth/HC05.c#L4) | 预处理 | `#define HC05_BUFFERSIZE  HC05_DATASIZE+2  // HC05发送和接收数据buffer大小，不得大于256` |
| [6](../modules/bluetooth/HC05.c#L6) | 预处理 | `#define FRAME_HEAD  0XAA    // 帧头` |
| [7](../modules/bluetooth/HC05.c#L7) | 预处理 | `#define FRAME_END   0X55    // 帧尾` |

## modules/bluetooth/HC05.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/bluetooth/HC05.h#L1) | 预处理 | `#ifndef HC05_H` |
| [2](../modules/bluetooth/HC05.h#L2) | 预处理 | `#define HC05_H` |
| [8](../modules/bluetooth/HC05.h#L8) | 预处理 | `#define  HC05_DATASIZE  4   // HC05接收和发送数据大小，根据需要修改` |
| [23](../modules/bluetooth/HC05.h#L23) | 预处理 | `#endif` |

## modules/BMI088/bmi088.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [83](../modules/BMI088/bmi088.c#L83) | 预处理 | `#define BMI088REG 0` |
| [84](../modules/BMI088/bmi088.c#L84) | 预处理 | `#define BMI088DATA 1` |
| [85](../modules/BMI088/bmi088.c#L85) | 预处理 | `#define BMI088ERROR 2` |
| [300](../modules/BMI088/bmi088.c#L300) | 预处理 | `#define BMI088_PRE_CALI_ACC_X_OFFSET 0.0f` |
| [301](../modules/BMI088/bmi088.c#L301) | 预处理 | `#define BMI088_PRE_CALI_ACC_Y_OFFSET 0.0f` |
| [302](../modules/BMI088/bmi088.c#L302) | 预处理 | `#define BMI088_PRE_CALI_ACC_Z_OFFSET 0.0f` |
| [303](../modules/BMI088/bmi088.c#L303) | 预处理 | `#define BMI088_PRE_CALI_G_NORM 9.805f` |

## modules/BMI088/bmi088.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/BMI088/bmi088.h#L1) | 预处理 | `#ifndef __BMI088_H__ // 防止重复包含` |
| [2](../modules/BMI088/bmi088.h#L2) | 预处理 | `#define __BMI088_H__` |
| [116](../modules/BMI088/bmi088.h#L116) | 预处理 | `#endif // !__BMI088_H__` |

## modules/BMI088/bmi088_regNdef.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/BMI088/bmi088_regNdef.h#L1) | 预处理 | `#ifndef BMI088REG_H` |
| [2](../modules/BMI088/bmi088_regNdef.h#L2) | 预处理 | `#define BMI088REG_H` |
| [6](../modules/BMI088/bmi088_regNdef.h#L6) | 预处理 | `#define BMI088_ACC_CHIP_ID 0x00 // the register is  " Who am I "` |
| [7](../modules/BMI088/bmi088_regNdef.h#L7) | 预处理 | `#define BMI088_ACC_CHIP_ID_VALUE 0x1E` |
| [9](../modules/BMI088/bmi088_regNdef.h#L9) | 预处理 | `#define BMI088_ACC_ERR_REG 0x02` |
| [10](../modules/BMI088/bmi088_regNdef.h#L10) | 预处理 | `#define BMI088_ACCEL_CONGIF_ERROR_SHFITS 0x2` |
| [11](../modules/BMI088/bmi088_regNdef.h#L11) | 预处理 | `#define BMI088_ACCEL_CONGIF_ERROR (1 << BMI088_ACCEL_CONGIF_ERROR_SHFITS)` |
| [12](../modules/BMI088/bmi088_regNdef.h#L12) | 预处理 | `#define BMI088_FATAL_ERROR_SHFITS 0x0` |
| [13](../modules/BMI088/bmi088_regNdef.h#L13) | 预处理 | `#define BMI088_FATAL_ERROR (1 << BMI088_FATAL_ERROR)` |
| [15](../modules/BMI088/bmi088_regNdef.h#L15) | 预处理 | `#define BMI088_ACC_STATUS 0x03` |
| [16](../modules/BMI088/bmi088_regNdef.h#L16) | 预处理 | `#define BMI088_ACCEL_DRDY_SHFITS 0x7` |
| [17](../modules/BMI088/bmi088_regNdef.h#L17) | 预处理 | `#define BMI088_ACCEL_DRDY (1 << BMI088_ACCEL_DRDY_SHFITS)` |
| [19](../modules/BMI088/bmi088_regNdef.h#L19) | 预处理 | `#define BMI088_ACCEL_XOUT_L 0x12` |
| [20](../modules/BMI088/bmi088_regNdef.h#L20) | 预处理 | `#define BMI088_ACCEL_XOUT_M 0x13` |
| [21](../modules/BMI088/bmi088_regNdef.h#L21) | 预处理 | `#define BMI088_ACCEL_YOUT_L 0x14` |
| [22](../modules/BMI088/bmi088_regNdef.h#L22) | 预处理 | `#define BMI088_ACCEL_YOUT_M 0x15` |
| [23](../modules/BMI088/bmi088_regNdef.h#L23) | 预处理 | `#define BMI088_ACCEL_ZOUT_L 0x16` |
| [24](../modules/BMI088/bmi088_regNdef.h#L24) | 预处理 | `#define BMI088_ACCEL_ZOUT_M 0x17` |
| [26](../modules/BMI088/bmi088_regNdef.h#L26) | 预处理 | `#define BMI088_SENSORTIME_DATA_L 0x18` |
| [27](../modules/BMI088/bmi088_regNdef.h#L27) | 预处理 | `#define BMI088_SENSORTIME_DATA_M 0x19` |
| [28](../modules/BMI088/bmi088_regNdef.h#L28) | 预处理 | `#define BMI088_SENSORTIME_DATA_H 0x1A` |
| [30](../modules/BMI088/bmi088_regNdef.h#L30) | 预处理 | `#define BMI088_ACC_INT_STAT_1 0x1D` |
| [31](../modules/BMI088/bmi088_regNdef.h#L31) | 预处理 | `#define BMI088_ACCEL_DRDY_INTERRUPT_SHFITS 0x7` |
| [32](../modules/BMI088/bmi088_regNdef.h#L32) | 预处理 | `#define BMI088_ACCEL_DRDY_INTERRUPT (1 << BMI088_ACCEL_DRDY_INTERRUPT_SHFITS)` |
| [34](../modules/BMI088/bmi088_regNdef.h#L34) | 预处理 | `#define BMI088_TEMP_M 0x22` |
| [36](../modules/BMI088/bmi088_regNdef.h#L36) | 预处理 | `#define BMI088_TEMP_L 0x23` |
| [38](../modules/BMI088/bmi088_regNdef.h#L38) | 预处理 | `#define BMI088_ACC_CONF 0x40` |
| [39](../modules/BMI088/bmi088_regNdef.h#L39) | 预处理 | `#define BMI088_ACC_CONF_MUST_Set 0x80` |
| [40](../modules/BMI088/bmi088_regNdef.h#L40) | 预处理 | `#define BMI088_ACC_BWP_SHFITS 0x4` |
| [41](../modules/BMI088/bmi088_regNdef.h#L41) | 预处理 | `#define BMI088_ACC_OSR4 (0x0 << BMI088_ACC_BWP_SHFITS)` |
| [42](../modules/BMI088/bmi088_regNdef.h#L42) | 预处理 | `#define BMI088_ACC_OSR2 (0x1 << BMI088_ACC_BWP_SHFITS)` |
| [43](../modules/BMI088/bmi088_regNdef.h#L43) | 预处理 | `#define BMI088_ACC_NORMAL (0x2 << BMI088_ACC_BWP_SHFITS)` |
| [45](../modules/BMI088/bmi088_regNdef.h#L45) | 预处理 | `#define BMI088_ACC_ODR_SHFITS 0x0` |
| [46](../modules/BMI088/bmi088_regNdef.h#L46) | 预处理 | `#define BMI088_ACC_12_5_HZ (0x5 << BMI088_ACC_ODR_SHFITS)` |
| [47](../modules/BMI088/bmi088_regNdef.h#L47) | 预处理 | `#define BMI088_ACC_25_HZ (0x6 << BMI088_ACC_ODR_SHFITS)` |
| [48](../modules/BMI088/bmi088_regNdef.h#L48) | 预处理 | `#define BMI088_ACC_50_HZ (0x7 << BMI088_ACC_ODR_SHFITS)` |
| [49](../modules/BMI088/bmi088_regNdef.h#L49) | 预处理 | `#define BMI088_ACC_100_HZ (0x8 << BMI088_ACC_ODR_SHFITS)` |
| [50](../modules/BMI088/bmi088_regNdef.h#L50) | 预处理 | `#define BMI088_ACC_200_HZ (0x9 << BMI088_ACC_ODR_SHFITS)` |
| [51](../modules/BMI088/bmi088_regNdef.h#L51) | 预处理 | `#define BMI088_ACC_400_HZ (0xA << BMI088_ACC_ODR_SHFITS)` |
| [52](../modules/BMI088/bmi088_regNdef.h#L52) | 预处理 | `#define BMI088_ACC_800_HZ (0xB << BMI088_ACC_ODR_SHFITS)` |
| [53](../modules/BMI088/bmi088_regNdef.h#L53) | 预处理 | `#define BMI088_ACC_1600_HZ (0xC << BMI088_ACC_ODR_SHFITS)` |
| [55](../modules/BMI088/bmi088_regNdef.h#L55) | 预处理 | `#define BMI088_ACC_RANGE 0x41` |
| [57](../modules/BMI088/bmi088_regNdef.h#L57) | 预处理 | `#define BMI088_ACC_RANGE_SHFITS 0x0` |
| [58](../modules/BMI088/bmi088_regNdef.h#L58) | 预处理 | `#define BMI088_ACC_RANGE_3G (0x0 << BMI088_ACC_RANGE_SHFITS)` |
| [59](../modules/BMI088/bmi088_regNdef.h#L59) | 预处理 | `#define BMI088_ACC_RANGE_6G (0x1 << BMI088_ACC_RANGE_SHFITS)` |
| [60](../modules/BMI088/bmi088_regNdef.h#L60) | 预处理 | `#define BMI088_ACC_RANGE_12G (0x2 << BMI088_ACC_RANGE_SHFITS)` |
| [61](../modules/BMI088/bmi088_regNdef.h#L61) | 预处理 | `#define BMI088_ACC_RANGE_24G (0x3 << BMI088_ACC_RANGE_SHFITS)` |
| [63](../modules/BMI088/bmi088_regNdef.h#L63) | 预处理 | `#define BMI088_INT1_IO_CTRL 0x53` |
| [64](../modules/BMI088/bmi088_regNdef.h#L64) | 预处理 | `#define BMI088_ACC_INT1_IO_ENABLE_SHFITS 0x3` |
| [65](../modules/BMI088/bmi088_regNdef.h#L65) | 预处理 | `#define BMI088_ACC_INT1_IO_ENABLE (0x1 << BMI088_ACC_INT1_IO_ENABLE_SHFITS)` |
| [66](../modules/BMI088/bmi088_regNdef.h#L66) | 预处理 | `#define BMI088_ACC_INT1_GPIO_MODE_SHFITS 0x2` |
| [67](../modules/BMI088/bmi088_regNdef.h#L67) | 预处理 | `#define BMI088_ACC_INT1_GPIO_PP (0x0 << BMI088_ACC_INT1_GPIO_MODE_SHFITS)` |
| [68](../modules/BMI088/bmi088_regNdef.h#L68) | 预处理 | `#define BMI088_ACC_INT1_GPIO_OD (0x1 << BMI088_ACC_INT1_GPIO_MODE_SHFITS)` |
| [69](../modules/BMI088/bmi088_regNdef.h#L69) | 预处理 | `#define BMI088_ACC_INT1_GPIO_LVL_SHFITS 0x1` |
| [70](../modules/BMI088/bmi088_regNdef.h#L70) | 预处理 | `#define BMI088_ACC_INT1_GPIO_LOW (0x0 << BMI088_ACC_INT1_GPIO_LVL_SHFITS)` |
| [71](../modules/BMI088/bmi088_regNdef.h#L71) | 预处理 | `#define BMI088_ACC_INT1_GPIO_HIGH (0x1 << BMI088_ACC_INT1_GPIO_LVL_SHFITS)` |
| [73](../modules/BMI088/bmi088_regNdef.h#L73) | 预处理 | `#define BMI088_INT2_IO_CTRL 0x54` |
| [74](../modules/BMI088/bmi088_regNdef.h#L74) | 预处理 | `#define BMI088_ACC_INT2_IO_ENABLE_SHFITS 0x3` |
| [75](../modules/BMI088/bmi088_regNdef.h#L75) | 预处理 | `#define BMI088_ACC_INT2_IO_ENABLE (0x1 << BMI088_ACC_INT2_IO_ENABLE_SHFITS)` |
| [76](../modules/BMI088/bmi088_regNdef.h#L76) | 预处理 | `#define BMI088_ACC_INT2_GPIO_MODE_SHFITS 0x2` |
| [77](../modules/BMI088/bmi088_regNdef.h#L77) | 预处理 | `#define BMI088_ACC_INT2_GPIO_PP (0x0 << BMI088_ACC_INT2_GPIO_MODE_SHFITS)` |
| [78](../modules/BMI088/bmi088_regNdef.h#L78) | 预处理 | `#define BMI088_ACC_INT2_GPIO_OD (0x1 << BMI088_ACC_INT2_GPIO_MODE_SHFITS)` |
| [79](../modules/BMI088/bmi088_regNdef.h#L79) | 预处理 | `#define BMI088_ACC_INT2_GPIO_LVL_SHFITS 0x1` |
| [80](../modules/BMI088/bmi088_regNdef.h#L80) | 预处理 | `#define BMI088_ACC_INT2_GPIO_LOW (0x0 << BMI088_ACC_INT2_GPIO_LVL_SHFITS)` |
| [81](../modules/BMI088/bmi088_regNdef.h#L81) | 预处理 | `#define BMI088_ACC_INT2_GPIO_HIGH (0x1 << BMI088_ACC_INT2_GPIO_LVL_SHFITS)` |
| [83](../modules/BMI088/bmi088_regNdef.h#L83) | 预处理 | `#define BMI088_INT_MAP_DATA 0x58` |
| [84](../modules/BMI088/bmi088_regNdef.h#L84) | 预处理 | `#define BMI088_ACC_INT2_DRDY_INTERRUPT_SHFITS 0x6` |
| [85](../modules/BMI088/bmi088_regNdef.h#L85) | 预处理 | `#define BMI088_ACC_INT2_DRDY_INTERRUPT (0x1 << BMI088_ACC_INT2_DRDY_INTERRUPT_SHFITS)` |
| [86](../modules/BMI088/bmi088_regNdef.h#L86) | 预处理 | `#define BMI088_ACC_INT1_DRDY_INTERRUPT_SHFITS 0x2` |
| [87](../modules/BMI088/bmi088_regNdef.h#L87) | 预处理 | `#define BMI088_ACC_INT1_DRDY_INTERRUPT (0x1 << BMI088_ACC_INT1_DRDY_INTERRUPT_SHFITS)` |
| [89](../modules/BMI088/bmi088_regNdef.h#L89) | 预处理 | `#define BMI088_ACC_SELF_TEST 0x6D` |
| [90](../modules/BMI088/bmi088_regNdef.h#L90) | 预处理 | `#define BMI088_ACC_SELF_TEST_OFF 0x00` |
| [91](../modules/BMI088/bmi088_regNdef.h#L91) | 预处理 | `#define BMI088_ACC_SELF_TEST_POSITIVE_SIGNAL 0x0D` |
| [92](../modules/BMI088/bmi088_regNdef.h#L92) | 预处理 | `#define BMI088_ACC_SELF_TEST_NEGATIVE_SIGNAL 0x09` |
| [94](../modules/BMI088/bmi088_regNdef.h#L94) | 预处理 | `#define BMI088_ACC_PWR_CONF 0x7C` |
| [95](../modules/BMI088/bmi088_regNdef.h#L95) | 预处理 | `#define BMI088_ACC_PWR_SUSPEND_MODE 0x03` |
| [96](../modules/BMI088/bmi088_regNdef.h#L96) | 预处理 | `#define BMI088_ACC_PWR_ACTIVE_MODE 0x00` |
| [98](../modules/BMI088/bmi088_regNdef.h#L98) | 预处理 | `#define BMI088_ACC_PWR_CTRL 0x7D` |
| [99](../modules/BMI088/bmi088_regNdef.h#L99) | 预处理 | `#define BMI088_ACC_ENABLE_ACC_OFF 0x00` |
| [100](../modules/BMI088/bmi088_regNdef.h#L100) | 预处理 | `#define BMI088_ACC_ENABLE_ACC_ON 0x04` |
| [102](../modules/BMI088/bmi088_regNdef.h#L102) | 预处理 | `#define BMI088_ACC_SOFTRESET 0x7E` |
| [103](../modules/BMI088/bmi088_regNdef.h#L103) | 预处理 | `#define BMI088_ACC_SOFTRESET_VALUE 0xB6` |
| [105](../modules/BMI088/bmi088_regNdef.h#L105) | 预处理 | `#define BMI088_GYRO_CHIP_ID 0x00` |
| [106](../modules/BMI088/bmi088_regNdef.h#L106) | 预处理 | `#define BMI088_GYRO_CHIP_ID_VALUE 0x0F` |
| [108](../modules/BMI088/bmi088_regNdef.h#L108) | 预处理 | `#define BMI088_GYRO_X_L 0x02` |
| [109](../modules/BMI088/bmi088_regNdef.h#L109) | 预处理 | `#define BMI088_GYRO_X_H 0x03` |
| [110](../modules/BMI088/bmi088_regNdef.h#L110) | 预处理 | `#define BMI088_GYRO_Y_L 0x04` |
| [111](../modules/BMI088/bmi088_regNdef.h#L111) | 预处理 | `#define BMI088_GYRO_Y_H 0x05` |
| [112](../modules/BMI088/bmi088_regNdef.h#L112) | 预处理 | `#define BMI088_GYRO_Z_L 0x06` |
| [113](../modules/BMI088/bmi088_regNdef.h#L113) | 预处理 | `#define BMI088_GYRO_Z_H 0x07` |
| [115](../modules/BMI088/bmi088_regNdef.h#L115) | 预处理 | `#define BMI088_GYRO_INT_STAT_1 0x0A` |
| [116](../modules/BMI088/bmi088_regNdef.h#L116) | 预处理 | `#define BMI088_GYRO_DYDR_SHFITS 0x7` |
| [117](../modules/BMI088/bmi088_regNdef.h#L117) | 预处理 | `#define BMI088_GYRO_DYDR (0x1 << BMI088_GYRO_DYDR_SHFITS)` |
| [119](../modules/BMI088/bmi088_regNdef.h#L119) | 预处理 | `#define BMI088_GYRO_RANGE 0x0F` |
| [120](../modules/BMI088/bmi088_regNdef.h#L120) | 预处理 | `#define BMI088_GYRO_RANGE_SHFITS 0x0` |
| [121](../modules/BMI088/bmi088_regNdef.h#L121) | 预处理 | `#define BMI088_GYRO_2000 (0x0 << BMI088_GYRO_RANGE_SHFITS)` |
| [122](../modules/BMI088/bmi088_regNdef.h#L122) | 预处理 | `#define BMI088_GYRO_1000 (0x1 << BMI088_GYRO_RANGE_SHFITS)` |
| [123](../modules/BMI088/bmi088_regNdef.h#L123) | 预处理 | `#define BMI088_GYRO_500 (0x2 << BMI088_GYRO_RANGE_SHFITS)` |
| [124](../modules/BMI088/bmi088_regNdef.h#L124) | 预处理 | `#define BMI088_GYRO_250 (0x3 << BMI088_GYRO_RANGE_SHFITS)` |
| [125](../modules/BMI088/bmi088_regNdef.h#L125) | 预处理 | `#define BMI088_GYRO_125 (0x4 << BMI088_GYRO_RANGE_SHFITS)` |
| [127](../modules/BMI088/bmi088_regNdef.h#L127) | 预处理 | `#define BMI088_GYRO_BANDWIDTH 0x10` |
| [129](../modules/BMI088/bmi088_regNdef.h#L129) | 预处理 | `#define BMI088_GYRO_BANDWIDTH_MUST_Set 0x80` |
| [130](../modules/BMI088/bmi088_regNdef.h#L130) | 预处理 | `#define BMI088_GYRO_2000_532_HZ 0x00` |
| [131](../modules/BMI088/bmi088_regNdef.h#L131) | 预处理 | `#define BMI088_GYRO_2000_230_HZ 0x01` |
| [132](../modules/BMI088/bmi088_regNdef.h#L132) | 预处理 | `#define BMI088_GYRO_1000_116_HZ 0x02` |
| [133](../modules/BMI088/bmi088_regNdef.h#L133) | 预处理 | `#define BMI088_GYRO_400_47_HZ 0x03` |
| [134](../modules/BMI088/bmi088_regNdef.h#L134) | 预处理 | `#define BMI088_GYRO_200_23_HZ 0x04` |
| [135](../modules/BMI088/bmi088_regNdef.h#L135) | 预处理 | `#define BMI088_GYRO_100_12_HZ 0x05` |
| [136](../modules/BMI088/bmi088_regNdef.h#L136) | 预处理 | `#define BMI088_GYRO_200_64_HZ 0x06` |
| [137](../modules/BMI088/bmi088_regNdef.h#L137) | 预处理 | `#define BMI088_GYRO_100_32_HZ 0x07` |
| [139](../modules/BMI088/bmi088_regNdef.h#L139) | 预处理 | `#define BMI088_GYRO_LPM1 0x11` |
| [140](../modules/BMI088/bmi088_regNdef.h#L140) | 预处理 | `#define BMI088_GYRO_NORMAL_MODE 0x00` |
| [141](../modules/BMI088/bmi088_regNdef.h#L141) | 预处理 | `#define BMI088_GYRO_SUSPEND_MODE 0x80` |
| [142](../modules/BMI088/bmi088_regNdef.h#L142) | 预处理 | `#define BMI088_GYRO_DEEP_SUSPEND_MODE 0x20` |
| [144](../modules/BMI088/bmi088_regNdef.h#L144) | 预处理 | `#define BMI088_GYRO_SOFTRESET 0x14` |
| [145](../modules/BMI088/bmi088_regNdef.h#L145) | 预处理 | `#define BMI088_GYRO_SOFTRESET_VALUE 0xB6` |
| [147](../modules/BMI088/bmi088_regNdef.h#L147) | 预处理 | `#define BMI088_GYRO_CTRL 0x15` |
| [148](../modules/BMI088/bmi088_regNdef.h#L148) | 预处理 | `#define BMI088_DRDY_OFF 0x00` |
| [149](../modules/BMI088/bmi088_regNdef.h#L149) | 预处理 | `#define BMI088_DRDY_ON 0x80` |
| [151](../modules/BMI088/bmi088_regNdef.h#L151) | 预处理 | `#define BMI088_GYRO_INT3_INT4_IO_CONF 0x16` |
| [152](../modules/BMI088/bmi088_regNdef.h#L152) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_MODE_SHFITS 0x3` |
| [153](../modules/BMI088/bmi088_regNdef.h#L153) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_PP (0x0 << BMI088_GYRO_INT4_GPIO_MODE_SHFITS)` |
| [154](../modules/BMI088/bmi088_regNdef.h#L154) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_OD (0x1 << BMI088_GYRO_INT4_GPIO_MODE_SHFITS)` |
| [155](../modules/BMI088/bmi088_regNdef.h#L155) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_LVL_SHFITS 0x2` |
| [156](../modules/BMI088/bmi088_regNdef.h#L156) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_LOW (0x0 << BMI088_GYRO_INT4_GPIO_LVL_SHFITS)` |
| [157](../modules/BMI088/bmi088_regNdef.h#L157) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_HIGH (0x1 << BMI088_GYRO_INT4_GPIO_LVL_SHFITS)` |
| [158](../modules/BMI088/bmi088_regNdef.h#L158) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_MODE_SHFITS 0x1` |
| [159](../modules/BMI088/bmi088_regNdef.h#L159) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_PP (0x0 << BMI088_GYRO_INT3_GPIO_MODE_SHFITS)` |
| [160](../modules/BMI088/bmi088_regNdef.h#L160) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_OD (0x1 << BMI088_GYRO_INT3_GPIO_MODE_SHFITS)` |
| [161](../modules/BMI088/bmi088_regNdef.h#L161) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_LVL_SHFITS 0x0` |
| [162](../modules/BMI088/bmi088_regNdef.h#L162) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_LOW (0x0 << BMI088_GYRO_INT3_GPIO_LVL_SHFITS)` |
| [163](../modules/BMI088/bmi088_regNdef.h#L163) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_HIGH (0x1 << BMI088_GYRO_INT3_GPIO_LVL_SHFITS)` |
| [165](../modules/BMI088/bmi088_regNdef.h#L165) | 预处理 | `#define BMI088_GYRO_INT3_INT4_IO_MAP 0x18` |
| [167](../modules/BMI088/bmi088_regNdef.h#L167) | 预处理 | `#define BMI088_GYRO_DRDY_IO_OFF 0x00` |
| [168](../modules/BMI088/bmi088_regNdef.h#L168) | 预处理 | `#define BMI088_GYRO_DRDY_IO_INT3 0x01` |
| [169](../modules/BMI088/bmi088_regNdef.h#L169) | 预处理 | `#define BMI088_GYRO_DRDY_IO_INT4 0x80` |
| [170](../modules/BMI088/bmi088_regNdef.h#L170) | 预处理 | `#define BMI088_GYRO_DRDY_IO_BOTH (BMI088_GYRO_DRDY_IO_INT3 &#124; BMI088_GYRO_DRDY_IO_INT4)` |
| [172](../modules/BMI088/bmi088_regNdef.h#L172) | 预处理 | `#define BMI088_GYRO_SELF_TEST 0x3C` |
| [173](../modules/BMI088/bmi088_regNdef.h#L173) | 预处理 | `#define BMI088_GYRO_RATE_OK_SHFITS 0x4` |
| [174](../modules/BMI088/bmi088_regNdef.h#L174) | 预处理 | `#define BMI088_GYRO_RATE_OK (0x1 << BMI088_GYRO_RATE_OK_SHFITS)` |
| [175](../modules/BMI088/bmi088_regNdef.h#L175) | 预处理 | `#define BMI088_GYRO_BIST_FAIL_SHFITS 0x2` |
| [176](../modules/BMI088/bmi088_regNdef.h#L176) | 预处理 | `#define BMI088_GYRO_BIST_FAIL (0x1 << BMI088_GYRO_BIST_FAIL_SHFITS)` |
| [177](../modules/BMI088/bmi088_regNdef.h#L177) | 预处理 | `#define BMI088_GYRO_BIST_RDY_SHFITS 0x1` |
| [178](../modules/BMI088/bmi088_regNdef.h#L178) | 预处理 | `#define BMI088_GYRO_BIST_RDY (0x1 << BMI088_GYRO_BIST_RDY_SHFITS)` |
| [179](../modules/BMI088/bmi088_regNdef.h#L179) | 预处理 | `#define BMI088_GYRO_TRIG_BIST_SHFITS 0x0` |
| [180](../modules/BMI088/bmi088_regNdef.h#L180) | 预处理 | `#define BMI088_GYRO_TRIG_BIST (0x1 << BMI088_GYRO_TRIG_BIST_SHFITS)` |
| [184](../modules/BMI088/bmi088_regNdef.h#L184) | 预处理 | `#define BMI088_TEMP_FACTOR 0.125f` |
| [185](../modules/BMI088/bmi088_regNdef.h#L185) | 预处理 | `#define BMI088_TEMP_OFFSET 23.0f` |
| [187](../modules/BMI088/bmi088_regNdef.h#L187) | 预处理 | `#define BMI088_WRITE_ACCEL_REG_NUM 6` |
| [188](../modules/BMI088/bmi088_regNdef.h#L188) | 预处理 | `#define BMI088_WRITE_GYRO_REG_NUM 6` |
| [190](../modules/BMI088/bmi088_regNdef.h#L190) | 预处理 | `#define BMI088_GYRO_DATA_READY_BIT 0` |
| [191](../modules/BMI088/bmi088_regNdef.h#L191) | 预处理 | `#define BMI088_ACCEL_DATA_READY_BIT 1` |
| [192](../modules/BMI088/bmi088_regNdef.h#L192) | 预处理 | `#define BMI088_ACCEL_TEMP_DATA_READY_BIT 2` |
| [194](../modules/BMI088/bmi088_regNdef.h#L194) | 预处理 | `#define BMI088_LONG_DELAY_TIME 80` |
| [195](../modules/BMI088/bmi088_regNdef.h#L195) | 预处理 | `#define BMI088_COM_WAIT_SENSOR_TIME 150` |
| [197](../modules/BMI088/bmi088_regNdef.h#L197) | 预处理 | `#define BMI088_ACCEL_IIC_ADDRESSE (0x18 << 1)` |
| [198](../modules/BMI088/bmi088_regNdef.h#L198) | 预处理 | `#define BMI088_GYRO_IIC_ADDRESSE (0x68 << 1)` |
| [200](../modules/BMI088/bmi088_regNdef.h#L200) | 预处理 | `#define BMI088_ACCEL_3G_SEN 0.0008974358974f` |
| [201](../modules/BMI088/bmi088_regNdef.h#L201) | 预处理 | `#define BMI088_ACCEL_6G_SEN 0.00179443359375f` |
| [202](../modules/BMI088/bmi088_regNdef.h#L202) | 预处理 | `#define BMI088_ACCEL_12G_SEN 0.0035888671875f` |
| [203](../modules/BMI088/bmi088_regNdef.h#L203) | 预处理 | `#define BMI088_ACCEL_24G_SEN 0.007177734375f` |
| [205](../modules/BMI088/bmi088_regNdef.h#L205) | 预处理 | `#define BMI088_GYRO_2000_SEN 0.00106526443603169529841533860381f` |
| [206](../modules/BMI088/bmi088_regNdef.h#L206) | 预处理 | `#define BMI088_GYRO_1000_SEN 0.00053263221801584764920766930190693f` |
| [207](../modules/BMI088/bmi088_regNdef.h#L207) | 预处理 | `#define BMI088_GYRO_500_SEN 0.00026631610900792382460383465095346f` |
| [208](../modules/BMI088/bmi088_regNdef.h#L208) | 预处理 | `#define BMI088_GYRO_250_SEN 0.00013315805450396191230191732547673f` |
| [209](../modules/BMI088/bmi088_regNdef.h#L209) | 预处理 | `#define BMI088_GYRO_125_SEN 0.000066579027251980956150958662738366f` |
| [234](../modules/BMI088/bmi088_regNdef.h#L234) | 预处理 | `#endif` |

## modules/can_comm/can_comm.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [104](../modules/can_comm/can_comm.c#L104) | 配置初值 | `Daemon_Init_Config_s daemon_config = {` |
| [105](../modules/can_comm/can_comm.c#L105) | 配置初值 | `.callback = CANCommLostCallback,` |
| [106](../modules/can_comm/can_comm.c#L106) | 配置初值 | `.owner_id = (void *)ins,` |
| [107](../modules/can_comm/can_comm.c#L107) | 配置初值 | `.reload_count = comm_config->daemon_count,` |

## modules/can_comm/can_comm.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/can_comm/can_comm.h#L11) | 预处理 | `#ifndef CAN_COMM_H` |
| [12](../modules/can_comm/can_comm.h#L12) | 预处理 | `#define CAN_COMM_H` |
| [17](../modules/can_comm/can_comm.h#L17) | 预处理 | `#define MX_CAN_COMM_COUNT 4 // 注意均衡负载,一条总线上不要挂载过多的外设` |
| [19](../modules/can_comm/can_comm.h#L19) | 预处理 | `#define CAN_COMM_MAX_BUFFSIZE 128 // 最大发送/接收字节数,如果不够可以增加此数值` |
| [20](../modules/can_comm/can_comm.h#L20) | 预处理 | `#define CAN_COMM_HEADER 's'      // 帧头` |
| [21](../modules/can_comm/can_comm.h#L21) | 预处理 | `#define CAN_COMM_TAIL 'e'        // 帧尾` |
| [22](../modules/can_comm/can_comm.h#L22) | 预处理 | `#define CAN_COMM_OFFSET_BYTES 4  // 's'+ datalen + 'e' + crc8` |
| [98](../modules/can_comm/can_comm.h#L98) | 预处理 | `#endif // !CAN_COMM_H` |

## modules/chassis_force/chassis_force_allocator.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_allocator.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_ALLOCATOR_H` |
| [2](../modules/chassis_force/chassis_force_allocator.h#L2) | 预处理 | `#define CHASSIS_FORCE_ALLOCATOR_H` |
| [20](../modules/chassis_force/chassis_force_allocator.h#L20) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_command.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [39](../modules/chassis_force/chassis_force_command.c#L39) | 预处理 | `#if CHASSIS_FORCE_EXIT_MODE == CHASSIS_FORCE_EXIT_BRAKE` |
| [42](../modules/chassis_force/chassis_force_command.c#L42) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_command.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_command.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_COMMAND_H` |
| [2](../modules/chassis_force/chassis_force_command.h#L2) | 预处理 | `#define CHASSIS_FORCE_COMMAND_H` |
| [15](../modules/chassis_force/chassis_force_command.h#L15) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_control.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [178](../modules/chassis_force/chassis_force_control.c#L178) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE` |
| [193](../modules/chassis_force/chassis_force_control.c#L193) | 预处理 | `#endif` |
| [204](../modules/chassis_force/chassis_force_control.c#L204) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE` |
| [217](../modules/chassis_force/chassis_force_control.c#L217) | 预处理 | `#endif` |
| [269](../modules/chassis_force/chassis_force_control.c#L269) | 预处理 | `#if CHASSIS_FORCE_SPIN_MODE == CHASSIS_FORCE_SPIN_POWER_ADAPTIVE` |
| [274](../modules/chassis_force/chassis_force_control.c#L274) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_control.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_control.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_CONTROL_H` |
| [2](../modules/chassis_force/chassis_force_control.h#L2) | 预处理 | `#define CHASSIS_FORCE_CONTROL_H` |
| [72](../modules/chassis_force/chassis_force_control.h#L72) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_geometry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_geometry.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_GEOMETRY_H` |
| [2](../modules/chassis_force/chassis_force_geometry.h#L2) | 预处理 | `#define CHASSIS_FORCE_GEOMETRY_H` |
| [11](../modules/chassis_force/chassis_force_geometry.h#L11) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_intent.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_intent.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_INTENT_H` |
| [2](../modules/chassis_force/chassis_force_intent.h#L2) | 预处理 | `#define CHASSIS_FORCE_INTENT_H` |
| [7](../modules/chassis_force/chassis_force_intent.h#L7) | 预处理 | `#define CHASSIS_FORCE_WIRE_SIZE 44U` |
| [25](../modules/chassis_force/chassis_force_intent.h#L25) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_spin_power.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_spin_power.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_SPIN_POWER_H` |
| [2](../modules/chassis_force/chassis_force_spin_power.h#L2) | 预处理 | `#define CHASSIS_FORCE_SPIN_POWER_H` |
| [18](../modules/chassis_force/chassis_force_spin_power.h#L18) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_types.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force/chassis_force_types.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_TYPES_H` |
| [2](../modules/chassis_force/chassis_force_types.h#L2) | 预处理 | `#define CHASSIS_FORCE_TYPES_H` |
| [4](../modules/chassis_force/chassis_force_types.h#L4) | 预处理 | `#define CHASSIS_FORCE_COMMAND_MAX_AGE_MS 10U` |
| [15](../modules/chassis_force/chassis_force_types.h#L15) | 预处理 | `#endif` |

## modules/chassis_force/chassis_force_validation.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [57](../modules/chassis_force/chassis_force_validation.c#L57) | 预处理 | `#if CHASSIS_FORCE_EXIT_MODE == CHASSIS_FORCE_EXIT_BRAKE` |
| [61](../modules/chassis_force/chassis_force_validation.c#L61) | 预处理 | `#endif` |

## modules/chassis_force_calibration/force_calibration.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force_calibration/force_calibration.h#L1) | 预处理 | `#ifndef FORCE_CALIBRATION_H` |
| [2](../modules/chassis_force_calibration/force_calibration.h#L2) | 预处理 | `#define FORCE_CALIBRATION_H` |
| [56](../modules/chassis_force_calibration/force_calibration.h#L56) | 预处理 | `#endif` |

## modules/chassis_force_calibration/force_calibration_capture.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_force_calibration/force_calibration_capture.h#L1) | 预处理 | `#ifndef FORCE_CALIBRATION_CAPTURE_H` |
| [2](../modules/chassis_force_calibration/force_calibration_capture.h#L2) | 预处理 | `#define FORCE_CALIBRATION_CAPTURE_H` |
| [29](../modules/chassis_force_calibration/force_calibration_capture.h#L29) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [32](../modules/chassis_power/chassis_power.c#L32) | 预处理 | `#define VOFA_POWER_FLOAT(member) hash=HashFloat(hash,c->member);` |
| [33](../modules/chassis_power/chassis_power.c#L33) | 预处理 | `#define VOFA_POWER_U32(member) hash=HashU32(hash,(uint32_t)c->member);` |

## modules/chassis_power/chassis_power.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_H` |
| [2](../modules/chassis_power/chassis_power.h#L2) | 预处理 | `#define CHASSIS_POWER_H` |
| [21](../modules/chassis_power/chassis_power.h#L21) | 预处理 | `#define CHASSIS_POWER_RTT_DIVIDER 20U /* 40 ms at a 2 ms motor period. */` |
| [23](../modules/chassis_power/chassis_power.h#L23) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_force.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [2](../modules/chassis_power/chassis_power_force.c#L2) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [107](../modules/chassis_power/chassis_power_force.c#L107) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_force.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power_force.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_FORCE_H` |
| [2](../modules/chassis_power/chassis_power_force.h#L2) | 预处理 | `#define CHASSIS_POWER_FORCE_H` |
| [10](../modules/chassis_power/chassis_power_force.h#L10) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_internal.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power_internal.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_INTERNAL_H` |
| [2](../modules/chassis_power/chassis_power_internal.h#L2) | 预处理 | `#define CHASSIS_POWER_INTERNAL_H` |
| [7](../modules/chassis_power/chassis_power_internal.h#L7) | 预处理 | `#ifdef STM32F427xx` |
| [10](../modules/chassis_power/chassis_power_internal.h#L10) | 预处理 | `#define POWER_LOCK() taskENTER_CRITICAL()` |
| [11](../modules/chassis_power/chassis_power_internal.h#L11) | 预处理 | `#define POWER_UNLOCK() taskEXIT_CRITICAL()` |
| [12](../modules/chassis_power/chassis_power_internal.h#L12) | 预处理 | `#else` |
| [13](../modules/chassis_power/chassis_power_internal.h#L13) | 预处理 | `#define POWER_LOCK() ((void)0)` |
| [14](../modules/chassis_power/chassis_power_internal.h#L14) | 预处理 | `#define POWER_UNLOCK() ((void)0)` |
| [15](../modules/chassis_power/chassis_power_internal.h#L15) | 预处理 | `#endif` |
| [39](../modules/chassis_power/chassis_power_internal.h#L39) | 预处理 | `#define chassis_power (ChassisPowerPrivate())` |
| [43](../modules/chassis_power/chassis_power_internal.h#L43) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_limiter.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [89](../modules/chassis_power/chassis_power_limiter.c#L89) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW` |
| [91](../modules/chassis_power/chassis_power_limiter.c#L91) | 预处理 | `#endif` |
| [123](../modules/chassis_power/chassis_power_limiter.c#L123) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW` |
| [125](../modules/chassis_power/chassis_power_limiter.c#L125) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_policy.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [193](../modules/chassis_power/chassis_power_policy.c#L193) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW` |
| [195](../modules/chassis_power/chassis_power_policy.c#L195) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_policy.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power_policy.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_POLICY_H` |
| [2](../modules/chassis_power/chassis_power_policy.h#L2) | 预处理 | `#define CHASSIS_POWER_POLICY_H` |
| [23](../modules/chassis_power/chassis_power_policy.h#L23) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_shadow.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../modules/chassis_power/chassis_power_shadow.c#L3) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW` |
| [5](../modules/chassis_power/chassis_power_shadow.c#L5) | 预处理 | `#ifdef STM32F427xx` |
| [7](../modules/chassis_power/chassis_power_shadow.c#L7) | 预处理 | `#endif` |
| [15](../modules/chassis_power/chassis_power_shadow.c#L15) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [20](../modules/chassis_power/chassis_power_shadow.c#L20) | 预处理 | `#endif` |
| [23](../modules/chassis_power/chassis_power_shadow.c#L23) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [31](../modules/chassis_power/chassis_power_shadow.c#L31) | 预处理 | `#else` |
| [33](../modules/chassis_power/chassis_power_shadow.c#L33) | 预处理 | `#endif` |
| [37](../modules/chassis_power/chassis_power_shadow.c#L37) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [40](../modules/chassis_power/chassis_power_shadow.c#L40) | 预处理 | `#else` |
| [42](../modules/chassis_power/chassis_power_shadow.c#L42) | 预处理 | `#endif` |
| [49](../modules/chassis_power/chassis_power_shadow.c#L49) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [51](../modules/chassis_power/chassis_power_shadow.c#L51) | 预处理 | `#endif` |
| [76](../modules/chassis_power/chassis_power_shadow.c#L76) | 预处理 | `#if CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [92](../modules/chassis_power/chassis_power_shadow.c#L92) | 预处理 | `#ifdef STM32F427xx` |
| [94](../modules/chassis_power/chassis_power_shadow.c#L94) | 预处理 | `#endif` |
| [120](../modules/chassis_power/chassis_power_shadow.c#L120) | 预处理 | `#ifdef STM32F427xx` |
| [123](../modules/chassis_power/chassis_power_shadow.c#L123) | 预处理 | `#else` |
| [125](../modules/chassis_power/chassis_power_shadow.c#L125) | 预处理 | `#endif` |
| [126](../modules/chassis_power/chassis_power_shadow.c#L126) | 预处理 | `#else` |
| [128](../modules/chassis_power/chassis_power_shadow.c#L128) | 预处理 | `#endif` |
| [234](../modules/chassis_power/chassis_power_shadow.c#L234) | 预处理 | `#ifdef STM32F427xx` |
| [236](../modules/chassis_power/chassis_power_shadow.c#L236) | 预处理 | `#endif` |
| [238](../modules/chassis_power/chassis_power_shadow.c#L238) | 预处理 | `#ifdef STM32F427xx` |
| [241](../modules/chassis_power/chassis_power_shadow.c#L241) | 预处理 | `#else` |
| [243](../modules/chassis_power/chassis_power_shadow.c#L243) | 预处理 | `#endif` |
| [256](../modules/chassis_power/chassis_power_shadow.c#L256) | 预处理 | `#ifdef STM32F427xx` |
| [258](../modules/chassis_power/chassis_power_shadow.c#L258) | 预处理 | `#endif` |
| [260](../modules/chassis_power/chassis_power_shadow.c#L260) | 预处理 | `#ifdef STM32F427xx` |
| [263](../modules/chassis_power/chassis_power_shadow.c#L263) | 预处理 | `#endif` |
| [276](../modules/chassis_power/chassis_power_shadow.c#L276) | 预处理 | `#else` |
| [288](../modules/chassis_power/chassis_power_shadow.c#L288) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_shadow.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power_shadow.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_SHADOW_H` |
| [2](../modules/chassis_power/chassis_power_shadow.h#L2) | 预处理 | `#define CHASSIS_POWER_SHADOW_H` |
| [57](../modules/chassis_power/chassis_power_shadow.h#L57) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_shadow_energy.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../modules/chassis_power/chassis_power_shadow_energy.c#L3) | 预处理 | `#if CHASSIS_PREDICTIVE_POWER_MODE == CHASSIS_PREDICTIVE_SHADOW` |
| [5](../modules/chassis_power/chassis_power_shadow_energy.c#L5) | 预处理 | `#ifdef STM32F427xx` |
| [7](../modules/chassis_power/chassis_power_shadow_energy.c#L7) | 预处理 | `#endif` |
| [43](../modules/chassis_power/chassis_power_shadow_energy.c#L43) | 预处理 | `#ifdef STM32F427xx` |
| [45](../modules/chassis_power/chassis_power_shadow_energy.c#L45) | 预处理 | `#endif` |
| [47](../modules/chassis_power/chassis_power_shadow_energy.c#L47) | 配置初值 | `const PPEnergyConfig config={.buffer_max_j=b->max_energy_j,` |
| [48](../modules/chassis_power/chassis_power_shadow_energy.c#L48) | 配置初值 | `.buffer_floor_j=b->hard_floor_j+b->energy_margin_j,` |
| [49](../modules/chassis_power/chassis_power_shadow_energy.c#L49) | 配置初值 | `.buffer_terminal_j=fmaxf(b->recovery_enter_j,b->hard_floor_j+b->energy_margin_j),` |
| [50](../modules/chassis_power/chassis_power_shadow_energy.c#L50) | 配置初值 | `.input_max_w=i->referee.chassis_power_limit_w+b->max_boost_w,` |
| [51](../modules/chassis_power/chassis_power_shadow_energy.c#L51) | 配置初值 | `.load_max_w=i->referee.chassis_power_limit_w+b->max_boost_w};` |
| [64](../modules/chassis_power/chassis_power_shadow_energy.c#L64) | 预处理 | `#ifdef STM32F427xx` |
| [68](../modules/chassis_power/chassis_power_shadow_energy.c#L68) | 预处理 | `#else` |
| [70](../modules/chassis_power/chassis_power_shadow_energy.c#L70) | 预处理 | `#endif` |
| [73](../modules/chassis_power/chassis_power_shadow_energy.c#L73) | 预处理 | `#else` |
| [80](../modules/chassis_power/chassis_power_shadow_energy.c#L80) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_telemetry.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [4](../modules/chassis_power/chassis_power_telemetry.c#L4) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE` |
| [9](../modules/chassis_power/chassis_power_telemetry.c#L9) | 预处理 | `#endif` |
| [26](../modules/chassis_power/chassis_power_telemetry.c#L26) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE` |
| [50](../modules/chassis_power/chassis_power_telemetry.c#L50) | 预处理 | `#endif` |
| [68](../modules/chassis_power/chassis_power_telemetry.c#L68) | 预处理 | `#if CHASSIS_POWER_CAPTURE_ENABLE && BOARD_LOG_UART_ENABLED` |
| [83](../modules/chassis_power/chassis_power_telemetry.c#L83) | 预处理 | `#endif` |

## modules/chassis_power/chassis_power_types.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/chassis_power/chassis_power_types.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_TYPES_H` |
| [2](../modules/chassis_power/chassis_power_types.h#L2) | 预处理 | `#define CHASSIS_POWER_TYPES_H` |
| [129](../modules/chassis_power/chassis_power_types.h#L129) | 预处理 | `#endif` |

## modules/daemon/daemon.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/daemon/daemon.h#L1) | 预处理 | `#ifndef MONITOR_H` |
| [2](../modules/daemon/daemon.h#L2) | 预处理 | `#define MONITOR_H` |
| [7](../modules/daemon/daemon.h#L7) | 预处理 | `#define DAEMON_MX_CNT 64` |
| [62](../modules/daemon/daemon.h#L62) | 预处理 | `#endif // !MONITOR_H` |

## modules/general_def.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/general_def.h#L1) | 预处理 | `#ifndef GENERAL_DEF_H` |
| [2](../modules/general_def.h#L2) | 预处理 | `#define GENERAL_DEF_H` |
| [6](../modules/general_def.h#L6) | 预处理 | `#ifndef PI` |
| [7](../modules/general_def.h#L7) | 预处理 | `#define PI 3.1415926535f` |
| [8](../modules/general_def.h#L8) | 预处理 | `#endif` |
| [9](../modules/general_def.h#L9) | 预处理 | `#define PI2 (PI * 2.0f) // 2 pi` |
| [11](../modules/general_def.h#L11) | 预处理 | `#define RAD_2_DEGREE 57.2957795f    // 180/pi` |
| [12](../modules/general_def.h#L12) | 预处理 | `#define DEGREE_2_RAD 0.01745329252f // pi/180` |
| [14](../modules/general_def.h#L14) | 预处理 | `#define RPM_2_ANGLE_PER_SEC 6.0f       // ×360°/60sec` |
| [15](../modules/general_def.h#L15) | 预处理 | `#define RPM_2_RAD_PER_SEC 0.104719755f // ×2pi/60sec` |
| [17](../modules/general_def.h#L17) | 预处理 | `#endif // !GENERAL_DEF_H` |

## modules/heat_control/heat_control.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/heat_control/heat_control.h#L1) | 预处理 | `#ifndef HEAT_CONTROL_H` |
| [2](../modules/heat_control/heat_control.h#L2) | 预处理 | `#define HEAT_CONTROL_H` |
| [8](../modules/heat_control/heat_control.h#L8) | 预处理 | `#define HEAT_CONTROL_DATA_TOPIC "heat_control_data"` |
| [72](../modules/heat_control/heat_control.h#L72) | 预处理 | `#endif` |

## modules/imu/BMI088driver.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [23](../modules/imu/BMI088driver.c#L23) | 预处理 | `#if defined(BMI088_USE_SPI)` |
| [25](../modules/imu/BMI088driver.c#L25) | 预处理 | `#define BMI088_accel_write_single_reg(reg, data) \` |
| [31](../modules/imu/BMI088driver.c#L31) | 预处理 | `#define BMI088_accel_read_single_reg(reg, data) \` |
| [39](../modules/imu/BMI088driver.c#L39) | 预处理 | `#define BMI088_accel_read_muli_reg(reg, data, len) \` |
| [47](../modules/imu/BMI088driver.c#L47) | 预处理 | `#define BMI088_gyro_write_single_reg(reg, data) \` |
| [53](../modules/imu/BMI088driver.c#L53) | 预处理 | `#define BMI088_gyro_read_single_reg(reg, data)  \` |
| [59](../modules/imu/BMI088driver.c#L59) | 预处理 | `#define BMI088_gyro_read_muli_reg(reg, data, len)   \` |
| [70](../modules/imu/BMI088driver.c#L70) | 预处理 | `#elif defined(BMI088_USE_IIC)` |
| [71](../modules/imu/BMI088driver.c#L71) | 预处理 | `#endif` |
| [378](../modules/imu/BMI088driver.c#L378) | 预处理 | `#if defined(BMI088_USE_SPI)` |
| [403](../modules/imu/BMI088driver.c#L403) | 预处理 | `#elif defined(BMI088_USE_IIC)` |
| [405](../modules/imu/BMI088driver.c#L405) | 预处理 | `#endif` |

## modules/imu/BMI088driver.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/BMI088driver.h#L1) | 预处理 | `#ifndef BMI088DRIVER_H` |
| [2](../modules/imu/BMI088driver.h#L2) | 预处理 | `#define BMI088DRIVER_H` |
| [7](../modules/imu/BMI088driver.h#L7) | 预处理 | `#define BMI088_TEMP_FACTOR 0.125f` |
| [8](../modules/imu/BMI088driver.h#L8) | 预处理 | `#define BMI088_TEMP_OFFSET 23.0f` |
| [10](../modules/imu/BMI088driver.h#L10) | 预处理 | `#define BMI088_WRITE_ACCEL_REG_NUM 6` |
| [11](../modules/imu/BMI088driver.h#L11) | 预处理 | `#define BMI088_WRITE_GYRO_REG_NUM 6` |
| [13](../modules/imu/BMI088driver.h#L13) | 预处理 | `#define BMI088_GYRO_DATA_READY_BIT 0` |
| [14](../modules/imu/BMI088driver.h#L14) | 预处理 | `#define BMI088_ACCEL_DATA_READY_BIT 1` |
| [15](../modules/imu/BMI088driver.h#L15) | 预处理 | `#define BMI088_ACCEL_TEMP_DATA_READY_BIT 2` |
| [17](../modules/imu/BMI088driver.h#L17) | 预处理 | `#define BMI088_LONG_DELAY_TIME 80` |
| [18](../modules/imu/BMI088driver.h#L18) | 预处理 | `#define BMI088_COM_WAIT_SENSOR_TIME 150` |
| [20](../modules/imu/BMI088driver.h#L20) | 预处理 | `#define BMI088_ACCEL_IIC_ADDRESSE (0x18 << 1)` |
| [21](../modules/imu/BMI088driver.h#L21) | 预处理 | `#define BMI088_GYRO_IIC_ADDRESSE (0x68 << 1)` |
| [23](../modules/imu/BMI088driver.h#L23) | 预处理 | `#define BMI088_ACCEL_3G_SEN 0.0008974358974f` |
| [24](../modules/imu/BMI088driver.h#L24) | 预处理 | `#define BMI088_ACCEL_6G_SEN 0.00179443359375f` |
| [25](../modules/imu/BMI088driver.h#L25) | 预处理 | `#define BMI088_ACCEL_12G_SEN 0.0035888671875f` |
| [26](../modules/imu/BMI088driver.h#L26) | 预处理 | `#define BMI088_ACCEL_24G_SEN 0.007177734375f` |
| [28](../modules/imu/BMI088driver.h#L28) | 预处理 | `#define BMI088_GYRO_2000_SEN 0.00106526443603169529841533860381f` |
| [29](../modules/imu/BMI088driver.h#L29) | 预处理 | `#define BMI088_GYRO_1000_SEN 0.00053263221801584764920766930190693f` |
| [30](../modules/imu/BMI088driver.h#L30) | 预处理 | `#define BMI088_GYRO_500_SEN 0.00026631610900792382460383465095346f` |
| [31](../modules/imu/BMI088driver.h#L31) | 预处理 | `#define BMI088_GYRO_250_SEN 0.00013315805450396191230191732547673f` |
| [32](../modules/imu/BMI088driver.h#L32) | 预处理 | `#define BMI088_GYRO_125_SEN 0.000066579027251980956150958662738366f` |
| [35](../modules/imu/BMI088driver.h#L35) | 预处理 | `#if INFANTRY_ID == 0` |
| [36](../modules/imu/BMI088driver.h#L36) | 预处理 | `#define GxOFFSET 0.00247530174f` |
| [37](../modules/imu/BMI088driver.h#L37) | 预处理 | `#define GyOFFSET 0.000393082853f` |
| [38](../modules/imu/BMI088driver.h#L38) | 预处理 | `#define GzOFFSET 0.000393082853f` |
| [39](../modules/imu/BMI088driver.h#L39) | 预处理 | `#define gNORM 9.69293118f` |
| [40](../modules/imu/BMI088driver.h#L40) | 预处理 | `#elif INFANTRY_ID == 1` |
| [41](../modules/imu/BMI088driver.h#L41) | 预处理 | `#define GxOFFSET 0.0007222f` |
| [42](../modules/imu/BMI088driver.h#L42) | 预处理 | `#define GyOFFSET -0.001786f` |
| [43](../modules/imu/BMI088driver.h#L43) | 预处理 | `#define GzOFFSET 0.0004346f` |
| [44](../modules/imu/BMI088driver.h#L44) | 预处理 | `#define gNORM 9.876785f` |
| [45](../modules/imu/BMI088driver.h#L45) | 预处理 | `#elif INFANTRY_ID == 2` |
| [46](../modules/imu/BMI088driver.h#L46) | 预处理 | `#define GxOFFSET 0.0007222f` |
| [47](../modules/imu/BMI088driver.h#L47) | 预处理 | `#define GyOFFSET -0.001786f` |
| [48](../modules/imu/BMI088driver.h#L48) | 预处理 | `#define GzOFFSET 0.0004346f` |
| [49](../modules/imu/BMI088driver.h#L49) | 预处理 | `#define gNORM 9.876785f` |
| [50](../modules/imu/BMI088driver.h#L50) | 预处理 | `#elif INFANTRY_ID == 3` |
| [51](../modules/imu/BMI088driver.h#L51) | 预处理 | `#define GxOFFSET 0.00270364084f` |
| [52](../modules/imu/BMI088driver.h#L52) | 预处理 | `#define GyOFFSET -0.000532632112f` |
| [53](../modules/imu/BMI088driver.h#L53) | 预处理 | `#define GzOFFSET 0.00478090625f` |
| [54](../modules/imu/BMI088driver.h#L54) | 预处理 | `#define gNORM 9.73574924f` |
| [55](../modules/imu/BMI088driver.h#L55) | 预处理 | `#elif INFANTRY_ID == 4` |
| [56](../modules/imu/BMI088driver.h#L56) | 预处理 | `#define GxOFFSET 0.0007222f` |
| [57](../modules/imu/BMI088driver.h#L57) | 预处理 | `#define GyOFFSET -0.001786f` |
| [58](../modules/imu/BMI088driver.h#L58) | 预处理 | `#define GzOFFSET 0.0004346f` |
| [59](../modules/imu/BMI088driver.h#L59) | 预处理 | `#define gNORM 9.876785f` |
| [60](../modules/imu/BMI088driver.h#L60) | 预处理 | `#endif` |
| [133](../modules/imu/BMI088driver.h#L133) | 预处理 | `#endif` |

## modules/imu/BMI088Middleware.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/BMI088Middleware.h#L1) | 预处理 | `#ifndef BMI088MIDDLEWARE_H` |
| [2](../modules/imu/BMI088Middleware.h#L2) | 预处理 | `#define BMI088MIDDLEWARE_H` |
| [6](../modules/imu/BMI088Middleware.h#L6) | 预处理 | `#define BMI088_USE_SPI` |
| [10](../modules/imu/BMI088Middleware.h#L10) | 预处理 | `#define CS1_ACCEL_GPIO_Port ACCEL_NSS_GPIO_Port` |
| [11](../modules/imu/BMI088Middleware.h#L11) | 预处理 | `#define CS1_ACCEL_Pin ACCEL_NSS_Pin` |
| [12](../modules/imu/BMI088Middleware.h#L12) | 预处理 | `#define CS1_GYRO_GPIO_Port GYRO_NSS_GPIO_Port` |
| [13](../modules/imu/BMI088Middleware.h#L13) | 预处理 | `#define CS1_GYRO_Pin GYRO_NSS_Pin` |
| [17](../modules/imu/BMI088Middleware.h#L17) | 预处理 | `#if defined(BMI088_USE_SPI)` |
| [28](../modules/imu/BMI088Middleware.h#L28) | 预处理 | `#elif defined(BMI088_USE_IIC)` |
| [30](../modules/imu/BMI088Middleware.h#L30) | 预处理 | `#endif` |
| [32](../modules/imu/BMI088Middleware.h#L32) | 预处理 | `#endif` |

## modules/imu/BMI088reg.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/BMI088reg.h#L1) | 预处理 | `#ifndef BMI088REG_H` |
| [2](../modules/imu/BMI088reg.h#L2) | 预处理 | `#define BMI088REG_H` |
| [4](../modules/imu/BMI088reg.h#L4) | 预处理 | `#define BMI088_ACC_CHIP_ID 0x00 // the register is  " Who am I "` |
| [5](../modules/imu/BMI088reg.h#L5) | 预处理 | `#define BMI088_ACC_CHIP_ID_VALUE 0x1E` |
| [7](../modules/imu/BMI088reg.h#L7) | 预处理 | `#define BMI088_ACC_ERR_REG 0x02` |
| [8](../modules/imu/BMI088reg.h#L8) | 预处理 | `#define BMI088_ACCEL_CONGIF_ERROR_SHFITS 0x2` |
| [9](../modules/imu/BMI088reg.h#L9) | 预处理 | `#define BMI088_ACCEL_CONGIF_ERROR (1 << BMI088_ACCEL_CONGIF_ERROR_SHFITS)` |
| [10](../modules/imu/BMI088reg.h#L10) | 预处理 | `#define BMI088_FATAL_ERROR_SHFITS 0x0` |
| [11](../modules/imu/BMI088reg.h#L11) | 预处理 | `#define BMI088_FATAL_ERROR (1 << BMI088_FATAL_ERROR)` |
| [13](../modules/imu/BMI088reg.h#L13) | 预处理 | `#define BMI088_ACC_STATUS 0x03` |
| [14](../modules/imu/BMI088reg.h#L14) | 预处理 | `#define BMI088_ACCEL_DRDY_SHFITS 0x7` |
| [15](../modules/imu/BMI088reg.h#L15) | 预处理 | `#define BMI088_ACCEL_DRDY (1 << BMI088_ACCEL_DRDY_SHFITS)` |
| [17](../modules/imu/BMI088reg.h#L17) | 预处理 | `#define BMI088_ACCEL_XOUT_L 0x12` |
| [18](../modules/imu/BMI088reg.h#L18) | 预处理 | `#define BMI088_ACCEL_XOUT_M 0x13` |
| [19](../modules/imu/BMI088reg.h#L19) | 预处理 | `#define BMI088_ACCEL_YOUT_L 0x14` |
| [20](../modules/imu/BMI088reg.h#L20) | 预处理 | `#define BMI088_ACCEL_YOUT_M 0x15` |
| [21](../modules/imu/BMI088reg.h#L21) | 预处理 | `#define BMI088_ACCEL_ZOUT_L 0x16` |
| [22](../modules/imu/BMI088reg.h#L22) | 预处理 | `#define BMI088_ACCEL_ZOUT_M 0x17` |
| [24](../modules/imu/BMI088reg.h#L24) | 预处理 | `#define BMI088_SENSORTIME_DATA_L 0x18` |
| [25](../modules/imu/BMI088reg.h#L25) | 预处理 | `#define BMI088_SENSORTIME_DATA_M 0x19` |
| [26](../modules/imu/BMI088reg.h#L26) | 预处理 | `#define BMI088_SENSORTIME_DATA_H 0x1A` |
| [28](../modules/imu/BMI088reg.h#L28) | 预处理 | `#define BMI088_ACC_INT_STAT_1 0x1D` |
| [29](../modules/imu/BMI088reg.h#L29) | 预处理 | `#define BMI088_ACCEL_DRDY_INTERRUPT_SHFITS 0x7` |
| [30](../modules/imu/BMI088reg.h#L30) | 预处理 | `#define BMI088_ACCEL_DRDY_INTERRUPT (1 << BMI088_ACCEL_DRDY_INTERRUPT_SHFITS)` |
| [32](../modules/imu/BMI088reg.h#L32) | 预处理 | `#define BMI088_TEMP_M 0x22` |
| [34](../modules/imu/BMI088reg.h#L34) | 预处理 | `#define BMI088_TEMP_L 0x23` |
| [36](../modules/imu/BMI088reg.h#L36) | 预处理 | `#define BMI088_ACC_CONF 0x40` |
| [37](../modules/imu/BMI088reg.h#L37) | 预处理 | `#define BMI088_ACC_CONF_MUST_Set 0x80` |
| [38](../modules/imu/BMI088reg.h#L38) | 预处理 | `#define BMI088_ACC_BWP_SHFITS 0x4` |
| [39](../modules/imu/BMI088reg.h#L39) | 预处理 | `#define BMI088_ACC_OSR4 (0x0 << BMI088_ACC_BWP_SHFITS)` |
| [40](../modules/imu/BMI088reg.h#L40) | 预处理 | `#define BMI088_ACC_OSR2 (0x1 << BMI088_ACC_BWP_SHFITS)` |
| [41](../modules/imu/BMI088reg.h#L41) | 预处理 | `#define BMI088_ACC_NORMAL (0x2 << BMI088_ACC_BWP_SHFITS)` |
| [43](../modules/imu/BMI088reg.h#L43) | 预处理 | `#define BMI088_ACC_ODR_SHFITS 0x0` |
| [44](../modules/imu/BMI088reg.h#L44) | 预处理 | `#define BMI088_ACC_12_5_HZ (0x5 << BMI088_ACC_ODR_SHFITS)` |
| [45](../modules/imu/BMI088reg.h#L45) | 预处理 | `#define BMI088_ACC_25_HZ (0x6 << BMI088_ACC_ODR_SHFITS)` |
| [46](../modules/imu/BMI088reg.h#L46) | 预处理 | `#define BMI088_ACC_50_HZ (0x7 << BMI088_ACC_ODR_SHFITS)` |
| [47](../modules/imu/BMI088reg.h#L47) | 预处理 | `#define BMI088_ACC_100_HZ (0x8 << BMI088_ACC_ODR_SHFITS)` |
| [48](../modules/imu/BMI088reg.h#L48) | 预处理 | `#define BMI088_ACC_200_HZ (0x9 << BMI088_ACC_ODR_SHFITS)` |
| [49](../modules/imu/BMI088reg.h#L49) | 预处理 | `#define BMI088_ACC_400_HZ (0xA << BMI088_ACC_ODR_SHFITS)` |
| [50](../modules/imu/BMI088reg.h#L50) | 预处理 | `#define BMI088_ACC_800_HZ (0xB << BMI088_ACC_ODR_SHFITS)` |
| [51](../modules/imu/BMI088reg.h#L51) | 预处理 | `#define BMI088_ACC_1600_HZ (0xC << BMI088_ACC_ODR_SHFITS)` |
| [53](../modules/imu/BMI088reg.h#L53) | 预处理 | `#define BMI088_ACC_RANGE 0x41` |
| [55](../modules/imu/BMI088reg.h#L55) | 预处理 | `#define BMI088_ACC_RANGE_SHFITS 0x0` |
| [56](../modules/imu/BMI088reg.h#L56) | 预处理 | `#define BMI088_ACC_RANGE_3G (0x0 << BMI088_ACC_RANGE_SHFITS)` |
| [57](../modules/imu/BMI088reg.h#L57) | 预处理 | `#define BMI088_ACC_RANGE_6G (0x1 << BMI088_ACC_RANGE_SHFITS)` |
| [58](../modules/imu/BMI088reg.h#L58) | 预处理 | `#define BMI088_ACC_RANGE_12G (0x2 << BMI088_ACC_RANGE_SHFITS)` |
| [59](../modules/imu/BMI088reg.h#L59) | 预处理 | `#define BMI088_ACC_RANGE_24G (0x3 << BMI088_ACC_RANGE_SHFITS)` |
| [61](../modules/imu/BMI088reg.h#L61) | 预处理 | `#define BMI088_INT1_IO_CTRL 0x53` |
| [62](../modules/imu/BMI088reg.h#L62) | 预处理 | `#define BMI088_ACC_INT1_IO_ENABLE_SHFITS 0x3` |
| [63](../modules/imu/BMI088reg.h#L63) | 预处理 | `#define BMI088_ACC_INT1_IO_ENABLE (0x1 << BMI088_ACC_INT1_IO_ENABLE_SHFITS)` |
| [64](../modules/imu/BMI088reg.h#L64) | 预处理 | `#define BMI088_ACC_INT1_GPIO_MODE_SHFITS 0x2` |
| [65](../modules/imu/BMI088reg.h#L65) | 预处理 | `#define BMI088_ACC_INT1_GPIO_PP (0x0 << BMI088_ACC_INT1_GPIO_MODE_SHFITS)` |
| [66](../modules/imu/BMI088reg.h#L66) | 预处理 | `#define BMI088_ACC_INT1_GPIO_OD (0x1 << BMI088_ACC_INT1_GPIO_MODE_SHFITS)` |
| [67](../modules/imu/BMI088reg.h#L67) | 预处理 | `#define BMI088_ACC_INT1_GPIO_LVL_SHFITS 0x1` |
| [68](../modules/imu/BMI088reg.h#L68) | 预处理 | `#define BMI088_ACC_INT1_GPIO_LOW (0x0 << BMI088_ACC_INT1_GPIO_LVL_SHFITS)` |
| [69](../modules/imu/BMI088reg.h#L69) | 预处理 | `#define BMI088_ACC_INT1_GPIO_HIGH (0x1 << BMI088_ACC_INT1_GPIO_LVL_SHFITS)` |
| [71](../modules/imu/BMI088reg.h#L71) | 预处理 | `#define BMI088_INT2_IO_CTRL 0x54` |
| [72](../modules/imu/BMI088reg.h#L72) | 预处理 | `#define BMI088_ACC_INT2_IO_ENABLE_SHFITS 0x3` |
| [73](../modules/imu/BMI088reg.h#L73) | 预处理 | `#define BMI088_ACC_INT2_IO_ENABLE (0x1 << BMI088_ACC_INT2_IO_ENABLE_SHFITS)` |
| [74](../modules/imu/BMI088reg.h#L74) | 预处理 | `#define BMI088_ACC_INT2_GPIO_MODE_SHFITS 0x2` |
| [75](../modules/imu/BMI088reg.h#L75) | 预处理 | `#define BMI088_ACC_INT2_GPIO_PP (0x0 << BMI088_ACC_INT2_GPIO_MODE_SHFITS)` |
| [76](../modules/imu/BMI088reg.h#L76) | 预处理 | `#define BMI088_ACC_INT2_GPIO_OD (0x1 << BMI088_ACC_INT2_GPIO_MODE_SHFITS)` |
| [77](../modules/imu/BMI088reg.h#L77) | 预处理 | `#define BMI088_ACC_INT2_GPIO_LVL_SHFITS 0x1` |
| [78](../modules/imu/BMI088reg.h#L78) | 预处理 | `#define BMI088_ACC_INT2_GPIO_LOW (0x0 << BMI088_ACC_INT2_GPIO_LVL_SHFITS)` |
| [79](../modules/imu/BMI088reg.h#L79) | 预处理 | `#define BMI088_ACC_INT2_GPIO_HIGH (0x1 << BMI088_ACC_INT2_GPIO_LVL_SHFITS)` |
| [81](../modules/imu/BMI088reg.h#L81) | 预处理 | `#define BMI088_INT_MAP_DATA 0x58` |
| [82](../modules/imu/BMI088reg.h#L82) | 预处理 | `#define BMI088_ACC_INT2_DRDY_INTERRUPT_SHFITS 0x6` |
| [83](../modules/imu/BMI088reg.h#L83) | 预处理 | `#define BMI088_ACC_INT2_DRDY_INTERRUPT (0x1 << BMI088_ACC_INT2_DRDY_INTERRUPT_SHFITS)` |
| [84](../modules/imu/BMI088reg.h#L84) | 预处理 | `#define BMI088_ACC_INT1_DRDY_INTERRUPT_SHFITS 0x2` |
| [85](../modules/imu/BMI088reg.h#L85) | 预处理 | `#define BMI088_ACC_INT1_DRDY_INTERRUPT (0x1 << BMI088_ACC_INT1_DRDY_INTERRUPT_SHFITS)` |
| [87](../modules/imu/BMI088reg.h#L87) | 预处理 | `#define BMI088_ACC_SELF_TEST 0x6D` |
| [88](../modules/imu/BMI088reg.h#L88) | 预处理 | `#define BMI088_ACC_SELF_TEST_OFF 0x00` |
| [89](../modules/imu/BMI088reg.h#L89) | 预处理 | `#define BMI088_ACC_SELF_TEST_POSITIVE_SIGNAL 0x0D` |
| [90](../modules/imu/BMI088reg.h#L90) | 预处理 | `#define BMI088_ACC_SELF_TEST_NEGATIVE_SIGNAL 0x09` |
| [92](../modules/imu/BMI088reg.h#L92) | 预处理 | `#define BMI088_ACC_PWR_CONF 0x7C` |
| [93](../modules/imu/BMI088reg.h#L93) | 预处理 | `#define BMI088_ACC_PWR_SUSPEND_MODE 0x03` |
| [94](../modules/imu/BMI088reg.h#L94) | 预处理 | `#define BMI088_ACC_PWR_ACTIVE_MODE 0x00` |
| [96](../modules/imu/BMI088reg.h#L96) | 预处理 | `#define BMI088_ACC_PWR_CTRL 0x7D` |
| [97](../modules/imu/BMI088reg.h#L97) | 预处理 | `#define BMI088_ACC_ENABLE_ACC_OFF 0x00` |
| [98](../modules/imu/BMI088reg.h#L98) | 预处理 | `#define BMI088_ACC_ENABLE_ACC_ON 0x04` |
| [100](../modules/imu/BMI088reg.h#L100) | 预处理 | `#define BMI088_ACC_SOFTRESET 0x7E` |
| [101](../modules/imu/BMI088reg.h#L101) | 预处理 | `#define BMI088_ACC_SOFTRESET_VALUE 0xB6` |
| [103](../modules/imu/BMI088reg.h#L103) | 预处理 | `#define BMI088_GYRO_CHIP_ID 0x00` |
| [104](../modules/imu/BMI088reg.h#L104) | 预处理 | `#define BMI088_GYRO_CHIP_ID_VALUE 0x0F` |
| [106](../modules/imu/BMI088reg.h#L106) | 预处理 | `#define BMI088_GYRO_X_L 0x02` |
| [107](../modules/imu/BMI088reg.h#L107) | 预处理 | `#define BMI088_GYRO_X_H 0x03` |
| [108](../modules/imu/BMI088reg.h#L108) | 预处理 | `#define BMI088_GYRO_Y_L 0x04` |
| [109](../modules/imu/BMI088reg.h#L109) | 预处理 | `#define BMI088_GYRO_Y_H 0x05` |
| [110](../modules/imu/BMI088reg.h#L110) | 预处理 | `#define BMI088_GYRO_Z_L 0x06` |
| [111](../modules/imu/BMI088reg.h#L111) | 预处理 | `#define BMI088_GYRO_Z_H 0x07` |
| [113](../modules/imu/BMI088reg.h#L113) | 预处理 | `#define BMI088_GYRO_INT_STAT_1 0x0A` |
| [114](../modules/imu/BMI088reg.h#L114) | 预处理 | `#define BMI088_GYRO_DYDR_SHFITS 0x7` |
| [115](../modules/imu/BMI088reg.h#L115) | 预处理 | `#define BMI088_GYRO_DYDR (0x1 << BMI088_GYRO_DYDR_SHFITS)` |
| [117](../modules/imu/BMI088reg.h#L117) | 预处理 | `#define BMI088_GYRO_RANGE 0x0F` |
| [118](../modules/imu/BMI088reg.h#L118) | 预处理 | `#define BMI088_GYRO_RANGE_SHFITS 0x0` |
| [119](../modules/imu/BMI088reg.h#L119) | 预处理 | `#define BMI088_GYRO_2000 (0x0 << BMI088_GYRO_RANGE_SHFITS)` |
| [120](../modules/imu/BMI088reg.h#L120) | 预处理 | `#define BMI088_GYRO_1000 (0x1 << BMI088_GYRO_RANGE_SHFITS)` |
| [121](../modules/imu/BMI088reg.h#L121) | 预处理 | `#define BMI088_GYRO_500 (0x2 << BMI088_GYRO_RANGE_SHFITS)` |
| [122](../modules/imu/BMI088reg.h#L122) | 预处理 | `#define BMI088_GYRO_250 (0x3 << BMI088_GYRO_RANGE_SHFITS)` |
| [123](../modules/imu/BMI088reg.h#L123) | 预处理 | `#define BMI088_GYRO_125 (0x4 << BMI088_GYRO_RANGE_SHFITS)` |
| [125](../modules/imu/BMI088reg.h#L125) | 预处理 | `#define BMI088_GYRO_BANDWIDTH 0x10` |
| [127](../modules/imu/BMI088reg.h#L127) | 预处理 | `#define BMI088_GYRO_BANDWIDTH_MUST_Set 0x80` |
| [128](../modules/imu/BMI088reg.h#L128) | 预处理 | `#define BMI088_GYRO_2000_532_HZ 0x00` |
| [129](../modules/imu/BMI088reg.h#L129) | 预处理 | `#define BMI088_GYRO_2000_230_HZ 0x01` |
| [130](../modules/imu/BMI088reg.h#L130) | 预处理 | `#define BMI088_GYRO_1000_116_HZ 0x02` |
| [131](../modules/imu/BMI088reg.h#L131) | 预处理 | `#define BMI088_GYRO_400_47_HZ 0x03` |
| [132](../modules/imu/BMI088reg.h#L132) | 预处理 | `#define BMI088_GYRO_200_23_HZ 0x04` |
| [133](../modules/imu/BMI088reg.h#L133) | 预处理 | `#define BMI088_GYRO_100_12_HZ 0x05` |
| [134](../modules/imu/BMI088reg.h#L134) | 预处理 | `#define BMI088_GYRO_200_64_HZ 0x06` |
| [135](../modules/imu/BMI088reg.h#L135) | 预处理 | `#define BMI088_GYRO_100_32_HZ 0x07` |
| [137](../modules/imu/BMI088reg.h#L137) | 预处理 | `#define BMI088_GYRO_LPM1 0x11` |
| [138](../modules/imu/BMI088reg.h#L138) | 预处理 | `#define BMI088_GYRO_NORMAL_MODE 0x00` |
| [139](../modules/imu/BMI088reg.h#L139) | 预处理 | `#define BMI088_GYRO_SUSPEND_MODE 0x80` |
| [140](../modules/imu/BMI088reg.h#L140) | 预处理 | `#define BMI088_GYRO_DEEP_SUSPEND_MODE 0x20` |
| [142](../modules/imu/BMI088reg.h#L142) | 预处理 | `#define BMI088_GYRO_SOFTRESET 0x14` |
| [143](../modules/imu/BMI088reg.h#L143) | 预处理 | `#define BMI088_GYRO_SOFTRESET_VALUE 0xB6` |
| [145](../modules/imu/BMI088reg.h#L145) | 预处理 | `#define BMI088_GYRO_CTRL 0x15` |
| [146](../modules/imu/BMI088reg.h#L146) | 预处理 | `#define BMI088_DRDY_OFF 0x00` |
| [147](../modules/imu/BMI088reg.h#L147) | 预处理 | `#define BMI088_DRDY_ON 0x80` |
| [149](../modules/imu/BMI088reg.h#L149) | 预处理 | `#define BMI088_GYRO_INT3_INT4_IO_CONF 0x16` |
| [150](../modules/imu/BMI088reg.h#L150) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_MODE_SHFITS 0x3` |
| [151](../modules/imu/BMI088reg.h#L151) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_PP (0x0 << BMI088_GYRO_INT4_GPIO_MODE_SHFITS)` |
| [152](../modules/imu/BMI088reg.h#L152) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_OD (0x1 << BMI088_GYRO_INT4_GPIO_MODE_SHFITS)` |
| [153](../modules/imu/BMI088reg.h#L153) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_LVL_SHFITS 0x2` |
| [154](../modules/imu/BMI088reg.h#L154) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_LOW (0x0 << BMI088_GYRO_INT4_GPIO_LVL_SHFITS)` |
| [155](../modules/imu/BMI088reg.h#L155) | 预处理 | `#define BMI088_GYRO_INT4_GPIO_HIGH (0x1 << BMI088_GYRO_INT4_GPIO_LVL_SHFITS)` |
| [156](../modules/imu/BMI088reg.h#L156) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_MODE_SHFITS 0x1` |
| [157](../modules/imu/BMI088reg.h#L157) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_PP (0x0 << BMI088_GYRO_INT3_GPIO_MODE_SHFITS)` |
| [158](../modules/imu/BMI088reg.h#L158) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_OD (0x1 << BMI088_GYRO_INT3_GPIO_MODE_SHFITS)` |
| [159](../modules/imu/BMI088reg.h#L159) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_LVL_SHFITS 0x0` |
| [160](../modules/imu/BMI088reg.h#L160) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_LOW (0x0 << BMI088_GYRO_INT3_GPIO_LVL_SHFITS)` |
| [161](../modules/imu/BMI088reg.h#L161) | 预处理 | `#define BMI088_GYRO_INT3_GPIO_HIGH (0x1 << BMI088_GYRO_INT3_GPIO_LVL_SHFITS)` |
| [163](../modules/imu/BMI088reg.h#L163) | 预处理 | `#define BMI088_GYRO_INT3_INT4_IO_MAP 0x18` |
| [165](../modules/imu/BMI088reg.h#L165) | 预处理 | `#define BMI088_GYRO_DRDY_IO_OFF 0x00` |
| [166](../modules/imu/BMI088reg.h#L166) | 预处理 | `#define BMI088_GYRO_DRDY_IO_INT3 0x01` |
| [167](../modules/imu/BMI088reg.h#L167) | 预处理 | `#define BMI088_GYRO_DRDY_IO_INT4 0x80` |
| [168](../modules/imu/BMI088reg.h#L168) | 预处理 | `#define BMI088_GYRO_DRDY_IO_BOTH (BMI088_GYRO_DRDY_IO_INT3 &#124; BMI088_GYRO_DRDY_IO_INT4)` |
| [170](../modules/imu/BMI088reg.h#L170) | 预处理 | `#define BMI088_GYRO_SELF_TEST 0x3C` |
| [171](../modules/imu/BMI088reg.h#L171) | 预处理 | `#define BMI088_GYRO_RATE_OK_SHFITS 0x4` |
| [172](../modules/imu/BMI088reg.h#L172) | 预处理 | `#define BMI088_GYRO_RATE_OK (0x1 << BMI088_GYRO_RATE_OK_SHFITS)` |
| [173](../modules/imu/BMI088reg.h#L173) | 预处理 | `#define BMI088_GYRO_BIST_FAIL_SHFITS 0x2` |
| [174](../modules/imu/BMI088reg.h#L174) | 预处理 | `#define BMI088_GYRO_BIST_FAIL (0x1 << BMI088_GYRO_BIST_FAIL_SHFITS)` |
| [175](../modules/imu/BMI088reg.h#L175) | 预处理 | `#define BMI088_GYRO_BIST_RDY_SHFITS 0x1` |
| [176](../modules/imu/BMI088reg.h#L176) | 预处理 | `#define BMI088_GYRO_BIST_RDY (0x1 << BMI088_GYRO_BIST_RDY_SHFITS)` |
| [177](../modules/imu/BMI088reg.h#L177) | 预处理 | `#define BMI088_GYRO_TRIG_BIST_SHFITS 0x0` |
| [178](../modules/imu/BMI088reg.h#L178) | 预处理 | `#define BMI088_GYRO_TRIG_BIST (0x1 << BMI088_GYRO_TRIG_BIST_SHFITS)` |
| [180](../modules/imu/BMI088reg.h#L180) | 预处理 | `#endif` |

## modules/imu/chassis_imu_can_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/chassis_imu_can_protocol.h#L1) | 预处理 | `#ifndef CHASSIS_IMU_CAN_PROTOCOL_H` |
| [2](../modules/imu/chassis_imu_can_protocol.h#L2) | 预处理 | `#define CHASSIS_IMU_CAN_PROTOCOL_H` |
| [6](../modules/imu/chassis_imu_can_protocol.h#L6) | 预处理 | `#define CHASSIS_IMU_CAN_DATA_ID 0x180U` |
| [7](../modules/imu/chassis_imu_can_protocol.h#L7) | 预处理 | `#define CHASSIS_IMU_CAN_FRAME_COUNT 9U` |
| [8](../modules/imu/chassis_imu_can_protocol.h#L8) | 预处理 | `#define CHASSIS_IMU_CAN_FRAME_BYTES 8U` |
| [9](../modules/imu/chassis_imu_can_protocol.h#L9) | 预处理 | `#define CHASSIS_IMU_CAN_PAYLOAD_BYTES 54U` |
| [38](../modules/imu/chassis_imu_can_protocol.h#L38) | 预处理 | `#endif` |

## modules/imu/chassis_imu_sample.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/chassis_imu_sample.h#L1) | 预处理 | `#ifndef CHASSIS_IMU_SAMPLE_H` |
| [2](../modules/imu/chassis_imu_sample.h#L2) | 预处理 | `#define CHASSIS_IMU_SAMPLE_H` |
| [8](../modules/imu/chassis_imu_sample.h#L8) | 预处理 | `#define CHASSIS_IMU_CAN_ASSEMBLY_MAX_MS 10U` |
| [27](../modules/imu/chassis_imu_sample.h#L27) | 预处理 | `#endif` |

## modules/imu/imu_sample.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/imu_sample.h#L1) | 预处理 | `#ifndef IMU_SAMPLE_H` |
| [2](../modules/imu/imu_sample.h#L2) | 预处理 | `#define IMU_SAMPLE_H` |
| [12](../modules/imu/imu_sample.h#L12) | 预处理 | `#endif` |

## modules/imu/ins_math.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/imu/ins_math.h#L1) | 预处理 | `#ifndef INS_MATH_H` |
| [2](../modules/imu/ins_math.h#L2) | 预处理 | `#define INS_MATH_H` |
| [6](../modules/imu/ins_math.h#L6) | 预处理 | `#endif` |

## modules/imu/ins_service.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [32](../modules/imu/ins_service.c#L32) | 预处理 | `#ifdef STM32F427xx` |
| [35](../modules/imu/ins_service.c#L35) | 预处理 | `#define INS_LOCK() taskENTER_CRITICAL()` |
| [36](../modules/imu/ins_service.c#L36) | 预处理 | `#define INS_UNLOCK() taskEXIT_CRITICAL()` |
| [37](../modules/imu/ins_service.c#L37) | 预处理 | `#else` |
| [38](../modules/imu/ins_service.c#L38) | 预处理 | `#define INS_LOCK() ((void)0)` |
| [39](../modules/imu/ins_service.c#L39) | 预处理 | `#define INS_UNLOCK() ((void)0)` |
| [40](../modules/imu/ins_service.c#L40) | 预处理 | `#endif` |
| [46](../modules/imu/ins_service.c#L46) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [49](../modules/imu/ins_service.c#L49) | 预处理 | `#endif` |
| [73](../modules/imu/ins_service.c#L73) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [83](../modules/imu/ins_service.c#L83) | 预处理 | `#endif` |
| [89](../modules/imu/ins_service.c#L89) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [91](../modules/imu/ins_service.c#L91) | 预处理 | `#endif` |
| [112](../modules/imu/ins_service.c#L112) | 配置初值 | `static PIDInstance TempCtrl = {0};` |
| [130](../modules/imu/ins_service.c#L130) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [151](../modules/imu/ins_service.c#L151) | 预处理 | `#endif` |
| [189](../modules/imu/ins_service.c#L189) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [191](../modules/imu/ins_service.c#L191) | 预处理 | `#endif` |
| [428](../modules/imu/ins_service.c#L428) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [431](../modules/imu/ins_service.c#L431) | 预处理 | `#endif` |
| [440](../modules/imu/ins_service.c#L440) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [447](../modules/imu/ins_service.c#L447) | 预处理 | `#endif` |

## modules/imu/ins_task.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [27](../modules/imu/ins_task.c#L27) | 配置初值 | `static PIDInstance TempCtrl = {0};` |

## modules/imu/ins_task.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [15](../modules/imu/ins_task.h#L15) | 预处理 | `#ifndef __INS_TASK_H` |
| [16](../modules/imu/ins_task.h#L16) | 预处理 | `#define __INS_TASK_H` |
| [26](../modules/imu/ins_task.h#L26) | 预处理 | `#define INS_TASK_PERIOD 1` |
| [49](../modules/imu/ins_task.h#L49) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE &#124;&#124; IMU_CAPTURE_ONLY` |
| [61](../modules/imu/ins_task.h#L61) | 预处理 | `#endif` |
| [167](../modules/imu/ins_task.h#L167) | 预处理 | `#endif` |

## modules/ist8310/ist8310.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [10](../modules/ist8310/ist8310.c#L10) | 预处理 | `#define IST8310_WRITE_REG_NUM 4     // 方便阅读` |
| [11](../modules/ist8310/ist8310.c#L11) | 预处理 | `#define IST8310_DATA_REG 0x03       // ist8310的数据寄存器` |
| [12](../modules/ist8310/ist8310.c#L12) | 预处理 | `#define IST8310_WHO_AM_I 0x00       // ist8310 id 寄存器值` |
| [13](../modules/ist8310/ist8310.c#L13) | 预处理 | `#define IST8310_WHO_AM_I_VALUE 0x10 // 用于检测是否连接成功,读取ist8310的whoami会返回该值` |

## modules/ist8310/ist8310.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [8](../modules/ist8310/ist8310.h#L8) | 预处理 | `#define MAG_SEN 0.3f // raw int16 data change to uT unit. 原始整型数据变成 单位ut` |
| [11](../modules/ist8310/ist8310.h#L11) | 预处理 | `#define IST8310_NO_ERROR 0x00    // seldom used. 一般不会用到` |
| [12](../modules/ist8310/ist8310.h#L12) | 预处理 | `#define IST8310_NO_SENSOR 0x40   // seldom used. 一般不会用到` |
| [13](../modules/ist8310/ist8310.h#L13) | 预处理 | `#define IST8310_IIC_ADDRESS 0x0E // the I2C slave address of IST8310` |

## modules/jam_detector/jam_detector.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/jam_detector/jam_detector.c#L11) | 预处理 | `#define DEFAULT_CURRENT_THRESHOLD   3000.0f  /**< 默认电流阈值, CAN原始值; 调用者应按对应电机的lCur实测调整 */` |
| [12](../modules/jam_detector/jam_detector.c#L12) | 预处理 | `#define DEFAULT_SUSPECT_TIMEOUT_MS  150.0f   /**< 默认嫌疑超时 150ms */` |
| [13](../modules/jam_detector/jam_detector.c#L13) | 预处理 | `#define DEFAULT_HANDLING_TIMEOUT_MS 800.0f   /**< 默认处理超时 800ms */` |
| [14](../modules/jam_detector/jam_detector.c#L14) | 预处理 | `#define DEFAULT_REVERSE_ANGLE       45.0f   /**< 默认回退角度 45° (电机轴) */` |
| [15](../modules/jam_detector/jam_detector.c#L15) | 预处理 | `#define DEFAULT_SPEED_THRESHOLD     200.0f  /**< 默认低速阈值 200°/s */` |
| [16](../modules/jam_detector/jam_detector.c#L16) | 预处理 | `#define DEFAULT_MIN_ANGLE_ERROR     100.0f  /**< 默认最小角度误差 100° (电机轴) */` |
| [17](../modules/jam_detector/jam_detector.c#L17) | 预处理 | `#define DEFAULT_MIN_SPEED_REFERENCE 100.0f  /**< 默认最小速度指令 100°/s */` |
| [18](../modules/jam_detector/jam_detector.c#L18) | 预处理 | `#define JAM_ANGLE_TOLERANCE         8.0f    /**< 认为已到达回退目标的角度误差 */` |
| [19](../modules/jam_detector/jam_detector.c#L19) | 预处理 | `#define JAM_HANDLING_SPEED_TOLERANCE 200.0f /**< 回退完成时允许的最大电机轴速度 */` |

## modules/jam_detector/jam_detector.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/jam_detector/jam_detector.h#L1) | 预处理 | `#ifndef JAM_DETECTOR_H` |
| [2](../modules/jam_detector/jam_detector.h#L2) | 预处理 | `#define JAM_DETECTOR_H` |
| [103](../modules/jam_detector/jam_detector.h#L103) | 预处理 | `#endif` |

## modules/jscope/jscope.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../modules/jscope/jscope.c#L12) | 预处理 | `#define JSCOPE_BUFFER_SIZE 1024U` |
| [13](../modules/jscope/jscope.c#L13) | 预处理 | `#define JSCOPE_ENTER_CRITICAL() uint32_t interrupt_mask=__get_PRIMASK(); __disable_irq()` |
| [14](../modules/jscope/jscope.c#L14) | 预处理 | `#define JSCOPE_EXIT_CRITICAL()  __set_PRIMASK(interrupt_mask)` |
| [30](../modules/jscope/jscope.c#L30) | 预处理 | `#if CHASSIS_SHADOW_JSCOPE_ENABLE` |
| [33](../modules/jscope/jscope.c#L33) | 预处理 | `#else` |
| [35](../modules/jscope/jscope.c#L35) | 预处理 | `#endif` |

## modules/jscope/jscope.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/jscope/jscope.h#L1) | 预处理 | `#ifndef __JSCOPE_H__` |
| [2](../modules/jscope/jscope.h#L2) | 预处理 | `#define __JSCOPE_H__` |
| [6](../modules/jscope/jscope.h#L6) | 预处理 | `#ifdef __cplusplus` |
| [8](../modules/jscope/jscope.h#L8) | 预处理 | `#endif` |
| [10](../modules/jscope/jscope.h#L10) | 预处理 | `#define JSCOPE_CHANNEL_COUNT 4U` |
| [12](../modules/jscope/jscope.h#L12) | 预处理 | `#define JSCOPE_RTT_BUFFER_INDEX 1U` |
| [13](../modules/jscope/jscope.h#L13) | 预处理 | `#define JSCOPE_RTT_BUFFER_NAME "JScope_f4f4f4f4"` |
| [47](../modules/jscope/jscope.h#L47) | 预处理 | `#ifdef __cplusplus` |
| [49](../modules/jscope/jscope.h#L49) | 预处理 | `#endif` |
| [51](../modules/jscope/jscope.h#L51) | 预处理 | `#endif` |

## modules/key/key.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/key/key.h#L1) | 预处理 | `#ifndef KEY_H` |
| [2](../modules/key/key.h#L2) | 预处理 | `#define KEY_H` |
| [4](../modules/key/key.h#L4) | 预处理 | `#define KEY_CNT 4` |
| [75](../modules/key/key.h#L75) | 预处理 | `#endif // !KEY_H` |

## modules/master_machine/master_process.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../modules/master_machine/master_process.c#L21) | 预处理 | `#if defined(VISION_USE_UART) &#124;&#124; (defined(VISION_USE_VCP) && !USB_TELEMETRY_ENABLE)` |
| [23](../modules/master_machine/master_process.c#L23) | 预处理 | `#ifdef VISION_USE_UART` |
| [25](../modules/master_machine/master_process.c#L25) | 预处理 | `#endif` |
| [26](../modules/master_machine/master_process.c#L26) | 预处理 | `#endif` |
| [49](../modules/master_machine/master_process.c#L49) | 预处理 | `#if defined(VISION_USE_UART) &#124;&#124; (defined(VISION_USE_VCP) && !USB_TELEMETRY_ENABLE)` |
| [52](../modules/master_machine/master_process.c#L52) | 预处理 | `#ifdef VISION_USE_UART` |
| [54](../modules/master_machine/master_process.c#L54) | 预处理 | `#endif // !VISION_USE_UART` |
| [57](../modules/master_machine/master_process.c#L57) | 预处理 | `#endif` |
| [59](../modules/master_machine/master_process.c#L59) | 预处理 | `#ifdef VISION_USE_UART` |
| [87](../modules/master_machine/master_process.c#L87) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [88](../modules/master_machine/master_process.c#L88) | 配置初值 | `.callback = VisionOfflineCallback, // 离线时调用的回调函数,会重启串口接收` |
| [89](../modules/master_machine/master_process.c#L89) | 配置初值 | `.owner_id = vision_usart_instance,` |
| [90](../modules/master_machine/master_process.c#L90) | 配置初值 | `.reload_count = 10,` |
| [121](../modules/master_machine/master_process.c#L121) | 预处理 | `#endif // VISION_USE_UART` |
| [123](../modules/master_machine/master_process.c#L123) | 预处理 | `#if defined(VISION_USE_VCP) && !USB_TELEMETRY_ENABLE` |
| [139](../modules/master_machine/master_process.c#L139) | 配置初值 | `USB_Init_Config_s conf = {.rx_cbk = DecodeVision};` |
| [143](../modules/master_machine/master_process.c#L143) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [144](../modules/master_machine/master_process.c#L144) | 配置初值 | `.callback = VisionOfflineCallback, // 离线时调用的回调函数,会重启串口接收` |
| [145](../modules/master_machine/master_process.c#L145) | 配置初值 | `.owner_id = NULL,` |
| [146](../modules/master_machine/master_process.c#L146) | 配置初值 | `.reload_count = 5, // 50ms` |
| [165](../modules/master_machine/master_process.c#L165) | 预处理 | `#endif // VISION_USE_VCP` |
| [167](../modules/master_machine/master_process.c#L167) | 预处理 | `#if defined(VISION_USE_VCP) && USB_TELEMETRY_ENABLE && !defined(VISION_USE_UART)` |
| [175](../modules/master_machine/master_process.c#L175) | 预处理 | `#endif` |

## modules/master_machine/master_process.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/master_machine/master_process.h#L1) | 预处理 | `#ifndef MASTER_PROCESS_H` |
| [2](../modules/master_machine/master_process.h#L2) | 预处理 | `#define MASTER_PROCESS_H` |
| [7](../modules/master_machine/master_process.h#L7) | 预处理 | `#define VISION_RECV_SIZE 18u // 当前为固定值,36字节` |
| [8](../modules/master_machine/master_process.h#L8) | 预处理 | `#define VISION_SEND_SIZE 36u` |
| [114](../modules/master_machine/master_process.h#L114) | 预处理 | `#endif // !MASTER_PROCESS_H` |

## modules/master_machine/seasky_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/master_machine/seasky_protocol.h#L1) | 预处理 | `#ifndef __SEASKY_PROTOCOL_H` |
| [2](../modules/master_machine/seasky_protocol.h#L2) | 预处理 | `#define __SEASKY_PROTOCOL_H` |
| [7](../modules/master_machine/seasky_protocol.h#L7) | 预处理 | `#define PROTOCOL_CMD_ID 0XA5` |
| [8](../modules/master_machine/seasky_protocol.h#L8) | 预处理 | `#define OFFSET_BYTE 8 // 出数据段外，其他部分所占字节数` |
| [36](../modules/master_machine/seasky_protocol.h#L36) | 预处理 | `#endif` |

## modules/message_center/message_center.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../modules/message_center/message_center.h#L12) | 预处理 | `#ifndef PUBSUB_H` |
| [13](../modules/message_center/message_center.h#L13) | 预处理 | `#define PUBSUB_H` |
| [17](../modules/message_center/message_center.h#L17) | 预处理 | `#define MAX_TOPIC_NAME_LEN 32 // 最大的话题名长度,每个话题都有字符串来命名` |
| [18](../modules/message_center/message_center.h#L18) | 预处理 | `#define MAX_TOPIC_COUNT 12    // 最多支持的话题数量` |
| [19](../modules/message_center/message_center.h#L19) | 预处理 | `#define QUEUE_SIZE 1` |
| [85](../modules/message_center/message_center.h#L85) | 预处理 | `#endif // !PUBSUB_H` |

## modules/motor/DJImotor/dji_force_adapter.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/motor/DJImotor/dji_force_adapter.c#L11) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [36](../modules/motor/DJImotor/dji_force_adapter.c#L36) | 预处理 | `#else` |
| [55](../modules/motor/DJImotor/dji_force_adapter.c#L55) | 预处理 | `#if POWER_METER_ENABLE` |
| [57](../modules/motor/DJImotor/dji_force_adapter.c#L57) | 预处理 | `#endif` |
| [180](../modules/motor/DJImotor/dji_force_adapter.c#L180) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_force_adapter.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/DJImotor/dji_force_adapter.h#L1) | 预处理 | `#ifndef DJI_FORCE_ADAPTER_H` |
| [2](../modules/motor/DJImotor/dji_force_adapter.h#L2) | 预处理 | `#define DJI_FORCE_ADAPTER_H` |
| [15](../modules/motor/DJImotor/dji_force_adapter.h#L15) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_force_calibration.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/DJImotor/dji_force_calibration.h#L1) | 预处理 | `#ifndef DJI_FORCE_CALIBRATION_H` |
| [2](../modules/motor/DJImotor/dji_force_calibration.h#L2) | 预处理 | `#define DJI_FORCE_CALIBRATION_H` |
| [28](../modules/motor/DJImotor/dji_force_calibration.h#L28) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_motor.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [43](../modules/motor/DJImotor/dji_motor.c#L43) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [56](../modules/motor/DJImotor/dji_motor.c#L56) | 预处理 | `#endif` |
| [216](../modules/motor/DJImotor/dji_motor.c#L216) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [238](../modules/motor/DJImotor/dji_motor.c#L238) | 预处理 | `#endif` |
| [247](../modules/motor/DJImotor/dji_motor.c#L247) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [251](../modules/motor/DJImotor/dji_motor.c#L251) | 预处理 | `#endif` |
| [272](../modules/motor/DJImotor/dji_motor.c#L272) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [274](../modules/motor/DJImotor/dji_motor.c#L274) | 预处理 | `#else` |
| [276](../modules/motor/DJImotor/dji_motor.c#L276) | 预处理 | `#endif` |
| [278](../modules/motor/DJImotor/dji_motor.c#L278) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE &#124;&#124; CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE &#124;&#124; CHASSIS_VELOCITY_FUSION_ENABLE` |
| [281](../modules/motor/DJImotor/dji_motor.c#L281) | 预处理 | `#if CHASSIS_VELOCITY_FUSION_ENABLE` |
| [283](../modules/motor/DJImotor/dji_motor.c#L283) | 预处理 | `#else` |
| [285](../modules/motor/DJImotor/dji_motor.c#L285) | 预处理 | `#endif` |
| [286](../modules/motor/DJImotor/dji_motor.c#L286) | 预处理 | `#endif` |
| [287](../modules/motor/DJImotor/dji_motor.c#L287) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [289](../modules/motor/DJImotor/dji_motor.c#L289) | 预处理 | `#endif` |
| [290](../modules/motor/DJImotor/dji_motor.c#L290) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [294](../modules/motor/DJImotor/dji_motor.c#L294) | 预处理 | `#endif` |
| [339](../modules/motor/DJImotor/dji_motor.c#L339) | 配置初值 | `Daemon_Init_Config_s daemon_config = {` |
| [340](../modules/motor/DJImotor/dji_motor.c#L340) | 配置初值 | `.callback = DJIMotorLostCallback,` |
| [341](../modules/motor/DJImotor/dji_motor.c#L341) | 配置初值 | `.owner_id = instance,` |
| [342](../modules/motor/DJImotor/dji_motor.c#L342) | 配置初值 | `.reload_count = 2, // 20ms未收到数据则丢失` |
| [575](../modules/motor/DJImotor/dji_motor.c#L575) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [577](../modules/motor/DJImotor/dji_motor.c#L577) | 预处理 | `#endif` |
| [580](../modules/motor/DJImotor/dji_motor.c#L580) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [582](../modules/motor/DJImotor/dji_motor.c#L582) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_motor.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [15](../modules/motor/DJImotor/dji_motor.h#L15) | 预处理 | `#ifndef DJI_MOTOR_H` |
| [16](../modules/motor/DJImotor/dji_motor.h#L16) | 预处理 | `#define DJI_MOTOR_H` |
| [28](../modules/motor/DJImotor/dji_motor.h#L28) | 预处理 | `#define DJI_SENDER_GROUP_COUNT 6U` |
| [30](../modules/motor/DJImotor/dji_motor.h#L30) | 预处理 | `#define ECD_ANGLE_COEF_DJI 0.043945f // (360/8192),将编码器值转化为角度制` |
| [46](../modules/motor/DJImotor/dji_motor.h#L46) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE &#124;&#124; CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE &#124;&#124; CHASSIS_VELOCITY_FUSION_ENABLE` |
| [49](../modules/motor/DJImotor/dji_motor.h#L49) | 预处理 | `#endif` |
| [52](../modules/motor/DJImotor/dji_motor.h#L52) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [57](../modules/motor/DJImotor/dji_motor.h#L57) | 预处理 | `#endif` |
| [81](../modules/motor/DJImotor/dji_motor.h#L81) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [84](../modules/motor/DJImotor/dji_motor.h#L84) | 预处理 | `#endif` |
| [221](../modules/motor/DJImotor/dji_motor.h#L221) | 预处理 | `#endif // !DJI_MOTOR_H` |

## modules/motor/DJImotor/dji_power_adapter.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [20](../modules/motor/DJImotor/dji_power_adapter.c#L20) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [22](../modules/motor/DJImotor/dji_power_adapter.c#L22) | 预处理 | `#endif` |
| [23](../modules/motor/DJImotor/dji_power_adapter.c#L23) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [33](../modules/motor/DJImotor/dji_power_adapter.c#L33) | 预处理 | `#endif` |
| [47](../modules/motor/DJImotor/dji_power_adapter.c#L47) | 预处理 | `#if POWER_METER_ENABLE` |
| [49](../modules/motor/DJImotor/dji_power_adapter.c#L49) | 预处理 | `#endif` |
| [51](../modules/motor/DJImotor/dji_power_adapter.c#L51) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [54](../modules/motor/DJImotor/dji_power_adapter.c#L54) | 预处理 | `#endif` |
| [63](../modules/motor/DJImotor/dji_power_adapter.c#L63) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE &#124;&#124; CHASSIS_SPEED_KF_SHADOW_ENABLE` |
| [70](../modules/motor/DJImotor/dji_power_adapter.c#L70) | 预处理 | `#endif` |
| [76](../modules/motor/DJImotor/dji_power_adapter.c#L76) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [78](../modules/motor/DJImotor/dji_power_adapter.c#L78) | 预处理 | `#endif` |
| [114](../modules/motor/DJImotor/dji_power_adapter.c#L114) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [116](../modules/motor/DJImotor/dji_power_adapter.c#L116) | 预处理 | `#endif` |
| [169](../modules/motor/DJImotor/dji_power_adapter.c#L169) | 预处理 | `#if CHASSIS_SPEED_CAPTURE_ENABLE` |
| [331](../modules/motor/DJImotor/dji_power_adapter.c#L331) | 预处理 | `#else` |
| [336](../modules/motor/DJImotor/dji_power_adapter.c#L336) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_power_adapter.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/DJImotor/dji_power_adapter.h#L1) | 预处理 | `#ifndef DJI_POWER_ADAPTER_H` |
| [2](../modules/motor/DJImotor/dji_power_adapter.h#L2) | 预处理 | `#define DJI_POWER_ADAPTER_H` |
| [23](../modules/motor/DJImotor/dji_power_adapter.h#L23) | 预处理 | `#endif` |

## modules/motor/DJImotor/dji_tuning_adapter.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [29](../modules/motor/DJImotor/dji_tuning_adapter.c#L29) | 配置初值 | `const PIDAutotuneFeedback denied={.now_ms=now,.permitted=false};` |
| [35](../modules/motor/DJImotor/dji_tuning_adapter.c#L35) | 配置初值 | `PIDAutotuneFeedback f={.now_ms=now,.can_ok=true,.feedback_ok=true,` |
| [36](../modules/motor/DJImotor/dji_tuning_adapter.c#L36) | 配置初值 | `.permitted=VehicleSafetyAllowsAutotune(now) && tuning_control->autotune_permitted && tuning_control->control_enabled &&` |
| [37](../modules/motor/DJImotor/dji_tuning_adapter.c#L37) | 配置初值 | `(uint32_t)(now-tuning_control->tick_ms)<=10 && !autotune_inhibit};` |
| [46](../modules/motor/DJImotor/dji_tuning_adapter.c#L46) | 配置初值 | `AutotuneConfiguration config={.improve=pid->Improve,.coef_a=pid->CoefA,.coef_b=pid->CoefB,` |
| [47](../modules/motor/DJImotor/dji_tuning_adapter.c#L47) | 配置初值 | `.output_rc=pid->Output_LPF_RC,.derivative_rc=pid->Derivative_LPF_RC,` |
| [48](../modules/motor/DJImotor/dji_tuning_adapter.c#L48) | 配置初值 | `.speed_smooth=SPEED_SMOOTH_COEF,.current_smooth=CURRENT_SMOOTH_COEF,` |
| [49](../modules/motor/DJImotor/dji_tuning_adapter.c#L49) | 配置初值 | `.output_limit=fminf(pid->MaxOut,tuning_group->max_current_command),` |
| [50](../modules/motor/DJImotor/dji_tuning_adapter.c#L50) | 配置初值 | `.motor_flags=(uint32_t)settings->close_loop_type &#124; ((uint32_t)settings->outer_loop_type<<8) &#124;` |
| [51](../modules/motor/DJImotor/dji_tuning_adapter.c#L51) | 配置初值 | `((uint32_t)settings->motor_reverse_flag<<16) &#124; ((uint32_t)settings->feedback_reverse_flag<<20) &#124;` |
| [52](../modules/motor/DJImotor/dji_tuning_adapter.c#L52) | 配置初值 | `((uint32_t)settings->speed_feedback_source<<24) &#124; ((uint32_t)settings->feedforward_flag<<28)};` |

## modules/motor/DJImotor/dji_tuning_adapter.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/DJImotor/dji_tuning_adapter.h#L1) | 预处理 | `#ifndef DJI_TUNING_ADAPTER_H` |
| [2](../modules/motor/DJImotor/dji_tuning_adapter.h#L2) | 预处理 | `#define DJI_TUNING_ADAPTER_H` |
| [9](../modules/motor/DJImotor/dji_tuning_adapter.h#L9) | 预处理 | `#endif` |

## modules/motor/DMmotor/dmmotor.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [83](../modules/motor/DMmotor/dmmotor.c#L83) | 配置初值 | `Daemon_Init_Config_s conf = {` |
| [84](../modules/motor/DMmotor/dmmotor.c#L84) | 配置初值 | `.callback = DMMotorLostCallback,` |
| [85](../modules/motor/DMmotor/dmmotor.c#L85) | 配置初值 | `.owner_id = motor,` |
| [86](../modules/motor/DMmotor/dmmotor.c#L86) | 配置初值 | `.reload_count = 10,` |

## modules/motor/DMmotor/dmmotor.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/DMmotor/dmmotor.h#L1) | 预处理 | `#ifndef DMMOTOR_H` |
| [2](../modules/motor/DMmotor/dmmotor.h#L2) | 预处理 | `#define DMMOTOR_H` |
| [9](../modules/motor/DMmotor/dmmotor.h#L9) | 预处理 | `#define DM_MOTOR_CNT 4` |
| [11](../modules/motor/DMmotor/dmmotor.h#L11) | 预处理 | `#define DM_P_MIN  (-12.5f)` |
| [12](../modules/motor/DMmotor/dmmotor.h#L12) | 预处理 | `#define DM_P_MAX  12.5f` |
| [13](../modules/motor/DMmotor/dmmotor.h#L13) | 预处理 | `#define DM_V_MIN  (-45.0f)` |
| [14](../modules/motor/DMmotor/dmmotor.h#L14) | 预处理 | `#define DM_V_MAX  45.0f` |
| [15](../modules/motor/DMmotor/dmmotor.h#L15) | 预处理 | `#define DM_T_MIN  (-18.0f)` |
| [16](../modules/motor/DMmotor/dmmotor.h#L16) | 预处理 | `#define DM_T_MAX   18.0f` |
| [76](../modules/motor/DMmotor/dmmotor.h#L76) | 预处理 | `#endif // !DMMOTOR` |

## modules/motor/HTmotor/HT04.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [122](../modules/motor/HTmotor/HT04.c#L122) | 配置初值 | `Daemon_Init_Config_s conf = {` |
| [123](../modules/motor/HTmotor/HT04.c#L123) | 配置初值 | `.callback = HTMotorLostCallback,` |
| [124](../modules/motor/HTmotor/HT04.c#L124) | 配置初值 | `.owner_id = motor,` |
| [125](../modules/motor/HTmotor/HT04.c#L125) | 配置初值 | `.reload_count = 5, // 50ms` |

## modules/motor/HTmotor/HT04.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/HTmotor/HT04.h#L1) | 预处理 | `#ifndef HT04_H` |
| [2](../modules/motor/HTmotor/HT04.h#L2) | 预处理 | `#define HT04_H` |
| [10](../modules/motor/HTmotor/HT04.h#L10) | 预处理 | `#define HT_MOTOR_CNT 4` |
| [11](../modules/motor/HTmotor/HT04.h#L11) | 预处理 | `#define CURRENT_SMOOTH_COEF 0.9f` |
| [12](../modules/motor/HTmotor/HT04.h#L12) | 预处理 | `#define SPEED_BUFFER_SIZE 5` |
| [13](../modules/motor/HTmotor/HT04.h#L13) | 预处理 | `#define HT_SPEED_BIAS -0.0109901428f // 电机速度偏差,单位rad/s` |
| [14](../modules/motor/HTmotor/HT04.h#L14) | 预处理 | `#define TORQUE_COEF_HT 3.5  // 扭矩系数,单位N.m/A` |
| [16](../modules/motor/HTmotor/HT04.h#L16) | 预处理 | `#define P_MIN -95.5f // Radians` |
| [17](../modules/motor/HTmotor/HT04.h#L17) | 预处理 | `#define P_MAX 95.5f` |
| [18](../modules/motor/HTmotor/HT04.h#L18) | 预处理 | `#define V_MIN -45.0f // Rad/s` |
| [19](../modules/motor/HTmotor/HT04.h#L19) | 预处理 | `#define V_MAX 45.0f` |
| [20](../modules/motor/HTmotor/HT04.h#L20) | 预处理 | `#define T_MIN -18.0f // N·m` |
| [21](../modules/motor/HTmotor/HT04.h#L21) | 预处理 | `#define T_MAX 18.0f` |
| [22](../modules/motor/HTmotor/HT04.h#L22) | 预处理 | `#define KP_MIN 0.0f // N-m/rad` |
| [23](../modules/motor/HTmotor/HT04.h#L23) | 预处理 | `#define KP_MAX 500.0f` |
| [24](../modules/motor/HTmotor/HT04.h#L24) | 预处理 | `#define KD_MIN 0.0f // N-m/rad/s` |
| [25](../modules/motor/HTmotor/HT04.h#L25) | 预处理 | `#define KD_MAX 5.0f` |
| [124](../modules/motor/HTmotor/HT04.h#L124) | 预处理 | `#endif // !HT04_H#define HT04_H` |

## modules/motor/LKmotor/LK9025.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [82](../modules/motor/LKmotor/LK9025.c#L82) | 配置初值 | `Daemon_Init_Config_s daemon_config = {` |
| [83](../modules/motor/LKmotor/LK9025.c#L83) | 配置初值 | `.callback = LKMotorLostCallback,` |
| [84](../modules/motor/LKmotor/LK9025.c#L84) | 配置初值 | `.owner_id = motor,` |
| [85](../modules/motor/LKmotor/LK9025.c#L85) | 配置初值 | `.reload_count = 5, // 50ms` |

## modules/motor/LKmotor/LK9025.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/LKmotor/LK9025.h#L1) | 预处理 | `#ifndef LK9025_H` |
| [2](../modules/motor/LKmotor/LK9025.h#L2) | 预处理 | `#define LK9025_H` |
| [10](../modules/motor/LKmotor/LK9025.h#L10) | 预处理 | `#define LK_MOTOR_MX_CNT 4 // 最多允许4个LK电机使用多电机指令,挂载在一条总线上` |
| [12](../modules/motor/LKmotor/LK9025.h#L12) | 预处理 | `#define I_MIN -2000` |
| [13](../modules/motor/LKmotor/LK9025.h#L13) | 预处理 | `#define I_MAX 2000` |
| [14](../modules/motor/LKmotor/LK9025.h#L14) | 预处理 | `#define CURRENT_SMOOTH_COEF 0.9f` |
| [15](../modules/motor/LKmotor/LK9025.h#L15) | 预处理 | `#define SPEED_SMOOTH_COEF 0.85f` |
| [16](../modules/motor/LKmotor/LK9025.h#L16) | 预处理 | `#define REDUCTION_RATIO_DRIVEN 1` |
| [17](../modules/motor/LKmotor/LK9025.h#L17) | 预处理 | `#define ECD_ANGLE_COEF_LK (360.0f / 65536.0f)` |
| [18](../modules/motor/LKmotor/LK9025.h#L18) | 预处理 | `#define CURRENT_TORQUE_COEF_LK 0.00512f  // 电流设定值转换成扭矩的系数，这里对应的是16T` |
| [98](../modules/motor/LKmotor/LK9025.h#L98) | 预处理 | `#endif // LK9025_H` |

## modules/motor/motor_def.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../modules/motor/motor_def.h#L12) | 预处理 | `#ifndef MOTOR_DEF_H` |
| [13](../modules/motor/motor_def.h#L13) | 预处理 | `#define MOTOR_DEF_H` |
| [18](../modules/motor/motor_def.h#L18) | 预处理 | `#define LIMIT_MIN_MAX(x, min, max) (x) = (((x) <= (min)) ? (min) : (((x) >= (max)) ? (max) : (x)))` |
| [139](../modules/motor/motor_def.h#L139) | 预处理 | `#endif // !MOTOR_DEF_H` |

## modules/motor/motor_task.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/motor/motor_task.h#L11) | 预处理 | `#ifndef MOTOR_TASK_H` |
| [12](../modules/motor/motor_task.h#L12) | 预处理 | `#define MOTOR_TASK_H` |
| [24](../modules/motor/motor_task.h#L24) | 预处理 | `#endif // !MOTOR_TASK_H` |

## modules/motor/servo_motor/servo_motor.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/motor/servo_motor/servo_motor.h#L1) | 预处理 | `#ifndef SERVO_MOTOR_H` |
| [2](../modules/motor/servo_motor/servo_motor.h#L2) | 预处理 | `#define SERVO_MOTOR_H` |
| [10](../modules/motor/servo_motor/servo_motor.h#L10) | 预处理 | `#define SERVO_MOTOR_CNT 7` |
| [11](../modules/motor/servo_motor/servo_motor.h#L11) | 预处理 | `#define Servo_Frame_First 0x55` |
| [12](../modules/motor/servo_motor/servo_motor.h#L12) | 预处理 | `#define Servo_Frame_Second 0x55` |
| [13](../modules/motor/servo_motor/servo_motor.h#L13) | 预处理 | `#define Servo_MAX_BUFF 10` |
| [14](../modules/motor/servo_motor/servo_motor.h#L14) | 预处理 | `#define SERVO_MOVE_CMD 0x03` |
| [15](../modules/motor/servo_motor/servo_motor.h#L15) | 预处理 | `#define SERVO_UNLOAD_CMD 0x14` |
| [16](../modules/motor/servo_motor/servo_motor.h#L16) | 预处理 | `#define SERVO_POS_READ_CMD 0x15` |
| [45](../modules/motor/servo_motor/servo_motor.h#L45) | 预处理 | `#endif // SERVO_MOTOR_H` |

## modules/mpu6500/imu_temperature.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/mpu6500/imu_temperature.h#L1) | 预处理 | `#ifndef IMU_TEMPERATURE_H` |
| [2](../modules/mpu6500/imu_temperature.h#L2) | 预处理 | `#define IMU_TEMPERATURE_H` |
| [9](../modules/mpu6500/imu_temperature.h#L9) | 预处理 | `#endif` |

## modules/mpu6500/ist8310_reg.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/mpu6500/ist8310_reg.h#L1) | 预处理 | `#ifndef IST8310_REG_H` |
| [2](../modules/mpu6500/ist8310_reg.h#L2) | 预处理 | `#define IST8310_REG_H` |
| [5](../modules/mpu6500/ist8310_reg.h#L5) | 预处理 | `#define IST8310_ADDRESS 0x0E` |
| [6](../modules/mpu6500/ist8310_reg.h#L6) | 预处理 | `#define IST8310_DEVICE_ID_A 0x10` |
| [9](../modules/mpu6500/ist8310_reg.h#L9) | 预处理 | `#define IST8310_WHO_AM_I 0x00` |
| [10](../modules/mpu6500/ist8310_reg.h#L10) | 预处理 | `#define IST8310_R_CONFA 0x0A` |
| [11](../modules/mpu6500/ist8310_reg.h#L11) | 预处理 | `#define IST8310_R_CONFB 0x0B` |
| [12](../modules/mpu6500/ist8310_reg.h#L12) | 预处理 | `#define IST8310_R_MODE 0x02` |
| [14](../modules/mpu6500/ist8310_reg.h#L14) | 预处理 | `#define IST8310_R_XL 0x03` |
| [15](../modules/mpu6500/ist8310_reg.h#L15) | 预处理 | `#define IST8310_R_XM 0x04` |
| [16](../modules/mpu6500/ist8310_reg.h#L16) | 预处理 | `#define IST8310_R_YL 0x05` |
| [17](../modules/mpu6500/ist8310_reg.h#L17) | 预处理 | `#define IST8310_R_YM 0x06` |
| [18](../modules/mpu6500/ist8310_reg.h#L18) | 预处理 | `#define IST8310_R_ZL 0x07` |
| [19](../modules/mpu6500/ist8310_reg.h#L19) | 预处理 | `#define IST8310_R_ZM 0x08` |
| [21](../modules/mpu6500/ist8310_reg.h#L21) | 预处理 | `#define IST8310_AVGCNTL 0x41` |
| [22](../modules/mpu6500/ist8310_reg.h#L22) | 预处理 | `#define IST8310_PDCNTL 0x42` |
| [24](../modules/mpu6500/ist8310_reg.h#L24) | 预处理 | `#define IST8310_ODR_MODE 0x01 //sigle measure mode` |
| [27](../modules/mpu6500/ist8310_reg.h#L27) | 预处理 | `#endif // !IST` |

## modules/mpu6500/mpu6500_math.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/mpu6500/mpu6500_math.h#L1) | 预处理 | `#ifndef MPU6500_MATH_H` |
| [2](../modules/mpu6500/mpu6500_math.h#L2) | 预处理 | `#define MPU6500_MATH_H` |
| [9](../modules/mpu6500/mpu6500_math.h#L9) | 预处理 | `#endif` |

## modules/mpu6500/mpu6500_reg.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/mpu6500/mpu6500_reg.h#L1) | 预处理 | `#ifndef MPU6500_REG_H` |
| [2](../modules/mpu6500/mpu6500_reg.h#L2) | 预处理 | `#define MPU6500_REG_H` |
| [5](../modules/mpu6500/mpu6500_reg.h#L5) | 预处理 | `#define MPU6500_SELF_TEST_XG        (0x00)` |
| [6](../modules/mpu6500/mpu6500_reg.h#L6) | 预处理 | `#define MPU6500_SELF_TEST_YG        (0x01)` |
| [7](../modules/mpu6500/mpu6500_reg.h#L7) | 预处理 | `#define MPU6500_SELF_TEST_ZG        (0x02)` |
| [8](../modules/mpu6500/mpu6500_reg.h#L8) | 预处理 | `#define MPU6500_SELF_TEST_XA        (0x0D)` |
| [9](../modules/mpu6500/mpu6500_reg.h#L9) | 预处理 | `#define MPU6500_SELF_TEST_YA        (0x0E)` |
| [10](../modules/mpu6500/mpu6500_reg.h#L10) | 预处理 | `#define MPU6500_SELF_TEST_ZA        (0x0F)` |
| [11](../modules/mpu6500/mpu6500_reg.h#L11) | 预处理 | `#define MPU6500_XG_OFFSET_H         (0x13)` |
| [12](../modules/mpu6500/mpu6500_reg.h#L12) | 预处理 | `#define MPU6500_XG_OFFSET_L         (0x14)` |
| [13](../modules/mpu6500/mpu6500_reg.h#L13) | 预处理 | `#define MPU6500_YG_OFFSET_H         (0x15)` |
| [14](../modules/mpu6500/mpu6500_reg.h#L14) | 预处理 | `#define MPU6500_YG_OFFSET_L         (0x16)` |
| [15](../modules/mpu6500/mpu6500_reg.h#L15) | 预处理 | `#define MPU6500_ZG_OFFSET_H         (0x17)` |
| [16](../modules/mpu6500/mpu6500_reg.h#L16) | 预处理 | `#define MPU6500_ZG_OFFSET_L         (0x18)` |
| [17](../modules/mpu6500/mpu6500_reg.h#L17) | 预处理 | `#define MPU6500_SMPLRT_DIV          (0x19)` |
| [18](../modules/mpu6500/mpu6500_reg.h#L18) | 预处理 | `#define MPU6500_CONFIG              (0x1A)` |
| [19](../modules/mpu6500/mpu6500_reg.h#L19) | 预处理 | `#define MPU6500_GYRO_CONFIG         (0x1B)` |
| [20](../modules/mpu6500/mpu6500_reg.h#L20) | 预处理 | `#define MPU6500_ACCEL_CONFIG        (0x1C)` |
| [21](../modules/mpu6500/mpu6500_reg.h#L21) | 预处理 | `#define MPU6500_ACCEL_CONFIG_2      (0x1D)` |
| [22](../modules/mpu6500/mpu6500_reg.h#L22) | 预处理 | `#define MPU6500_LP_ACCEL_ODR        (0x1E)` |
| [23](../modules/mpu6500/mpu6500_reg.h#L23) | 预处理 | `#define MPU6500_MOT_THR             (0x1F)` |
| [24](../modules/mpu6500/mpu6500_reg.h#L24) | 预处理 | `#define MPU6500_FIFO_EN             (0x23)` |
| [25](../modules/mpu6500/mpu6500_reg.h#L25) | 预处理 | `#define MPU6500_I2C_MST_CTRL        (0x24)` |
| [26](../modules/mpu6500/mpu6500_reg.h#L26) | 预处理 | `#define MPU6500_I2C_SLV0_ADDR       (0x25)` |
| [27](../modules/mpu6500/mpu6500_reg.h#L27) | 预处理 | `#define MPU6500_I2C_SLV0_REG        (0x26)` |
| [28](../modules/mpu6500/mpu6500_reg.h#L28) | 预处理 | `#define MPU6500_I2C_SLV0_CTRL       (0x27)` |
| [29](../modules/mpu6500/mpu6500_reg.h#L29) | 预处理 | `#define MPU6500_I2C_SLV1_ADDR       (0x28)` |
| [30](../modules/mpu6500/mpu6500_reg.h#L30) | 预处理 | `#define MPU6500_I2C_SLV1_REG        (0x29)` |
| [31](../modules/mpu6500/mpu6500_reg.h#L31) | 预处理 | `#define MPU6500_I2C_SLV1_CTRL       (0x2A)` |
| [32](../modules/mpu6500/mpu6500_reg.h#L32) | 预处理 | `#define MPU6500_I2C_SLV2_ADDR       (0x2B)` |
| [33](../modules/mpu6500/mpu6500_reg.h#L33) | 预处理 | `#define MPU6500_I2C_SLV2_REG        (0x2C)` |
| [34](../modules/mpu6500/mpu6500_reg.h#L34) | 预处理 | `#define MPU6500_I2C_SLV2_CTRL       (0x2D)` |
| [35](../modules/mpu6500/mpu6500_reg.h#L35) | 预处理 | `#define MPU6500_I2C_SLV3_ADDR       (0x2E)` |
| [36](../modules/mpu6500/mpu6500_reg.h#L36) | 预处理 | `#define MPU6500_I2C_SLV3_REG        (0x2F)` |
| [37](../modules/mpu6500/mpu6500_reg.h#L37) | 预处理 | `#define MPU6500_I2C_SLV3_CTRL       (0x30)` |
| [38](../modules/mpu6500/mpu6500_reg.h#L38) | 预处理 | `#define MPU6500_I2C_SLV4_ADDR       (0x31)` |
| [39](../modules/mpu6500/mpu6500_reg.h#L39) | 预处理 | `#define MPU6500_I2C_SLV4_REG        (0x32)` |
| [40](../modules/mpu6500/mpu6500_reg.h#L40) | 预处理 | `#define MPU6500_I2C_SLV4_DO         (0x33)` |
| [41](../modules/mpu6500/mpu6500_reg.h#L41) | 预处理 | `#define MPU6500_I2C_SLV4_CTRL       (0x34)` |
| [42](../modules/mpu6500/mpu6500_reg.h#L42) | 预处理 | `#define MPU6500_I2C_SLV4_DI         (0x35)` |
| [43](../modules/mpu6500/mpu6500_reg.h#L43) | 预处理 | `#define MPU6500_I2C_MST_STATUS      (0x36)` |
| [44](../modules/mpu6500/mpu6500_reg.h#L44) | 预处理 | `#define MPU6500_INT_PIN_CFG         (0x37)` |
| [45](../modules/mpu6500/mpu6500_reg.h#L45) | 预处理 | `#define MPU6500_INT_ENABLE          (0x38)` |
| [46](../modules/mpu6500/mpu6500_reg.h#L46) | 预处理 | `#define MPU6500_INT_STATUS          (0x3A)` |
| [47](../modules/mpu6500/mpu6500_reg.h#L47) | 预处理 | `#define MPU6500_ACCEL_XOUT_H        (0x3B)` |
| [48](../modules/mpu6500/mpu6500_reg.h#L48) | 预处理 | `#define MPU6500_ACCEL_XOUT_L        (0x3C)` |
| [49](../modules/mpu6500/mpu6500_reg.h#L49) | 预处理 | `#define MPU6500_ACCEL_YOUT_H        (0x3D)` |
| [50](../modules/mpu6500/mpu6500_reg.h#L50) | 预处理 | `#define MPU6500_ACCEL_YOUT_L        (0x3E)` |
| [51](../modules/mpu6500/mpu6500_reg.h#L51) | 预处理 | `#define MPU6500_ACCEL_ZOUT_H        (0x3F)` |
| [52](../modules/mpu6500/mpu6500_reg.h#L52) | 预处理 | `#define MPU6500_ACCEL_ZOUT_L        (0x40)` |
| [53](../modules/mpu6500/mpu6500_reg.h#L53) | 预处理 | `#define MPU6500_TEMP_OUT_H          (0x41)` |
| [54](../modules/mpu6500/mpu6500_reg.h#L54) | 预处理 | `#define MPU6500_TEMP_OUT_L          (0x42)` |
| [55](../modules/mpu6500/mpu6500_reg.h#L55) | 预处理 | `#define MPU6500_GYRO_XOUT_H         (0x43)` |
| [56](../modules/mpu6500/mpu6500_reg.h#L56) | 预处理 | `#define MPU6500_GYRO_XOUT_L         (0x44)` |
| [57](../modules/mpu6500/mpu6500_reg.h#L57) | 预处理 | `#define MPU6500_GYRO_YOUT_H         (0x45)` |
| [58](../modules/mpu6500/mpu6500_reg.h#L58) | 预处理 | `#define MPU6500_GYRO_YOUT_L         (0x46)` |
| [59](../modules/mpu6500/mpu6500_reg.h#L59) | 预处理 | `#define MPU6500_GYRO_ZOUT_H         (0x47)` |
| [60](../modules/mpu6500/mpu6500_reg.h#L60) | 预处理 | `#define MPU6500_GYRO_ZOUT_L         (0x48)` |
| [61](../modules/mpu6500/mpu6500_reg.h#L61) | 预处理 | `#define MPU6500_EXT_SENS_DATA_00    (0x49)` |
| [62](../modules/mpu6500/mpu6500_reg.h#L62) | 预处理 | `#define MPU6500_EXT_SENS_DATA_01    (0x4A)` |
| [63](../modules/mpu6500/mpu6500_reg.h#L63) | 预处理 | `#define MPU6500_EXT_SENS_DATA_02    (0x4B)` |
| [64](../modules/mpu6500/mpu6500_reg.h#L64) | 预处理 | `#define MPU6500_EXT_SENS_DATA_03    (0x4C)` |
| [65](../modules/mpu6500/mpu6500_reg.h#L65) | 预处理 | `#define MPU6500_EXT_SENS_DATA_04    (0x4D)` |
| [66](../modules/mpu6500/mpu6500_reg.h#L66) | 预处理 | `#define MPU6500_EXT_SENS_DATA_05    (0x4E)` |
| [67](../modules/mpu6500/mpu6500_reg.h#L67) | 预处理 | `#define MPU6500_EXT_SENS_DATA_06    (0x4F)` |
| [68](../modules/mpu6500/mpu6500_reg.h#L68) | 预处理 | `#define MPU6500_EXT_SENS_DATA_07    (0x50)` |
| [69](../modules/mpu6500/mpu6500_reg.h#L69) | 预处理 | `#define MPU6500_EXT_SENS_DATA_08    (0x51)` |
| [70](../modules/mpu6500/mpu6500_reg.h#L70) | 预处理 | `#define MPU6500_EXT_SENS_DATA_09    (0x52)` |
| [71](../modules/mpu6500/mpu6500_reg.h#L71) | 预处理 | `#define MPU6500_EXT_SENS_DATA_10    (0x53)` |
| [72](../modules/mpu6500/mpu6500_reg.h#L72) | 预处理 | `#define MPU6500_EXT_SENS_DATA_11    (0x54)` |
| [73](../modules/mpu6500/mpu6500_reg.h#L73) | 预处理 | `#define MPU6500_EXT_SENS_DATA_12    (0x55)` |
| [74](../modules/mpu6500/mpu6500_reg.h#L74) | 预处理 | `#define MPU6500_EXT_SENS_DATA_13    (0x56)` |
| [75](../modules/mpu6500/mpu6500_reg.h#L75) | 预处理 | `#define MPU6500_EXT_SENS_DATA_14    (0x57)` |
| [76](../modules/mpu6500/mpu6500_reg.h#L76) | 预处理 | `#define MPU6500_EXT_SENS_DATA_15    (0x58)` |
| [77](../modules/mpu6500/mpu6500_reg.h#L77) | 预处理 | `#define MPU6500_EXT_SENS_DATA_16    (0x59)` |
| [78](../modules/mpu6500/mpu6500_reg.h#L78) | 预处理 | `#define MPU6500_EXT_SENS_DATA_17    (0x5A)` |
| [79](../modules/mpu6500/mpu6500_reg.h#L79) | 预处理 | `#define MPU6500_EXT_SENS_DATA_18    (0x5B)` |
| [80](../modules/mpu6500/mpu6500_reg.h#L80) | 预处理 | `#define MPU6500_EXT_SENS_DATA_19    (0x5C)` |
| [81](../modules/mpu6500/mpu6500_reg.h#L81) | 预处理 | `#define MPU6500_EXT_SENS_DATA_20    (0x5D)` |
| [82](../modules/mpu6500/mpu6500_reg.h#L82) | 预处理 | `#define MPU6500_EXT_SENS_DATA_21    (0x5E)` |
| [83](../modules/mpu6500/mpu6500_reg.h#L83) | 预处理 | `#define MPU6500_EXT_SENS_DATA_22    (0x5F)` |
| [84](../modules/mpu6500/mpu6500_reg.h#L84) | 预处理 | `#define MPU6500_EXT_SENS_DATA_23    (0x60)` |
| [85](../modules/mpu6500/mpu6500_reg.h#L85) | 预处理 | `#define MPU6500_I2C_SLV0_DO         (0x63)` |
| [86](../modules/mpu6500/mpu6500_reg.h#L86) | 预处理 | `#define MPU6500_I2C_SLV1_DO         (0x64)` |
| [87](../modules/mpu6500/mpu6500_reg.h#L87) | 预处理 | `#define MPU6500_I2C_SLV2_DO         (0x65)` |
| [88](../modules/mpu6500/mpu6500_reg.h#L88) | 预处理 | `#define MPU6500_I2C_SLV3_DO         (0x66)` |
| [89](../modules/mpu6500/mpu6500_reg.h#L89) | 预处理 | `#define MPU6500_I2C_MST_DELAY_CTRL  (0x67)` |
| [90](../modules/mpu6500/mpu6500_reg.h#L90) | 预处理 | `#define MPU6500_SIGNAL_PATH_RESET   (0x68)` |
| [91](../modules/mpu6500/mpu6500_reg.h#L91) | 预处理 | `#define MPU6500_MOT_DETECT_CTRL     (0x69)` |
| [92](../modules/mpu6500/mpu6500_reg.h#L92) | 预处理 | `#define MPU6500_USER_CTRL           (0x6A)` |
| [93](../modules/mpu6500/mpu6500_reg.h#L93) | 预处理 | `#define MPU6500_PWR_MGMT_1          (0x6B)` |
| [94](../modules/mpu6500/mpu6500_reg.h#L94) | 预处理 | `#define MPU6500_PWR_MGMT_2          (0x6C)` |
| [95](../modules/mpu6500/mpu6500_reg.h#L95) | 预处理 | `#define MPU6500_FIFO_COUNTH         (0x72)` |
| [96](../modules/mpu6500/mpu6500_reg.h#L96) | 预处理 | `#define MPU6500_FIFO_COUNTL         (0x73)` |
| [97](../modules/mpu6500/mpu6500_reg.h#L97) | 预处理 | `#define MPU6500_FIFO_R_W            (0x74)` |
| [98](../modules/mpu6500/mpu6500_reg.h#L98) | 预处理 | `#define MPU6500_WHO_AM_I            (0x74)` |
| [99](../modules/mpu6500/mpu6500_reg.h#L99) | 预处理 | `#define MPU6500_XA_OFFSET_H         (0x77)` |
| [100](../modules/mpu6500/mpu6500_reg.h#L100) | 预处理 | `#define MPU6500_XA_OFFSET_L         (0x78)` |
| [101](../modules/mpu6500/mpu6500_reg.h#L101) | 预处理 | `#define MPU6500_YA_OFFSET_H         (0x7A)` |
| [102](../modules/mpu6500/mpu6500_reg.h#L102) | 预处理 | `#define MPU6500_YA_OFFSET_L         (0x7B)` |
| [103](../modules/mpu6500/mpu6500_reg.h#L103) | 预处理 | `#define MPU6500_ZA_OFFSET_H         (0x7D)` |
| [104](../modules/mpu6500/mpu6500_reg.h#L104) | 预处理 | `#define MPU6500_ZA_OFFSET_L         (0x7E)` |
| [106](../modules/mpu6500/mpu6500_reg.h#L106) | 预处理 | `#define MPU6050_ID									(0x68)` |
| [107](../modules/mpu6500/mpu6500_reg.h#L107) | 预处理 | `#define MPU6500_ID									(0x70)// mpu6500 id = 0x70` |
| [109](../modules/mpu6500/mpu6500_reg.h#L109) | 预处理 | `#define MPU_IIC_ADDR								0x68` |
| [111](../modules/mpu6500/mpu6500_reg.h#L111) | 预处理 | `#endif // !MP` |

## modules/mpu6500/mpu6500driver.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/mpu6500/mpu6500driver.h#L1) | 预处理 | `#ifndef MPU6500Driver_H` |
| [2](../modules/mpu6500/mpu6500driver.h#L2) | 预处理 | `#define MPU6500Driver_H` |
| [8](../modules/mpu6500/mpu6500driver.h#L8) | 预处理 | `#define MPU_DELAY(x) DWT_Delay_ms(x)` |
| [15](../modules/mpu6500/mpu6500driver.h#L15) | 预处理 | `#define MPU6500_ACCEL_2G_SEN 0.00059814453125f` |
| [16](../modules/mpu6500/mpu6500driver.h#L16) | 预处理 | `#define MPU6500_ACCEL_4G_SEN 0.0011962890625f` |
| [17](../modules/mpu6500/mpu6500driver.h#L17) | 预处理 | `#define MPU6500_ACCEL_8G_SEN 0.002392578125f` |
| [18](../modules/mpu6500/mpu6500driver.h#L18) | 预处理 | `#define MPU6500_ACCEL_16G_SEN 0.00478515625f` |
| [20](../modules/mpu6500/mpu6500driver.h#L20) | 预处理 | `#define MPU6500_GYRO_2000_SEN 0.00106422513550135501355013550136` |
| [21](../modules/mpu6500/mpu6500driver.h#L21) | 预处理 | `#define MPU6500_GYRO_1000_SEN 5.3211256775067750677506775067751e-4` |
| [22](../modules/mpu6500/mpu6500driver.h#L22) | 预处理 | `#define MPU6500_GYRO_500_SEN 2.6646247667514843087362171331637e-4` |
| [23](../modules/mpu6500/mpu6500driver.h#L23) | 预处理 | `#define MPU6500_GYRO_250_SEN 1.3323123833757421543681085665818e-4` |
| [25](../modules/mpu6500/mpu6500driver.h#L25) | 预处理 | `#endif // !MPU6500Driver_H` |

## modules/oled/oled.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [2](../modules/oled/oled.c#L2) | 预处理 | `#if OLED_USE_I2C` |
| [373](../modules/oled/oled.c#L373) | 预处理 | `#endif` |

## modules/oled/oled.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../modules/oled/oled.h#L3) | 预处理 | `#if OLED_USE_I2C` |
| [16](../modules/oled/oled.h#L16) | 预处理 | `#ifndef OLED_H` |
| [17](../modules/oled/oled.h#L17) | 预处理 | `#define OLED_H` |
| [21](../modules/oled/oled.h#L21) | 预处理 | `#define OLED_I2C_ADDRESS 0x3C /* 7-bit BSP address */` |
| [24](../modules/oled/oled.h#L24) | 预处理 | `#define MAX_COLUMN 128` |
| [25](../modules/oled/oled.h#L25) | 预处理 | `#define MAX_ROW 64` |
| [27](../modules/oled/oled.h#L27) | 预处理 | `#define X_WIDTH MAX_COLUMN` |
| [28](../modules/oled/oled.h#L28) | 预处理 | `#define Y_WIDTH MAX_ROW` |
| [30](../modules/oled/oled.h#L30) | 预处理 | `#define OLED_CMD 0x00` |
| [31](../modules/oled/oled.h#L31) | 预处理 | `#define OLED_DATA 0x01` |
| [33](../modules/oled/oled.h#L33) | 预处理 | `#define CHAR_SIZE_WIDTH 6` |
| [34](../modules/oled/oled.h#L34) | 预处理 | `#define CHAR_SIZE_HIGHT 12` |
| [148](../modules/oled/oled.h#L148) | 预处理 | `#endif` |
| [150](../modules/oled/oled.h#L150) | 预处理 | `#else` |
| [152](../modules/oled/oled.h#L152) | 预处理 | `#endif` |

## modules/oled/oled_input.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/oled/oled_input.h#L1) | 预处理 | `#ifndef OLED_INPUT_H` |
| [2](../modules/oled/oled_input.h#L2) | 预处理 | `#define OLED_INPUT_H` |
| [5](../modules/oled/oled_input.h#L5) | 预处理 | `#endif` |

## modules/oled/oled_spi.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [2](../modules/oled/oled_spi.c#L2) | 预处理 | `#if !OLED_USE_I2C` |
| [417](../modules/oled/oled_spi.c#L417) | 预处理 | `#endif` |

## modules/oled/oled_spi.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [2](../modules/oled/oled_spi.h#L2) | 预处理 | `#ifndef OLED_H` |
| [3](../modules/oled/oled_spi.h#L3) | 预处理 | `#define OLED_H` |
| [6](../modules/oled/oled_spi.h#L6) | 预处理 | `#define Max_Column      128` |
| [7](../modules/oled/oled_spi.h#L7) | 预处理 | `#define Max_Row         64` |
| [9](../modules/oled/oled_spi.h#L9) | 预处理 | `#define X_WIDTH         128` |
| [10](../modules/oled/oled_spi.h#L10) | 预处理 | `#define Y_WIDTH         64` |
| [12](../modules/oled/oled_spi.h#L12) | 预处理 | `#define OLED_CMD        0x00` |
| [13](../modules/oled/oled_spi.h#L13) | 预处理 | `#define OLED_DATA       0x01` |
| [15](../modules/oled/oled_spi.h#L15) | 预处理 | `#define CHAR_SIZE_WIDTH     6` |
| [16](../modules/oled/oled_spi.h#L16) | 预处理 | `#define VHAR_SIZE_HIGHT     12` |
| [124](../modules/oled/oled_spi.h#L124) | 预处理 | `#endif` |

## modules/oled/oled_transport.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/oled/oled_transport.h#L1) | 预处理 | `#ifndef OLED_TRANSPORT_H` |
| [2](../modules/oled/oled_transport.h#L2) | 预处理 | `#define OLED_TRANSPORT_H` |
| [25](../modules/oled/oled_transport.h#L25) | 预处理 | `#endif` |

## modules/oled/oledfont.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/oled/oledfont.h#L11) | 预处理 | `#ifndef __OLED__FONT__H` |
| [12](../modules/oled/oledfont.h#L12) | 预处理 | `#define __OLED__FONT__H` |
| [247](../modules/oled/oledfont.h#L247) | 预处理 | `#endif` |

## modules/oled/oledfont_spi.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/oled/oledfont_spi.h#L11) | 预处理 | `#ifndef __OLED__FONT__H` |
| [12](../modules/oled/oledfont_spi.h#L12) | 预处理 | `#define __OLED__FONT__H` |
| [247](../modules/oled/oledfont_spi.h#L247) | 预处理 | `#endif` |

## modules/power_meter/power_meter.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [9](../modules/power_meter/power_meter.c#L9) | 预处理 | `#define POWER_METER_DEFAULT_OFFLINE_RELOAD_COUNT 10U    // 默认离线重载值,即100ms内没有接收到数据则认为离线` |
| [10](../modules/power_meter/power_meter.c#L10) | 预处理 | `#define POWER_METER_DAEMON_PERIOD_MS 10U    // 守护进程周期,即每10ms调用一次守护进程的回调函数` |
| [80](../modules/power_meter/power_meter.c#L80) | 配置初值 | `Daemon_Init_Config_s daemon_config = {` |
| [81](../modules/power_meter/power_meter.c#L81) | 配置初值 | `.reload_count = reload_count,` |
| [82](../modules/power_meter/power_meter.c#L82) | 配置初值 | `.init_count = reload_count,` |
| [83](../modules/power_meter/power_meter.c#L83) | 配置初值 | `.callback = PowerMeterOffline,` |
| [84](../modules/power_meter/power_meter.c#L84) | 配置初值 | `.owner_id = instance,` |

## modules/power_meter/power_meter.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/power_meter/power_meter.h#L1) | 预处理 | `#ifndef POWER_METER_H` |
| [2](../modules/power_meter/power_meter.h#L2) | 预处理 | `#define POWER_METER_H` |
| [32](../modules/power_meter/power_meter.h#L32) | 预处理 | `#endif` |

## modules/power_meter/power_meter_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/power_meter/power_meter_protocol.h#L1) | 预处理 | `#ifndef POWER_METER_PROTOCOL_H` |
| [2](../modules/power_meter/power_meter_protocol.h#L2) | 预处理 | `#define POWER_METER_PROTOCOL_H` |
| [7](../modules/power_meter/power_meter_protocol.h#L7) | 预处理 | `#define POWER_METER_STATUS_CAN_ID 0x605U` |
| [8](../modules/power_meter/power_meter_protocol.h#L8) | 预处理 | `#define POWER_METER_CAN_DLC 8U` |
| [22](../modules/power_meter/power_meter_protocol.h#L22) | 预处理 | `#endif` |

## modules/power_meter/power_meter_types.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/power_meter/power_meter_types.h#L1) | 预处理 | `#ifndef POWER_METER_TYPES_H` |
| [2](../modules/power_meter/power_meter_types.h#L2) | 预处理 | `#define POWER_METER_TYPES_H` |
| [18](../modules/power_meter/power_meter_types.h#L18) | 预处理 | `#endif` |

## modules/predictive_power/energy_scheduler.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/energy_scheduler.h#L1) | 预处理 | `#ifndef ENERGY_SCHEDULER_H` |
| [2](../modules/predictive_power/energy_scheduler.h#L2) | 预处理 | `#define ENERGY_SCHEDULER_H` |
| [5](../modules/predictive_power/energy_scheduler.h#L5) | 预处理 | `#define PP_ENERGY_NODES 10U` |
| [6](../modules/predictive_power/energy_scheduler.h#L6) | 预处理 | `#define PP_ENERGY_BLOCKS 5U` |
| [35](../modules/predictive_power/energy_scheduler.h#L35) | 预处理 | `#endif` |

## modules/predictive_power/power_allocator.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/power_allocator.h#L1) | 预处理 | `#ifndef POWER_ALLOCATOR_H` |
| [2](../modules/predictive_power/power_allocator.h#L2) | 预处理 | `#define POWER_ALLOCATOR_H` |
| [18](../modules/predictive_power/power_allocator.h#L18) | 预处理 | `#endif` |

## modules/predictive_power/power_observer.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/power_observer.h#L1) | 预处理 | `#ifndef POWER_OBSERVER_H` |
| [2](../modules/predictive_power/power_observer.h#L2) | 预处理 | `#define POWER_OBSERVER_H` |
| [4](../modules/predictive_power/power_observer.h#L4) | 预处理 | `#define PP_HISTORY_LENGTH 32U` |
| [33](../modules/predictive_power/power_observer.h#L33) | 预处理 | `#endif` |

## modules/predictive_power/power_prediction.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/power_prediction.h#L1) | 预处理 | `#ifndef POWER_PREDICTION_H` |
| [2](../modules/predictive_power/power_prediction.h#L2) | 预处理 | `#define POWER_PREDICTION_H` |
| [35](../modules/predictive_power/power_prediction.h#L35) | 预处理 | `#endif` |

## modules/predictive_power/power_supervisor.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/power_supervisor.h#L1) | 预处理 | `#ifndef POWER_SUPERVISOR_H` |
| [2](../modules/predictive_power/power_supervisor.h#L2) | 预处理 | `#define POWER_SUPERVISOR_H` |
| [21](../modules/predictive_power/power_supervisor.h#L21) | 预处理 | `#endif` |

## modules/predictive_power/speed_observation_time.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/speed_observation_time.h#L1) | 预处理 | `#ifndef SPEED_OBSERVATION_TIME_H` |
| [2](../modules/predictive_power/speed_observation_time.h#L2) | 预处理 | `#define SPEED_OBSERVATION_TIME_H` |
| [17](../modules/predictive_power/speed_observation_time.h#L17) | 预处理 | `#endif` |

## modules/predictive_power/wheel_speed_kf.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/predictive_power/wheel_speed_kf.h#L1) | 预处理 | `#ifndef WHEEL_SPEED_KF_H` |
| [2](../modules/predictive_power/wheel_speed_kf.h#L2) | 预处理 | `#define WHEEL_SPEED_KF_H` |
| [46](../modules/predictive_power/wheel_speed_kf.h#L46) | 预处理 | `#endif` |

## modules/referee/crc_ref.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/crc_ref.h#L1) | 预处理 | `#ifndef __CRC_H_` |
| [2](../modules/referee/crc_ref.h#L2) | 预处理 | `#define __CRC_H_` |
| [8](../modules/referee/crc_ref.h#L8) | 预处理 | `#define TRUE 1` |
| [9](../modules/referee/crc_ref.h#L9) | 预处理 | `#define FALSE 0` |
| [20](../modules/referee/crc_ref.h#L20) | 预处理 | `#endif` |

## modules/referee/referee_power.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/referee_power.h#L1) | 预处理 | `#ifndef REFEREE_POWER_H` |
| [2](../modules/referee/referee_power.h#L2) | 预处理 | `#define REFEREE_POWER_H` |
| [7](../modules/referee/referee_power.h#L7) | 预处理 | `#define REFEREE_ROBOT_STATE_COMMAND_ID 0x0201U` |
| [8](../modules/referee/referee_power.h#L8) | 预处理 | `#define REFEREE_POWER_HEAT_COMMAND_ID 0x0202U` |
| [9](../modules/referee/referee_power.h#L9) | 预处理 | `#define REFEREE_ROBOT_STATE_PAYLOAD_LENGTH 13U` |
| [10](../modules/referee/referee_power.h#L10) | 预处理 | `#define REFEREE_ROBOT_STATE_EXTENDED_PAYLOAD_LENGTH 17U` |
| [11](../modules/referee/referee_power.h#L11) | 预处理 | `#define REFEREE_POWER_HEAT_PAYLOAD_LENGTH 14U` |
| [12](../modules/referee/referee_power.h#L12) | 预处理 | `#define REFEREE_POWER_HEAT_LEGACY_PAYLOAD_LENGTH 16U` |
| [62](../modules/referee/referee_power.h#L62) | 预处理 | `#endif` |

## modules/referee/referee_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/referee/referee_protocol.h#L11) | 预处理 | `#ifndef referee_protocol_H` |
| [12](../modules/referee/referee_protocol.h#L12) | 预处理 | `#define referee_protocol_H` |
| [18](../modules/referee/referee_protocol.h#L18) | 预处理 | `#define REFEREE_SOF 0xA5 // 起始字节,协议固定为0xA5` |
| [19](../modules/referee/referee_protocol.h#L19) | 预处理 | `#define Robot_Red 0` |
| [20](../modules/referee/referee_protocol.h#L20) | 预处理 | `#define Robot_Blue 1` |
| [21](../modules/referee/referee_protocol.h#L21) | 预处理 | `#define Communicate_Data_LEN 5 // 自定义交互数据长度，该长度决定了我方发送和他方接收，自定义交互数据协议更改时只需要更改此宏定义即可` |
| [396](../modules/referee/referee_protocol.h#L396) | 预处理 | `#endif` |

## modules/referee/referee_stream_parser.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [7](../modules/referee/referee_stream_parser.c#L7) | 预处理 | `#define REFEREE_SOF 0xA5U` |
| [8](../modules/referee/referee_stream_parser.c#L8) | 预处理 | `#define REFEREE_HEADER_LENGTH 5U` |
| [9](../modules/referee/referee_stream_parser.c#L9) | 预处理 | `#define REFEREE_COMMAND_LENGTH 2U` |
| [10](../modules/referee/referee_stream_parser.c#L10) | 预处理 | `#define REFEREE_TAIL_LENGTH 2U` |
| [11](../modules/referee/referee_stream_parser.c#L11) | 预处理 | `#define REFEREE_FRAME_OVERHEAD \` |

## modules/referee/referee_stream_parser.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/referee_stream_parser.h#L1) | 预处理 | `#ifndef REFEREE_STREAM_PARSER_H` |
| [2](../modules/referee/referee_stream_parser.h#L2) | 预处理 | `#define REFEREE_STREAM_PARSER_H` |
| [8](../modules/referee/referee_stream_parser.h#L8) | 预处理 | `#define REFEREE_STREAM_BUFFER_CAPACITY 256U` |
| [50](../modules/referee/referee_stream_parser.h#L50) | 预处理 | `#endif` |

## modules/referee/referee_task.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/referee_task.h#L1) | 预处理 | `#ifndef REFEREE_H` |
| [2](../modules/referee/referee_task.h#L2) | 预处理 | `#define REFEREE_H` |
| [25](../modules/referee/referee_task.h#L25) | 预处理 | `#endif // REFEREE_H` |

## modules/referee/referee_UI.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/referee_UI.h#L1) | 预处理 | `#ifndef REFEREE_UI_H` |
| [2](../modules/referee/referee_UI.h#L2) | 预处理 | `#define REFEREE_UI_H` |
| [72](../modules/referee/referee_UI.h#L72) | 预处理 | `#endif` |

## modules/referee/rm_referee.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [21](../modules/referee/rm_referee.c#L21) | 预处理 | `#define RE_RX_BUFFER_SIZE 255u // 裁判系统接收缓冲区大小` |
| [229](../modules/referee/rm_referee.c#L229) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [230](../modules/referee/rm_referee.c#L230) | 配置初值 | `.callback = RefereeLostCallback,` |
| [231](../modules/referee/rm_referee.c#L231) | 配置初值 | `.owner_id = referee_usart_instance,` |
| [232](../modules/referee/rm_referee.c#L232) | 配置初值 | `.reload_count = 30, // 0.3s没有收到数据,则认为丢失,重启串口接收` |

## modules/referee/rm_referee.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/referee/rm_referee.h#L1) | 预处理 | `#ifndef RM_REFEREE_H` |
| [2](../modules/referee/rm_referee.h#L2) | 预处理 | `#define RM_REFEREE_H` |
| [121](../modules/referee/rm_referee.h#L121) | 预处理 | `#endif // !REFEREE_H` |

## modules/remote/remote_control.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/remote/remote_control.c#L11) | 预处理 | `#define REMOTE_CONTROL_FRAME_SIZE 18u // 遥控器接收的buffer大小` |
| [135](../modules/remote/remote_control.c#L135) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [136](../modules/remote/remote_control.c#L136) | 配置初值 | `.reload_count = 10, // 100ms未收到数据视为离线,遥控器的接收频率实际上是1000/14Hz(大约70Hz)` |
| [137](../modules/remote/remote_control.c#L137) | 配置初值 | `.callback = RCLostCallback,` |
| [138](../modules/remote/remote_control.c#L138) | 配置初值 | `.owner_id = NULL, // 只有1个遥控器,不需要owner_id` |

## modules/remote/remote_control.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [13](../modules/remote/remote_control.h#L13) | 预处理 | `#ifndef REMOTE_CONTROL_H` |
| [14](../modules/remote/remote_control.h#L14) | 预处理 | `#define REMOTE_CONTROL_H` |
| [21](../modules/remote/remote_control.h#L21) | 预处理 | `#define LAST 1` |
| [22](../modules/remote/remote_control.h#L22) | 预处理 | `#define TEMP 0` |
| [25](../modules/remote/remote_control.h#L25) | 预处理 | `#define KEY_PRESS 0` |
| [26](../modules/remote/remote_control.h#L26) | 预处理 | `#define KEY_STATE 1` |
| [27](../modules/remote/remote_control.h#L27) | 预处理 | `#define KEY_PRESS_WITH_CTRL 1` |
| [28](../modules/remote/remote_control.h#L28) | 预处理 | `#define KEY_PRESS_WITH_SHIFT 2` |
| [31](../modules/remote/remote_control.h#L31) | 预处理 | `#define RC_CH_VALUE_MIN ((uint16_t)364)` |
| [32](../modules/remote/remote_control.h#L32) | 预处理 | `#define RC_CH_VALUE_OFFSET ((uint16_t)1024)` |
| [33](../modules/remote/remote_control.h#L33) | 预处理 | `#define RC_CH_VALUE_MAX ((uint16_t)1684)` |
| [36](../modules/remote/remote_control.h#L36) | 预处理 | `#define RC_SW_UP ((uint16_t)1)   // 开关向上时的值` |
| [37](../modules/remote/remote_control.h#L37) | 预处理 | `#define RC_SW_MID ((uint16_t)3)  // 开关中间时的值` |
| [38](../modules/remote/remote_control.h#L38) | 预处理 | `#define RC_SW_DOWN ((uint16_t)2) // 开关向下时的值` |
| [40](../modules/remote/remote_control.h#L40) | 预处理 | `#define switch_is_down(s) (s == RC_SW_DOWN)` |
| [41](../modules/remote/remote_control.h#L41) | 预处理 | `#define switch_is_mid(s) (s == RC_SW_MID)` |
| [42](../modules/remote/remote_control.h#L42) | 预处理 | `#define switch_is_up(s) (s == RC_SW_UP)` |
| [46](../modules/remote/remote_control.h#L46) | 预处理 | `#define Key_W 0` |
| [47](../modules/remote/remote_control.h#L47) | 预处理 | `#define Key_S 1` |
| [48](../modules/remote/remote_control.h#L48) | 预处理 | `#define Key_D 2` |
| [49](../modules/remote/remote_control.h#L49) | 预处理 | `#define Key_A 3` |
| [50](../modules/remote/remote_control.h#L50) | 预处理 | `#define Key_Shift 4` |
| [51](../modules/remote/remote_control.h#L51) | 预处理 | `#define Key_Ctrl 5` |
| [52](../modules/remote/remote_control.h#L52) | 预处理 | `#define Key_Q 6` |
| [53](../modules/remote/remote_control.h#L53) | 预处理 | `#define Key_E 7` |
| [54](../modules/remote/remote_control.h#L54) | 预处理 | `#define Key_R 8` |
| [55](../modules/remote/remote_control.h#L55) | 预处理 | `#define Key_F 9` |
| [56](../modules/remote/remote_control.h#L56) | 预处理 | `#define Key_G 10` |
| [57](../modules/remote/remote_control.h#L57) | 预处理 | `#define Key_Z 11` |
| [58](../modules/remote/remote_control.h#L58) | 预处理 | `#define Key_X 12` |
| [59](../modules/remote/remote_control.h#L59) | 预处理 | `#define Key_C 13` |
| [60](../modules/remote/remote_control.h#L60) | 预处理 | `#define Key_V 14` |
| [61](../modules/remote/remote_control.h#L61) | 预处理 | `#define Key_B 15` |
| [142](../modules/remote/remote_control.h#L142) | 预处理 | `#endif` |

## modules/super_cap/super_cap.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [9](../modules/super_cap/super_cap.c#L9) | 预处理 | `#define SUPERCAP_DEFAULT_OFFLINE_RELOAD_COUNT 20U` |
| [89](../modules/super_cap/super_cap.c#L89) | 配置初值 | `Daemon_Init_Config_s daemon_config = {` |
| [90](../modules/super_cap/super_cap.c#L90) | 配置初值 | `.reload_count = reload_count,` |
| [91](../modules/super_cap/super_cap.c#L91) | 配置初值 | `.init_count = reload_count,` |
| [92](../modules/super_cap/super_cap.c#L92) | 配置初值 | `.callback = SuperCapOffline,` |
| [93](../modules/super_cap/super_cap.c#L93) | 配置初值 | `.owner_id = instance,` |

## modules/super_cap/super_cap.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/super_cap/super_cap.h#L1) | 预处理 | `#ifndef SUPER_CAP_H` |
| [2](../modules/super_cap/super_cap.h#L2) | 预处理 | `#define SUPER_CAP_H` |
| [19](../modules/super_cap/super_cap.h#L19) | 预处理 | `#endif` |

## modules/super_cap/super_cap_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/super_cap/super_cap_protocol.h#L1) | 预处理 | `#ifndef SUPER_CAP_PROTOCOL_H` |
| [2](../modules/super_cap/super_cap_protocol.h#L2) | 预处理 | `#define SUPER_CAP_PROTOCOL_H` |
| [7](../modules/super_cap/super_cap_protocol.h#L7) | 预处理 | `#define SUPERCAP_COMMAND_CAN_ID 0x061U` |
| [8](../modules/super_cap/super_cap_protocol.h#L8) | 预处理 | `#define SUPERCAP_STATUS_CAN_ID 0x051U` |
| [9](../modules/super_cap/super_cap_protocol.h#L9) | 预处理 | `#define SUPERCAP_INPUT_POWER_CAN_ID 0x052U` |
| [10](../modules/super_cap/super_cap_protocol.h#L10) | 预处理 | `#define SUPERCAP_INPUT_COMMAND_TAG 0xA5U` |
| [11](../modules/super_cap/super_cap_protocol.h#L11) | 预处理 | `#define SUPERCAP_INPUT_FEEDBACK_VERSION 1U` |
| [12](../modules/super_cap/super_cap_protocol.h#L12) | 预处理 | `#define SUPERCAP_CAN_DLC 8U` |
| [14](../modules/super_cap/super_cap_protocol.h#L14) | 预处理 | `#define SUPERCAP_ERROR_UNDER_VOLTAGE 0x01U` |
| [15](../modules/super_cap/super_cap_protocol.h#L15) | 预处理 | `#define SUPERCAP_ERROR_OVER_VOLTAGE 0x02U` |
| [16](../modules/super_cap/super_cap_protocol.h#L16) | 预处理 | `#define SUPERCAP_ERROR_BUCK_BOOST 0x04U` |
| [17](../modules/super_cap/super_cap_protocol.h#L17) | 预处理 | `#define SUPERCAP_ERROR_SHORT_CIRCUIT 0x08U` |
| [18](../modules/super_cap/super_cap_protocol.h#L18) | 预处理 | `#define SUPERCAP_ERROR_HIGH_TEMPERATURE 0x10U` |
| [19](../modules/super_cap/super_cap_protocol.h#L19) | 预处理 | `#define SUPERCAP_ERROR_NO_POWER_INPUT 0x20U` |
| [20](../modules/super_cap/super_cap_protocol.h#L20) | 预处理 | `#define SUPERCAP_ERROR_CAPACITOR 0x40U` |
| [21](../modules/super_cap/super_cap_protocol.h#L21) | 预处理 | `#define SUPERCAP_OUTPUT_DISABLED_MASK 0x80U` |
| [22](../modules/super_cap/super_cap_protocol.h#L22) | 预处理 | `#define SUPERCAP_ERROR_MASK 0x7FU` |
| [61](../modules/super_cap/super_cap_protocol.h#L61) | 预处理 | `#endif` |

## modules/super_cap/super_cap_types.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/super_cap/super_cap_types.h#L1) | 预处理 | `#ifndef SUPER_CAP_TYPES_H` |
| [2](../modules/super_cap/super_cap_types.h#L2) | 预处理 | `#define SUPER_CAP_TYPES_H` |
| [22](../modules/super_cap/super_cap_types.h#L22) | 预处理 | `#endif` |

## modules/telemetry/autotune_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/autotune_protocol.h#L1) | 预处理 | `#ifndef AUTOTUNE_PROTOCOL_H` |
| [2](../modules/telemetry/autotune_protocol.h#L2) | 预处理 | `#define AUTOTUNE_PROTOCOL_H` |
| [23](../modules/telemetry/autotune_protocol.h#L23) | 预处理 | `#endif` |

## modules/telemetry/chassis_force_telemetry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/chassis_force_telemetry.h#L1) | 预处理 | `#ifndef CHASSIS_FORCE_TELEMETRY_H` |
| [2](../modules/telemetry/chassis_force_telemetry.h#L2) | 预处理 | `#define CHASSIS_FORCE_TELEMETRY_H` |
| [9](../modules/telemetry/chassis_force_telemetry.h#L9) | 预处理 | `#endif` |

## modules/telemetry/chassis_power_capture.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/chassis_power_capture.h#L1) | 预处理 | `#ifndef CHASSIS_POWER_CAPTURE_H` |
| [2](../modules/telemetry/chassis_power_capture.h#L2) | 预处理 | `#define CHASSIS_POWER_CAPTURE_H` |
| [7](../modules/telemetry/chassis_power_capture.h#L7) | 预处理 | `#define CHASSIS_POWER_CAPTURE_RECORDS 150U` |
| [8](../modules/telemetry/chassis_power_capture.h#L8) | 预处理 | `#define CHASSIS_POWER_CAPTURE_RECORD_BYTES 96U` |
| [9](../modules/telemetry/chassis_power_capture.h#L9) | 预处理 | `#define CHASSIS_POWER_CAPTURE_FRAME_BYTES 102U` |
| [10](../modules/telemetry/chassis_power_capture.h#L10) | 预处理 | `#define CHASSIS_POWER_CAPTURE_QUANTIZED 0x80U` |
| [51](../modules/telemetry/chassis_power_capture.h#L51) | 预处理 | `#endif` |

## modules/telemetry/chassis_speed_capture.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/chassis_speed_capture.h#L1) | 预处理 | `#ifndef CHASSIS_SPEED_CAPTURE_H` |
| [2](../modules/telemetry/chassis_speed_capture.h#L2) | 预处理 | `#define CHASSIS_SPEED_CAPTURE_H` |
| [6](../modules/telemetry/chassis_speed_capture.h#L6) | 预处理 | `#define CSC_RAW_CAPACITY 192U` |
| [7](../modules/telemetry/chassis_speed_capture.h#L7) | 预处理 | `#define CSC_CYCLE_CAPACITY 32U` |
| [8](../modules/telemetry/chassis_speed_capture.h#L8) | 预处理 | `#define CSC_RAW_BYTES 40U` |
| [9](../modules/telemetry/chassis_speed_capture.h#L9) | 预处理 | `#define CSC_CYCLE_BYTES 512U` |
| [10](../modules/telemetry/chassis_speed_capture.h#L10) | 预处理 | `#define CSC_METADATA_BYTES 1024U` |
| [11](../modules/telemetry/chassis_speed_capture.h#L11) | 预处理 | `#define CSC_END_BYTES 48U` |
| [12](../modules/telemetry/chassis_speed_capture.h#L12) | 预处理 | `#define CSC_MAX_FRAME_BYTES 1044U` |
| [13](../modules/telemetry/chassis_speed_capture.h#L13) | 预处理 | `#define CSC_KIND_METADATA 1U` |
| [14](../modules/telemetry/chassis_speed_capture.h#L14) | 预处理 | `#define CSC_KIND_RAW 2U` |
| [15](../modules/telemetry/chassis_speed_capture.h#L15) | 预处理 | `#define CSC_KIND_CYCLE 3U` |
| [16](../modules/telemetry/chassis_speed_capture.h#L16) | 预处理 | `#define CSC_KIND_END 4U` |
| [17](../modules/telemetry/chassis_speed_capture.h#L17) | 预处理 | `#define CSC_FREEZE_RAW_FULL 1U` |
| [18](../modules/telemetry/chassis_speed_capture.h#L18) | 预处理 | `#define CSC_FREEZE_CYCLE_FULL 2U` |
| [19](../modules/telemetry/chassis_speed_capture.h#L19) | 预处理 | `#define CSC_FREEZE_MANUAL 3U` |
| [20](../modules/telemetry/chassis_speed_capture.h#L20) | 预处理 | `#define CSC_FREEZE_VERSION 4U` |
| [21](../modules/telemetry/chassis_speed_capture.h#L21) | 预处理 | `#define CSC_FREEZE_ERROR 5U` |
| [73](../modules/telemetry/chassis_speed_capture.h#L73) | 预处理 | `#endif` |

## modules/telemetry/force_calibration_protocol.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [6](../modules/telemetry/force_calibration_protocol.c#L6) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [10](../modules/telemetry/force_calibration_protocol.c#L10) | 预处理 | `#ifndef ATYPE_BENCH_MODE` |
| [11](../modules/telemetry/force_calibration_protocol.c#L11) | 预处理 | `#define ATYPE_BENCH_MODE 0` |
| [12](../modules/telemetry/force_calibration_protocol.c#L12) | 预处理 | `#endif` |
| [13](../modules/telemetry/force_calibration_protocol.c#L13) | 预处理 | `#endif` |
| [18](../modules/telemetry/force_calibration_protocol.c#L18) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [20](../modules/telemetry/force_calibration_protocol.c#L20) | 预处理 | `#endif` |
| [36](../modules/telemetry/force_calibration_protocol.c#L36) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [60](../modules/telemetry/force_calibration_protocol.c#L60) | 预处理 | `#endif` |
| [69](../modules/telemetry/force_calibration_protocol.c#L69) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [72](../modules/telemetry/force_calibration_protocol.c#L72) | 预处理 | `#else` |
| [74](../modules/telemetry/force_calibration_protocol.c#L74) | 预处理 | `#endif` |
| [88](../modules/telemetry/force_calibration_protocol.c#L88) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [95](../modules/telemetry/force_calibration_protocol.c#L95) | 预处理 | `#else` |
| [98](../modules/telemetry/force_calibration_protocol.c#L98) | 预处理 | `#endif` |
| [112](../modules/telemetry/force_calibration_protocol.c#L112) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [114](../modules/telemetry/force_calibration_protocol.c#L114) | 预处理 | `#endif` |
| [121](../modules/telemetry/force_calibration_protocol.c#L121) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [172](../modules/telemetry/force_calibration_protocol.c#L172) | 预处理 | `#else` |
| [174](../modules/telemetry/force_calibration_protocol.c#L174) | 预处理 | `#endif` |
| [178](../modules/telemetry/force_calibration_protocol.c#L178) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [187](../modules/telemetry/force_calibration_protocol.c#L187) | 预处理 | `#endif` |
| [193](../modules/telemetry/force_calibration_protocol.c#L193) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [195](../modules/telemetry/force_calibration_protocol.c#L195) | 预处理 | `#endif` |

## modules/telemetry/force_calibration_protocol.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/force_calibration_protocol.h#L1) | 预处理 | `#ifndef FORCE_CALIBRATION_PROTOCOL_H` |
| [2](../modules/telemetry/force_calibration_protocol.h#L2) | 预处理 | `#define FORCE_CALIBRATION_PROTOCOL_H` |
| [12](../modules/telemetry/force_calibration_protocol.h#L12) | 预处理 | `#endif` |

## modules/telemetry/pid_autotune.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/pid_autotune.h#L1) | 预处理 | `#ifndef PID_AUTOTUNE_H` |
| [2](../modules/telemetry/pid_autotune.h#L2) | 预处理 | `#define PID_AUTOTUNE_H` |
| [39](../modules/telemetry/pid_autotune.h#L39) | 预处理 | `#endif` |

## modules/telemetry/rtt_telemetry.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [14](../modules/telemetry/rtt_telemetry.c#L14) | 预处理 | `#if CHASSIS_POWER_CALIBRATION_ENABLE` |
| [17](../modules/telemetry/rtt_telemetry.c#L17) | 预处理 | `#endif` |
| [147](../modules/telemetry/rtt_telemetry.c#L147) | 预处理 | `#if CHASSIS_POWER_CALIBRATION_ENABLE` |
| [187](../modules/telemetry/rtt_telemetry.c#L187) | 预处理 | `#endif` |
| [203](../modules/telemetry/rtt_telemetry.c#L203) | 预处理 | `#if CHASSIS_POWER_CALIBRATION_ENABLE` |
| [205](../modules/telemetry/rtt_telemetry.c#L205) | 预处理 | `#else` |
| [207](../modules/telemetry/rtt_telemetry.c#L207) | 预处理 | `#endif` |

## modules/telemetry/rtt_telemetry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/rtt_telemetry.h#L1) | 预处理 | `#ifndef RTT_TELEMETRY_H` |
| [2](../modules/telemetry/rtt_telemetry.h#L2) | 预处理 | `#define RTT_TELEMETRY_H` |
| [46](../modules/telemetry/rtt_telemetry.h#L46) | 预处理 | `#endif // RTT_TELEMETRY_H` |

## modules/telemetry/runtime_tuning.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/runtime_tuning.h#L1) | 预处理 | `#ifndef RUNTIME_TUNING_H` |
| [2](../modules/telemetry/runtime_tuning.h#L2) | 预处理 | `#define RUNTIME_TUNING_H` |
| [12](../modules/telemetry/runtime_tuning.h#L12) | 预处理 | `#define TUNING_MAX_VALUES 24U` |
| [13](../modules/telemetry/runtime_tuning.h#L13) | 预处理 | `#define TUNING_LINE_SIZE 512U` |
| [41](../modules/telemetry/runtime_tuning.h#L41) | 预处理 | `#endif` |

## modules/telemetry/tuning_flash.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/tuning_flash.h#L1) | 预处理 | `#ifndef TUNING_FLASH_H` |
| [2](../modules/telemetry/tuning_flash.h#L2) | 预处理 | `#define TUNING_FLASH_H` |
| [13](../modules/telemetry/tuning_flash.h#L13) | 预处理 | `#endif` |

## modules/telemetry/tuning_storage.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../modules/telemetry/tuning_storage.c#L3) | 预处理 | `#define MAGIC 0x54554e45U` |
| [4](../modules/telemetry/tuning_storage.c#L4) | 预处理 | `#define SCHEMA 2U /* A-board vehicle; intentionally rejects C-board records. */` |
| [5](../modules/telemetry/tuning_storage.c#L5) | 预处理 | `#define COMMIT 0x434f4d4dU` |
| [6](../modules/telemetry/tuning_storage.c#L6) | 预处理 | `#define CRC_WORD (TUNING_RECORD_WORDS - 2U)` |
| [7](../modules/telemetry/tuning_storage.c#L7) | 预处理 | `#define COMMIT_WORD (TUNING_RECORD_WORDS - 1U)` |

## modules/telemetry/tuning_storage.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/tuning_storage.h#L1) | 预处理 | `#ifndef TUNING_STORAGE_H` |
| [2](../modules/telemetry/tuning_storage.h#L2) | 预处理 | `#define TUNING_STORAGE_H` |
| [5](../modules/telemetry/tuning_storage.h#L5) | 预处理 | `#define TUNING_RECORD_WORDS 128U` |
| [6](../modules/telemetry/tuning_storage.h#L6) | 预处理 | `#define TUNING_STORED_GROUPS 3U` |
| [7](../modules/telemetry/tuning_storage.h#L7) | 预处理 | `#define TUNING_STORED_VALUES 24U` |
| [28](../modules/telemetry/tuning_storage.h#L28) | 预处理 | `#endif` |

## modules/telemetry/usb_telemetry.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/usb_telemetry.h#L1) | 预处理 | `#ifndef USB_TELEMETRY_H` |
| [2](../modules/telemetry/usb_telemetry.h#L2) | 预处理 | `#define USB_TELEMETRY_H` |
| [11](../modules/telemetry/usb_telemetry.h#L11) | 预处理 | `#endif` |

## modules/telemetry/usb_telemetry_format.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [8](../modules/telemetry/usb_telemetry_format.c#L8) | 预处理 | `#define USB_HEADER_FIELD(name) #name ","` |
| [92](../modules/telemetry/usb_telemetry_format.c#L92) | 预处理 | `#define USB_ROW_U(name) \` |
| [96](../modules/telemetry/usb_telemetry_format.c#L96) | 预处理 | `#define USB_ROW_I(name) \` |

## modules/telemetry/usb_telemetry_format.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/usb_telemetry_format.h#L1) | 预处理 | `#ifndef USB_TELEMETRY_FORMAT_H` |
| [2](../modules/telemetry/usb_telemetry_format.h#L2) | 预处理 | `#define USB_TELEMETRY_FORMAT_H` |
| [9](../modules/telemetry/usb_telemetry_format.h#L9) | 预处理 | `#define USB_TELEMETRY_FIELDS(U, I) \` |
| [51](../modules/telemetry/usb_telemetry_format.h#L51) | 预处理 | `#define USB_FIELD_U(name) uint32_t name;` |
| [52](../modules/telemetry/usb_telemetry_format.h#L52) | 预处理 | `#define USB_FIELD_I(name) int32_t name;` |
| [69](../modules/telemetry/usb_telemetry_format.h#L69) | 预处理 | `#endif` |

## modules/telemetry/usb_telemetry_imu.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/telemetry/usb_telemetry_imu.h#L1) | 预处理 | `#ifndef USB_TELEMETRY_IMU_H` |
| [2](../modules/telemetry/usb_telemetry_imu.h#L2) | 预处理 | `#define USB_TELEMETRY_IMU_H` |
| [11](../modules/telemetry/usb_telemetry_imu.h#L11) | 预处理 | `#endif` |

## modules/telemetry/usb_telemetry_stream.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [7](../modules/telemetry/usb_telemetry_stream.c#L7) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [9](../modules/telemetry/usb_telemetry_stream.c#L9) | 预处理 | `#endif` |
| [11](../modules/telemetry/usb_telemetry_stream.c#L11) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [20](../modules/telemetry/usb_telemetry_stream.c#L20) | 预处理 | `#endif` |
| [51](../modules/telemetry/usb_telemetry_stream.c#L51) | 预处理 | `#if CHASSIS_FORCE_CALIBRATION_ENABLE` |
| [54](../modules/telemetry/usb_telemetry_stream.c#L54) | 预处理 | `#endif` |
| [95](../modules/telemetry/usb_telemetry_stream.c#L95) | 配置初值 | `USB_Init_Config_s config = {.rx_cbk = Receive};` |
| [103](../modules/telemetry/usb_telemetry_stream.c#L103) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [105](../modules/telemetry/usb_telemetry_stream.c#L105) | 预处理 | `#else` |
| [107](../modules/telemetry/usb_telemetry_stream.c#L107) | 预处理 | `#endif` |
| [165](../modules/telemetry/usb_telemetry_stream.c#L165) | 预处理 | `#if CHASSIS_CONTROL_MODE == CHASSIS_CONTROL_FORCE` |
| [171](../modules/telemetry/usb_telemetry_stream.c#L171) | 预处理 | `#else` |
| [173](../modules/telemetry/usb_telemetry_stream.c#L173) | 预处理 | `#endif` |

## modules/TFminiPlus/tfminiplus.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [45](../modules/TFminiPlus/tfminiplus.c#L45) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [46](../modules/TFminiPlus/tfminiplus.c#L46) | 配置初值 | `.callback = TFminiPlusOffineCallback,` |
| [47](../modules/TFminiPlus/tfminiplus.c#L47) | 配置初值 | `.owner_id = tfminiplus_usart_instance,` |
| [48](../modules/TFminiPlus/tfminiplus.c#L48) | 配置初值 | `.reload_count = 10,` |

## modules/TFminiPlus/tfminiplus.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/TFminiPlus/tfminiplus.h#L1) | 预处理 | `#ifndef TFMINIPLUS_H` |
| [2](../modules/TFminiPlus/tfminiplus.h#L2) | 预处理 | `#define TFMINIPLUS_H` |
| [6](../modules/TFminiPlus/tfminiplus.h#L6) | 预处理 | `#define TFMINIPLUS_FRAME_HEAD_1 0x59` |
| [7](../modules/TFminiPlus/tfminiplus.h#L7) | 预处理 | `#define TFMINIPLUS_FRAME_HEAD_2 0x59` |
| [8](../modules/TFminiPlus/tfminiplus.h#L8) | 预处理 | `#define TFMINIPLUS_RECV_SIZE 9u` |
| [20](../modules/TFminiPlus/tfminiplus.h#L20) | 预处理 | `#endif // !TFMINIPLUS_H` |

## modules/vehicle_safety/vehicle_safety.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [3](../modules/vehicle_safety/vehicle_safety.c#L3) | 预处理 | `#ifdef STM32F427xx` |
| [6](../modules/vehicle_safety/vehicle_safety.c#L6) | 预处理 | `#define LOCK() taskENTER_CRITICAL()` |
| [7](../modules/vehicle_safety/vehicle_safety.c#L7) | 预处理 | `#define UNLOCK() taskEXIT_CRITICAL()` |
| [8](../modules/vehicle_safety/vehicle_safety.c#L8) | 预处理 | `#else` |
| [9](../modules/vehicle_safety/vehicle_safety.c#L9) | 预处理 | `#define LOCK() ((void)0)` |
| [10](../modules/vehicle_safety/vehicle_safety.c#L10) | 预处理 | `#define UNLOCK() ((void)0)` |
| [11](../modules/vehicle_safety/vehicle_safety.c#L11) | 预处理 | `#endif` |

## modules/vehicle_safety/vehicle_safety.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [1](../modules/vehicle_safety/vehicle_safety.h#L1) | 预处理 | `#ifndef VEHICLE_SAFETY_H` |
| [2](../modules/vehicle_safety/vehicle_safety.h#L2) | 预处理 | `#define VEHICLE_SAFETY_H` |
| [20](../modules/vehicle_safety/vehicle_safety.h#L20) | 预处理 | `#endif` |

## modules/vtx/vt02_control.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [11](../modules/vtx/vt02_control.c#L11) | 预处理 | `#define VT02_RX_BUFFER_SIZE 256u` |
| [12](../modules/vtx/vt02_control.c#L12) | 预处理 | `#define VT02_DAEMON_RELOAD 10 // 约330ms未收到数据视为离线(30Hz)` |
| [14](../modules/vtx/vt02_control.c#L14) | 预处理 | `#define VT02_SOF 0xA5u` |
| [15](../modules/vtx/vt02_control.c#L15) | 预处理 | `#define VT02_HEADER_SIZE 5u` |
| [16](../modules/vtx/vt02_control.c#L16) | 预处理 | `#define VT02_CMD_ID_SIZE 2u` |
| [17](../modules/vtx/vt02_control.c#L17) | 预处理 | `#define VT02_CRC16_SIZE 2u` |
| [18](../modules/vtx/vt02_control.c#L18) | 预处理 | `#define VT02_DATA_OFFSET (VT02_HEADER_SIZE + VT02_CMD_ID_SIZE)` |
| [19](../modules/vtx/vt02_control.c#L19) | 预处理 | `#define VT02_CONTROL_CMD_ID 0x0304u` |
| [20](../modules/vtx/vt02_control.c#L20) | 预处理 | `#define VT02_CONTROL_DATA_SIZE 12u` |
| [21](../modules/vtx/vt02_control.c#L21) | 预处理 | `#define VT02_MIN_FRAME_SIZE (VT02_HEADER_SIZE + VT02_CMD_ID_SIZE + VT02_CRC16_SIZE)` |
| [22](../modules/vtx/vt02_control.c#L22) | 预处理 | `#define VT02_DEBUG_DATA_SIZE 32u` |
| [318](../modules/vtx/vt02_control.c#L318) | 配置初值 | `USART_Init_Config_s usart_conf = {` |
| [319](../modules/vtx/vt02_control.c#L319) | 配置初值 | `.module_callback = Vt02RxCallback,` |
| [320](../modules/vtx/vt02_control.c#L320) | 配置初值 | `.usart_handle = vt02_usart_handle,` |
| [321](../modules/vtx/vt02_control.c#L321) | 配置初值 | `.recv_buff_size = (uint8_t)(VT02_RX_BUFFER_SIZE - 1u),` |
| [324](../modules/vtx/vt02_control.c#L324) | 配置初值 | `Daemon_Init_Config_s daemon_conf = {` |
| [325](../modules/vtx/vt02_control.c#L325) | 配置初值 | `.reload_count = VT02_DAEMON_RELOAD,` |
| [326](../modules/vtx/vt02_control.c#L326) | 配置初值 | `.callback = Vt02LostCallback,` |
| [327](../modules/vtx/vt02_control.c#L327) | 配置初值 | `.owner_id = NULL,` |

## modules/vtx/vt02_control.h

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [12](../modules/vtx/vt02_control.h#L12) | 预处理 | `#ifndef VT02_CONTROL_H` |
| [13](../modules/vtx/vt02_control.h#L13) | 预处理 | `#define VT02_CONTROL_H` |
| [66](../modules/vtx/vt02_control.h#L66) | 预处理 | `#endif` |

## Src/adc.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [46](../Src/adc.c#L46) | HAL/NVIC | `hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;` |
| [47](../Src/adc.c#L47) | HAL/NVIC | `hadc1.Init.Resolution = ADC_RESOLUTION_12B;` |
| [48](../Src/adc.c#L48) | HAL/NVIC | `hadc1.Init.ScanConvMode = DISABLE;` |
| [49](../Src/adc.c#L49) | HAL/NVIC | `hadc1.Init.ContinuousConvMode = DISABLE;` |
| [50](../Src/adc.c#L50) | HAL/NVIC | `hadc1.Init.DiscontinuousConvMode = DISABLE;` |
| [51](../Src/adc.c#L51) | HAL/NVIC | `hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;` |
| [52](../Src/adc.c#L52) | HAL/NVIC | `hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;` |
| [53](../Src/adc.c#L53) | HAL/NVIC | `hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;` |
| [54](../Src/adc.c#L54) | HAL/NVIC | `hadc1.Init.NbrOfConversion = 1;` |
| [55](../Src/adc.c#L55) | HAL/NVIC | `hadc1.Init.DMAContinuousRequests = DISABLE;` |
| [56](../Src/adc.c#L56) | HAL/NVIC | `hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;` |

## Src/can.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [42](../Src/can.c#L42) | HAL/NVIC | `hcan1.Init.Prescaler = 3;` |
| [43](../Src/can.c#L43) | HAL/NVIC | `hcan1.Init.Mode = CAN_MODE_NORMAL;` |
| [44](../Src/can.c#L44) | HAL/NVIC | `hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;` |
| [45](../Src/can.c#L45) | HAL/NVIC | `hcan1.Init.TimeSeg1 = CAN_BS1_10TQ;` |
| [46](../Src/can.c#L46) | HAL/NVIC | `hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;` |
| [47](../Src/can.c#L47) | HAL/NVIC | `hcan1.Init.TimeTriggeredMode = DISABLE;` |
| [48](../Src/can.c#L48) | HAL/NVIC | `hcan1.Init.AutoBusOff = ENABLE;` |
| [49](../Src/can.c#L49) | HAL/NVIC | `hcan1.Init.AutoWakeUp = DISABLE;` |
| [50](../Src/can.c#L50) | HAL/NVIC | `hcan1.Init.AutoRetransmission = ENABLE;` |
| [51](../Src/can.c#L51) | HAL/NVIC | `hcan1.Init.ReceiveFifoLocked = DISABLE;` |
| [52](../Src/can.c#L52) | HAL/NVIC | `hcan1.Init.TransmitFifoPriority = ENABLE;` |
| [74](../Src/can.c#L74) | HAL/NVIC | `hcan2.Init.Prescaler = 3;` |
| [75](../Src/can.c#L75) | HAL/NVIC | `hcan2.Init.Mode = CAN_MODE_NORMAL;` |
| [76](../Src/can.c#L76) | HAL/NVIC | `hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;` |
| [77](../Src/can.c#L77) | HAL/NVIC | `hcan2.Init.TimeSeg1 = CAN_BS1_10TQ;` |
| [78](../Src/can.c#L78) | HAL/NVIC | `hcan2.Init.TimeSeg2 = CAN_BS2_3TQ;` |
| [79](../Src/can.c#L79) | HAL/NVIC | `hcan2.Init.TimeTriggeredMode = DISABLE;` |
| [80](../Src/can.c#L80) | HAL/NVIC | `hcan2.Init.AutoBusOff = ENABLE;` |
| [81](../Src/can.c#L81) | HAL/NVIC | `hcan2.Init.AutoWakeUp = DISABLE;` |
| [82](../Src/can.c#L82) | HAL/NVIC | `hcan2.Init.AutoRetransmission = ENABLE;` |
| [83](../Src/can.c#L83) | HAL/NVIC | `hcan2.Init.ReceiveFifoLocked = DISABLE;` |
| [84](../Src/can.c#L84) | HAL/NVIC | `hcan2.Init.TransmitFifoPriority = ENABLE;` |
| [125](../Src/can.c#L125) | HAL/NVIC | `HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);` |
| [126](../Src/can.c#L126) | HAL/NVIC | `HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);` |
| [127](../Src/can.c#L127) | HAL/NVIC | `HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);` |
| [128](../Src/can.c#L128) | HAL/NVIC | `HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);` |
| [158](../Src/can.c#L158) | HAL/NVIC | `HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);` |
| [159](../Src/can.c#L159) | HAL/NVIC | `HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);` |
| [160](../Src/can.c#L160) | HAL/NVIC | `HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0);` |
| [161](../Src/can.c#L161) | HAL/NVIC | `HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);` |
| [189](../Src/can.c#L189) | HAL/NVIC | `HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);` |
| [190](../Src/can.c#L190) | HAL/NVIC | `HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);` |
| [214](../Src/can.c#L214) | HAL/NVIC | `HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);` |
| [215](../Src/can.c#L215) | HAL/NVIC | `HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);` |

## Src/dma.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [48](../Src/dma.c#L48) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);` |
| [49](../Src/dma.c#L49) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);` |
| [51](../Src/dma.c#L51) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);` |
| [52](../Src/dma.c#L52) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);` |
| [54](../Src/dma.c#L54) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);` |
| [55](../Src/dma.c#L55) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);` |
| [57](../Src/dma.c#L57) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);` |
| [58](../Src/dma.c#L58) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);` |
| [60](../Src/dma.c#L60) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 5, 0);` |
| [61](../Src/dma.c#L61) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);` |
| [63](../Src/dma.c#L63) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);` |
| [64](../Src/dma.c#L64) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);` |
| [66](../Src/dma.c#L66) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);` |
| [67](../Src/dma.c#L67) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);` |
| [69](../Src/dma.c#L69) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);` |
| [70](../Src/dma.c#L70) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);` |
| [72](../Src/dma.c#L72) | HAL/NVIC | `HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 5, 0);` |
| [73](../Src/dma.c#L73) | HAL/NVIC | `HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);` |

## Src/freertos.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [129](../Src/freertos.c#L129) | 预处理 | `#if !IMU_CAPTURE_ONLY` |
| [131](../Src/freertos.c#L131) | 预处理 | `#endif` |

## Src/gpio.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [173](../Src/gpio.c#L173) | HAL/NVIC | `HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);` |
| [174](../Src/gpio.c#L174) | HAL/NVIC | `HAL_NVIC_EnableIRQ(EXTI2_IRQn);` |

## Src/i2c.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [41](../Src/i2c.c#L41) | HAL/NVIC | `hi2c2.Init.ClockSpeed = 100000;` |
| [42](../Src/i2c.c#L42) | HAL/NVIC | `hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;` |
| [43](../Src/i2c.c#L43) | HAL/NVIC | `hi2c2.Init.OwnAddress1 = 0;` |
| [44](../Src/i2c.c#L44) | HAL/NVIC | `hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;` |
| [45](../Src/i2c.c#L45) | HAL/NVIC | `hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;` |
| [46](../Src/i2c.c#L46) | HAL/NVIC | `hi2c2.Init.OwnAddress2 = 0;` |
| [47](../Src/i2c.c#L47) | HAL/NVIC | `hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;` |
| [48](../Src/i2c.c#L48) | HAL/NVIC | `hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;` |

## Src/main.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [231](../Src/main.c#L231) | 预处理 | `#ifdef  USE_FULL_ASSERT` |
| [246](../Src/main.c#L246) | 预处理 | `#endif /* USE_FULL_ASSERT */` |

## Src/rtc.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [44](../Src/rtc.c#L44) | HAL/NVIC | `hrtc.Init.HourFormat = RTC_HOURFORMAT_24;` |
| [45](../Src/rtc.c#L45) | HAL/NVIC | `hrtc.Init.AsynchPrediv = 124;` |
| [46](../Src/rtc.c#L46) | HAL/NVIC | `hrtc.Init.SynchPrediv = 3199;` |
| [47](../Src/rtc.c#L47) | HAL/NVIC | `hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;` |
| [48](../Src/rtc.c#L48) | HAL/NVIC | `hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;` |
| [49](../Src/rtc.c#L49) | HAL/NVIC | `hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;` |

## Src/spi.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [42](../Src/spi.c#L42) | HAL/NVIC | `hspi1.Init.Mode = SPI_MODE_MASTER;` |
| [43](../Src/spi.c#L43) | HAL/NVIC | `hspi1.Init.Direction = SPI_DIRECTION_2LINES;` |
| [44](../Src/spi.c#L44) | HAL/NVIC | `hspi1.Init.DataSize = SPI_DATASIZE_8BIT;` |
| [45](../Src/spi.c#L45) | HAL/NVIC | `hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;` |
| [46](../Src/spi.c#L46) | HAL/NVIC | `hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;` |
| [47](../Src/spi.c#L47) | HAL/NVIC | `hspi1.Init.NSS = SPI_NSS_SOFT;` |
| [48](../Src/spi.c#L48) | HAL/NVIC | `hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;` |
| [49](../Src/spi.c#L49) | HAL/NVIC | `hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;` |
| [50](../Src/spi.c#L50) | HAL/NVIC | `hspi1.Init.TIMode = SPI_TIMODE_DISABLE;` |
| [51](../Src/spi.c#L51) | HAL/NVIC | `hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;` |
| [52](../Src/spi.c#L52) | HAL/NVIC | `hspi1.Init.CRCPolynomial = 10;` |
| [74](../Src/spi.c#L74) | HAL/NVIC | `hspi5.Init.Mode = SPI_MODE_MASTER;` |
| [75](../Src/spi.c#L75) | HAL/NVIC | `hspi5.Init.Direction = SPI_DIRECTION_2LINES;` |
| [76](../Src/spi.c#L76) | HAL/NVIC | `hspi5.Init.DataSize = SPI_DATASIZE_8BIT;` |
| [77](../Src/spi.c#L77) | HAL/NVIC | `hspi5.Init.CLKPolarity = SPI_POLARITY_HIGH;` |
| [78](../Src/spi.c#L78) | HAL/NVIC | `hspi5.Init.CLKPhase = SPI_PHASE_2EDGE;` |
| [79](../Src/spi.c#L79) | HAL/NVIC | `hspi5.Init.NSS = SPI_NSS_SOFT;` |
| [80](../Src/spi.c#L80) | HAL/NVIC | `hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;` |
| [81](../Src/spi.c#L81) | HAL/NVIC | `hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;` |
| [82](../Src/spi.c#L82) | HAL/NVIC | `hspi5.Init.TIMode = SPI_TIMODE_DISABLE;` |
| [83](../Src/spi.c#L83) | HAL/NVIC | `hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;` |
| [84](../Src/spi.c#L84) | HAL/NVIC | `hspi5.Init.CRCPolynomial = 10;` |
| [154](../Src/spi.c#L154) | HAL/NVIC | `HAL_NVIC_SetPriority(SPI5_IRQn, 5, 0);` |
| [155](../Src/spi.c#L155) | HAL/NVIC | `HAL_NVIC_EnableIRQ(SPI5_IRQn);` |
| [202](../Src/spi.c#L202) | HAL/NVIC | `HAL_NVIC_DisableIRQ(SPI5_IRQn);` |

## Src/stm32f4xx_hal_msp.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [75](../Src/stm32f4xx_hal_msp.c#L75) | HAL/NVIC | `HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);` |

## Src/stm32f4xx_hal_timebase_tim.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [81](../Src/stm32f4xx_hal_timebase_tim.c#L81) | HAL/NVIC | `htim14.Init.Period = (1000000U / 1000U) - 1U;` |
| [82](../Src/stm32f4xx_hal_timebase_tim.c#L82) | HAL/NVIC | `htim14.Init.Prescaler = uwPrescalerValue;` |
| [83](../Src/stm32f4xx_hal_timebase_tim.c#L83) | HAL/NVIC | `htim14.Init.ClockDivision = 0;` |
| [84](../Src/stm32f4xx_hal_timebase_tim.c#L84) | HAL/NVIC | `htim14.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [85](../Src/stm32f4xx_hal_timebase_tim.c#L85) | HAL/NVIC | `htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [95](../Src/stm32f4xx_hal_timebase_tim.c#L95) | HAL/NVIC | `HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);` |
| [100](../Src/stm32f4xx_hal_timebase_tim.c#L100) | HAL/NVIC | `HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, TickPriority, 0U);` |

## Src/syscalls.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [179](../Src/syscalls.c#L179) | 预处理 | `#if defined(__PICOLIBC__)` |
| [244](../Src/syscalls.c#L244) | 预处理 | `#endif //__PICOLIBC__` |

## Src/sysmem.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [82](../Src/sysmem.c#L82) | 预处理 | `#if defined(__PICOLIBC__)` |
| [87](../Src/sysmem.c#L87) | 预处理 | `#endif` |

## Src/system_stm32f4xx.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [50](../Src/system_stm32f4xx.c#L50) | 预处理 | `#if !defined  (HSE_VALUE)` |
| [51](../Src/system_stm32f4xx.c#L51) | 预处理 | `#define HSE_VALUE    ((uint32_t)25000000) /*!< Default value of the External oscillator in Hz */` |
| [52](../Src/system_stm32f4xx.c#L52) | 预处理 | `#endif /* HSE_VALUE */` |
| [54](../Src/system_stm32f4xx.c#L54) | 预处理 | `#if !defined  (HSI_VALUE)` |
| [55](../Src/system_stm32f4xx.c#L55) | 预处理 | `#define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/` |
| [56](../Src/system_stm32f4xx.c#L56) | 预处理 | `#endif /* HSI_VALUE */` |
| [76](../Src/system_stm32f4xx.c#L76) | 预处理 | `#if defined(STM32F405xx) &#124;&#124; defined(STM32F415xx) &#124;&#124; defined(STM32F407xx) &#124;&#124; defined(STM32F417xx)\` |
| [80](../Src/system_stm32f4xx.c#L80) | 预处理 | `#endif /* STM32F40xxx &#124;&#124; STM32F41xxx &#124;&#124; STM32F42xxx &#124;&#124; STM32F43xxx &#124;&#124; STM32F469xx &#124;&#124; STM32F479xx &#124;&#124;\` |
| [83](../Src/system_stm32f4xx.c#L83) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)\` |
| [86](../Src/system_stm32f4xx.c#L86) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx &#124;&#124; STM32F446xx &#124;&#124; STM32F469xx &#124;&#124;\` |
| [96](../Src/system_stm32f4xx.c#L96) | 预处理 | `#if defined(USER_VECT_TAB_ADDRESS)` |
| [100](../Src/system_stm32f4xx.c#L100) | 预处理 | `#if defined(VECT_TAB_SRAM)` |
| [101](../Src/system_stm32f4xx.c#L101) | 预处理 | `#define VECT_TAB_BASE_ADDRESS   SRAM_BASE       /*!< Vector Table base address field.` |
| [103](../Src/system_stm32f4xx.c#L103) | 预处理 | `#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.` |
| [105](../Src/system_stm32f4xx.c#L105) | 预处理 | `#else` |
| [106](../Src/system_stm32f4xx.c#L106) | 预处理 | `#define VECT_TAB_BASE_ADDRESS   FLASH_BASE      /*!< Vector Table base address field.` |
| [108](../Src/system_stm32f4xx.c#L108) | 预处理 | `#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.` |
| [110](../Src/system_stm32f4xx.c#L110) | 预处理 | `#endif /* VECT_TAB_SRAM */` |
| [111](../Src/system_stm32f4xx.c#L111) | 预处理 | `#endif /* USER_VECT_TAB_ADDRESS */` |
| [148](../Src/system_stm32f4xx.c#L148) | 预处理 | `#if defined (DATA_IN_ExtSRAM) &#124;&#124; defined (DATA_IN_ExtSDRAM)` |
| [150](../Src/system_stm32f4xx.c#L150) | 预处理 | `#endif /* DATA_IN_ExtSRAM &#124;&#124; DATA_IN_ExtSDRAM */` |
| [170](../Src/system_stm32f4xx.c#L170) | 预处理 | `#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)` |
| [172](../Src/system_stm32f4xx.c#L172) | 预处理 | `#endif` |
| [174](../Src/system_stm32f4xx.c#L174) | 预处理 | `#if defined (DATA_IN_ExtSRAM) &#124;&#124; defined (DATA_IN_ExtSDRAM)` |
| [176](../Src/system_stm32f4xx.c#L176) | 预处理 | `#endif /* DATA_IN_ExtSRAM &#124;&#124; DATA_IN_ExtSDRAM */` |
| [179](../Src/system_stm32f4xx.c#L179) | 预处理 | `#if defined(USER_VECT_TAB_ADDRESS)` |
| [181](../Src/system_stm32f4xx.c#L181) | 预处理 | `#endif /* USER_VECT_TAB_ADDRESS */` |
| [268](../Src/system_stm32f4xx.c#L268) | 预处理 | `#if defined (DATA_IN_ExtSRAM) && defined (DATA_IN_ExtSDRAM)` |
| [269](../Src/system_stm32f4xx.c#L269) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)\` |
| [420](../Src/system_stm32f4xx.c#L420) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)` |
| [425](../Src/system_stm32f4xx.c#L425) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx */` |
| [426](../Src/system_stm32f4xx.c#L426) | 预处理 | `#if defined(STM32F469xx) &#124;&#124; defined(STM32F479xx)` |
| [431](../Src/system_stm32f4xx.c#L431) | 预处理 | `#endif /* STM32F469xx &#124;&#124; STM32F479xx */` |
| [435](../Src/system_stm32f4xx.c#L435) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx &#124;&#124; STM32F469xx &#124;&#124; STM32F479xx */` |
| [436](../Src/system_stm32f4xx.c#L436) | 预处理 | `#elif defined (DATA_IN_ExtSRAM) &#124;&#124; defined (DATA_IN_ExtSDRAM)` |
| [448](../Src/system_stm32f4xx.c#L448) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)\` |
| [450](../Src/system_stm32f4xx.c#L450) | 预处理 | `#if defined (DATA_IN_ExtSDRAM)` |
| [454](../Src/system_stm32f4xx.c#L454) | 预处理 | `#if defined(STM32F446xx)` |
| [458](../Src/system_stm32f4xx.c#L458) | 预处理 | `#else` |
| [462](../Src/system_stm32f4xx.c#L462) | 预处理 | `#endif /* STM32F446xx */` |
| [466](../Src/system_stm32f4xx.c#L466) | 预处理 | `#if defined(STM32F446xx)` |
| [490](../Src/system_stm32f4xx.c#L490) | 预处理 | `#endif /* STM32F446xx */` |
| [540](../Src/system_stm32f4xx.c#L540) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)\` |
| [565](../Src/system_stm32f4xx.c#L565) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx &#124;&#124; STM32F469xx &#124;&#124; STM32F479xx */` |
| [574](../Src/system_stm32f4xx.c#L574) | 预处理 | `#if defined(STM32F446xx)` |
| [576](../Src/system_stm32f4xx.c#L576) | 预处理 | `#else` |
| [578](../Src/system_stm32f4xx.c#L578) | 预处理 | `#endif /* STM32F446xx */` |
| [603](../Src/system_stm32f4xx.c#L603) | 预处理 | `#if defined(STM32F446xx)` |
| [605](../Src/system_stm32f4xx.c#L605) | 预处理 | `#else` |
| [607](../Src/system_stm32f4xx.c#L607) | 预处理 | `#endif /* STM32F446xx */` |
| [616](../Src/system_stm32f4xx.c#L616) | 预处理 | `#if defined(STM32F446xx)` |
| [618](../Src/system_stm32f4xx.c#L618) | 预处理 | `#else` |
| [620](../Src/system_stm32f4xx.c#L620) | 预处理 | `#endif /* STM32F446xx */` |
| [630](../Src/system_stm32f4xx.c#L630) | 预处理 | `#if defined(STM32F446xx)` |
| [632](../Src/system_stm32f4xx.c#L632) | 预处理 | `#else` |
| [634](../Src/system_stm32f4xx.c#L634) | 预处理 | `#endif /* STM32F446xx */` |
| [639](../Src/system_stm32f4xx.c#L639) | 预处理 | `#endif /* DATA_IN_ExtSDRAM */` |
| [640](../Src/system_stm32f4xx.c#L640) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx &#124;&#124; STM32F446xx &#124;&#124; STM32F469xx &#124;&#124; STM32F479xx */` |
| [642](../Src/system_stm32f4xx.c#L642) | 预处理 | `#if defined(STM32F405xx) &#124;&#124; defined(STM32F415xx) &#124;&#124; defined(STM32F407xx) &#124;&#124; defined(STM32F417xx)\` |
| [646](../Src/system_stm32f4xx.c#L646) | 预处理 | `#if defined(DATA_IN_ExtSRAM)` |
| [705](../Src/system_stm32f4xx.c#L705) | 预处理 | `#if defined(STM32F427xx) &#124;&#124; defined(STM32F437xx) &#124;&#124; defined(STM32F429xx) &#124;&#124; defined(STM32F439xx)` |
| [712](../Src/system_stm32f4xx.c#L712) | 预处理 | `#endif /* STM32F427xx &#124;&#124; STM32F437xx &#124;&#124; STM32F429xx &#124;&#124; STM32F439xx */` |
| [713](../Src/system_stm32f4xx.c#L713) | 预处理 | `#if defined(STM32F469xx) &#124;&#124; defined(STM32F479xx)` |
| [720](../Src/system_stm32f4xx.c#L720) | 预处理 | `#endif /* STM32F469xx &#124;&#124; STM32F479xx */` |
| [721](../Src/system_stm32f4xx.c#L721) | 预处理 | `#if defined(STM32F405xx) &#124;&#124; defined(STM32F415xx) &#124;&#124; defined(STM32F407xx)&#124;&#124; defined(STM32F417xx)\` |
| [729](../Src/system_stm32f4xx.c#L729) | 预处理 | `#endif /* STM32F405xx &#124;&#124; STM32F415xx &#124;&#124; STM32F407xx &#124;&#124; STM32F417xx &#124;&#124; STM32F412Zx &#124;&#124; STM32F412Vx */` |
| [731](../Src/system_stm32f4xx.c#L731) | 预处理 | `#endif /* DATA_IN_ExtSRAM */` |
| [732](../Src/system_stm32f4xx.c#L732) | 预处理 | `#endif /* STM32F405xx &#124;&#124; STM32F415xx &#124;&#124; STM32F407xx &#124;&#124; STM32F417xx &#124;&#124; STM32F427xx &#124;&#124; STM32F437xx &#124;&#124;\` |
| [736](../Src/system_stm32f4xx.c#L736) | 预处理 | `#endif /* DATA_IN_ExtSRAM && DATA_IN_ExtSDRAM */` |

## Src/tim.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [43](../Src/tim.c#L43) | 配置初值 | `TIM_ClockConfigTypeDef sClockSourceConfig = {0};` |
| [44](../Src/tim.c#L44) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [46](../Src/tim.c#L46) | 配置初值 | `TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};` |
| [52](../Src/tim.c#L52) | HAL/NVIC | `htim1.Init.Prescaler = 168-1;` |
| [53](../Src/tim.c#L53) | HAL/NVIC | `htim1.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [54](../Src/tim.c#L54) | HAL/NVIC | `htim1.Init.Period = 20000-1;` |
| [55](../Src/tim.c#L55) | HAL/NVIC | `htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [56](../Src/tim.c#L56) | HAL/NVIC | `htim1.Init.RepetitionCounter = 0;` |
| [57](../Src/tim.c#L57) | HAL/NVIC | `htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [125](../Src/tim.c#L125) | 配置初值 | `TIM_ClockConfigTypeDef sClockSourceConfig = {0};` |
| [126](../Src/tim.c#L126) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [133](../Src/tim.c#L133) | HAL/NVIC | `htim2.Init.Prescaler = 0;` |
| [134](../Src/tim.c#L134) | HAL/NVIC | `htim2.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [135](../Src/tim.c#L135) | HAL/NVIC | `htim2.Init.Period = 65535;` |
| [136](../Src/tim.c#L136) | HAL/NVIC | `htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [137](../Src/tim.c#L137) | HAL/NVIC | `htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [191](../Src/tim.c#L191) | 配置初值 | `TIM_ClockConfigTypeDef sClockSourceConfig = {0};` |
| [192](../Src/tim.c#L192) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [199](../Src/tim.c#L199) | HAL/NVIC | `htim3.Init.Prescaler = 84-1;` |
| [200](../Src/tim.c#L200) | HAL/NVIC | `htim3.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [201](../Src/tim.c#L201) | HAL/NVIC | `htim3.Init.Period = 10000-1;` |
| [202](../Src/tim.c#L202) | HAL/NVIC | `htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [203](../Src/tim.c#L203) | HAL/NVIC | `htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [245](../Src/tim.c#L245) | 配置初值 | `TIM_ClockConfigTypeDef sClockSourceConfig = {0};` |
| [246](../Src/tim.c#L246) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [253](../Src/tim.c#L253) | HAL/NVIC | `htim4.Init.Prescaler = 0;` |
| [254](../Src/tim.c#L254) | HAL/NVIC | `htim4.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [255](../Src/tim.c#L255) | HAL/NVIC | `htim4.Init.Period = 65535;` |
| [256](../Src/tim.c#L256) | HAL/NVIC | `htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [257](../Src/tim.c#L257) | HAL/NVIC | `htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [312](../Src/tim.c#L312) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [318](../Src/tim.c#L318) | HAL/NVIC | `htim5.Init.Prescaler = 0;` |
| [319](../Src/tim.c#L319) | HAL/NVIC | `htim5.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [320](../Src/tim.c#L320) | HAL/NVIC | `htim5.Init.Period = 4294967295;` |
| [321](../Src/tim.c#L321) | HAL/NVIC | `htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [322](../Src/tim.c#L322) | HAL/NVIC | `htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [357](../Src/tim.c#L357) | 配置初值 | `TIM_MasterConfigTypeDef sMasterConfig = {0};` |
| [363](../Src/tim.c#L363) | HAL/NVIC | `htim8.Init.Prescaler = 0;` |
| [364](../Src/tim.c#L364) | HAL/NVIC | `htim8.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [365](../Src/tim.c#L365) | HAL/NVIC | `htim8.Init.Period = 65535;` |
| [366](../Src/tim.c#L366) | HAL/NVIC | `htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [367](../Src/tim.c#L367) | HAL/NVIC | `htim8.Init.RepetitionCounter = 0;` |
| [368](../Src/tim.c#L368) | HAL/NVIC | `htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [402](../Src/tim.c#L402) | 配置初值 | `TIM_ClockConfigTypeDef sClockSourceConfig = {0};` |
| [409](../Src/tim.c#L409) | HAL/NVIC | `htim12.Init.Prescaler = 84-1;` |
| [410](../Src/tim.c#L410) | HAL/NVIC | `htim12.Init.CounterMode = TIM_COUNTERMODE_UP;` |
| [411](../Src/tim.c#L411) | HAL/NVIC | `htim12.Init.Period = 10000-1;` |
| [412](../Src/tim.c#L412) | HAL/NVIC | `htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;` |
| [413](../Src/tim.c#L413) | HAL/NVIC | `htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;` |
| [551](../Src/tim.c#L551) | HAL/NVIC | `HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 15, 0);` |
| [552](../Src/tim.c#L552) | HAL/NVIC | `HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);` |
| [782](../Src/tim.c#L782) | HAL/NVIC | `HAL_NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);` |

## Src/usart.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [56](../Src/usart.c#L56) | HAL/NVIC | `huart7.Init.BaudRate = BOARD_UART7_BAUDRATE;` |
| [57](../Src/usart.c#L57) | HAL/NVIC | `huart7.Init.WordLength = UART_WORDLENGTH_8B;` |
| [58](../Src/usart.c#L58) | HAL/NVIC | `huart7.Init.StopBits = UART_STOPBITS_1;` |
| [59](../Src/usart.c#L59) | HAL/NVIC | `huart7.Init.Parity = UART_PARITY_NONE;` |
| [60](../Src/usart.c#L60) | HAL/NVIC | `huart7.Init.Mode = UART_MODE_TX_RX;` |
| [61](../Src/usart.c#L61) | HAL/NVIC | `huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [62](../Src/usart.c#L62) | HAL/NVIC | `huart7.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [84](../Src/usart.c#L84) | HAL/NVIC | `huart8.Init.BaudRate = 115200;` |
| [85](../Src/usart.c#L85) | HAL/NVIC | `huart8.Init.WordLength = UART_WORDLENGTH_8B;` |
| [86](../Src/usart.c#L86) | HAL/NVIC | `huart8.Init.StopBits = UART_STOPBITS_1;` |
| [87](../Src/usart.c#L87) | HAL/NVIC | `huart8.Init.Parity = UART_PARITY_NONE;` |
| [88](../Src/usart.c#L88) | HAL/NVIC | `huart8.Init.Mode = UART_MODE_TX_RX;` |
| [89](../Src/usart.c#L89) | HAL/NVIC | `huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [90](../Src/usart.c#L90) | HAL/NVIC | `huart8.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [113](../Src/usart.c#L113) | HAL/NVIC | `huart1.Init.BaudRate = 100000;` |
| [114](../Src/usart.c#L114) | HAL/NVIC | `huart1.Init.WordLength = UART_WORDLENGTH_9B;` |
| [115](../Src/usart.c#L115) | HAL/NVIC | `huart1.Init.StopBits = UART_STOPBITS_1;` |
| [116](../Src/usart.c#L116) | HAL/NVIC | `huart1.Init.Parity = UART_PARITY_EVEN;` |
| [117](../Src/usart.c#L117) | HAL/NVIC | `huart1.Init.Mode = UART_MODE_TX_RX;` |
| [118](../Src/usart.c#L118) | HAL/NVIC | `huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [119](../Src/usart.c#L119) | HAL/NVIC | `huart1.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [142](../Src/usart.c#L142) | HAL/NVIC | `huart2.Init.BaudRate = 115200;` |
| [143](../Src/usart.c#L143) | HAL/NVIC | `huart2.Init.WordLength = UART_WORDLENGTH_8B;` |
| [144](../Src/usart.c#L144) | HAL/NVIC | `huart2.Init.StopBits = UART_STOPBITS_1;` |
| [145](../Src/usart.c#L145) | HAL/NVIC | `huart2.Init.Parity = UART_PARITY_NONE;` |
| [146](../Src/usart.c#L146) | HAL/NVIC | `huart2.Init.Mode = UART_MODE_TX_RX;` |
| [147](../Src/usart.c#L147) | HAL/NVIC | `huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [148](../Src/usart.c#L148) | HAL/NVIC | `huart2.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [171](../Src/usart.c#L171) | HAL/NVIC | `huart3.Init.BaudRate = 9600;` |
| [172](../Src/usart.c#L172) | HAL/NVIC | `huart3.Init.WordLength = UART_WORDLENGTH_8B;` |
| [173](../Src/usart.c#L173) | HAL/NVIC | `huart3.Init.StopBits = UART_STOPBITS_1;` |
| [174](../Src/usart.c#L174) | HAL/NVIC | `huart3.Init.Parity = UART_PARITY_NONE;` |
| [175](../Src/usart.c#L175) | HAL/NVIC | `huart3.Init.Mode = UART_MODE_TX_RX;` |
| [176](../Src/usart.c#L176) | HAL/NVIC | `huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [177](../Src/usart.c#L177) | HAL/NVIC | `huart3.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [200](../Src/usart.c#L200) | HAL/NVIC | `huart6.Init.BaudRate = 115200;` |
| [201](../Src/usart.c#L201) | HAL/NVIC | `huart6.Init.WordLength = UART_WORDLENGTH_8B;` |
| [202](../Src/usart.c#L202) | HAL/NVIC | `huart6.Init.StopBits = UART_STOPBITS_1;` |
| [203](../Src/usart.c#L203) | HAL/NVIC | `huart6.Init.Parity = UART_PARITY_NONE;` |
| [204](../Src/usart.c#L204) | HAL/NVIC | `huart6.Init.Mode = UART_MODE_TX_RX;` |
| [205](../Src/usart.c#L205) | HAL/NVIC | `huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;` |
| [206](../Src/usart.c#L206) | HAL/NVIC | `huart6.Init.OverSampling = UART_OVERSAMPLING_16;` |
| [244](../Src/usart.c#L244) | HAL/NVIC | `hdma_uart7_rx.Init.Channel = DMA_CHANNEL_5;` |
| [245](../Src/usart.c#L245) | HAL/NVIC | `hdma_uart7_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [246](../Src/usart.c#L246) | HAL/NVIC | `hdma_uart7_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [247](../Src/usart.c#L247) | HAL/NVIC | `hdma_uart7_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [248](../Src/usart.c#L248) | HAL/NVIC | `hdma_uart7_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [249](../Src/usart.c#L249) | HAL/NVIC | `hdma_uart7_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [250](../Src/usart.c#L250) | HAL/NVIC | `hdma_uart7_rx.Init.Mode = DMA_NORMAL;` |
| [251](../Src/usart.c#L251) | HAL/NVIC | `hdma_uart7_rx.Init.Priority = DMA_PRIORITY_LOW;` |
| [252](../Src/usart.c#L252) | HAL/NVIC | `hdma_uart7_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [253](../Src/usart.c#L253) | HAL/NVIC | `hdma_uart7_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;` |
| [254](../Src/usart.c#L254) | HAL/NVIC | `hdma_uart7_rx.Init.MemBurst = DMA_MBURST_SINGLE;` |
| [255](../Src/usart.c#L255) | HAL/NVIC | `hdma_uart7_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;` |
| [264](../Src/usart.c#L264) | HAL/NVIC | `HAL_NVIC_SetPriority(UART7_IRQn, 5, 0);` |
| [265](../Src/usart.c#L265) | HAL/NVIC | `HAL_NVIC_EnableIRQ(UART7_IRQn);` |
| [293](../Src/usart.c#L293) | HAL/NVIC | `hdma_uart8_rx.Init.Channel = DMA_CHANNEL_5;` |
| [294](../Src/usart.c#L294) | HAL/NVIC | `hdma_uart8_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [295](../Src/usart.c#L295) | HAL/NVIC | `hdma_uart8_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [296](../Src/usart.c#L296) | HAL/NVIC | `hdma_uart8_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [297](../Src/usart.c#L297) | HAL/NVIC | `hdma_uart8_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [298](../Src/usart.c#L298) | HAL/NVIC | `hdma_uart8_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [299](../Src/usart.c#L299) | HAL/NVIC | `hdma_uart8_rx.Init.Mode = DMA_NORMAL;` |
| [300](../Src/usart.c#L300) | HAL/NVIC | `hdma_uart8_rx.Init.Priority = DMA_PRIORITY_LOW;` |
| [301](../Src/usart.c#L301) | HAL/NVIC | `hdma_uart8_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [302](../Src/usart.c#L302) | HAL/NVIC | `hdma_uart8_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;` |
| [303](../Src/usart.c#L303) | HAL/NVIC | `hdma_uart8_rx.Init.MemBurst = DMA_MBURST_SINGLE;` |
| [304](../Src/usart.c#L304) | HAL/NVIC | `hdma_uart8_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;` |
| [314](../Src/usart.c#L314) | HAL/NVIC | `hdma_uart8_tx.Init.Channel = DMA_CHANNEL_5;` |
| [315](../Src/usart.c#L315) | HAL/NVIC | `hdma_uart8_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;` |
| [316](../Src/usart.c#L316) | HAL/NVIC | `hdma_uart8_tx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [317](../Src/usart.c#L317) | HAL/NVIC | `hdma_uart8_tx.Init.MemInc = DMA_MINC_ENABLE;` |
| [318](../Src/usart.c#L318) | HAL/NVIC | `hdma_uart8_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [319](../Src/usart.c#L319) | HAL/NVIC | `hdma_uart8_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [320](../Src/usart.c#L320) | HAL/NVIC | `hdma_uart8_tx.Init.Mode = DMA_NORMAL;` |
| [321](../Src/usart.c#L321) | HAL/NVIC | `hdma_uart8_tx.Init.Priority = DMA_PRIORITY_LOW;` |
| [322](../Src/usart.c#L322) | HAL/NVIC | `hdma_uart8_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [331](../Src/usart.c#L331) | HAL/NVIC | `HAL_NVIC_SetPriority(UART8_IRQn, 5, 0);` |
| [332](../Src/usart.c#L332) | HAL/NVIC | `HAL_NVIC_EnableIRQ(UART8_IRQn);` |
| [360](../Src/usart.c#L360) | HAL/NVIC | `hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;` |
| [361](../Src/usart.c#L361) | HAL/NVIC | `hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [362](../Src/usart.c#L362) | HAL/NVIC | `hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [363](../Src/usart.c#L363) | HAL/NVIC | `hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [364](../Src/usart.c#L364) | HAL/NVIC | `hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [365](../Src/usart.c#L365) | HAL/NVIC | `hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [366](../Src/usart.c#L366) | HAL/NVIC | `hdma_usart1_rx.Init.Mode = DMA_NORMAL;` |
| [367](../Src/usart.c#L367) | HAL/NVIC | `hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;` |
| [368](../Src/usart.c#L368) | HAL/NVIC | `hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [377](../Src/usart.c#L377) | HAL/NVIC | `HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);` |
| [378](../Src/usart.c#L378) | HAL/NVIC | `HAL_NVIC_EnableIRQ(USART1_IRQn);` |
| [406](../Src/usart.c#L406) | HAL/NVIC | `hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;` |
| [407](../Src/usart.c#L407) | HAL/NVIC | `hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [408](../Src/usart.c#L408) | HAL/NVIC | `hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [409](../Src/usart.c#L409) | HAL/NVIC | `hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [410](../Src/usart.c#L410) | HAL/NVIC | `hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [411](../Src/usart.c#L411) | HAL/NVIC | `hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [412](../Src/usart.c#L412) | HAL/NVIC | `hdma_usart2_rx.Init.Mode = DMA_NORMAL;` |
| [413](../Src/usart.c#L413) | HAL/NVIC | `hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;` |
| [414](../Src/usart.c#L414) | HAL/NVIC | `hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [423](../Src/usart.c#L423) | HAL/NVIC | `HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);` |
| [424](../Src/usart.c#L424) | HAL/NVIC | `HAL_NVIC_EnableIRQ(USART2_IRQn);` |
| [452](../Src/usart.c#L452) | HAL/NVIC | `hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;` |
| [453](../Src/usart.c#L453) | HAL/NVIC | `hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [454](../Src/usart.c#L454) | HAL/NVIC | `hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [455](../Src/usart.c#L455) | HAL/NVIC | `hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [456](../Src/usart.c#L456) | HAL/NVIC | `hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [457](../Src/usart.c#L457) | HAL/NVIC | `hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [458](../Src/usart.c#L458) | HAL/NVIC | `hdma_usart3_rx.Init.Mode = DMA_NORMAL;` |
| [459](../Src/usart.c#L459) | HAL/NVIC | `hdma_usart3_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;` |
| [460](../Src/usart.c#L460) | HAL/NVIC | `hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [470](../Src/usart.c#L470) | HAL/NVIC | `hdma_usart3_tx.Init.Channel = DMA_CHANNEL_7;` |
| [471](../Src/usart.c#L471) | HAL/NVIC | `hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;` |
| [472](../Src/usart.c#L472) | HAL/NVIC | `hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [473](../Src/usart.c#L473) | HAL/NVIC | `hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;` |
| [474](../Src/usart.c#L474) | HAL/NVIC | `hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [475](../Src/usart.c#L475) | HAL/NVIC | `hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [476](../Src/usart.c#L476) | HAL/NVIC | `hdma_usart3_tx.Init.Mode = DMA_NORMAL;` |
| [477](../Src/usart.c#L477) | HAL/NVIC | `hdma_usart3_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;` |
| [478](../Src/usart.c#L478) | HAL/NVIC | `hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [487](../Src/usart.c#L487) | HAL/NVIC | `HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);` |
| [488](../Src/usart.c#L488) | HAL/NVIC | `HAL_NVIC_EnableIRQ(USART3_IRQn);` |
| [516](../Src/usart.c#L516) | HAL/NVIC | `hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;` |
| [517](../Src/usart.c#L517) | HAL/NVIC | `hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;` |
| [518](../Src/usart.c#L518) | HAL/NVIC | `hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [519](../Src/usart.c#L519) | HAL/NVIC | `hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;` |
| [520](../Src/usart.c#L520) | HAL/NVIC | `hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [521](../Src/usart.c#L521) | HAL/NVIC | `hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [522](../Src/usart.c#L522) | HAL/NVIC | `hdma_usart6_rx.Init.Mode = DMA_NORMAL;` |
| [523](../Src/usart.c#L523) | HAL/NVIC | `hdma_usart6_rx.Init.Priority = DMA_PRIORITY_HIGH;` |
| [524](../Src/usart.c#L524) | HAL/NVIC | `hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [534](../Src/usart.c#L534) | HAL/NVIC | `hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;` |
| [535](../Src/usart.c#L535) | HAL/NVIC | `hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;` |
| [536](../Src/usart.c#L536) | HAL/NVIC | `hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;` |
| [537](../Src/usart.c#L537) | HAL/NVIC | `hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;` |
| [538](../Src/usart.c#L538) | HAL/NVIC | `hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;` |
| [539](../Src/usart.c#L539) | HAL/NVIC | `hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;` |
| [540](../Src/usart.c#L540) | HAL/NVIC | `hdma_usart6_tx.Init.Mode = DMA_NORMAL;` |
| [541](../Src/usart.c#L541) | HAL/NVIC | `hdma_usart6_tx.Init.Priority = DMA_PRIORITY_HIGH;` |
| [542](../Src/usart.c#L542) | HAL/NVIC | `hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;` |
| [551](../Src/usart.c#L551) | HAL/NVIC | `HAL_NVIC_SetPriority(USART6_IRQn, 5, 0);` |
| [552](../Src/usart.c#L552) | HAL/NVIC | `HAL_NVIC_EnableIRQ(USART6_IRQn);` |
| [580](../Src/usart.c#L580) | HAL/NVIC | `HAL_NVIC_DisableIRQ(UART7_IRQn);` |
| [604](../Src/usart.c#L604) | HAL/NVIC | `HAL_NVIC_DisableIRQ(UART8_IRQn);` |
| [627](../Src/usart.c#L627) | HAL/NVIC | `HAL_NVIC_DisableIRQ(USART1_IRQn);` |
| [650](../Src/usart.c#L650) | HAL/NVIC | `HAL_NVIC_DisableIRQ(USART2_IRQn);` |
| [674](../Src/usart.c#L674) | HAL/NVIC | `HAL_NVIC_DisableIRQ(USART3_IRQn);` |
| [698](../Src/usart.c#L698) | HAL/NVIC | `HAL_NVIC_DisableIRQ(USART6_IRQn);` |

## Src/usbd_conf.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [95](../Src/usbd_conf.c#L95) | HAL/NVIC | `HAL_NVIC_SetPriority(OTG_FS_IRQn, 5, 0);` |
| [96](../Src/usbd_conf.c#L96) | HAL/NVIC | `HAL_NVIC_EnableIRQ(OTG_FS_IRQn);` |
| [127](../Src/usbd_conf.c#L127) | HAL/NVIC | `HAL_NVIC_DisableIRQ(OTG_FS_IRQn);` |
| [140](../Src/usbd_conf.c#L140) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [142](../Src/usbd_conf.c#L142) | 预处理 | `#else` |
| [144](../Src/usbd_conf.c#L144) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [155](../Src/usbd_conf.c#L155) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [157](../Src/usbd_conf.c#L157) | 预处理 | `#else` |
| [159](../Src/usbd_conf.c#L159) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [170](../Src/usbd_conf.c#L170) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [172](../Src/usbd_conf.c#L172) | 预处理 | `#else` |
| [174](../Src/usbd_conf.c#L174) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [184](../Src/usbd_conf.c#L184) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [186](../Src/usbd_conf.c#L186) | 预处理 | `#else` |
| [188](../Src/usbd_conf.c#L188) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [198](../Src/usbd_conf.c#L198) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [200](../Src/usbd_conf.c#L200) | 预处理 | `#else` |
| [202](../Src/usbd_conf.c#L202) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [231](../Src/usbd_conf.c#L231) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [233](../Src/usbd_conf.c#L233) | 预处理 | `#else` |
| [235](../Src/usbd_conf.c#L235) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [256](../Src/usbd_conf.c#L256) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [258](../Src/usbd_conf.c#L258) | 预处理 | `#else` |
| [260](../Src/usbd_conf.c#L260) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [274](../Src/usbd_conf.c#L274) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [276](../Src/usbd_conf.c#L276) | 预处理 | `#else` |
| [278](../Src/usbd_conf.c#L278) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [289](../Src/usbd_conf.c#L289) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [291](../Src/usbd_conf.c#L291) | 预处理 | `#else` |
| [293](../Src/usbd_conf.c#L293) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [303](../Src/usbd_conf.c#L303) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [305](../Src/usbd_conf.c#L305) | 预处理 | `#else` |
| [307](../Src/usbd_conf.c#L307) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [317](../Src/usbd_conf.c#L317) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [319](../Src/usbd_conf.c#L319) | 预处理 | `#else` |
| [321](../Src/usbd_conf.c#L321) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [344](../Src/usbd_conf.c#L344) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.dev_endpoints = 4;` |
| [345](../Src/usbd_conf.c#L345) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;` |
| [346](../Src/usbd_conf.c#L346) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;` |
| [347](../Src/usbd_conf.c#L347) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;` |
| [348](../Src/usbd_conf.c#L348) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;` |
| [349](../Src/usbd_conf.c#L349) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;` |
| [350](../Src/usbd_conf.c#L350) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;` |
| [351](../Src/usbd_conf.c#L351) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;` |
| [352](../Src/usbd_conf.c#L352) | HAL/NVIC | `hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;` |
| [358](../Src/usbd_conf.c#L358) | 预处理 | `#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)` |
| [372](../Src/usbd_conf.c#L372) | 预处理 | `#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */` |
| [612](../Src/usbd_conf.c#L612) | 预处理 | `#ifdef USBD_HS_TESTMODE_ENABLE` |
| [626](../Src/usbd_conf.c#L626) | 预处理 | `#endif /* USBD_HS_TESTMODE_ENABLE */` |

## Src/usbd_desc.c

| 行 | 类型 | 源码 |
| --- | --- | --- |
| [66](../Src/usbd_desc.c#L66) | 预处理 | `#define USBD_VID     1155` |
| [67](../Src/usbd_desc.c#L67) | 预处理 | `#define USBD_LANGID_STRING     1033` |
| [68](../Src/usbd_desc.c#L68) | 预处理 | `#define USBD_MANUFACTURER_STRING     "YueLuEmbedded"` |
| [69](../Src/usbd_desc.c#L69) | 预处理 | `#define USBD_PID_FS     202` |
| [70](../Src/usbd_desc.c#L70) | 预处理 | `#if USB_TELEMETRY_ENABLE` |
| [71](../Src/usbd_desc.c#L71) | 预处理 | `#define USBD_PRODUCT_STRING_FS     "A Board Telemetry"` |
| [72](../Src/usbd_desc.c#L72) | 预处理 | `#else` |
| [73](../Src/usbd_desc.c#L73) | 预处理 | `#define USBD_PRODUCT_STRING_FS     "Vision Comm port"` |
| [74](../Src/usbd_desc.c#L74) | 预处理 | `#endif` |
| [75](../Src/usbd_desc.c#L75) | 预处理 | `#define USBD_CONFIGURATION_STRING_FS     "CDC Config"` |
| [76](../Src/usbd_desc.c#L76) | 预处理 | `#define USBD_INTERFACE_STRING_FS     "CDC Interface"` |
| [78](../Src/usbd_desc.c#L78) | 预处理 | `#define USB_SIZ_BOS_DESC            0x0C` |
| [129](../Src/usbd_desc.c#L129) | 预处理 | `#if (USBD_LPM_ENABLED == 1)` |
| [131](../Src/usbd_desc.c#L131) | 预处理 | `#endif /* (USBD_LPM_ENABLED == 1) */` |
| [151](../Src/usbd_desc.c#L151) | 预处理 | `#if (USBD_LPM_ENABLED == 1)` |
| [153](../Src/usbd_desc.c#L153) | 预处理 | `#endif /* (USBD_LPM_ENABLED == 1) */` |
| [156](../Src/usbd_desc.c#L156) | 预处理 | `#if defined ( __ICCARM__ ) /* IAR Compiler */` |
| [158](../Src/usbd_desc.c#L158) | 预处理 | `#endif /* defined ( __ICCARM__ ) */` |
| [164](../Src/usbd_desc.c#L164) | 预处理 | `#if (USBD_LPM_ENABLED == 1)` |
| [168](../Src/usbd_desc.c#L168) | 预处理 | `#else` |
| [170](../Src/usbd_desc.c#L170) | 预处理 | `#endif /* (USBD_LPM_ENABLED == 1) */` |
| [190](../Src/usbd_desc.c#L190) | 预处理 | `#if (USBD_LPM_ENABLED == 1)` |
| [191](../Src/usbd_desc.c#L191) | 预处理 | `#if defined ( __ICCARM__ ) /* IAR Compiler */` |
| [193](../Src/usbd_desc.c#L193) | 预处理 | `#endif /* defined ( __ICCARM__ ) */` |
| [210](../Src/usbd_desc.c#L210) | 预处理 | `#endif /* (USBD_LPM_ENABLED == 1) */` |
| [221](../Src/usbd_desc.c#L221) | 预处理 | `#if defined ( __ICCARM__ ) /* IAR Compiler */` |
| [223](../Src/usbd_desc.c#L223) | 预处理 | `#endif /* defined ( __ICCARM__ ) */` |
| [234](../Src/usbd_desc.c#L234) | 预处理 | `#if defined ( __ICCARM__ ) /* IAR Compiler */` |
| [236](../Src/usbd_desc.c#L236) | 预处理 | `#endif /* defined ( __ICCARM__ ) */` |
| [240](../Src/usbd_desc.c#L240) | 预处理 | `#if defined ( __ICCARM__ ) /*!< IAR Compiler */` |
| [242](../Src/usbd_desc.c#L242) | 预处理 | `#endif` |
| [373](../Src/usbd_desc.c#L373) | 预处理 | `#if (USBD_LPM_ENABLED == 1)` |
| [386](../Src/usbd_desc.c#L386) | 预处理 | `#endif /* (USBD_LPM_ENABLED == 1) */` |
