CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ROOT = ../..
ALG = $(ROOT)/modules/algorithm
INCLUDES = -Istubs -I$(ROOT)/Inc -I$(ROOT)/application -I$(ROOT)/application/runtime -I$(ROOT)/modules/imu -I$(ALG) -I$(ROOT)/modules/telemetry
FLAGS = -DROBOT_DEF_H -DONE_BOARD -DCHASSIS_CONTROL_MODE=0 -DCHASSIS_VELOCITY_FUSION_CALIBRATED=0 -DCHASSIS_VELOCITY_FUSION_ENABLE=1 -DCHASSIS_REMOTE_IMU_FUSION_ENABLE=1 -DCHASSIS_IMU_CAN_RX_ENABLE=1 -DVOFA_TELEMETRY_ENABLE=1 -DUSB_TELEMETRY_ENABLE=0
SOURCES = test_runtime.c $(ROOT)/application/runtime/chassis_imu_can.c $(ROOT)/modules/imu/dm_imu_can_protocol.c $(ALG)/chassis_remote_fusion.c $(ALG)/chassis_velocity_center.c $(ALG)/chassis_velocity_fusion.c $(ALG)/chassis_gimbal_imu.c
.PHONY: test
test:
	$(CC) $(CFLAGS) $(INCLUDES) $(FLAGS) -DVOFA_TELEMETRY_MODE=1 $(SOURCES) -lm -o test_runtime.exe
	./test_runtime.exe
	$(CC) $(CFLAGS) $(INCLUDES) $(FLAGS) -DVOFA_TELEMETRY_MODE=2 $(SOURCES) -lm -o test_runtime_justfloat.exe
	./test_runtime_justfloat.exe
