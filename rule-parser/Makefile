.PHONY: clean klee

CLANG=clang-3.4
KLEE_INCLUDE_DIR=$(HOME)/src/klee/include

rule: rule.c
	gcc rule.c -o rule

rule.bc: rule.c
	$(CLANG) -I$(KLEE_INCLUDE_DIR) -emit-llvm -c -DRUN_KLEE rule.c -o rule.bc

clean:
	-rm rule rule.bc
