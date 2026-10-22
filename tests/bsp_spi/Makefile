CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O0 -g
SOURCE := test_bsp_spi.c ../../bsp/spi/bsp_spi.c
.PHONY: test
test: test_bsp_spi.exe
	./test_bsp_spi.exe
test_bsp_spi.exe: $(SOURCE) $(wildcard stubs/*.h) ../../bsp/spi/bsp_spi.h
	$(CC) $(CFLAGS) -Istubs -I../../bsp/spi $(SOURCE) -o $@
