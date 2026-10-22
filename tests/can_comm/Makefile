CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Istubs -I../dji_chassis_power_rtt/stubs -I../../modules/can_comm -I../../modules/algorithm
.PHONY: test
test: test_can_comm.exe
	./test_can_comm.exe
test_can_comm.exe: test_can_comm.c ../../modules/can_comm/can_comm.c ../../modules/can_comm/can_comm.h
	$(CC) $(CFLAGS) test_can_comm.c ../../modules/can_comm/can_comm.c ../../modules/algorithm/crc8.c -o $@
