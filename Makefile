
all: build;

build: setuid_wrapper.c
	gcc -Wall -O2 -o setuid_wrapper setuid_wrapper.c

clean:
	rm -f setuid_wrapper