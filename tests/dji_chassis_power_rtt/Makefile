CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g \
          -ffunction-sections -fdata-sections -fno-fast-math
CFLAGS += $(foreach option,CHASSIS_SPEED_CAPTURE_ENABLE CHASSIS_SPEED_KF_SHADOW_ENABLE CHASSIS_PREDICTIVE_POWER_MODE LOG_OUTPUT,$(if $($(option)),-D$(option)=$($(option))))
ifneq ($(strip $(CHASSIS_POWER_CALIBRATION_ENABLE)),)
CFLAGS += -DCHASSIS_POWER_CALIBRATION_ENABLE=$(CHASSIS_POWER_CALIBRATION_ENABLE)
endif
LDFLAGS := -Wl,--gc-sections
ifneq ($(strip $(CHASSIS_POWER_CONTROL_STAGE)),)
CFLAGS += -DCHASSIS_POWER_CONTROL_STAGE=$(CHASSIS_POWER_CONTROL_STAGE)
endif
ifneq ($(strip $(POWER_METER_ENABLE)),)
CFLAGS += -DPOWER_METER_ENABLE=$(POWER_METER_ENABLE)
endif
TARGET := test_dji_chassis_power_rtt.exe
BYPASS_TARGET := test_dji_power_bypass.exe
SOURCE := test_dji_chassis_power_rtt.c \
          ../../modules/algorithm/power_model.c \
          ../../modules/algorithm/chassis_power_transient.c \
          ../../modules/telemetry/rtt_telemetry.c
SOURCE += ../../modules/telemetry/runtime_tuning.c
SOURCE += ../../modules/telemetry/pid_autotune.c ../../modules/telemetry/autotune_protocol.c
SOURCE += $(wildcard ../../modules/predictive_power/*.c) ../../modules/telemetry/chassis_speed_capture.c
SOURCE += ../../modules/algorithm/crc16.c
INCLUDES := -Istubs \
            -I../../modules/motor/DJImotor \
            -I../../modules/motor \
            -I../../modules/algorithm \
            -I../../modules/telemetry
INCLUDES += -I../../modules/vehicle_safety -I../../modules/power_meter -I../../application -I../../Inc

.PHONY: all test clean

all: $(TARGET) $(BYPASS_TARGET)

$(TARGET): $(SOURCE) $(wildcard ../../modules/motor/DJImotor/*.[ch])
	$(CC) $(CFLAGS) -DCHASSIS_POWER_CONTROL_ENABLE=1 $(INCLUDES) $(SOURCE) $(LDFLAGS) -lm -o $(TARGET)

$(BYPASS_TARGET): test_dji_power_bypass.c $(SOURCE) $(wildcard ../../modules/motor/DJImotor/*.[ch])
	$(CC) $(CFLAGS) -DCHASSIS_POWER_CONTROL_ENABLE=0 $(INCLUDES) test_dji_power_bypass.c $(filter-out test_dji_chassis_power_rtt.c,$(SOURCE)) $(LDFLAGS) -lm -o $@

test: $(TARGET) $(BYPASS_TARGET)
	./$(TARGET)
	./$(BYPASS_TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
	-if exist $(BYPASS_TARGET) del /Q $(BYPASS_TARGET)

TRACTION_SOURCE = test_traction_integration.c $(filter-out test_dji_chassis_power_rtt.c,$(SOURCE)) ../../modules/algorithm/chassis_traction.c
TRACTION_HEADERS := ../../modules/algorithm/chassis_traction.h ../../config/30_velocity_observation/chassis_traction_config.h
TRACTION_DEFINES := -DCHASSIS_VELOCITY_FUSION_ENABLE=1
test_traction_shadow.exe: $(TRACTION_SOURCE) $(TRACTION_HEADERS)
	$(CC) $(CFLAGS) $(TRACTION_DEFINES) -DCHASSIS_POWER_CONTROL_ENABLE=0 -DCHASSIS_TRACTION_MODE=1 $(INCLUDES) $(TRACTION_SOURCE) $(LDFLAGS) -lm -o $@
test_traction_limit.exe: $(TRACTION_SOURCE) $(TRACTION_HEADERS)
	$(CC) $(CFLAGS) $(TRACTION_DEFINES) -DCHASSIS_POWER_CONTROL_ENABLE=0 -DCHASSIS_TRACTION_MODE=2 -DCHASSIS_TRACTION_CALIBRATED=1 $(INCLUDES) $(TRACTION_SOURCE) $(LDFLAGS) -lm -o $@
test_traction_brake.exe: $(TRACTION_SOURCE) $(TRACTION_HEADERS)
	$(CC) $(CFLAGS) $(TRACTION_DEFINES) -DCHASSIS_POWER_CONTROL_ENABLE=0 -DCHASSIS_TRACTION_MODE=3 -DCHASSIS_TRACTION_CALIBRATED=1 $(INCLUDES) $(TRACTION_SOURCE) $(LDFLAGS) -lm -o $@
test_traction_power.exe: $(TRACTION_SOURCE) $(TRACTION_HEADERS)
	$(CC) $(CFLAGS) $(TRACTION_DEFINES) -DCHASSIS_POWER_CONTROL_ENABLE=1 -DCHASSIS_TRACTION_MODE=3 -DCHASSIS_TRACTION_CALIBRATED=1 $(INCLUDES) $(TRACTION_SOURCE) $(LDFLAGS) -lm -o $@
.PHONY: test_traction
test: test_traction
test_traction: test_traction_shadow.exe test_traction_limit.exe test_traction_brake.exe test_traction_power.exe
	./test_traction_shadow.exe
	./test_traction_limit.exe
	./test_traction_brake.exe
	./test_traction_power.exe

SOURCE += $(filter-out ../../modules/chassis_power/chassis_power.c,$(wildcard ../../modules/chassis_power/*.c)) ../../modules/algorithm/chassis_power_budget.c ../../modules/algorithm/chassis_power_outer_loop.c
INCLUDES += -I../../modules/chassis_power -I../../modules/super_cap -I../../modules/referee
INCLUDES += -I../../modules/predictive_power
