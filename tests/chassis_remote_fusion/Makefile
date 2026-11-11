CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
ALGORITHM = ../../modules/algorithm
SOURCES = test_chassis_remote_fusion.c $(ALGORITHM)/chassis_remote_fusion.c \
    $(ALGORITHM)/chassis_velocity_fusion.c $(ALGORITHM)/chassis_velocity_center.c \
    $(ALGORITHM)/chassis_gimbal_imu.c
HEADERS = $(ALGORITHM)/chassis_remote_fusion.h $(ALGORITHM)/chassis_velocity.h \
    $(ALGORITHM)/chassis_velocity_fusion.h $(ALGORITHM)/chassis_velocity_center.h \
    $(ALGORITHM)/chassis_gimbal_imu.h ../../modules/imu/chassis_imu_can_protocol.h

test_chassis_remote_fusion.exe: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -I$(ALGORITHM) $(SOURCES) -lm -o $@

.PHONY: test
test: test_chassis_remote_fusion.exe
	./test_chassis_remote_fusion.exe
