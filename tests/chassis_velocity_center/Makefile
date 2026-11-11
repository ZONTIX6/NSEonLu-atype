CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ALGO = ../../modules/algorithm
SOURCES = test_chassis_velocity_center.c $(ALGO)/chassis_velocity_center.c

test_chassis_velocity_center.exe: $(SOURCES) $(ALGO)/chassis_velocity_center.h
	$(CC) $(CFLAGS) -I$(ALGO) $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_velocity_center.exe
	./test_chassis_velocity_center.exe
