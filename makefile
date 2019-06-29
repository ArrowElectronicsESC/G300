
CC=/home/andrew/XCompile/openwrt-sdk-18.06.1-ramips-mt76x8_gcc-7.3.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-7.3.0_musl/bin/mipsel-openwrt-linux-musl-gcc

CFLAGS=-static -Wall

INCLUDES=-Igecko_bglib/include

LFLAGS=

LIBS=libcurl.a libssl.a libcrypto.a

SRC=gecko_bglib/src/main.c gecko_bglib/src/app.c gecko_bglib/src/uart_posix.c gecko_bglib/src/gecko_bglib.c

OBJ=$(SRC:.c=.o)

MAIN=g300demo

RM=rm -rf

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

$(MAIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJ) $(LFLAGS) $(LIBS)

all: $(MAIN)

clean: 

	$(RM) $(MAIN) *.o *~
