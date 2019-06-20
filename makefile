
CFLAGS = -Iinclude
SOURCES = src/main.c src/cmd_def.c src/ble_handlers.c src/stubs.c
CC = /opt/buildroot-gcc463/usr/bin/mipsel-linux-gcc

all:g300

g300:	
	$(CC) $(CFLAGS) $(SOURCES) -o g300demo

linux:
	gcc $(CFLAGS) $(SOURCES) -o g300demo

serial:
	$(CC) src/serial_test.c -o serial_test

gecko:
	$(CC) gecko_bglib/src/main.c gecko_bglib/src/app.c gecko_bglib/src/uart_posix.c gecko_bglib/src/gecko_bglib.c -Igecko_bglib/include -o g300demo