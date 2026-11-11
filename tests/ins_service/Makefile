CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_ins_service.c ../../modules/mpu6500/mpu6500_math.c ../../config/01_pid/pid_config.c
INCLUDES := -I../atype/imu_stubs -I../../modules/imu -I../../modules/mpu6500 -I../../config/01_pid
.PHONY: test
test: test_ins_service.exe test_ins_capture.exe test_bridge_one.exe test_bridge_gimbal.exe test_bridge_chassis.exe
	./test_ins_service.exe
	./test_ins_capture.exe
	./test_bridge_one.exe
	./test_bridge_gimbal.exe
	./test_bridge_chassis.exe
test_ins_service.exe: $(SOURCE) ../../modules/imu/ins_service.c ../../config/01_pid/pid_config.h ../../config/92_imu/imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $@
test_ins_capture.exe: $(SOURCE) ../../modules/imu/ins_service.c ../../config/01_pid/pid_config.h ../../config/92_imu/imu_config.h
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_VELOCITY_FUSION_ENABLE=0 -DIMU_CAPTURE_ONLY=1 $(SOURCE) -lm -o $@
test_bridge_one.exe: test_ins_bridge.c ../../application/runtime/ins_bridge.c
	$(CC) $(CFLAGS) -Istubs $(INCLUDES) -I../../application -DROBOT_DEF_H -DONE_BOARD $^ -o $@
test_bridge_gimbal.exe: test_ins_bridge.c ../../application/runtime/ins_bridge.c
	$(CC) $(CFLAGS) -Istubs $(INCLUDES) -I../../application -DROBOT_DEF_H -DGIMBAL_BOARD $^ -o $@
test_bridge_chassis.exe: test_ins_bridge.c ../../application/runtime/ins_bridge.c
	$(CC) $(CFLAGS) -Istubs $(INCLUDES) -I../../application -DROBOT_DEF_H -DCHASSIS_BOARD $^ -o $@
