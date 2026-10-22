# Separate directories are required when changing compile-time options.
ifneq ($(origin UART1_USE),undefined)
$(error UART1_USE is retired: USART1 is DBUS; use REFEREE_ENABLE and LOG_OUTPUT)
endif
PROFILE ?= atype-vehicle
ifeq ($(filter $(PROFILE),atype-vehicle atype-bench atype-direct),)
$(error PROFILE must be atype-vehicle, atype-bench, or atype-direct)
endif
ifeq ($(PROFILE),atype-bench)
ATYPE_BENCH_MODE ?= 1
CHASSIS_POWER_BENCH_TEST ?= 1
CHASSIS_POWER_SOURCE_MODE ?= 0
else ifeq ($(PROFILE),atype-direct)
CHASSIS_POWER_SOURCE_MODE ?= 0
endif

# Same override list is consumed by CMake (config/atype-options.txt).
ATYPE_OPTIONS := $(shell powershell -NoProfile -Command "Get-Content config/atype-options.txt")
C_DEFS += $(foreach option,$(ATYPE_OPTIONS),$(if $($(option)),-D$(option)=$($(option))))

ifeq ($(ATYPE_LINK_MODULES),1)
ATYPE_LINK_SYMBOLS := $(shell powershell -NoProfile -Command "Get-Content config/atype-link-symbols.txt")
ATYPE_MODULE_LDFLAGS += $(foreach symbol,$(ATYPE_LINK_SYMBOLS),-Wl,--undefined=$(symbol))
endif
