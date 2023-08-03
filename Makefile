.PHONY: run

SOURCE=source/main.c source/pcg_basic.c source/genetic.c source/random.c
REFERENCES=$(SOURCE) source/problem.h source/spritePacking.c 

#SOURCE=source/main.o source/pcg_basic.o source/genetic.o source/random.o

build/main: $(REFERENCES)
	cc -g -Wall -Wno-unused $(SOURCE) -o build/main -lm

run:
	build/main

test:
	build/main
