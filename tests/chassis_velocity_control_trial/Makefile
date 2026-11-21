CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
SOURCE = test_chassis_velocity_control_trial.c ../../application/chassis/chassis_velocity_control_trial.c
HEADERS = ../../application/chassis/chassis_velocity_control_trial.h

test_chassis_velocity_control_trial.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCE) -lm -o $@

.PHONY: test
test: test_chassis_velocity_control_trial.exe
	./test_chassis_velocity_control_trial.exe
