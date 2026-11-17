CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g
INCLUDES = -Istubs -I../chassis_motion/stubs -I../dji_chassis_power_rtt/stubs \
 -I../../application -I../../modules/algorithm -I../../modules/imu \
 -I../../modules/master_machine -I../../modules/referee -I../../modules/telemetry \
 -I../../modules/motor/DJImotor -I../../modules/motor -I../../modules/daemon \
 -I../../modules/remote -I../../modules/vtx -I../../modules/message_center \
 -I../../modules/heat_control -I../../modules/vehicle_safety -I../../modules/jscope -I../../config/01_pid -I../../application/chassis
SOURCES = test_vehicle_motion.c ../../modules/algorithm/user_lib.c \
 ../../modules/heat_control/heat_control.c ../../modules/vehicle_safety/vehicle_safety.c ../../config/01_pid/pid_config.c ../../application/gimbal/gimbal_yaw_feedforward.c
.PHONY: test
test_vehicle_force.exe: $(SOURCES) $(wildcard ../../config/*/*.h) ../../modules/chassis_force/chassis_force_intent.c $(wildcard ../../application/cmd/*.c)
	$(CC) $(CFLAGS) $(INCLUDES) -I../../modules/chassis_force -I../../modules/chassis_power -I../../modules/super_cap -I../../modules/power_meter -DCHASSIS_CONTROL_MODE=1 $(SOURCES) ../../modules/chassis_force/chassis_force_intent.c -lm -o $@
test_force: test_vehicle_force.exe
	./test_vehicle_force.exe force
test_vehicle_motion.exe: $(SOURCES) $(wildcard ../../config/*/*.h) $(wildcard ../../application/cmd/*.c ../../application/gimbal/*.c ../../application/shoot/*.c)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -lm -o $@
test: test_vehicle_motion.exe
	./test_vehicle_motion.exe speed
	./test_vehicle_motion.exe manual
	./test_vehicle_motion.exe gimbal_deadband
	./test_vehicle_motion.exe yaw
	./test_vehicle_motion.exe yaw_feedforward
	./test_vehicle_motion.exe heat
	./test_vehicle_motion.exe friction
	./test_vehicle_motion.exe state
	./test_vehicle_motion.exe stale
	./test_vehicle_motion.exe spinup
	./test_vehicle_motion.exe modes
	./test_vehicle_motion.exe motor_stale
