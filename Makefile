SHELL=/bin/bash
CC=arm-linux-gnueabihf-gcc
CFLAGS=-W -Wall
CFLAGS+=
LDLIBS=-l:libws2811.a
LDFLAGS=-L./lib/rpi_ws281x/

led-daemon: led-daemon.o lib/rpi_ws281x/libws2811.a

FORCE: ;

lib/rpi_ws281x/libws2811.a: FORCE
	pushd lib/rpi_ws281x && scons TOOLCHAIN=arm-linux-gnueabihf && popd
clean:
	rm -f led-daemon led-daemon.o
	pushd lib/rpi_ws281x && scons -c && popd
