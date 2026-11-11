CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
ROOT = ../..
INCLUDES = -Istubs -I$(ROOT)/Inc -I$(ROOT)/application -I$(ROOT)/application/runtime -I$(ROOT)/modules/telemetry -I$(ROOT)/modules/algorithm -I$(ROOT)/modules/imu
DEFINES = -DIMU_CAPTURE_ONLY=1 -DROBOT_DEF_H -DONE_BOARD -DROBOT_DEF_PARAM_WARNING -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 -DUSB_TELEMETRY_ENABLE=0

.PHONY: test
test: test_startup.exe test_tasks.exe test_can_startup.exe test_can_tasks.exe
	./test_startup.exe
	./test_tasks.exe
	./test_can_startup.exe
	./test_can_tasks.exe

test_startup.exe: test_startup.c $(ROOT)/application/robot.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) test_startup.c $(ROOT)/application/robot.c -o $@

test_tasks.exe: test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h
	$(CC) $(CFLAGS) -Wno-unused-parameter $(INCLUDES) $(DEFINES) test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c -o $@

test_can_startup.exe: test_startup.c $(ROOT)/application/robot.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(filter-out -DVOFA_TELEMETRY_ENABLE=1,$(DEFINES)) -DVOFA_TELEMETRY_ENABLE=0 -DIMU_CAPTURE_CAN_TX_ENABLE=1 test_startup.c $(ROOT)/application/robot.c -o $@

test_can_tasks.exe: test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h
	$(CC) $(CFLAGS) -Wno-unused-parameter $(INCLUDES) $(filter-out -DVOFA_TELEMETRY_ENABLE=1,$(DEFINES)) -DVOFA_TELEMETRY_ENABLE=0 -DIMU_CAPTURE_CAN_TX_ENABLE=1 test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c -o $@
