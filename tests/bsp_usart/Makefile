CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_bsp_usart.c ../../bsp/usart/bsp_usart.c

.PHONY: test
test: test_bsp_usart.exe
	./test_bsp_usart.exe

test_bsp_usart.exe: $(SOURCE) $(wildcard stubs/*.h) ../../bsp/usart/bsp_usart.h
	$(CC) $(CFLAGS) -Istubs -I../../bsp/usart $(SOURCE) -o $@
