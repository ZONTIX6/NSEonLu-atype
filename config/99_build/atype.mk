# 构建配置入口；不同编译选项使用独立BUILD_DIR，避免混用固件产物。
# 用 make PROFILE=atype-vehicle/atype-bench/atype-direct 切换整车/台架/直供配置。
ifneq ($(origin UART1_USE),undefined)
$(error UART1_USE is retired: USART1 is DBUS; use REFEREE_ENABLE and LOG_OUTPUT)
endif
# 默认整车配置；台架开启下方门控与功率回退，直供显式选择电源模式0。
PROFILE ?= atype-vehicle
ifeq ($(filter $(PROFILE),atype-vehicle atype-bench atype-direct),)
$(error PROFILE must be atype-vehicle, atype-bench, or atype-direct)
endif
ifeq ($(PROFILE),atype-bench)
# 台架门控：1=开启并锁停云台/发射；0=关闭。
ATYPE_BENCH_MODE ?= 1
# 台架功率回退：1=裁判数据无效时允许回退；0=禁止。
CHASSIS_POWER_BENCH_TEST ?= 1
# 台架供电：0=直供，1=超级电容。
CHASSIS_POWER_SOURCE_MODE ?= 0
else ifeq ($(PROFILE),atype-direct)
# 直供配置固定默认0；命令行可显式覆盖。
CHASSIS_POWER_SOURCE_MODE ?= 0
endif

# Make与CMake共用atype-options.txt中的宏转发白名单；这里只登记名字，值由各配置头或命令行提供。
# 文本清单逐行读取，不能在txt行内追加注释；各宏中文用途见对应配置头和README导航。
ATYPE_OPTIONS := $(shell powershell -NoProfile -Command "Get-Content config/99_build/atype-options.txt")
C_DEFS += $(foreach option,$(ATYPE_OPTIONS),$(if $($(option)),-D$(option)=$($(option))))

ifeq ($(ATYPE_LINK_MODULES),1)
# 可选模块链接检查：ATYPE_LINK_MODULES=1强制保留下表符号，0或未传则不强制链接。
# 只用于构建/接口覆盖，不代表已注册这些硬件设备。
ATYPE_LINK_SYMBOLS := $(shell powershell -NoProfile -Command "Get-Content config/99_build/atype-link-symbols.txt")
ATYPE_MODULE_LDFLAGS += $(foreach symbol,$(ATYPE_LINK_SYMBOLS),-Wl,--undefined=$(symbol))
endif
