CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
IMU = ../../modules/imu
SOURCES = test_dm_imu_can_protocol.c $(IMU)/dm_imu_can_protocol.c

test_dm_imu_can_protocol.exe: $(SOURCES) $(IMU)/dm_imu_can_protocol.h $(IMU)/chassis_imu_sample.h ../../config/92_imu/dm_imu_config.h
	$(CC) $(CFLAGS) -I$(IMU) $(SOURCES) -lm -o $@

.PHONY: test
test: test_dm_imu_can_protocol.exe
	./test_dm_imu_can_protocol.exe
