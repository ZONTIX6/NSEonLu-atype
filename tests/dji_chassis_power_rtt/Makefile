CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g \
          -ffunction-sections -fdata-sections
LDFLAGS := -Wl,--gc-sections
TARGET := test_dji_chassis_power_rtt.exe
SOURCE := test_dji_chassis_power_rtt.c \
          ../../modules/algorithm/power_model.c \
          ../../modules/telemetry/rtt_telemetry.c
INCLUDES := -Istubs \
            -I../../modules/motor/DJImotor \
            -I../../modules/motor \
            -I../../modules/algorithm \
            -I../../modules/telemetry

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) $(LDFLAGS) -lm -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	-if exist $(TARGET) del /Q $(TARGET)
