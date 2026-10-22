CC ?= gcc
.PHONY: test
test: test_jscope.exe
	./test_jscope.exe
test_jscope.exe: test_jscope.c ../../modules/jscope/jscope.c $(wildcard stubs/*.h)
	$(CC) -std=c11 -Wall -Wextra -Werror -Istubs -I../../modules/jscope test_jscope.c ../../modules/jscope/jscope.c -o $@
