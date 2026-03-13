#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"
#include "sys/stat.h"
#include "sys/mman.h"
#include "stdio.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "../libs/cacheutils.h"

typedef struct {
    unsigned long long start;
    unsigned long long end;
    unsigned long long offset; // file offset
    char perms[5];   // e.g., "r-xp"
    char *path;      // dup'd path (starts with '/')
} mapping_t;

static int parse_maps_line(char *line, mapping_t *m)
{
    int n = 0;
    unsigned long long start, end, off;
    char perms[5];

    if (sscanf(line, "%llx-%llx %4s %llx %*x:%*x %*lu %n",
               &start, &end, perms, &off, &n) < 4)
        return 0;

    char *p = line + n;
    while (*p == ' ' || *p == '\t') p++;

    size_t len = strlen(p);
    while (len && (p[len-1] == '\n' || p[len-1] == '\r')) p[--len] = '\0';

    if (!(len > 0 && p[0] == '/')) return 0;
    if (perms[0] != 'r') return 0;
    if (end <= start) return 0;

    m->start = start;
    m->end   = end;
    m->offset = off;
    memcpy(m->perms, perms, sizeof(perms));
    m->path  = strdup(p);
    return m->path != NULL;
}

static const char *basename_or_path(const char *path)
{
    if (!path) return NULL;
    const char *b = strrchr(path, '/');
    return b ? b + 1 : path;
}

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
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
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

void pin_to_cpu(int cpu) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[])
{
    int cpu = 0;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    pin_to_cpu(cpu);

    print_self_maps();
    printf("\n\n");

    srand((unsigned)time(NULL));

    char exe_path[4096] = {0};
    ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (exe_len >= 0) exe_path[exe_len] = '\0';

    FILE *fp = fopen("/proc/self/maps", "re");
    if (!fp) { perror("fopen /proc/self/maps"); return 1; }

    mapping_t *vec = NULL;
    size_t len = 0, cap = 0;

    char *line = NULL;
    size_t lcap = 0;
    while (getline(&line, &lcap, fp) != -1) {
        mapping_t m = {0};
        if (!parse_maps_line(line, &m)) continue;

        if (exe_path[0] && strcmp(m.path, exe_path) == 0) { // skip the exec binary
            free(m.path);
            continue;
        }

        if (len == cap) {
            size_t ncap = cap ? cap * 2 : 32;
            mapping_t *tmp = realloc(vec, ncap * sizeof(*vec));
            if (!tmp) { perror("realloc"); free(line); fclose(fp); return 1; }
            vec = tmp; cap = ncap;
        }
        vec[len++] = m;
    }
    free(line);
    fclose(fp);

    if (len == 0) {
        fprintf(stderr, "No eligible file-backed mappings found.\n");
        free(vec);
        return 1;
    }

#ifdef DEBUG
    mapping_t sel = vec[0];
#else
    mapping_t sel = vec[rand() % len];
#endif

    const unsigned long long cl = 64;
    unsigned long long size = sel.end - sel.start;
    unsigned long long nlines = size / cl;
    if (nlines == 0) {
        fprintf(stderr, "Selected mapping too small.\n");
        for (size_t i = 0; i < len; i++) free(vec[i].path);
        free(vec);
        return 1;
    }

#ifdef DEBUG
    unsigned long long line_idx = 10;
#else
    unsigned long long line_idx = (unsigned long long)(rand() % nlines);
#endif
    unsigned long long target = sel.start + line_idx * cl;

    unsigned long long elf_base = sel.start - sel.offset;
    unsigned long long rel = target - elf_base;

    const char *fname = sel.path;
    const char *bname = basename_or_path(fname);

    fprintf(stderr,
            "Selected mapping: %016llx-%016llx perms=%s off=%llx\n"
            "File: %s\n"
            "Basename: %s\n"
            "Chosen cache line #%llu\n"
            "rel=0x%llx abs=0x%llx\n",
            sel.start, sel.end, sel.perms, sel.offset,
            fname, bname ? bname : "(null)",
            line_idx,
            rel, target);

    while (1) {
        sched_yield();
        maccess((void*)target);
    }
}
