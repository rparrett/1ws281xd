CC=arm-linux-gnueabihf-gcc
CFLAGS=-W -Wall
CFLAGS+=
LDLIBS=-l:libws2811.a
LDFLAGS=-L./lib/

led-daemon: led-daemon.o

clean:
	rm -f led-daemon led-daemon.o
run:
	./led-daemon
