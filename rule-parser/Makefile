.PHONY: clean klee

CLANG=clang-3.4
KLEE_INCLUDE_DIR=$(HOME)/src/klee/include
KLEE_INCLUDE_DIR=$(HOME)/src/klee/Release+Asserts/lib/

rule: rule.c
	gcc -g rule.c -o rule

rule.bc: rule.c
	$(CLANG) -I$(KLEE_INCLUDE_DIR) -emit-llvm -g -c -DRUN_KLEE rule.c -o rule.bc

rule.klee: rule.c
	$(CLANG) -I$(KLEE_INCLUDE_DIR) -L$(KLEE_LIB_DIR) -g -DRUN_KLEE rule.c -o rule.klee -lkleeRuntest

clean:
	-rm rule rule.bc rule.klee
