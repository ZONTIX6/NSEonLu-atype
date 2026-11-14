CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
IMU = ../../modules/imu
SOURCES = test_chassis_imu_can_protocol.c $(IMU)/chassis_imu_can_protocol.c

test_chassis_imu_can_protocol.exe: $(SOURCES) $(IMU)/chassis_imu_can_protocol.h $(IMU)/chassis_imu_sample.h
	$(CC) $(CFLAGS) -I$(IMU) $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_imu_can_protocol.exe
	./test_chassis_imu_can_protocol.exe
