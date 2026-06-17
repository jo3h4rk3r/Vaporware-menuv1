#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <unistd.h>

static uint8_t heap[2048];
static uint8_t *heap_ptr = heap;

caddr_t _sbrk(int incr)
{
    uint8_t *prev = heap_ptr;
    heap_ptr += incr;
    return (caddr_t)prev;
}

int _close(int file) { return -1; }

int _lseek(int file, int ptr, int dir) { return 0; }

int _read(int file, char *ptr, int len) {
    return 0;
}

int _write(int file, char *ptr, int len) {
    return len; // pretend everything printed successfully
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

void _exit(int status) {
    while (1) {}
}

caddr_t _sbrk(int incr) {
    extern char _end;
    static char *heap_end;

    if (!heap_end)
        heap_end = &_end;

    char *prev = heap_end;
    heap_end += incr;
    return (caddr_t) prev;
}