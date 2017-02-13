.PHONY: all clean

BITCODE=on

ifeq ($(BITCODE),on)
  CC=clang-3.4
  CFLAGS=-g -emit-llvm -c -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
  LIBS=
else
  CFLAGS=-I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include
  LIBS=-ldbus-1
  CC=gcc
endif


all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LIBS)

client: client.c
	$(CC) $(CFLAGS) client.c -o client $(LIBS)

clean:
	-rm server client
