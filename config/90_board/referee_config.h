#ifndef REFEREE_CONFIG_H
#define REFEREE_CONFIG_H

#define REFEREE_POWER_DATA_TIMEOUT_MS 300U /* 裁判功率/热量数据有效期，单位ms；超过后按离线处理。 */

/* 旧版裁判功率字段：0=不解析旧功率字段（默认），1=启用偏移4处的旧功率字段。
 * 仅用于已核实含该字段的协议（例如2024版）；2025版虽也有16字节帧，该区域为保留字节。
 * 切换示例：make REFEREE_LEGACY_POWER_FIELDS_ENABLED=1；须与实际裁判协议匹配。 */
#ifndef REFEREE_LEGACY_POWER_FIELDS_ENABLED
#define REFEREE_LEGACY_POWER_FIELDS_ENABLED 0 /* 0=禁用旧字段，1=启用已验证的旧字段。 */
#endif

#if REFEREE_LEGACY_POWER_FIELDS_ENABLED != 0 && REFEREE_LEGACY_POWER_FIELDS_ENABLED != 1
#error "REFEREE_LEGACY_POWER_FIELDS_ENABLED must be 0 or 1"
#endif

#endif
