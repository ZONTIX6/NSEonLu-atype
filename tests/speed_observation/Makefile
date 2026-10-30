CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
INCLUDES = -I../../modules/telemetry -I../../modules/algorithm -I../../modules/predictive_power
.PHONY: test
test_capture.exe: test_capture.c ../../modules/telemetry/chassis_speed_capture.c ../../modules/telemetry/chassis_speed_capture.h ../../modules/algorithm/crc16.c
	$(CC) $(CFLAGS) $(INCLUDES) test_capture.c ../../modules/telemetry/chassis_speed_capture.c ../../modules/algorithm/crc16.c -lm -o $@
test_time.exe: test_time.c ../../modules/predictive_power/speed_observation_time.c ../../modules/predictive_power/speed_observation_time.h
	$(CC) $(CFLAGS) $(INCLUDES) test_time.c ../../modules/predictive_power/speed_observation_time.c -lm -o $@
RUNTIME_INCLUDES = -Istubs -I../../application/runtime -I../dji_chassis_power_rtt/stubs -I../../modules/motor/DJImotor -I../../modules/motor -I../../modules/chassis_power -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee -I../../bsp/log
test_runtime.exe: test_runtime.c ../../application/runtime/chassis_speed_observation.c ../../modules/telemetry/chassis_speed_capture.c ../../modules/algorithm/crc16.c
	$(CC) $(filter-out -pedantic,$(CFLAGS)) $(RUNTIME_INCLUDES) $(INCLUDES) -DCHASSIS_SPEED_CAPTURE_ENABLE=1 -DLOG_OUTPUT=2 test_runtime.c ../../application/runtime/chassis_speed_observation.c ../../modules/telemetry/chassis_speed_capture.c ../../modules/algorithm/crc16.c -o $@
test: test_capture.exe test_time.exe test_runtime.exe
	./test_capture.exe
	./test_time.exe
	./test_runtime.exe
