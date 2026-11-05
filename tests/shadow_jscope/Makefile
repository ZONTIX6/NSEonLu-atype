CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES := -Istubs -I../../application/runtime -I../../modules/jscope -I../../modules/chassis_power -I../../modules/algorithm -I../../modules/power_meter -I../../modules/super_cap -I../../modules/referee
INCLUDES += -I../../modules/telemetry
FLAGS := -DCHASSIS_SHADOW_JSCOPE_ENABLE=1 -DCHASSIS_PREDICTIVE_POWER_MODE=1
.PHONY: test
test: test_shadow_jscope.exe test_wheel_rb.exe test_disabled.exe
	./test_shadow_jscope.exe
	./test_wheel_rb.exe
	./test_disabled.exe
test_wheel_rb.exe: test_shadow_jscope.c ../../application/runtime/shadow_jscope.c
	$(CC) $(CFLAGS) $(INCLUDES) $(FLAGS) -DCHASSIS_SHADOW_JSCOPE_WHEEL=3 $^ -lm -o $@
test_disabled.exe: test_disabled.c ../../application/runtime/shadow_jscope.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@
test_shadow_jscope.exe: test_shadow_jscope.c ../../application/runtime/shadow_jscope.c
	$(CC) $(CFLAGS) $(INCLUDES) $(FLAGS) $^ -lm -o $@
