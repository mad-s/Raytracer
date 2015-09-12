CC=g++
CFLAGS=-I.

default: src/main.cpp
	cd src && xxd -i raycast.cl > raycast.cl.h
	$(CC) -std=c++11 -g -o build/raytracer src/main.cpp -lOpenCL


