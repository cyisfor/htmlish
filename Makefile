all: make
	exec ./make
make: make.c
	gcc -o $@ $^
