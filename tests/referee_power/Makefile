CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
TARGET := test_referee_power.exe
SOURCE := test_referee_power.c ../../modules/referee/referee_power.c
INCLUDES := -I../../modules/referee

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
