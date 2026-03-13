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
#include "../libs/measure_helpers.h"
#include "../libs/b1_config.h"

FILE *fp_csv;

// List of test configurations: NAME, TRANSIENT, JUMP, LOAD, DEPENDENT, ACCESS_VICTIM
#define TEST_CONFIGS(X) \
  X(nothing,        0, 0, 0, 0, 0) \
  X(b_arch,         0, 1, 0, 0, 0) \
  X(ldr_arch,       0, 0, 1, 0, 0) \
  X(b_trans,        1, 1, 0, 0, 0) \
  X(ldr_trans,      1, 0, 1, 0, 0) \
  X(dep_b_trans,    1, 1, 0, 1, 0) \
  X(dep_ldr_trans,  1, 0, 1, 1, 0) \
  X(dep_b_trans,    1, 1, 0, 1, 1) \
  X(dep_ldr_trans,  1, 0, 1, 1, 1) \

#define NAME nothing
#define T 0
#define J 0
#define L 0
#define D 0
#include "b2_ctg_template.c"

#define NAME b_arch
#define T 0
#define J 1
#define L 0
#define D 0
#include "b2_ctg_template.c"

#define NAME ldr_arch
#define T 0
#define J 0
#define L 1
#define D 0
#include "b2_ctg_template.c"

#define NAME b_trans
#define T 1
#define J 1
#define L 0
#define D 0
#include "b2_ctg_template.c"

#define NAME ldr_trans
#define T 1
#define J 0
#define L 1
#define D 0
#include "b2_ctg_template.c"

#define NAME dep_b_trans
#define T 1
#define J 1
#define L 0
#define D 1
#include "b2_ctg_template.c"

#define NAME dep_ldr_trans
#define T 1
#define J 0
#define L 1
#define D 1
#include "b2_ctg_template.c"

// Registry for a simple driver
struct test_entry { const char *name; uint32_t* (*fn)(void*, int, int); int trans; int jump; int load; int dep; int access_victim; };

#define ENTRY(NAME, T, J, L, D, V) { #NAME, test_##NAME, T, J, L, D, V },
static struct test_entry TESTS[] = {
    TEST_CONFIGS(ENTRY)
};
static const size_t N_TESTS = sizeof(TESTS)/sizeof(TESTS[0]);

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    init_lib_probe_icache();

    struct b1_config cfg = get_b1_config();
    test_icache_prepare_with_nops(cfg.nops, cfg.mfence, cfg.jump);

    char filename[128];
    snprintf(filename, sizeof(filename), "b2_ctg_prevalence_results_%d.csv", cpu);
    printf("Writing to %s\n", filename);
    fp_csv = fopen(filename, "w");
    assert(fp_csv);
    fprintf(fp_csv, "transient,jump,load,dependent,access_victim,time\n");

    #if defined(__aarch64__)
    int iters = 300000;
    #elif defined(__riscv)
    int iters = 100000;
    #else
    int iters = 300000;
    #endif
    for (size_t i = 0; i < N_TESTS; i++) {
        uint64_t sum = 0;
        for (int k = 0; k < iters; k++) {
            sched_yield();
            void* victim = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
            flush(victim);
            mfence();
            nospec();
            if (TESTS[i].access_victim) {
                mfence();
                nospec();
                maccess(victim);
            }
            mfence();
            nospec();
            uint32_t* exec = TESTS[i].fn(victim, 0, 0);
            int t = measure_exec_time(exec);
            sum += (uint64_t)t;
        }
        float avg = (float)sum/iters;
        printf("%-16s acc-vic=%d %f\n", TESTS[i].name, TESTS[i].access_victim, avg);
        fprintf(fp_csv, "%d,%d,%d,%d,%d,%.3f\n", TESTS[i].trans, TESTS[i].jump, TESTS[i].load, TESTS[i].dep, TESTS[i].access_victim, avg);
    }
}
