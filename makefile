all: loader
	
# instead of automatic gcc linking we link manually in task2
# Assemble start.s into a 32-bit object file 
# Link everything together, map task2 to low address to avoid collision with loaded program
loader: loader.c
	gcc -m32 -c loader.c -o loader.o	
	nasm -f elf32 start.s -o start.o
	nasm -f elf32 startup.s -o startup.o
	ld -o loader loader.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

PHONY: clean
clean:
	rm -f loader loader.o start.o startup.o