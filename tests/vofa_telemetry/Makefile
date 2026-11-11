CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_vofa_telemetry.c ../../application/runtime/vofa_telemetry.c
HEADERS := stubs/usart.h ../../application/runtime/vofa_telemetry.h ../../Inc/board_config.h ../../modules/telemetry/usb_telemetry_format.h ../../application/chassis/chassis_velocity_observer.h
INCLUDES := -Istubs -I../../Inc -I../../application/runtime -I../../modules/telemetry -I../../modules/algorithm
MEASUREMENT_SOURCE := test_vofa_measurement.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
MEASUREMENT_INCLUDES := -I../../modules/chassis_power -I../../modules/power_meter -I../../modules/referee -I../../modules/super_cap -I../../application/chassis

.PHONY: test
test: test_vofa_telemetry.exe test_vofa_measurement.exe test_imu_capture.exe
	./test_vofa_telemetry.exe
	./test_vofa_measurement.exe
	./test_imu_capture.exe

test_vofa_telemetry.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=0 $(SOURCE) -o $@

test_vofa_measurement.exe: $(MEASUREMENT_SOURCE) $(HEADERS) ../../modules/chassis_power/chassis_power_types.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 $(MEASUREMENT_SOURCE) -o $@

IMU_SOURCE := test_imu_capture.c ../../application/runtime/robot_telemetry.c ../../application/runtime/vofa_telemetry.c ../../modules/telemetry/usb_telemetry_format.c
test_imu_capture.exe: $(IMU_SOURCE) $(HEADERS) ../../modules/imu/ins_task.h ../../Inc/imu_capture_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(MEASUREMENT_INCLUDES) -I../../modules/imu -DIMU_CAPTURE_ONLY=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 -DUSB_TELEMETRY_ENABLE=0 $(IMU_SOURCE) -o $@
