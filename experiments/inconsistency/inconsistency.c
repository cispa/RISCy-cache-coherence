#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

#include "../libs/lib_probe_icache.h"

FILE *fp_csv;

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    init_lib_probe_icache();

    char filename[128];
    snprintf(filename, sizeof(filename), "inconsistency_results_%d.csv", cpu);
    printf("Writing to %s\n\n", filename);
    fp_csv = fopen(filename, "w");
    assert(fp_csv);
    fprintf(fp_csv, "incon\n");

    void* exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    for (int i = 0; i < 1000000; i++) {
        test_icache_prepare(exec_page);
        int res1 = test_icache_is_stale(exec_page);
        ((volatile uint32_t*)exec_page)[0] = mov_0;
        mfence();
        int res2 = test_icache_is_stale(exec_page);
        if (res2) {
            fprintf(fp_csv, "1");
            exit(0);
        }
    }
    exit(1);
}
