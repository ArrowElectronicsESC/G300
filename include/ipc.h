#ifndef RWFULL_H
#define RWFULL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define SRC_UNKNOWN -1
#define SRC_IPC     0
#define SRC_NCP     1
#define SRC_STDIN   2

/**
 * Prototypes
 */

int ipcAddStdin(void);
int ipcAddUart(int fd);
int ipcClientOpen(void);
int ipcHostOpen(void);      // FIXME: This will return  connection socket, not data socket
int ipcClose(void);

int ipcWait(void);
int ipcPeek(int fd);

ssize_t ipcRead(void *buf, size_t sz);
ssize_t ipcWrite(void *buf, size_t sz);
ssize_t ipcReadAll(void *buf, size_t sz);
ssize_t ipcWriteAll(void *buf, size_t sz);

int ipcReadLine(char *buf, size_t sz);
int ipcWriteLine(char *str);

#ifdef __cplusplus
}
#endif
#endif

