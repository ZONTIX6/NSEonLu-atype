#ifndef USB_TELEMETRY_CONFIG_H
#define USB_TELEMETRY_CONFIG_H

/* USB用途：1=CSV网页遥测（默认），0=关闭该遥测，释放USB供视觉等用途。
 * 切换USB视觉需同时设置 VISION_USE_VCP=1 USB_TELEMETRY_ENABLE=0；
 * 仅关闭遥测不会自动启用视觉，串口视觉独立配置。 */
#ifndef USB_TELEMETRY_ENABLE
#define USB_TELEMETRY_ENABLE 1 /* USB遥测开关：0=关闭，1=开启；与USB视觉互斥。 */
#endif
#if USB_TELEMETRY_ENABLE != 0 && USB_TELEMETRY_ENABLE != 1
#error "USB_TELEMETRY_ENABLE must be 0 or 1"
#endif
#ifndef USB_TELEMETRY_PERIOD_MS
#define USB_TELEMETRY_PERIOD_MS 20U /* 遥测数据帧发送周期，单位ms；默认20（50Hz）。 */
#endif
#ifndef USB_TELEMETRY_HEADER_PERIOD_MS
#define USB_TELEMETRY_HEADER_PERIOD_MS 1000U /* CSV列名表头重发周期，单位ms；默认1000。 */
#endif
#ifndef USB_TELEMETRY_BUFFER_SIZE
#define USB_TELEMETRY_BUFFER_SIZE 3072U /* USB遥测发送缓冲区容量，单位字节；默认3072。 */
#endif

#endif
