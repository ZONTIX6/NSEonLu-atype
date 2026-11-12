CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
ROOT = ../..
INCLUDES = -Istubs -I$(ROOT)/Inc -I$(ROOT)/application -I$(ROOT)/application/runtime -I$(ROOT)/modules/telemetry -I$(ROOT)/modules/algorithm -I$(ROOT)/modules/imu
DEFINES = -DIMU_CAPTURE_ONLY=1 -DROBOT_DEF_H -DONE_BOARD -DROBOT_DEF_PARAM_WARNING -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=1 -DUSB_TELEMETRY_ENABLE=0

.PHONY: test
test: test_startup.exe test_tasks.exe
	./test_startup.exe
	./test_tasks.exe

test_startup.exe: test_startup.c $(ROOT)/application/robot.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h $(ROOT)/config/32_capture/imu_capture_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) test_startup.c $(ROOT)/application/robot.c -o $@

test_tasks.exe: test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c $(wildcard stubs/*.h) $(ROOT)/Inc/imu_capture_config.h $(ROOT)/config/32_capture/imu_capture_config.h
	$(CC) $(CFLAGS) -Wno-unused-parameter $(INCLUDES) $(DEFINES) test_tasks.c $(ROOT)/application/runtime/robot_task.c $(ROOT)/Src/freertos.c -o $@
