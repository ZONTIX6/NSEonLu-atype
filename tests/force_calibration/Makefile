CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
INCLUDES = -I../../modules/chassis_force_calibration -I../../application/board
CORE = ../../modules/chassis_force_calibration/force_calibration.c ../../application/board/board_force_calibration.c
.PHONY: test test_engine test_capture
test_engine.exe: test_engine.c test_support.h $(CORE)
	$(CC) $(CFLAGS) $(INCLUDES) test_engine.c $(CORE) -lm -o $@
test_engine: test_engine.exe
	./test_engine.exe
test: test_engine
test_capture.exe: test_capture.c ../../modules/chassis_force_calibration/force_calibration_capture.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -lm -o $@
test_capture: test_capture.exe
	./test_capture.exe
test: test_capture
POWER_INCLUDES = -I../dji_chassis_power_rtt/stubs -I../../modules/chassis_power -I../../modules/telemetry -I../../modules/super_cap -I../../modules/power_meter -I../../modules/referee
DRIVER_INCLUDES = -I../../modules/algorithm -I../../modules/chassis_force -I../../modules/motor/DJImotor -I../../modules/motor -I../../modules/vehicle_safety -I../../application -I../../Inc
DRIVER_INCLUDES += -I../vehicle_motion/stubs -I../chassis_motion/stubs -I../../modules/imu -I../../modules/master_machine
override CFLAGS += -DATYPE_BENCH_MODE=1 -DUSB_TELEMETRY_ENABLE=1 -DPOWER_METER_ENABLE=1 -DCHASSIS_FORCE_SPIN_MODE=0
POWER_SOURCES = $(filter-out ../../modules/chassis_power/chassis_power.c,$(wildcard ../../modules/chassis_power/*.c)) ../../modules/algorithm/power_model.c ../../modules/algorithm/chassis_power_transient.c ../../modules/algorithm/chassis_power_budget.c ../../modules/algorithm/chassis_power_outer_loop.c
ADAPTER_SOURCES = ../../modules/telemetry/rtt_telemetry.c ../../modules/telemetry/runtime_tuning.c ../../modules/telemetry/pid_autotune.c ../../modules/telemetry/autotune_protocol.c
ADAPTER_SOURCES += ../../modules/telemetry/force_calibration_protocol.c
test_adapter.exe: test_adapter.c test_power_permission.c $(CORE) ../../modules/chassis_force_calibration/force_calibration_capture.c ../../modules/motor/DJImotor/dji_force_calibration.c ../../application/runtime/force_calibration_app.c $(POWER_SOURCES) $(wildcard ../../modules/chassis_force/*.c) ../../config/22_chassis_force/chassis_force_config.c $(ADAPTER_SOURCES)
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -Istubs -include test_irq.h -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -ffunction-sections -fdata-sections $(INCLUDES) $(POWER_INCLUDES) $(DRIVER_INCLUDES) $(filter-out test_power_permission.c,$^) -Wl,--gc-sections -lm -o $@
test_adapter: test_adapter.exe
	./test_adapter.exe
test: test_adapter
test_protocol.exe: test_protocol.c $(CORE) ../../modules/chassis_force_calibration/force_calibration_capture.c ../../modules/motor/DJImotor/dji_force_calibration.c ../../application/runtime/force_calibration_app.c ../../modules/telemetry/force_calibration_protocol.c $(POWER_SOURCES) $(wildcard ../../modules/chassis_force/*.c) ../../config/22_chassis_force/chassis_force_config.c $(ADAPTER_SOURCES)
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -Istubs -include test_irq.h -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -ffunction-sections -fdata-sections $(INCLUDES) $(POWER_INCLUDES) $(DRIVER_INCLUDES) $^ -Wl,--gc-sections -lm -o $@
test_protocol: test_protocol.exe
	./test_protocol.exe
test: test_protocol
test_ownership.exe: test_ownership.c ../../modules/telemetry/runtime_tuning.c
	$(CC) $(CFLAGS) -Iownership_stubs -I../usb_telemetry/stubs -I../../modules/telemetry $^ -lm -o $@
test_ownership: test_ownership.exe
	./test_ownership.exe
test: test_ownership
test_can_failure.exe: test_can_failure.c $(CORE) ../../modules/chassis_force_calibration/force_calibration_capture.c ../../modules/motor/DJImotor/dji_force_calibration.c ../../application/runtime/force_calibration_app.c $(POWER_SOURCES) $(wildcard ../../modules/chassis_force/*.c) ../../config/22_chassis_force/chassis_force_config.c $(ADAPTER_SOURCES)
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -Istubs -include test_irq.h -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -ffunction-sections -fdata-sections $(INCLUDES) $(POWER_INCLUDES) $(DRIVER_INCLUDES) $^ -Wl,--gc-sections -lm -o $@
test_can_failure: test_can_failure.exe
	./test_can_failure.exe
test: test_can_failure
test_stop_race.exe: test_stop_race.c ../../modules/telemetry/force_calibration_protocol.c $(CORE) ../../modules/chassis_force_calibration/force_calibration_capture.c ../../modules/motor/DJImotor/dji_force_calibration.c ../../application/runtime/force_calibration_app.c $(POWER_SOURCES) $(wildcard ../../modules/chassis_force/*.c) ../../config/22_chassis_force/chassis_force_config.c $(ADAPTER_SOURCES)
	$(CC) $(filter-out -pedantic,$(CFLAGS)) -Istubs -include test_irq.h -DCHASSIS_CONTROL_MODE=1 -DCHASSIS_FORCE_CALIBRATION_ENABLE=1 -ffunction-sections -fdata-sections $(INCLUDES) $(POWER_INCLUDES) $(DRIVER_INCLUDES) $(filter-out ../../modules/telemetry/force_calibration_protocol.c,$^) -Wl,--gc-sections -lm -o $@
test_stop_race: test_stop_race.exe
	./test_stop_race.exe
test: test_stop_race
