CC := gcc
CFLAGS := -g -O0 -Wall -L./build -Wl,-rpath=./
LD_LIBRARY_PATH := build:$(LD_LIBRARY_PATH)
SHELL := env LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) /bin/bash

all: dirs libopal marc alex astro genie opal doc_res tar

# Create required directory structure
dirs:
	mkdir -pv build/{doc,log,report,res,tmp,examples}

# Build OPaL library
libopal: src/libopal.c include/libopal.h
	$(CC) -g -O0 -fPIC -c -Wall src/libopal.c -o build/libopal.o
	ld -shared build/libopal.o -o build/libopal.so
	rm build/libopal.o

# Build MARC preprocessor
marc: libopal src/marc.c
	$(CC) $(CFLAGS) src/marc.c -g -lopal -o build/marc

# Build ALEX lexical analyzer
alex: libopal src/alex.c
	$(CC) $(CFLAGS) src/alex.c -g -lopal -o build/alex

# Build ASTRO syntax analyzer
astro: libopal src/astro.c
	$(CC) $(CFLAGS) src/astro.c -g -lopal -o build/astro

# Build GENIE code generator
genie: libopal src/genie.c
	$(CC) $(CFLAGS) src/genie.c -g -lopal -o build/genie

# Copy man page, resource files and examples to build directory
doc_res:
	cp -v ref/* build/doc/
	cp -v res/* build/res/
	cp -v input/bool.hpl build/examples/bool.hpl
	cp -v input/calc.opl build/examples/calculator.opl
	cp -v input/PaperScissorsRock.opl build/examples/PaperScissorsRock.opl
	cp -v input/Sequences.opl build/examples/Sequences.opl

# Build orchestrator
opal: libopal src/opal.c
	$(CC) $(CFLAGS) src/opal.c -g -lopal -o build/opal

# Tar all files for release
tar: libopal opal doc_res
	tar -cvf build/opal.tar build/

.PHONY: test
test: clean all
	# MARC tests
	@printf "\n=== Test 1 ===\n"
	build/marc --debug --output=output/test1.opl input/test1.opl
	diff -s output/test1.opl test/test1.opl
	
	@printf "\n=== Test 2 ===\n"
	build/marc --debug --output=output/test2.opl input/test2.opl
	diff -s output/test2.opl test/test2.opl
	
	@printf "\n=== Test 3 ===\n"
	build/marc --debug --output=output/test3.opl input/test3.opl
	diff -s output/test3.opl test/test3.opl
	
	@printf "\n=== Test 4 ===\n"
	build/marc --debug --output=output/test4.opl input/test4.opl
	diff -s output/test4.opl test/test4.opl
		
	@printf "\n=== Test 5 ===\n"
	build/marc --debug --output=output/test5.opl input/test5.opl
	diff -s output/test5.opl test/test5.opl
	
	@printf "\n=== Test 6 ===\n"
	build/marc --debug --output=output/test6.opl input/test6.opl
	diff -s output/test6.opl test/test6.opl
	
	# ALEX tests
	@printf "\n=== Test 16 ===\n"
	build/alex --debug --output=output/test16.opl input/test16.opl
	diff -s output/test16.opl test/test16.opl
	
	@printf "\n=== Test 17 ===\n"
	build/alex --debug --output=output/test17.opl input/test17.opl
	diff -s output/test17.opl test/test17.opl
	
	#ASTRO tests
	@printf "\n=== Test 20 ===\n"
	build/astro --debug --output=output/test20.ast input/test20.opl
	diff -s output/test20.ast test/test20.ast

	@printf "\n=== Test 21 ===\n"
	build/astro --debug --output=output/test21.ast input/test21.opl
	diff -s output/test21.ast test/test21.ast
	
	@printf "\n=== Test 23 ===\n"
	build/astro --debug --output=output/test23.ast input/test23.opl
	diff -s output/test23.ast test/test23.ast
	
	@printf "\n=== Test 24 ===\n"
	build/astro --debug --output=output/test24.ast input/test24.opl
	diff -s output/test24.ast test/test24.ast
	
	#GENIE tests
	@printf "\n=== Test 25 ===\n"
	build/genie --debug --output=output/test25.asm input/test25.opl
	diff -s output/test25.asm test/test25.asm

	@printf "\n=== Test 26 ===\n"
	build/genie --debug --output=output/test26.asm input/test26.opl
	diff -s output/test26.asm test/test26.asm
	
	@printf "\n=== Test 27 ===\n"
	build/genie --debug --output=output/test27.asm input/test27.opl
	diff -s output/test27.asm test/test27.asm
	
	@printf "\n=== Test 28 ===\n"
	build/genie --debug --output=output/test28.asm input/test28.opl
	diff -s output/test28.asm test/test28.asm
	
	@printf "\n=== Test 29 ===\n"
	build/genie --debug --output=output/test29.asm input/test29.opl
	diff -s output/test29.asm test/test29.asm
	
	#OPAL tests
	@printf "\n=== Test 30 ===\n"
	build/opal --debug --output=output/calc.bin input/calc.opl
	@expect test/test30.exp
	
	@printf "\n=== Test 31 ===\n"
	build/opal --debug --output=output/PaperScissorsRock.bin input/PaperScissorsRock.opl
	@expect test/test31.exp

	@printf "\n=== Test 32 ===\n"
	build/opal --debug --output=output/Sequences.bin input/Sequences.opl
	@expect test/test32.exp

	@printf "\n=== Test 33 ===\n"
	build/opal --debug --output=output/optest.bin input/operands_test.opl
	@expect test/test33.exp

all_tests: test
	# Negative tests
	@printf "\n=== Test 7 ===\n"
	@bash test/test7.sh
	
	@printf "\n=== Test 8 ===\n"
	@bash test/test8.sh
	
	@printf "\n=== Test 9 ===\n"
	@bash test/test9.sh
	
	@printf "\n=== Test 10 ===\n"
	@bash test/test10.sh
	
	@printf "\n=== Test 11 ===\n"
	@bash test/test11.sh
	
	@printf "\n=== Test 12 ===\n"
	@bash test/test12.sh
	
	@printf "\n=== Test 13 ===\n"
	@bash test/test13.sh
	
	@printf "\n=== Test 14 ===\n"
	@bash test/test14.sh
	
	@printf "\n=== Test 15 ===\n"
	@bash test/test15.sh	
	
	@printf "\n=== Test 18 ===\n"
	@bash test/test18.sh
	
	@printf "\n=== Test 19 ===\n"
	@bash test/test19.sh
		
	@printf "\n=== Test 22 ===\n"
	@bash test/test22.sh
	
	@printf "\n=== Bug 98 ===\n"
	@bash test/testbug98.sh
	
	$(MAKE) clean
	
.PHONY: clean
clean:
	# Delete binaries, output, temporary & report files 
	rm -rfv build/* output/* tmp/* report/*

	
