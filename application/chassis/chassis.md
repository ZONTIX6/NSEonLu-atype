# chassis


## 底盘类型选择

底盘通过 `application/robot_def.h` 中的 `CHASSIS_DRIVE_TYPE` 在编译期选择：

```c
// 默认配置，保持原有麦克纳姆轮行为
#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_MECANUM

// 四轮 X 型全向轮
#define CHASSIS_DRIVE_TYPE CHASSIS_DRIVE_OMNI_X
```

四轮位置和 CAN ID 沿用现有接线：左前 1、右前 2、左后 4、右后 3。底盘坐标系以向前为 `+x`、向右为 `+y`；X 型全向轮按 45° 安装，运动学会对轮子滚动方向做 `1/sqrt(2)` 投影。

首次调试必须架空车轮，并依次给出纯前进、纯右移和原地旋转命令。如果只有某个车轮方向相反，将对应的 `OMNI_X_LF_DIRECTION`、`OMNI_X_RF_DIRECTION`、`OMNI_X_LB_DIRECTION` 或 `OMNI_X_RB_DIRECTION` 从 `1.0f` 改为 `-1.0f`，不要修改运动学公式。

## 工作流程

首先进行初始化，`ChasissInit()`会被`RobotInit()`调用，进行裁判系统、底盘电机的初始化。如果为双板模式，则还会初始化IMU，并且将消息订阅者和发布者的初始化改为`CANComm`的初始化。

操作系统启动后，工作顺序为：

1. 从cmd模块获取数据（如果双板则从CANComm获取）
2. 判断当前控制数据的模式，如果为停止则停止所有电机
3. 根据控制数据，计算底盘的旋转速度
4. 根据控制数据中yaw电机的编码器值`angle_offset`，将控制数据映射到底盘坐标系下
5. 根据编译期选择进行麦克纳姆轮或四轮 X 型全向轮运动学解算，得到每个电机的设定值
6. 获取裁判系统的数据，并根据底盘功率限制对输出进行限幅
7. 由电机的反馈数据和IMU（如果有），计算底盘当前的真实运动速度
8. 设置底盘反馈数据，包括运动速度和裁判系统数据
9. 将反馈数据推送到消息中心（如果双板则通过CANComm发送）


### 后续支持平衡底盘

新增一个app balance_chassis
