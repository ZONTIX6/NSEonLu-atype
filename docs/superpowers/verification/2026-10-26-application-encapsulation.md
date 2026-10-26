# APP 层封装整理验证记录

日期：2026-10-26

## 验证范围

本次只调整 `application/` 的目录、实现可见性和构建路径。控制算法、初始化顺序、任务优先级、周期、栈大小和板角色条件保持原值。

## 主机测试

- `tests/architecture/test_application_headers.py`：通过。两个独立消费者重复包含公共任务、设备和调参头文件，再执行可重定位链接，确认头文件不携带实现或存储对象。
- `tests/architecture/test_layers.py`：5 项通过；0 个已记录、新增或过期分层违规。
- `tests/structure_index`：55 项通过；结构索引生成及 stale 检查通过。
- `tests/ins_service`：INS 服务及 ONE/GIMBAL/CHASSIS 三种桥接角色通过。
- `tests/tuning_flash`：单板与底盘板保存门控、恢复和重试通过。
- `tests/oled`：SPI、I2C 基线及模拟按键范围/ADC 失败路径通过。
- `tests/chassis_force`：力控几何、协议、分配、功率、电机适配和板级未标定锁定通过。
- `tests/chassis_force/test_build_contracts.py`：宏合法性、后端符号隔离和 Release 有限浮点选项通过。

## 固件构建

| 构建 | 结果 | text | data | bss |
| --- | --- | ---: | ---: | ---: |
| Make 默认速控 | 通过 | 156064 | 1148 | 148512 |
| Make 直接力控 | 通过 | 162496 | 936 | 150120 |
| Make OLED/按键/辅助串口，裁判关闭 | 通过 | 152392 | 1148 | 147936 |
| Make 功率采集/UART 日志，裁判关闭 | 通过 | 151752 | 1148 | 161200 |
| CMake Debug 速控 | 通过 | 156080 | 1156 | 148528 |
| CMake Release 力控 | 通过 | 181288 | 944 | 149976 |

表中数据来自提交 `0afab3c` 的干净独立工作树。CMake 因自身构建选项与 Make 数值不同，只在同一构建系统内用于确认后端源选择、完整链接和编译选项。

## 尚需硬件确认

本次没有烧录。任务实时执行时间、栈余量、IMU 通信和可选外设引脚仍按原项目的上板验收流程确认；主机测试与交叉编译不证明实车时序。
