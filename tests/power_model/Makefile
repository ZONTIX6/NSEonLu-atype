CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
TARGET := test_power_model.exe
SOURCE := test_power_model.c ../../modules/algorithm/power_model.c
HEADERS := ../../modules/algorithm/power_model.h ../../modules/algorithm/chassis_power_model_config.h ../../config/20_chassis_power/chassis_power_model_config.h
INCLUDES := -I../../modules/algorithm

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
