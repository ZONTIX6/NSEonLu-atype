CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -Istubs -I../../modules/heat_control -I../../modules/message_center
.PHONY: test
test: heat_control_test.exe test_delayed_heat.exe
	./heat_control_test.exe
	./test_delayed_heat.exe
test_delayed_heat.exe: test_delayed_heat.c ../../modules/heat_control/heat_control.c ../../modules/heat_control/heat_control.h ../../config/04_shoot/heat_control_config.h
	$(CC) $(CFLAGS) -O2 test_delayed_heat.c ../../modules/heat_control/heat_control.c -lm -o $@
heat_control_test.exe: heat_control_test.c ../../modules/heat_control/heat_control.c ../../modules/heat_control/heat_control.h ../../config/04_shoot/heat_control_config.h
	$(CC) $(CFLAGS) heat_control_test.c ../../modules/heat_control/heat_control.c -lm -o $@
