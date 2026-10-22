# bsp_log

<p align='right'>neozng1@hnu.edu.cn</p>

## 使用说明

A 板日志出口由 `Inc/board_config.h` 的 `LOG_OUTPUT` 选择，默认 1。

| LOG_OUTPUT | 出口 |
| --- | --- |
| 0 | 关闭 |
| 1 | RTT 通道 0 |
| 2 | USART2 TX/PD5，115200 8N1 |
| 3 | RTT 与 USART2 |

USART1 固定用于 DBUS；裁判系统独立使用 USART6。旧 `UART1_USE` 已废止，显式传入会报错。

```powershell
make -j8 BUILD_DIR=build/atype-uart-log LOG_OUTPUT=2
```

USART2 TX/PD5 接串口适配器 RX 并共地；发送使用非阻塞中断队列，不占 TX DMA。队列空间不足时整条消息丢弃。辅助设备不能同时占用 USART2，板级配置会检查冲突。格式化日志移除 ANSI 颜色码，原始 `BSPLogWrite*` 保持字节内容。

`DISABLE_LOG_SYSTEM` 默认 0，可构建覆盖为 1 来关闭分级日志宏；`PrintLog`、`BSPLogWrite*` 仍按 `LOG_OUTPUT` 选择出口。J-Scope 二进制使用 RTT 通道 1，不与文本混流。功控高速采样配置要求 `LOG_OUTPUT=2/3`，使用 USART2 独立传输。

推荐使用`bsp_log.h`中提供了三级日志：

```c
#define LOGINFO(format,...)
#define LOGWARNING(format,...)
#define LOGERROR(format,...)
```

分别用于输出不同等级的日志。注意RTT不支持直接使用`%f`进行浮点格式化,要使用`void Float2Str(char *str, float va);`转化成字符串之后再发送。

## 代码放置约定

- 普通分级日志继续使用`bsp_log.h`中的`LOG`、`LOGINFO`、`LOGWARNING`和`LOGERROR`。
- 结构化RTT数据（CSV表头、字段格式、输出缓冲区）统一放在`modules/telemetry/rtt_telemetry.c/.h`。
- 业务模块只调用上述接口，不直接调用`SEGGER_RTT_*`。

**若想启用RTT，必须通过`launch.json`的`debug-jlink`启动调试（不论使用什么调试器）。** 按照`VSCode+Ozone环境配置`完成配置之后的cmsis dap和daplink是可以支持Jlink全家桶的。

另外，若你使用的是cmsis-dap和daplink，**请在 *jlink* 调试任务启动之后再打开`log`任务。**（均在项目文件夹下的.vsocde/task.json中，有注释自行查看）。否则可能出线RTT viewer无法连接客户端的情况。

在ozone中查看log输出，直接打开console调试任务台和terminal调试中断便可看到调试输出。

> 由于ozone版本的原因，可能出现日志不换行或没有颜色。

## 自定义输出

你也可以自定义输出格式，详见Segger RTT的文档。

```c
int PrintLog(const char *fmt, ...);
void Float2Str(char *str, float va); // 输出浮点需要先用此函数进行转换
```

调用第一个函数，可以通过jlink或dap-link向调试器连接的上位机发送信息，格式和printf相同，示例如下：

```c
PrintLog("Hello World!\n");
PrintLog("Motor %d met some problem, error code %d!\n",3,1);
```

第二个函数可以将浮点类型转换成字符串以方便发送：

```c
float current_feedback=114.514;
char* str_buff[64];
Float2Str(str_buff,current_feedback);
PrintLog("Motor %d met some problem, error code %d!\n",3,1);
```
