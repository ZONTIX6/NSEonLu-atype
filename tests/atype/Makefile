CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I../../application
.PHONY: test
test: test_safety.exe test_flash_bounds.exe test_imu_math.exe test_pwm_units.exe test_imu_init.exe
	./test_safety.exe
	./test_flash_bounds.exe
	./test_imu_math.exe
	./test_pwm_units.exe
	./test_imu_init.exe
test_imu_init.exe: test_imu_init.c ../../modules/mpu6500/mpu6500.c ../../modules/mpu6500/mpu6500_math.c
	$(CC) $(CFLAGS) -Iimu_stubs -Ipwm_stubs -I../../modules/imu -I../../modules/mpu6500 test_imu_init.c ../../modules/mpu6500/mpu6500_math.c -lm -o $@
test_pwm_units.exe: test_pwm_units.c ../../modules/mpu6500/imu_temperature.c
	$(CC) $(CFLAGS) -Ipwm_stubs -I../../modules/mpu6500 $^ -lm -o $@
test_imu_math.exe: test_imu_math.c ../../modules/mpu6500/mpu6500_math.c ../../modules/mpu6500/mpu6500_math.h ../../modules/imu/ins_math.c
	$(CC) $(CFLAGS) -I../../modules/mpu6500 -I../../modules/imu test_imu_math.c ../../modules/mpu6500/mpu6500_math.c ../../modules/imu/ins_math.c -lm -o $@
test_flash_bounds.exe: test_flash_bounds.c ../../bsp/flash/bsp_flash.c ../../bsp/flash/bsp_flash.h
	$(CC) $(CFLAGS) -I../tuning_flash/stubs -I../../bsp/flash test_flash_bounds.c ../../bsp/flash/bsp_flash.c -o $@
test_safety.exe: test_safety.c ../../application/vehicle_safety.c ../../application/vehicle_safety.h
	$(CC) $(CFLAGS) test_safety.c ../../application/vehicle_safety.c -o $@
