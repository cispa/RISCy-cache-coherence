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

#include "../libs/cacheutils.h"

typedef struct {
    unsigned long long start;
    unsigned long long end;
    char perms[5];
    unsigned long long offset;
    unsigned int dev_major;
    unsigned int dev_minor;
    unsigned long inode;
    const char *path;
} mapping_t;

static int parse_maps_line(char *line, mapping_t *m)
{
    if (!line || !m) return 0;
    int n = 0;
    int matched = sscanf(line,
                         "%llx-%llx %4s %llx %x:%x %lu %n",
                         &m->start, &m->end, m->perms, &m->offset,
                         &m->dev_major, &m->dev_minor, &m->inode, &n);
    if (matched < 7) return 0;
    char *p = line + n;
    while (*p == ' ' || *p == '\t') p++;
    size_t len = strlen(p);
    while (len && (p[len - 1] == '\n' || p[len - 1] == '\r')) p[--len] = '\0';
    m->path = (*p) ? p : NULL;
    return 1;
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

void pin_to_cpu(int cpu) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
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

static const char *basename_or_path(const char *path) {
    if (!path) return NULL;
    const char *b = strrchr(path, '/');
    return b ? b + 1 : path;
}

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

static void unmap_explicit(explicit_map_t *m)
{
    if (!m) return;
    if (m->addr && m->length) munmap(m->addr, m->length);
    free(m->dup_path);
    memset(m, 0, sizeof(*m));
}

FILE* fp_csv;
int threshold;
void scan(const char* name, const char* path, char *start, char *end, unsigned long long elf_base) {
    for (; start < end; start+=64) {
        int n = 1000000;
        int hits = 0;
        for (int i = 0; i < n; i++) {
            sched_yield();
            int t = flush_reload_t(start);
            if (t < threshold) hits++;
        }
        unsigned long long absaddr = (unsigned long long)start;
        unsigned long long reladdr = absaddr - elf_base;
        if (hits > 0.8*n) {
            printf("%s %lx (abs %lx): %d\n", name, reladdr, absaddr, hits);
        }
        fprintf(fp_csv, "%d,%s,%lx,%lx,%s\n", hits, name, reladdr, absaddr, path);
        sleep(1000);
    }
}

int main(int argc, char* argv[])
{
    int cpu = 7;
    if (argc > 1) cpu = atoi(argv[1]);
    pin_to_cpu(cpu);

    print_self_maps();
    printf("\n\n");
    printf("Pinning core %d...\n", cpu);

    char exe_path[4096];
    ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (exe_len >= 0) exe_path[exe_len] = '\0'; else exe_path[0] = '\0';

    char outname[128];
    snprintf(outname, sizeof(outname), "hit_scan_%d.csv", cpu);
    printf("Writing to %s\n", outname);
    fp_csv = fopen(outname, "w");
    assert(fp_csv);
    fprintf(fp_csv, "hits,name,rel-addr,abs-addr,path\n");

    fprintf(stdout, "[x] Start calibration... ");
    threshold = detect_flush_reload_threshold();
    fprintf(stdout, "%d\n", threshold);

    printf("Waiting...\n");
    getchar();

    FILE *flibs = fopen("libs", "r");
    if (!flibs) {
        perror("fopen libs");
    } else {
        char path[4096];
        while (fgets(path, sizeof path, flibs)) {
            path[strcspn(path, "\r\n")] = '\0';   // strip trailing newline(s)
            if (!*path) continue;                 // skip empty lines if any

            explicit_map_t em = (explicit_map_t){0};
            if (map_whole_file_ro(path, &em) == 0) {
                const char *base = basename_or_path(path);
                unsigned long long base_addr = (unsigned long long)(uintptr_t)em.addr;

                printf("  [explicit] %016llx-%016llx r--p off=0 %s\n",
                       base_addr, base_addr + em.length, path);

                scan(base, path, (char*)em.addr, (char*)em.addr + em.length, base_addr);
                printf("\n");
                unmap_explicit(&em);
            } else {
                fprintf(stderr, "[warn] could not map '%s'\n", path);
            }
        }
        fclose(flibs);
    }
    exit(0);

    FILE *fp = fopen("/proc/self/maps", "re");
    if (!fp) { perror("fopen /proc/self/maps"); return 1; }
    char *line = NULL;
    size_t cap = 0;
    mapping_t m;
    while (getline(&line, &cap, fp) != -1) {
        if (parse_maps_line(line, &m)) {
            if (m.path && m.path[0] == '/' &&
                !(exe_path[0] && strcmp(m.path, exe_path) == 0)) {
                const char *base = basename_or_path(m.path);
                unsigned long long elf_base = m.start - m.offset;
                printf("  %016llx-%016llx %-4s off=%llx dev=%x:%x inode=%lu %s\n",
                       m.start, m.end, m.perms, m.offset,
                       m.dev_major, m.dev_minor, m.inode,
                       m.path);
                scan(base, m.path, (char*)m.start, (char*)m.end, elf_base);
                printf("\n");
            }
        }
    }
    free(line);
    fclose(fp);
    fclose(fp_csv);
    return 0;
}
