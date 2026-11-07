CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_vofa_telemetry.c ../../application/runtime/vofa_telemetry.c
HEADERS := stubs/usart.h ../../application/runtime/vofa_telemetry.h ../../Inc/board_config.h ../../modules/telemetry/usb_telemetry_format.h
INCLUDES := -Istubs -I../../Inc -I../../application/runtime -I../../modules/telemetry -I../../modules/algorithm

.PHONY: test
test: test_vofa_telemetry.exe
	./test_vofa_telemetry.exe

test_vofa_telemetry.exe: $(SOURCE) $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -DVOFA_TELEMETRY_ENABLE=1 $(SOURCE) -o $@
