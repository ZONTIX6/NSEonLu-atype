CC ?= gcc
.PHONY: test
test: test_jscope.exe test_shadow_owner.exe
	./test_jscope.exe
	./test_shadow_owner.exe
test_shadow_owner.exe: test_jscope.c ../../modules/jscope/jscope.c ../../modules/jscope/shadow_scope_config.h ../../config/32_capture/shadow_scope_config.h $(wildcard stubs/*.h)
	$(CC) -std=c11 -Wall -Wextra -Werror -DCHASSIS_SHADOW_JSCOPE_ENABLE=1 -Istubs -I../../modules/jscope test_jscope.c ../../modules/jscope/jscope.c -o $@
test_jscope.exe: test_jscope.c ../../modules/jscope/jscope.c $(wildcard stubs/*.h)
	$(CC) -std=c11 -Wall -Wextra -Werror -Istubs -I../../modules/jscope test_jscope.c ../../modules/jscope/jscope.c -o $@
