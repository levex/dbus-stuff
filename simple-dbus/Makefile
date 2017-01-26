.PHONY: all clean

CFLAGS=-I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
LIBS=-ldbus-1

all: server client

server: server.c
	gcc $(CFLAGS) server.c -o server $(LIBS)

client: client.c
	gcc $(CFLAGS) client.c -o client $(LIBS)

clean:
	-rm server client
