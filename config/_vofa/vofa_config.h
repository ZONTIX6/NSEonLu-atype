#ifndef VOFA_CONFIG_H
#define VOFA_CONFIG_H

/* VOFA 日常观测档位：只修改下方数字，重新编译并烧录。
 * 1 = 功率/超级电容（schema 4，96 路）；2 = 卡尔曼闭环（FIFO schema 9，64 路，默认；快照 schema 7，108 路）。
 * 3 = 防滑诊断（schema 10，72路）；0 = 旧 schema 2/3；仅 VOFA_TELEMETRY_MODE=2 时生效。
 * 固件与 scripts/build-board.ps1 共用此默认值；保持单个整数定义，勿写表达式。
 * 显式 Make/CMake 参数或自定义烧录预设中的同名选项可覆盖它。 */
#ifndef VOFA_OBSERVATION_PROFILE
#define VOFA_OBSERVATION_PROFILE 2
#endif

/* 日常使用精简闭环诊断；置 1 恢复 109 路 schema 8，供滤波重放使用。
 * 仅影响远端融合 FIFO，不改变控制参数。 */
#ifndef VOFA_KALMAN_FULL_DETAIL
#define VOFA_KALMAN_FULL_DETAIL 0
#endif
#if VOFA_KALMAN_FULL_DETAIL != 0 && VOFA_KALMAN_FULL_DETAIL != 1
#error VOFA_KALMAN_FULL_DETAIL must be 0 or 1
#endif

#endif
