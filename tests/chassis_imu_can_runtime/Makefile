CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ROOT = ../..
INCLUDES = -Istubs -I$(ROOT)/Inc -I$(ROOT)/application -I$(ROOT)/application/runtime -I$(ROOT)/modules/imu -I$(ROOT)/modules/algorithm -I$(ROOT)/modules/telemetry
SOURCES = test_runtime.c $(ROOT)/application/runtime/chassis_imu_can.c $(ROOT)/modules/imu/chassis_imu_can_protocol.c
HEADERS = $(wildcard stubs/*.h) $(ROOT)/application/runtime/chassis_imu_can.h $(ROOT)/modules/imu/chassis_imu_can_protocol.h $(ROOT)/Inc/imu_capture_config.h $(ROOT)/config/32_capture/imu_capture_config.h
COMMON = -DROBOT_DEF_H -DONE_BOARD -DCHASSIS_CONTROL_MODE=0 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 -DUSB_TELEMETRY_ENABLE=0

.PHONY: test
test: test_tx.exe test_rx.exe test_rx_justfloat.exe
	./test_tx.exe
	./test_rx.exe
	./test_rx_justfloat.exe
test_tx.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(COMMON) -DIMU_CAPTURE_ONLY=1 -DIMU_CAPTURE_CAN_TX_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_ENABLE=0 -DVOFA_TELEMETRY_ENABLE=0 $(SOURCES) -o $@
test_rx.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(COMMON) -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 $(SOURCES) -o $@

test_rx_justfloat.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(COMMON) -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 $(SOURCES) -o $@
