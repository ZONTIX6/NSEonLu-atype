CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_mpu6500.c ../../modules/mpu6500/mpu6500driver.c ../../modules/mpu6500/mpu6500_math.c
.PHONY: test
test: test_mpu6500.exe
	./test_mpu6500.exe
	./test_mpu6500.exe failure
test_mpu6500.exe: $(SOURCE)
	$(CC) $(CFLAGS) -Istubs -I../bsp_spi/stubs -I../../bsp/spi -I../../modules/mpu6500 -I../../modules/imu $(SOURCE) -lm -o $@
