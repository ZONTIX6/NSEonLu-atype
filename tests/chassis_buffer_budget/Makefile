CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
TARGET := test_chassis_buffer_budget.exe
SOURCE := test_chassis_buffer_budget.c ../../modules/algorithm/chassis_power_budget.c
INCLUDES := -I../../modules/algorithm
.PHONY: all test
all: $(TARGET)
$(TARGET): $(SOURCE) ../../modules/algorithm/chassis_power_budget.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -lm -o $(TARGET)
test_spin_priority.exe: test_spin_priority.c ../../modules/algorithm/chassis_power_budget.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -lm -o $@

test: $(TARGET) test_spin_priority.exe
	./$(TARGET)
	./test_spin_priority.exe
