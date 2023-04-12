.PHONY: run

SOURCE=source/main.c source/pcg_basic.c

build/main: $(SOURCE)
	cc $(SOURCE) -o build/main

run:
	build/main
