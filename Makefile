CC=g++
CFLAGS=-I.

default: src/test.cpp
	$(CC) -std=c++11 -o build/raytracer src/test.cpp -lOpenCL


