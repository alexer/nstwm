PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) -I$(PREFIX)/include nstwm.c util.c -L$(PREFIX)/lib -lX11 -o nstwm

clean:
	rm -f nstwm

