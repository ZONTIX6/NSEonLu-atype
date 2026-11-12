CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ROOT = ../..
MODULES = $(ROOT)/modules
APPLICATION = $(ROOT)/application
ALGORITHM = $(MODULES)/algorithm
TIME = $(MODULES)/predictive_power
CHASSIS = $(APPLICATION)/chassis
DEFINES = -DONE_BOARD -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=1
INCLUDES = -Istubs -I$(ALGORITHM) -I$(TIME) -I$(CHASSIS) -I$(APPLICATION)/gimbal -I$(MODULES)/imu
PRODUCTION_SOURCES = $(CHASSIS)/chassis_velocity_observer.c \
    $(ALGORITHM)/chassis_velocity.c $(ALGORITHM)/chassis_velocity_fusion.c \
    $(ALGORITHM)/chassis_velocity_center.c $(ALGORITHM)/chassis_gimbal_imu.c \
    $(TIME)/speed_observation_time.c
SOURCES = test_chassis_velocity_observer.c $(PRODUCTION_SOURCES)
HEADERS = $(ROOT)/config/30_velocity_observation/chassis_velocity_config.h $(ROOT)/config/30_velocity_observation/speed_observation_config.h $(wildcard stubs/*.h) $(CHASSIS)/chassis_velocity_observer.h \
    $(CHASSIS)/chassis_internal.h $(wildcard $(ALGORITHM)/chassis_velocity*.h) \
    $(ALGORITHM)/chassis_gimbal_imu.h $(ALGORITHM)/chassis_remote_fusion.h $(APPLICATION)/gimbal/gimbal.h \
    $(MODULES)/imu/ins_task.h $(TIME)/speed_observation_time.h

test_chassis_velocity_observer.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -include stubs/platform.h $(SOURCES) -lm -o $@

test_chassis_velocity_diagnostics.exe: test_chassis_velocity_diagnostics.c $(PRODUCTION_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -include stubs/platform.h $< $(PRODUCTION_SOURCES) -lm -o $@

test_chassis_velocity_uncalibrated.exe: test_chassis_velocity_diagnostics.c $(PRODUCTION_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(filter-out -DCHASSIS_VELOCITY_FUSION_CALIBRATED=1,$(DEFINES)) \
	    -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 $(INCLUDES) -include stubs/platform.h $< $(PRODUCTION_SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_velocity_observer.exe test_chassis_velocity_diagnostics.exe test_chassis_velocity_uncalibrated.exe test_chassis_remote_observer.exe
	./test_chassis_velocity_observer.exe
	./test_chassis_velocity_diagnostics.exe
	./test_chassis_velocity_uncalibrated.exe
	./test_chassis_remote_observer.exe

test_chassis_remote_observer.exe: test_chassis_remote_observer.c $(PRODUCTION_SOURCES) $(ALGORITHM)/chassis_remote_fusion.c $(ALGORITHM)/chassis_remote_fusion.h $(ROOT)/config/30_velocity_observation/chassis_remote_imu_config.h $(HEADERS)
	$(CC) $(CFLAGS) -DONE_BOARD -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 \
	    -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 $(INCLUDES) \
	    -I$(ROOT)/Inc -I$(APPLICATION)/runtime -include stubs/platform.h $< $(PRODUCTION_SOURCES) $(ALGORITHM)/chassis_remote_fusion.c -lm -o $@
