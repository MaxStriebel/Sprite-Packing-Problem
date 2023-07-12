.PHONY: run

SOURCE=source/main.c source/pcg_basic.c 
REFERENCES=$(SOURCE) source/problem.h source/spritePacking.c source/genetic.c

build/main: $(REFERENCES)
	cc -g -Wall -Wno-unused $(SOURCE) -o build/main

run:
	build/main

test:
	build/main > ../Report/genetic_box.csv
