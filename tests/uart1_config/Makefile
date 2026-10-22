CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -I../../Inc

.PHONY: test clean

test: referee.exe uart.exe uart_rtt.exe
	./referee.exe
	./uart.exe
	./uart_rtt.exe

referee.exe: test_uart1_config.c
	$(CC) $(CFLAGS) -DLOG_OUTPUT=1 -DEXPECT_REFEREE=1 -DEXPECT_UART_LOG=0 -DEXPECT_RTT=1 $< -o $@

uart.exe: test_uart1_config.c
	$(CC) $(CFLAGS) -DLOG_OUTPUT=2 -DEXPECT_REFEREE=1 -DEXPECT_UART_LOG=1 -DEXPECT_RTT=0 $< -o $@

uart_rtt.exe: test_uart1_config.c
	$(CC) $(CFLAGS) -DLOG_OUTPUT=3 -DEXPECT_REFEREE=1 -DEXPECT_UART_LOG=1 -DEXPECT_RTT=1 $< -o $@

clean:
	-if exist *.exe del /Q *.exe
