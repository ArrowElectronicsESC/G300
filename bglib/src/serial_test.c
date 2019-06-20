#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>


#define TRUE  1
#define FALSE 0
typedef uint8_t bool;

typedef struct {
	char *port;
	int baudrate;
} SerialArgs;

int set_interface_attributes(int handle, int baudrate) {
	struct termios settings;
	memset(&settings, 0, sizeof(settings));

	if (ioctl(handle, TIOCEXCL) == -1) {
		printf("ERROR: %d (%s) from ioctl\n", errno, strerror(errno));
		return -1;
	}

	if (fcntl(handle, F_SETFL, 0) == -1) {
		printf("ERROR: %d (%s) from fcntl\n", errno, strerror(errno));
		return -1;
	}

	if (tcgetattr(handle, &settings) != 0) {
		printf("ERROR: %d (%s) from tcgetattr\n", errno, strerror(errno));
		return -1;
	}

	if (cfsetspeed(&settings, baudrate) == -1) {
		printf("ERROR: %d (%s) from cfsetspeed\n", errno, strerror(errno));
		return -1;
	}

	cfmakeraw(&settings);

	settings.c_cflag = (CLOCAL | CREAD | CS8);
	settings.c_cflag &= ~PARENB;
	settings.c_cflag &= ~CSTOPB;
	settings.c_cflag &= ~CRTSCTS;
	settings.c_iflag = (IGNBRK | IXON | IXOFF);
	settings.c_lflag = 0;
	settings.c_oflag = 0;
	settings.c_cc[VMIN]  = 0; 
	settings.c_cc[VTIME] = 5;

	if (tcsetattr(handle, TCSANOW, &settings)) {
		printf("ERROR: %d (%s) from tcsetattr\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

bool get_args(int argc, char* argv[], SerialArgs *serial_args) {
	bool valid_args = TRUE;

	switch (argc) {
		case 1:
			serial_args->port = "/dev/ttyS1";
			serial_args->baudrate = 115200;
		break;

		case 3:
			serial_args->port = argv[1];
			serial_args->baudrate = atoi(argv[2]);
		break;

		default:
			valid_args = FALSE;
			printf("Usage: %s <serial port> <baud rate>\n", argv[0]);
		break;
	}

	return valid_args;
}

void print_buffer(uint32_t length, uint8_t *data) {
	uint32_t byte_index;
	for (byte_index = 0; byte_index < length; byte_index++) {
		printf("%02X%s", data[byte_index], (byte_index == (length - 1)) ? "" : " ");
	}
}

int serial_write(int serial_handle, int length, uint8_t *data) {
	int bytes_written = 0;
	int bytes_remaining = length;

	printf("TX: ");
	print_buffer(length, data);
	printf("\n");

	while (bytes_written < length) {
		bytes_written = write(serial_handle, data, bytes_remaining);
		if (bytes_written == -1) {
			return -1;
		}
		bytes_remaining -= bytes_written;
		data += bytes_written;
	}

	return length;
}

int serial_read(int serial_handle, int length, uint8_t *data) {
	int bytes_read = 0;
	int bytes_remaining = length;

	while (bytes_read < length) {
		bytes_read = read(serial_handle, data, bytes_remaining);

		if (bytes_read == -1) {
			printf("ERROR: read failure - %d (%s)\n", errno, strerror(errno));
			return -1;
		} else if (bytes_read == 0) {
			printf("ERROR: No bytes read\n");
			return 0;
		}

		bytes_remaining -= bytes_read;
		data += bytes_read;
	}

	printf("RX: ");
	print_buffer(length, data);
	printf("\n");

	return length;
}

int main(int argc, char* argv[]) {
	int serial_handle = 0;
	SerialArgs serial_args = {0};
	get_args(argc, argv, &serial_args);

	printf("Settings:\n");
	printf("  PORT: %s\n  BAUD: %d\n", serial_args.port, serial_args.baudrate);

	int serial_flags = O_RDWR | O_NOCTTY | O_NONBLOCK;
	serial_handle = open(serial_args.port, serial_flags);

	if (serial_handle < 0) {
		printf("ERROR: Could not open serial port.\n");
		exit(1);
	}

	if (set_interface_attributes(serial_handle, B115200) == -1) {
		exit(1);
	}
	sleep(1);
	tcflush(serial_handle, TCIOFLUSH);

	printf("Serial port opened.\n");

	uint8_t payload[] = {0, 1, 0, 0, 0};
	uint8_t reset_cmd[5] = {0x20, 0x01, 0x01, 0x01, 0x00};
	serial_write(serial_handle, sizeof(reset_cmd), reset_cmd);

	printf("Giving module time to restart...\n");
	sleep(2);

	uint8_t response[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	serial_read(serial_handle, sizeof(response), response);

	printf("\nFinished\n");

	return 0;
}