# chassis

2026-10-24：默认恢复原随动；自适应直供自转新增先基础功率、再缓冲增补的策略。参数和遥测见 [自转功率说明](chassis-power-priority.md)。


## 底盘类型选择

底盘通过 `application/robot_def.h` 中的 `CHASSIS_DRIVE_TYPE` 在编译期选择：

```c
// 默认配置，保持原有麦克纳姆轮行为
#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_MECANUM

// 四轮 X 型全向轮
#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_OMNI_X
```

四轮位置和 CAN ID 沿用现有接线：左前 1、右前 4、左后 2、右后 3。底盘坐标系以向前为 `+x`、向右为 `+y`；X 型全向轮按 45° 安装，当前控制正解算只对平移量做 `1/sqrt(2)` 投影，Wz 沿用工程指令标度。实际速度逆解算使用物理几何：默认 350 mm × 300 mm 底盘的旋转有效力臂约为 229.81 mm，详见 [底盘速度反馈](chassis-velocity.md)。

首次调试必须架空车轮，并依次给出纯前进、纯右移和原地旋转命令。如果只有某个车轮方向相反，将对应的 `OMNI_X_LF_DIRECTION`、`OMNI_X_RF_DIRECTION`、`OMNI_X_LB_DIRECTION` 或 `OMNI_X_RB_DIRECTION` 从 `1.0f` 改为 `-1.0f`，不要修改运动学公式。

## 跟随档策略

`application/robot_def.h` 的 `CHASSIS_FOLLOW_MODE` 控制原跟随档：

| 宏值 | 平移参考 | 旋转目标 |
| --- | --- | --- |
| `1` | 云台当前实测朝向为前方 | 每周期 `wz = 0`，车身不主动跟转 |
| `0`（默认） | 云台当前实测朝向为前方 | 原比例跟转：`clamp(-50 * theta_format(offset_angle), -4500, +4500)` |

遥控器右侧开关中档、图传 X 键计数模 4 等于 2 时进入该档，云台仍使用 `GIMBAL_GYRO_MODE`。模式编号保持 `CHASSIS_FOLLOW_GIMBAL_YAW`，遥测验收需结合构建宏和 `wz`；未新增运行时按键或 Flash 参数。该宏独立于轮型 `CHASSIS_DRIVE_TYPE` 与自旋 `CHASSIS_SPIN_MODE`，非法数值会编译失败。

默认策略下，云台朝侧面时前进会沿该侧移动，朝后时前进会沿车身后方移动，后退及左右平移同样以云台为参考；边移动边转云台，移动方向随实际朝向连续改变。坐标转换继续使用实测 yaw 编码器与 `YAW_CHASSIS_ALIGN_ECD` 得到的相对角，保持原公式且只转换一次：

```text
chassis_vx = vx * cos(theta) - vy * sin(theta)
chassis_vy = vx * sin(theta) + vy * cos(theta)
theta = offset_angle * DEGREE_2_RAD
```

`wz` 使用原工程命令单位，不是电机 rpm。默认策略让车身跟转对齐云台；宏为 `1` 时每周期将旋转目标清零，但不提供绝对航向锁定。从小陀螺切入时当周期改用所选策略，实际转速仍受闭环和功率约束影响。

编译覆盖使用独立目录，切换后需重新构建并烧录：

```powershell
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-original CHASSIS_FOLLOW_MODE=0
make -j4 PROFILE=atype-vehicle BUILD_DIR=build/follow-gimbal-forward CHASSIS_FOLLOW_MODE=1
cmake -S . -B build/cmake-follow-0 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=0
cmake --build build/cmake-follow-0 -j4
cmake -S . -B build/cmake-follow-1 -G "MinGW Makefiles" -DATYPE_PROFILE=atype-vehicle -DCHASSIS_FOLLOW_MODE=1
cmake --build build/cmake-follow-1 -j4
```

双板部署时记录两板配置，策略在执行底盘控制的固件中生效。`CHASSIS_NO_FOLLOW`、小陀螺、零力、急停与失联处理沿用既有路径；跟随档仍不具备 `NO_FOLLOW` 的自动整定许可。

主机回归运行 `make -C tests/chassis_motion test CC=gcc`，覆盖默认值、两策略、角度环绕、平移与模式切换，并检查非法值。实车先架空核对轮向，再低速分别验证云台朝前/侧/后及移动中转动云台，遥控器和图传各验证一次；两策略分别检查小陀螺进出与急停。软件验证不代表已经完成烧录或实车验收。

## 自转功率余量调节

`modules/algorithm/chassis_spin_config.h` 中的宏用于对比：

```c
#define CHASSIS_SPIN_ADAPTIVE_ENABLE 0 // 默认：固定 wz=4000
// 改为 1：在跟上目标且仍有功率余量时逐步提高自转目标
```

也可以用 Make 参数覆盖；两组必须使用不同构建目录，避免复用不同宏的对象文件：

```powershell
New-Item -ItemType Directory -Force build/spin-fixed, build/spin-adaptive | Out-Null
make -j8 BUILD_DIR=build/spin-fixed CHASSIS_SPIN_ADAPTIVE_ENABLE=0
make -j8 BUILD_DIR=build/spin-adaptive CHASSIS_SPIN_ADAPTIVE_ENABLE=1
```

CMake 支持 `-DCHASSIS_SPIN_ADAPTIVE_ENABLE=0` 或 `1`；显式 `CHASSIS_SPIN_MODE=1` 优先选择自适应模式。自适应模式控制停用或反馈失效时输出零，恢复后重新从零按斜率建立目标；健康 IDLE 状态可以建立新的驱动需求。

直供且缓冲控制开启时，先跟踪有效限功率的 98%，新功率样本连续 100 ms 达标且能量充足后才开放缓冲增补。正常增长确认时间为 50 ms；四轮误差不超过 500 rpm 时正常增长，真实加速趋势允许有限的 25% 速率增长。模型基础限流时暂停目标上堆，由功率慢环校正模型；电流硬限、快保护、制动和数据降级禁止增长。

自适应默认增益 80，功率低通 50 ms，目标上升/下降率 1200/2400 Wz/s；稳态边界仍由 CHASSIS_SPIN_MIN_WZ、CHASSIS_SPIN_MAX_WZ 和运行时实际参数决定。入场斜坡可低于稳态最小目标，防止直接跳到高目标。固定/正弦参数独立处理。有效 Flash 设置会覆盖编译默认，USB CSV v3 可核对实际响应参数和来源。

完整阶段、门槛、获准功率额度和实车验收见 [自转功率说明](chassis-power-priority.md)。平移命令保持原值，平移消耗增加会通过总功率误差降低自转目标；尚未引入平移优先功率分配。达到机械、电流或转速能力边界时，应结合受限原因解释未用满功率的情况。

## 正弦自转

显式选择 `CHASSIS_SPIN_MODE=2` 可启用同方向正弦自转；`0` 为固定，`1` 为功率自适应。模式宏优先于旧开关。正弦参数集中于 `modules/algorithm/chassis_spin_config.h`：

| 参数 | 默认值 | 含义 |
| --- | --- | --- |
| `CHASSIS_SPIN_SINE_CENTER_WZ` | 800 | 正弦中心，独立于固定/自适应目标 |
| `CHASSIS_SPIN_SINE_MIN_WZ` | 200 | 起步及反馈失效时的同方向低速目标 |
| `CHASSIS_SPIN_SINE_AMPLITUDE_WZ` | 200 | 稳态振幅 |
| `CHASSIS_SPIN_SINE_PERIOD_S` | 4 | 周期，秒 |
| `CHASSIS_SPIN_SINE_RISE_RATE` | 400 | 正向增长速率上限，命令单位/秒 |

正常稳态指令为 `800 + 200 * sin(2*pi*t/4)`，方向由 `CHASSIS_SPIN_SINE_DIRECTION` 决定并在一次自转中锁定。以上 `wz` 均是现有工程的命令单位。当前默认几何、没有平移且经驱动边界除以 6 后，电机目标约为 567..945 rpm。进入自转后先等待保护和跟随条件稳定 200 ms，从 200 起步升到中心，再用 1 s 渐入振幅。

转速换算以当前源码输出链路为准：轮速解算结果经 `vt/6` 形成 rpm，驱动速度环使用度/秒。历史方案中的其他链路换算值不能直接套用。受限时正弦波形仍可能缩小或暂停，实车负载需要上板确认。

缓冲增补按电机需求变化，其正常撤销不会再按新旧预算比例缩放正弦中心、振幅或当前转速，也不单独重置稳定确认。实测功率超出当前预算时仍按已有速率降低中心并收缩振幅；限流或能量恢复时收缩振幅并暂停升速。目标始终受正常升降斜率约束，最终功率、电流硬保护继续直接限制电机输出。`CHASSIS_SPIN_BUDGET_DROP_RESET_W` 仅保留给功率自适应模式。

使用独立目录构建，避免复用其他模式的对象文件：

```powershell
New-Item -ItemType Directory -Force build/sine-fix-usb | Out-Null
make -j4 BUILD_DIR=build/sine-fix-usb CHASSIS_SPIN_MODE=2 USB_TELEMETRY_ENABLE=1 REFEREE_ENABLE=1 CHASSIS_POWER_CONTROL_STAGE=3
```

上板后在 USB 仪表盘同时查看四轮**目标和实际 rpm**，等待起步完成，观察至少三个 4 s 周期。先检查目标是否产生正弦，再比较实际转速的跟随程度；如目标有波动而实际没有，结合功率计、输出电流及反馈在线状态判断限流或跟随能力。主机测试包含基础功率模型、最终预测保护和独立惯性负载；它不等同于实机验证。

## 工作流程

首先进行初始化，`ChasissInit()`会被`RobotInit()`调用，进行裁判系统、底盘电机的初始化。如果为双板模式，则还会初始化IMU，并且将消息订阅者和发布者的初始化改为`CANComm`的初始化。

操作系统启动后，工作顺序为：

1. 从cmd模块获取数据（如果双板则从CANComm获取）
2. 判断当前控制数据的模式，如果为停止则停止所有电机
3. 根据控制数据，计算底盘的旋转速度
4. 根据控制数据中yaw电机的编码器值`angle_offset`，将控制数据映射到底盘坐标系下
5. 根据编译期选择进行麦克纳姆轮或四轮 X 型全向轮运动学解算，得到每个电机的设定值
6. 获取裁判系统的数据，并根据底盘功率限制对输出进行限幅
7. 由四轮 CAN 速度反馈估算底盘几何中心速度，同时换算下发轮速对应的目标速度
8. 更新统一指针 `chassis_velocity` 的目标/实际速度和 USB 遥测快照；另行更新裁判系统反馈数据
9. 将反馈数据推送到消息中心（如果双板则通过CANComm发送）


### 后续支持平衡底盘

新增一个app balance_chassis

### 整车速度读取

`chassis_velocity->target/actual` 分别提供目标/实际 `vx_mps`、`vy_mps`、`wz_rpm`（m/s、m/s、转/分钟）。跨任务读取用 `ChassisGetVelocitySnapshot()`。指针、单位、有效性和兼容的 CSV 字段见 [底盘速度说明](chassis-velocity.md)。
