# 双 A 板 IMU 采集与 116 mm 安装补偿

整车主板继续控制底盘、云台等设备；下方 A 板只采集 MPU6500、运行 INS/温控并发送 CAN 数据。两板都使用 `ONE_BOARD` 编译角色，采集板由 `IMU_CAPTURE_ONLY` 限制任务，并非 `CHASSIS_BOARD/GIMBAL_BOARD` 的控制分板模式。旧 DM 模块接收、协议换算和接收端重力扣除已移除。

## 安装坐标

用户图示为 **+X 前、+Y 右**，下方板位于图示 +Y 116 mm。融合内部几何坐标为 **+X 前、+Y 左、+Z 上**，所以 [远端配置](../config/30_velocity_observation/chassis_remote_imu_config.h) 中写入：

```c
#define CHASSIS_REMOTE_IMU_OFFSET_BODY_M {0.0f, -0.116f, 0.0f}
#define CHASSIS_REMOTE_IMU_SENSOR_TO_BODY {0.0f, 0.0f, 0.0f, 1.0f}
```

用户已确认 116 mm 量到 MPU6500 芯片位置，不是 A 板中心。源 INS 输出已去重力的导航系加速度，主板在融合前扣除旋转杆臂的二阶导数，包含向心和切向项；不修改原始 IMU 遥测，不重复补偿。安装旋转由官方图纸与安装照片推导为绕 Z 轴 180°，Hamilton `[w,x,y,z]=[0,0,0,1]`。推导尚未通过实物轴向测试，保持 `CHASSIS_REMOTE_IMU_CONFIG_VERIFIED=0`。

[RoboMaster 官方 A 板原理图与位号图](https://cdn-hz.robomaster.com/tem/RoboMaster%20%E5%BC%80%E5%8F%91%E6%9D%BF%E5%8E%9F%E7%90%86%E5%9B%BE%26%E4%BD%8D%E5%8F%B7%E5%9B%BE.zip) 中，XT30 的 J10 在左、USB 的 J14 在右，U3 的 1 脚位于芯片左下角。[TDK MPU-6500 数据手册第 36 页图 14](https://product.tdk.cn/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/mpu-6500-datasheet2.pdf) 将 1 脚角定义在 `(-X,+Y)`，+Z 朝封装上方。因此在官方板图中，传感器 +X 向上、+Y 向左。

照片中 USB 朝上、XT30 朝下，结合用户确认的“照片右方为车前、照片下方为车右”，可推导以下安装关系：

| 传感器轴 | 照片方向 | 实车方向 | 程序内部底盘轴 |
| --- | --- | --- | --- |
| +X | 左 | 后 | −X |
| +Y | 下 | 右 | −Y |
| +Z | 板面向上 | 上 | +Z |

对应 `v_body=(-v_sensor_x,-v_sensor_y,v_sensor_z)`。这只改变传感器到底盘的安装变换；图示右侧 116 mm 的位置向量仍为内部 `(0,-0.116,0) m`。

## 接线与构建

两板通过 CAN1 总线连接：CAN_H 对 CAN_H、CAN_L 对 CAN_L、参考地相连，使用收发器后的总线接口。当前速率 1 Mbps，标准 ID `0x520`；按整条总线两端设置终端电阻。主板 UART7 接 VOFA+ 接收器，下方板无需单独串口记录。

[板卡预设](../config/00_flash/board_flash_config.h) 的目标 `1` 为整车主板，目标 `2` 为下方采集板，各用独立目录：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Show -Board 2
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Build -Board 2
# 下载器连接对应物理板后，分别执行对应目标的烧录命令
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Flash -Board 1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-board.ps1 -Action Flash -Board 2
```

主板开启接收和远端融合；采集板开启 `IMU_CAPTURE_ONLY=1 IMU_CAPTURE_CAN_TX_ENABLE=1`，关闭整车观测、UART/USB 遥测和电机任务。脚本不会识别实际接入的板卡，构建通过也不代表已经烧录。

## 数据与时间

采集任务名义每 20 ms 尝试发送一组，由 9 个 8 字节 CAN 分片组成，携带四元数、去重力加速度、角速度、源样本序号、源毫秒时间、发送时源年龄和有效标志。主板校验顺序、tag、CRC16/CCITT-FALSE 和 10 ms 期限；缺片、错序或过期不刷新已发布数据。

两板时钟独立。主板用“首片接收时间 − 发送时源年龄”估计轮速配对时刻，中心补偿和滤波 dt 使用源板样本间隔。源年龄应 ≤20 ms，已知年龄下界应 ≤60 ms；首片仲裁等待和传感器内部延迟仍未测量。断流后四轮有效则回退轮速，恢复后重新预热；融合只用于观测。

日常 `config/_vofa/vofa_config.h` 保持 `VOFA_OBSERVATION_PROFILE=2`：主板 UART7、JustFloat、230400 / 8N1、20 ms 目标周期，schema 5 的 96 通道，`imu_source=1` 表示第二块 A 板。保存完整 `I0`–`I95` 与原表头。旧 profile 0/schema 3 保留 `I64=2` 的外部来源编码，bit 19 不再设置；两个 schema 的列号、来源编码和单位不同。

## 轴向与补偿检查

在底盘零输出、板固定的条件下，先静止等待 INS 初始化。用主板卡尔曼档的 `imu_gx_radps/imu_gy_radps/imu_gz_radps` 检查缓慢单轴转动时哪一轴响应及其正负；这些是传感器系角速度。按当前图纸推导，绕底盘前轴的右手正转应主要表现为 `gx<0`，绕底盘左轴正转为 `gy<0`，绕上轴正转为 `gz>0`。逐轴核对幅值主轴与符号，确认 `CHASSIS_REMOTE_IMU_SENSOR_TO_BODY`；若实物不同，先修正安装变换再评价偏移补偿。

再做水平及倾斜后静止检查：源导航系去重力加速度应回到接近零；导航系加速度不能直接当作图示车体系的前/右分量。轴向核对后，绕中心正反旋转并起停，比较原始 IMU 与融合输入中心加速度，检查 116 mm 杆臂补偿。缺少独立运动参考时，不将观察到 FUSED 当作精度验收；具体试次见 [实测手册](kalman-chassis-test-and-acceptance.md)。
