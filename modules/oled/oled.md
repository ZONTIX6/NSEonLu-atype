# OLED 后端与模拟按键

应用延后启动阶段先调用 `OLEDRegister` 注入后端和配置，再使用原有 `OLED_init`、绘图和刷新接口。SPI 后端使用 SPI/GPIO BSP，A 板配置 SPI1、无软件片选、PB9 DC、PB10 RST；I²C 后端使用 IIC BSP，设备地址填 7 位 `0x3C`，BSP 只左移一次。显示模块不读取 ADC。

`OLED_Init_Config_s` 还可注入返回有效按键值的 `read_button` 回调和 context。A 板通过 `BoardAnalogKeyRead` 读取 PA6/ADC1 IN6，并检查启动、转换、完成状态和停止结果；任一步失败均返回 false 和无按键。显示层 `OLEDReadButton`/旧 SPI `GetOlED_Button` 只返回有效键。

未注册时旧显示入口不隐式注册硬件。重复注册拒绝；半注册对象由 BSP 保留，业务不发布。初始化/整帧刷新调用 `OLEDTransportBegin`；一笔传输失败后停止本帧剩余传输，增加失败计数，下一次完整初始化/刷新可重试。`OLEDTransportHealthy` 和 `OLEDTransportFailureCount` 提供结果查询。SPI 每字节事务上限 10 ms，I²C 每次两字节帧超时 10 ms；仅在原延后启动或应用显示路径调用。

两种后端保留原初始化与绘图字节流。`tests/oled` 将新实现与迁移前实际输出夹具逐字节比较（SPI 2128 字节，I²C 2124 字节），另测未注册、重复注册、传输失败及 ADC 失败。
