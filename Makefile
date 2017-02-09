all: proj4

proj4: proj4.c proj4.h
	gcc -g -o proj4 proj4.c -lpthread

clean:
	rm proj4
