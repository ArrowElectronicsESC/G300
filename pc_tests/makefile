
#CC=/home/andrew/XCompile/openwrt-sdk-18.06.1-ramips-mt76x8_gcc-7.3.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-7.3.0_musl/bin/mipsel-openwrt-linux-musl-gcc

CFLAGS= -Wall

#INCLUDES=-Igecko_bglib/include

#INCLUDES=-I/home/andrew/Downloads/azure-iot-sdk-c/iothub_client/inc/ -I/home/andrew/Downloads/azure-iot-sdk-c/c-utility/inc/

INCLUDES=-I/usr/local/include/azureiot/ -I.

#LIBDIR=/home/andrew/XCompile/openwrt-sdk-18.06.0-ramips-mt76x8_gcc-7.3.0_musl.Linux-x86_64/staging_dir/toolchain-mipsel_24kc_gcc-7.3.0_musl/usr/lib/

LIBDIR=/usr/local/

LFLAGS=-m64

# LFLAGS=$(LIBDIR)/libiothub_client.a \
# $(LIBDIR)/libiothub_service_client.a \
# $(LIBDIR)/libiothub_client_http_transport.a \
# $(LIBDIR)/libserializer.a \
# $(LIBDIR)/libaziotsharedutil.a \
# $(LIBDIR)/libcurl.a \
# $(LIBDIR)/libssl.a \
# $(LIBDIR)/libcrypto.a \
# $(LIBDIR)/libz.a \
# $(LIBDIR)/libparson.a \


#$(LIBDIR)/libssl.a $(LIBDIR)/libcurl.a $(LIBDIR)/libiothub_client_http_transport.a $(LIBDIR)/libuhttp.a $(LIBDIR)/libaziotsharedutil.a $(LIBDIR)/libuuid.a   $(LIBDIR)/libiothub_client.a
# $(LIBDIR)/libaziotsharedutil.a 
#-uhttp -liothub_client -liothub_client_http_transport
#LIBS=libcurl.a libssl.a libcrypto.a

LIBS=-lcurl -ljson-c /usr/local/lib64/libaziotsharedutil.a /usr/local/lib64/libparson.a -lm

#LIBS=$(LIBDIR)/libcurl.a

#SRC=gecko_bglib/src/main.c gecko_bglib/src/app.c gecko_bglib/src/uart_posix.c gecko_bglib/src/gecko_bglib.c

SRC=azure_curl_test.c azure_functions.c

OBJ=$(SRC:.c=.o)

#MAIN=g300demo
MAIN=azure

RM=rm -rf

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

$(MAIN): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJ) $(LFLAGS) $(LIBS)

all: $(MAIN)

clean: 

	$(RM) $(MAIN) *.o *~
