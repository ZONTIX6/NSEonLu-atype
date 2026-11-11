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
SOURCES = test_chassis_velocity_observer.c $(CHASSIS)/chassis_velocity_observer.c \
    $(ALGORITHM)/chassis_velocity.c $(ALGORITHM)/chassis_velocity_fusion.c \
    $(ALGORITHM)/chassis_velocity_center.c $(ALGORITHM)/chassis_gimbal_imu.c \
    $(TIME)/speed_observation_time.c
HEADERS = $(wildcard stubs/*.h) $(CHASSIS)/chassis_velocity_observer.h \
    $(CHASSIS)/chassis_internal.h $(wildcard $(ALGORITHM)/chassis_velocity*.h) \
    $(ALGORITHM)/chassis_gimbal_imu.h $(APPLICATION)/gimbal/gimbal.h \
    $(MODULES)/imu/ins_task.h $(TIME)/speed_observation_time.h

test_chassis_velocity_observer.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -include stubs/platform.h $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_velocity_observer.exe
	./test_chassis_velocity_observer.exe
