/*
 * syscalls_stub.c
 *
 *  Created on: 17 de jan. de 2026
 *      Author: Matheus Markies
 */


#include <sys/stat.h>

int _close(int file) { return -1; }
int _fstat(int file, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _write(int file, char *ptr, int len) { return len; }
void _exit(int status) { while (1); }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }
