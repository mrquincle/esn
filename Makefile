#!/bin/make -f

all:
	mkdir -p build
	cd build && cmake ..
	cd build && make all

clean:
	cd build && make clean


.PHONY: all clean
