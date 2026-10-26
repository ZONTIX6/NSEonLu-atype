CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -Wno-type-limits -O0 -g
SOURCE := ../../modules/oled/oled_transport.c test_oled.c ../../modules/oled/oled.c ../../modules/oled/oled_spi.c
INCLUDES := -Istubs -I../../modules/oled
.PHONY: test baseline
test: test_spi.exe test_i2c.exe test_analog_key.exe
	./test_spi.exe
	./test_i2c.exe
	./test_analog_key.exe
baseline: test_spi.exe test_i2c.exe
	./test_spi.exe --record
	./test_i2c.exe --record
test_spi.exe: $(SOURCE) $(wildcard stubs/*.h)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCE) -o $@
test_i2c.exe: $(SOURCE) $(wildcard stubs/*.h)
	$(CC) $(CFLAGS) -DOLED_USE_I2C=1 $(INCLUDES) $(SOURCE) -o $@
test_analog_key.exe: test_analog_key.c ../../application/board/board_analog_key.c
	$(CC) $(CFLAGS) $(INCLUDES) -I../../application $^ -o $@
