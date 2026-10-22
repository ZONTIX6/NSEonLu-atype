CC ?= gcc
.PHONY: test
test: test_bsp_pwm.exe
	./test_bsp_pwm.exe
test_bsp_pwm.exe: test_bsp_pwm.c ../../bsp/pwm/bsp_pwm.c $(wildcard stubs/*.h)
	$(CC) -std=c11 -Wall -Wextra -Werror -Istubs -I../../bsp/pwm test_bsp_pwm.c ../../bsp/pwm/bsp_pwm.c -lm -o $@
