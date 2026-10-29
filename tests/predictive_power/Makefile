CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
CORE = $(wildcard ../../modules/predictive_power/*.c)
SHADOW = ../../modules/chassis_power/chassis_power_shadow.c ../../modules/chassis_power/chassis_power_shadow_energy.c
HEADERS = $(wildcard ../../modules/predictive_power/*.h)
INCLUDES = -I../../modules/predictive_power
MODULE_INCLUDES = -I../../modules/chassis_power -I../../modules/algorithm -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
.PHONY: test
test_core.exe: test_core.c $(CORE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) test_core.c $(CORE) -lm -o $@
test_shadow.exe: test_shadow.c $(CORE) $(HEADERS) $(SHADOW) ../../modules/algorithm/power_model.c
	$(CC) $(CFLAGS) $(INCLUDES) $(MODULE_INCLUDES) -DCHASSIS_PREDICTIVE_POWER_MODE=1 test_shadow.c $(CORE) $(SHADOW) ../../modules/algorithm/power_model.c -lm -o $@
test: test_core.exe test_shadow.exe
	./test_core.exe
	./test_shadow.exe
