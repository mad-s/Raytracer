CC=g++
CFLAGS=-I.

default: src/test.cpp
	$(CC) -std=c++11 -g -o build/raytracer src/main.cpp -lOpenCL


