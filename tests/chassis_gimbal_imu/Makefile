CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2 -fno-fast-math
TARGET := test_chassis_gimbal_imu.exe
ALGORITHM := ../../modules/algorithm
SOURCE := test_chassis_gimbal_imu.c $(ALGORITHM)/chassis_gimbal_imu.c

.PHONY: test clean

$(TARGET): $(SOURCE) $(ALGORITHM)/chassis_gimbal_imu.h
	$(CC) $(CFLAGS) -I$(ALGORITHM) $(SOURCE) -lm -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
