
CFLAGS=-static -Wall

INCLUDES=-Igecko_bglib/include -I$(TOOLCHAIN_SYSROOT)/usr/include/azureiot

LIBDIR=$(TOOLCHAIN_SYSROOT)/usr/lib/

LIBS=$(LIBDIR)/libiothub_client.a \
$(LIBDIR)/libiothub_service_client.a \
$(LIBDIR)/libiothub_client_http_transport.a \
$(LIBDIR)/libserializer.a \
$(LIBDIR)/libaziotsharedutil.a \
$(LIBDIR)/libcurl.a \
$(LIBDIR)/libssl.a \
$(LIBDIR)/libcrypto.a \
$(LIBDIR)/libz.a \
$(LIBDIR)/libparson.a \

SRCDIR=gecko_bglib/src

SRC=$(SRCDIR)/main.c\
$(SRCDIR)/app.c\
$(SRCDIR)/uart_posix.c\
$(SRCDIR)/gecko_bglib.c\
$(SRCDIR)/azure_functions.c\
$(SRCDIR)/log.c\
$(SRCDIR)/led_worker.c

OBJ=$(SRC:.c=.o)

MAIN=g300demo

RM=rm -rf

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

$(MAIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJ) $(LFLAGS) $(LIBS)

all: $(MAIN)

clean: 

	$(RM) $(MAIN) gecko_bglib/src/*.o *~
