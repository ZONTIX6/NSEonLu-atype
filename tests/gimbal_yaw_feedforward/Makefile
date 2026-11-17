CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -ffast-math -fno-fast-math
SOURCE = test_gimbal_yaw_feedforward.c ../../application/gimbal/gimbal_yaw_feedforward.c
HEADERS = ../../application/gimbal/gimbal_yaw_feedforward.h ../../config/03_gimbal/gimbal_config.h
.PHONY: test
test: test_feedforward.exe test_disabled.exe test_bad_gain.exe
	./test_feedforward.exe
	./test_disabled.exe
	./test_bad_gain.exe
test_feedforward.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCE) -lm -o $@
test_disabled.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -DYAW_SPIN_FEEDFORWARD_ENABLE=0 $(SOURCE) -lm -o $@
test_bad_gain.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) -DYAW_SPIN_FEEDFORWARD_GAIN=-1.0f $(SOURCE) -lm -o $@
