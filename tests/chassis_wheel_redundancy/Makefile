CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
TARGET := test_chassis_wheel_redundancy.exe
SOURCE := test_chassis_wheel_redundancy.c ../../modules/algorithm/chassis_wheel_redundancy.c
INCLUDES := -I../../modules/algorithm

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
