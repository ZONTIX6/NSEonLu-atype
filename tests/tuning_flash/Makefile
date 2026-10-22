CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCES = ../../bsp/flash/bsp_tuning_flash.c ../../application/tuning_policy.c test_flash.c ../../modules/telemetry/tuning_flash.c ../../modules/telemetry/tuning_storage.c ../../modules/telemetry/runtime_tuning.c
test_flash.exe: $(SOURCES) $(wildcard stubs/*.h ../../modules/telemetry/tuning*.h ../../modules/telemetry/runtime_tuning.h)
	$(CC) $(CFLAGS) -Istubs -I../../modules/telemetry -I../../application -I../../modules/vehicle_safety -I../../bsp/flash -I../dji_chassis_power_rtt/stubs $(SOURCES) -o $@
test_flash_dual.exe: $(SOURCES) $(wildcard stubs/*.h)
	$(CC) $(CFLAGS) -DCHASSIS_BOARD -Istubs -I../../modules/telemetry -I../../application -I../../modules/vehicle_safety -I../../bsp/flash $(SOURCES) -o $@
test: test_flash.exe test_flash_dual.exe
	./test_flash.exe
	./test_flash_dual.exe
