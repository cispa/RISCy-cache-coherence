#include "ncurses.h"
#include "sched.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "term.h"
#include "stdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "../libs/cacheutils.h"

static int write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    while (len) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p   += (size_t)w;
        len -= (size_t)w;
    }
    return 0;
}

void print_self_maps(void) {
    int fd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (fd < 0) { perror("open /proc/self/maps"); return; }

    char buf[16384];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n == 0) break;                // EOF
        if (n < 0) {
            if (errno == EINTR) continue; // retry
            perror("read /proc/self/maps");
            close(fd);
            return;
        }
        if (write_all(STDOUT_FILENO, buf, (size_t)n) < 0) {
            perror("write stdout");
            close(fd);
            return;
        }
    }
    close(fd);
}

#define a &cbreak

int main(int argc, char *argv[]) {
    print_self_maps();
    /* printf("%lx\n", a); */
    /* /\* printf("%s\n", strnames[1]); *\/ */
    /* /\* printf("%lx\n", &strnames[0]); *\/ */
    /* /\* printf("a %p\n", strnames[1]); *\/ */
    /* /\* printf("b %p\n", ((char**)(&strnames))[1]); *\/ */
    /* /\* printf("%s\n", ((char**)(&strnames))[1]); *\/ */
    /* while (1) { */
    /*     /\* printf("switch\n"); *\/ */
    /* int n = 100000; */
    /* /\* printf("now\n"); *\/ */
    /* for (int i = 0; i < n; i++) { */
    /*     maccess(a); */
    /*     /\* usleep(100); *\/ */
    /*     sched_yield(); */
    /* } */
    /* sleep(1); */
    /* sleep(1); */
    /* printf("switch\n"); */
    /* for (int i = 0; i < n; i++) { */
    /*     flush(a); */
    /*     usleep(100); */
    /*     /\* sched_yield(); *\/ */
    /* } */
    /* } */


    char* filename = "/system/lib64/libinput.so";

    /* Open file */
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return -1;
    }

    struct stat filestat;
    if (fstat(fd, &filestat) == -1) {
        fprintf(stderr, "Error: Could not obtain file information.\n");
        return -1;
    }

    int offset = 0;

    /* Map file */
    uint8_t* m = (uint8_t*)mmap(0, filestat.st_size-offset, PROT_READ, MAP_SHARED|MAP_POPULATE, fd, offset);
    if (m == MAP_FAILED) {
        fprintf(stderr, "Could not map file: %s\n", filename);
        return -1;
    }

    while (1) {
        maccess(m+0x400);
        sched_yield();
    }

    return 0;
}
