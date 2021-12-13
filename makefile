all: csim.c
	gcc -std=c99 -Wall -o csim csim.c
	
clean:
	rm -rf *.o csim