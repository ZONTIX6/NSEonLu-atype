CC = gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g
INCLUDES := -Istubs -I../../application/chassis -I../../application -I../../modules \
            -I../../modules/algorithm -I../../modules/imu -I../../modules/master_machine \
            -I../../modules/referee -I../../modules/telemetry
SOURCE := test_chassis_motion.c ../../modules/algorithm/user_lib.c
HEADERS := $(wildcard stubs/*.h ../../application/chassis/chassis_motion.h) \
           ../../application/robot_def.h ../../modules/algorithm/user_lib.h
TARGETS := test_default.exe test_original.exe test_gimbal_forward.exe

.PHONY: all test invalid
all: $(TARGETS)

test_default.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DEXPECT_GIMBAL_FORWARD=1 $(SOURCE) -lm -o $@
test_original.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_FOLLOW_MODE=0 -DEXPECT_GIMBAL_FORWARD=0 $(SOURCE) -lm -o $@
test_gimbal_forward.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_FOLLOW_MODE=1 -DEXPECT_GIMBAL_FORWARD=1 $(SOURCE) -lm -o $@

invalid:
	powershell -NoProfile -File check_invalid.ps1 -Compiler "$(CC)" -Flags "$(CFLAGS) $(INCLUDES)"

test: all invalid
	./test_default.exe
	./test_original.exe
	./test_gimbal_forward.exe
