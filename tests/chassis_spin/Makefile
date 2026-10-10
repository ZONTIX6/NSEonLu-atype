CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_chassis_spin.c ../../modules/algorithm/chassis_spin.c
HEADERS := ../../modules/algorithm/chassis_spin.h ../../modules/algorithm/chassis_spin_config.h
INCLUDES := -I../../modules/algorithm
TARGETS := test_chassis_spin_fixed.exe test_chassis_spin_adaptive.exe

.PHONY: all test clean
all: $(TARGETS)

test_chassis_spin_fixed.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_SPIN_ADAPTIVE_ENABLE=0 $(SOURCE) -lm -o $@

test_chassis_spin_adaptive.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DCHASSIS_SPIN_ADAPTIVE_ENABLE=1 $(SOURCE) -lm -o $@

test: $(TARGETS)
	./test_chassis_spin_fixed.exe
	./test_chassis_spin_adaptive.exe

clean:
	-if exist test_chassis_spin_fixed.exe del /Q test_chassis_spin_fixed.exe
	-if exist test_chassis_spin_adaptive.exe del /Q test_chassis_spin_adaptive.exe
