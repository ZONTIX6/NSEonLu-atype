#ifndef VOFA_CONFIG_H
#define VOFA_CONFIG_H

/* VOFA 日常观测档位：只修改下方数字，重新编译并烧录。
 * 1 = 功率/超级电容（schema 4，96 路）；2 = 卡尔曼/云台（schema 6，102 路，默认）。
 * 0 = 旧 schema 2/3；本选项仅在 VOFA_TELEMETRY_MODE=2 时生效。
 * 固件与 scripts/build-board.ps1 共用此默认值；保持单个整数定义，勿写表达式。
 * 显式 Make/CMake 参数或自定义烧录预设中的同名选项可覆盖它。 */
#ifndef VOFA_OBSERVATION_PROFILE
#define VOFA_OBSERVATION_PROFILE 2
#endif

#endif
