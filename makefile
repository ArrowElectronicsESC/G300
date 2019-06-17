CC = /opt/buildroot-gcc463/usr/bin/mipsel-linux-gcc
CFLAGS = -Iinclude
SOURCES = src/main.c src/uart.c src/cmd_def.c src/ble_handlers.c src/stubs.c
all:
	$(CC) $(CFLAGS) $(SOURCES)

