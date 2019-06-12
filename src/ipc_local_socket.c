/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include "ipc.h"

/**
 * Constants
 */
#define SOCKET_NAME "/tmp/silabs_123456_bt_gateway.socket"


/**
 *
 */
static int data_socket = -1;
static int connection_socket = -1;
static fd_set readfds;
static fd_set selectfds;
static int uart_fd = -1;
static int stdin_fd = -1;

/**
 * Add stdin to list of things to wait on in the call to ipcWait in the event loop.
 *
 * We assume IPC is using file descriptors for now, this is so that we can wait on multiple descriptors.
 * For the host this is the connection to the client and the NCP uart
 * For the client agents this is the connection to the host and stdin
 */
int ipcAddStdin(void)
{
  stdin_fd = STDIN_FILENO;
  FD_SET(stdin_fd, &readfds);
  return 0;
}

/**
 * Add the uart to the list of things to wait on in the call to ipcWait in the  event loop.
 */
int ipcAddUart(int fd)
{
  if (fd != -1) {
    uart_fd = fd;
    FD_SET(uart_fd, &readfds);
    return 0;
  }

  return -1;
}


/**
 * Open a connection in the client (cli_agent) to the host.
 */
int ipcClientOpen(void)
{
  struct sockaddr_un addr;
  int sc;

  data_socket = socket(AF_LOCAL, SOCK_STREAM, 0);

  if (data_socket == -1) {
    perror("socket: ");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

  sc = connect(data_socket, (const struct sockaddr *)&addr, sizeof (struct sockaddr_un));

  if (sc == -1) {
    perror("connect: ");
    exit(EXIT_FAILURE);
  }

  FD_SET(data_socket, &readfds);
  return sc;
}


/**
 * Initialize a connection on the host to accept messages from the client.
 * In the case of sockets we create a connection socket to listen on and create the data_socket in ipcWait.
 */
int ipcHostOpen(void)
{
  struct sockaddr_un addr;
  int sc;

  unlink(SOCKET_NAME);

  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_socket == -1) {
    perror("connection socket: ");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

  sc = bind(connection_socket, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
  if (sc == -1) {
    perror("bind: ");
    exit(EXIT_FAILURE);
  }

  sc = listen(connection_socket, 1);
  if (sc == -1) {
    perror("listen: ");
    exit(EXIT_FAILURE);
  }

  FD_SET(connection_socket, &readfds);
  return 0;
}

/**
 * Wait for events from multiple sources e.g. the host waits on incoming messages from the client and events from the NCP uart.
 * This is need as if we blocked on reading from the client we would not detect unsolicited messages from the NCP, for example
 * start_scan message is received and a command to start discovery is sent to the NCP.  We need to listen for responses from the
 * NCP containing advertising beacons AND listen for further commands such as stop_scan from the client.
 *
 * For the cli_agent we may wish to send a command and wait for responses AND also wait for further input from the user.
 *
 * For the case of sockets we accept a new connection here.
 */
int ipcWait(void)
{
  int max_select_fd;
  int sc;
  int source = SRC_UNKNOWN;

  do {
    max_select_fd = 0;
    if (stdin_fd != -1) {
      max_select_fd = stdin_fd;
    }
    max_select_fd = (connection_socket > max_select_fd) ? connection_socket : max_select_fd;
    if (uart_fd != -1) {
      max_select_fd = (uart_fd > max_select_fd) ? uart_fd : max_select_fd;
    }

    if (data_socket != -1) {
      max_select_fd = (data_socket > max_select_fd) ? data_socket : max_select_fd;
    }

    selectfds = readfds;
    sc = select(max_select_fd + 1, &selectfds, NULL, NULL, NULL);

    if (sc == -1) {
      perror("select: ");
      exit(EXIT_FAILURE);
    }

    if (connection_socket != -1 && FD_ISSET(connection_socket, &selectfds)) {
      data_socket = accept(connection_socket, NULL, NULL);

      if (data_socket == -1) {
        perror("accept: ");
        exit(EXIT_FAILURE);
      }

      FD_SET(data_socket, &readfds);
      FD_CLR(connection_socket, &readfds);
    }

    if (data_socket != -1 && FD_ISSET(data_socket, &selectfds)) {
      source = SRC_IPC;
    } else if (uart_fd != -1 && FD_ISSET(uart_fd, &selectfds)) {
      source = SRC_NCP;
    } else if (stdin_fd != -1 && FD_ISSET(stdin_fd, &selectfds)) {
      source = SRC_STDIN;
    }
  } while (source == SRC_UNKNOWN);

  return source;
}

/**
 * Close the ipc connection.
 */
int ipcClose(void)
{
  int sc;

  sc = close(data_socket);
  FD_CLR(data_socket, &readfds);
  if (connection_socket != -1) {
    FD_SET(connection_socket, &readfds);
  }
  data_socket = -1;
  return sc;
}

/**
 * Read upto sz bytes from the ipc channel.
 *
 * \return Number of bytes read, 0 if disconnected and -1 on error.
 */
ssize_t ipcRead(void *buf, size_t sz)
{
  return read(data_socket, buf, sz);
}

/**
 * Write upto sz bytes from the ipc channel.
 *
 * \return Number of bytes written, 0 if disconnected and -1 on error.
 */
ssize_t ipcWrite(void *buf, size_t sz)
{
  return write(data_socket, buf, sz);
}

/**
 * Wrapper around ipcRead to read the full sz bytes of data
 *
 * \return Number of bytes read, 0 if disconnected and -1 on error.
 */
ssize_t ipcReadAll(void *buf, size_t sz)
{
  ssize_t nbytes_recv = 0;
  ssize_t nbytes;

  while (nbytes_recv < sz) {
    nbytes = ipcRead((uint8_t*)buf + nbytes_recv, sz - nbytes_recv);

    if (nbytes == -1) {
      perror("ipcReadAll: ");
      return -1;
    } else if (nbytes == 0) {
      return nbytes_recv;
    }

    nbytes_recv += nbytes;
  }

  return nbytes_recv;
}

/**
 * Wrapper around ipcWrite to write all the data in the buffer
 *
 * \return Number of bytes written, 0 if disconnected and -1 on error.
 */
ssize_t ipcWriteAll(void *buf, size_t sz)
{
  ssize_t nbytes_sent = 0;
  ssize_t nbytes;

  while (nbytes_sent < sz) {
    nbytes = ipcWrite((uint8_t*)buf + nbytes_sent, sz - nbytes_sent);

    if (nbytes == -1) {
      perror("ipcWriteAll: ");
      return -1;
    } else if (nbytes == 0) {
      return nbytes_sent;
    }

    nbytes_sent += nbytes;
  }

  return nbytes_sent;
}

/**
 * Read a line of text from the ipc channel into a buffer with the newline character removed.
 *
 * \return 0 on success, -1 on error
 */
int ipcReadLine(char *buf, size_t sz)
{
  size_t len = 0;
  char ch;

  while (ipcRead(&ch, 1) == 1)
  {
    if (ch == '\n') {
      buf[len] = '\0';
      return 0;
    } else if (len < sz-1) {
      buf[len] = ch;
      len++;
    }
  }

  return -1;
}

/**
 * Read a line of text and appends a newline character to the ipc channel.
 *
 * \return 0 on success, -1 on error
 */
int ipcWriteLine(char *str)
{
  char newline = '\n';
  size_t len;
  len = strlen(str);

  if (ipcWriteAll(str, len) != len) {
    return -1;
  }

  if (ipcWriteAll(&newline, sizeof newline) != sizeof newline) {
    return -1;
  }

  return 0;
}

