CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ALGORITHM = ../../modules/algorithm
SOURCES = test_chassis_velocity_fusion.c $(ALGORITHM)/chassis_velocity_fusion.c

test_chassis_velocity_fusion.exe: $(SOURCES) $(ALGORITHM)/chassis_velocity_fusion.h
	$(CC) $(CFLAGS) -I$(ALGORITHM) $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_velocity_fusion.exe
	./test_chassis_velocity_fusion.exe
