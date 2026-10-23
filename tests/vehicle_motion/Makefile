CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O0 -g
INCLUDES = -Istubs -I../chassis_motion/stubs -I../dji_chassis_power_rtt/stubs \
 -I../../application -I../../modules/algorithm -I../../modules/imu \
 -I../../modules/master_machine -I../../modules/referee -I../../modules/telemetry \
 -I../../modules/motor/DJImotor -I../../modules/motor -I../../modules/daemon \
 -I../../modules/remote -I../../modules/vtx -I../../modules/message_center \
 -I../../modules/heat_control -I../../modules/vehicle_safety -I../../modules/jscope
SOURCES = test_vehicle_motion.c ../../modules/algorithm/user_lib.c \
 ../../modules/heat_control/heat_control.c ../../modules/vehicle_safety/vehicle_safety.c
.PHONY: test
test_vehicle_motion.exe: $(SOURCES) $(wildcard ../../application/cmd/*.c ../../application/gimbal/*.c ../../application/shoot/*.c)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) -lm -o $@
test: test_vehicle_motion.exe
	./test_vehicle_motion.exe speed
	./test_vehicle_motion.exe manual
	./test_vehicle_motion.exe yaw
	./test_vehicle_motion.exe heat
	./test_vehicle_motion.exe friction
	./test_vehicle_motion.exe state
	./test_vehicle_motion.exe stale
	./test_vehicle_motion.exe spinup
	./test_vehicle_motion.exe modes
	./test_vehicle_motion.exe motor_stale
