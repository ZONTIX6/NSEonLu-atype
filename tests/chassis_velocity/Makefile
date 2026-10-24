CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCES = test_chassis_velocity.c ../../modules/algorithm/chassis_velocity.c
test_chassis_velocity.exe: $(SOURCES) ../../modules/algorithm/chassis_velocity.h
	$(CC) $(CFLAGS) -I../../modules/algorithm $(SOURCES) -lm -o $@
.PHONY: test
test: test_chassis_velocity.exe
	./test_chassis_velocity.exe
