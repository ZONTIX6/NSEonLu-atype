#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

/* 编译期抑制分级日志：0=保留LOGINFO/LOGWARNING/LOGERROR（默认），非0=移除这些宏输出。
 * 通常设1关闭；示例：make DISABLE_LOG_SYSTEM=1。普通LOG/底层写入接口不受该宏影响。
 * 日志通道选择由board_config.h中的LOG_OUTPUT控制，此项不改变串口资源归属。 */
#ifndef DISABLE_LOG_SYSTEM
#define DISABLE_LOG_SYSTEM 0 /* 0=保留分级日志，1=禁用分级日志。 */
#endif

#endif
