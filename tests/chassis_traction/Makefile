CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g -fno-fast-math
INCLUDES := -I../../modules/algorithm
SOURCE := test_chassis_traction.c ../../modules/algorithm/chassis_traction.c

.PHONY: test
test_chassis_traction.exe: $(SOURCE) ../../modules/algorithm/chassis_traction.h ../../config/30_velocity_observation/chassis_traction_config.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $@
test: test_chassis_traction.exe test_traction_telemetry.exe
	./test_chassis_traction.exe
	./test_traction_telemetry.exe

TELEMETRY_INCLUDES := -I../vofa_telemetry/stubs -I../../Inc -I../../application/runtime -I../../application/chassis -I../../modules/telemetry -I../../modules/chassis_power -I../../modules/power_meter -I../../modules/referee -I../../modules/super_cap
test_traction_telemetry.exe: test_traction_telemetry.c ../../application/runtime/vofa_traction.c ../../modules/motor/DJImotor/dji_traction.h
	$(CC) $(CFLAGS) $(INCLUDES) $(TELEMETRY_INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 -DVOFA_TELEMETRY_MODE=2 -DVOFA_OBSERVATION_PROFILE=3 -DCHASSIS_TRACTION_MODE=1 $(filter %.c,$^) -lm -o $@
