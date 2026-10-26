CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES = -I../../modules/chassis_force_calibration -I../../application/board
CORE = ../../modules/chassis_force_calibration/force_calibration.c ../../application/board/board_force_calibration.c
.PHONY: test test_engine test_capture
test_engine.exe: test_engine.c test_support.h $(CORE)
	$(CC) $(CFLAGS) $(INCLUDES) test_engine.c $(CORE) -lm -o $@
test_engine: test_engine.exe
	./test_engine.exe
test: test_engine
test_capture.exe: test_capture.c ../../modules/chassis_force_calibration/force_calibration_capture.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -lm -o $@
test_capture: test_capture.exe
	./test_capture.exe
test: test_capture
