CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ALGORITHM = ../../modules/algorithm
SOURCES = test_chassis_remote_fusion.c $(ALGORITHM)/chassis_remote_fusion.c \
    $(ALGORITHM)/chassis_velocity_fusion.c $(ALGORITHM)/chassis_velocity_center.c \
    $(ALGORITHM)/chassis_gimbal_imu.c $(ALGORITHM)/chassis_wheel_redundancy.c
HEADERS = $(ALGORITHM)/chassis_remote_fusion.h $(ALGORITHM)/chassis_velocity.h \
    $(ALGORITHM)/chassis_velocity_fusion.h $(ALGORITHM)/chassis_velocity_center.h \
    $(ALGORITHM)/chassis_gimbal_imu.h $(ALGORITHM)/chassis_wheel_redundancy.h ../../modules/imu/chassis_imu_sample.h \
    ../../config/30_velocity_observation/chassis_remote_imu_config.h

test_chassis_remote_fusion.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -I$(ALGORITHM) $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_remote_fusion.exe test_chassis_remote_fusion_shadow.exe test_chassis_remote_fusion_active.exe
	./test_chassis_remote_fusion.exe
	./test_chassis_remote_fusion_shadow.exe
	./test_chassis_remote_fusion_active.exe

test_chassis_remote_fusion_shadow.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DCHASSIS_WHEEL_REDUNDANCY_MODE=1 -I$(ALGORITHM) $(SOURCES) -lm -o $@

test_chassis_remote_fusion_active.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DCHASSIS_WHEEL_REDUNDANCY_MODE=2 -I$(ALGORITHM) $(SOURCES) -lm -o $@
