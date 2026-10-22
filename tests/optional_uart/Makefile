CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I../usb_telemetry/stubs -I../dji_chassis_power_rtt/stubs -I../../bsp/usart -I../../modules/TFminiPlus -I../../modules/bluetooth
.PHONY: test
test: test_optional_uart.exe
	./test_optional_uart.exe
test_optional_uart.exe: test_optional_uart.c ../../modules/TFminiPlus/TFminiPlus.c ../../modules/bluetooth/HC05.c
	$(CC) $(CFLAGS) $^ -o $@
