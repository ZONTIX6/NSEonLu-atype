CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g
TARGET := test_chassis_power_feedback.exe
SOURCE := test_chassis_power_feedback.c \
          ../../application/chassis/chassis_power_feedback.c
INCLUDES := -Istubs -I../../application -I../../application/chassis \
            -I../../modules/algorithm

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
