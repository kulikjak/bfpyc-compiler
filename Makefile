
all: compiler

compiler: compiler.c compiler.h
	gcc -I/usr/include/python3.9/ -pedantic -Wall -Wextra -O2 compiler.c -o $@

clean:
	rm -f compiler
