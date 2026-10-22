CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Istubs -I../dji_chassis_power_rtt/stubs -I../../modules/vtx -I../../modules/remote -I../../modules/referee -I../../bsp/usart
.PHONY: test
test: test_vtx.exe
	./test_vtx.exe
test_vtx.exe: test_vtx.c ../../modules/vtx/vt02_control.c ../../modules/referee/crc_ref.c
	$(CC) $(CFLAGS) $^ -o $@
