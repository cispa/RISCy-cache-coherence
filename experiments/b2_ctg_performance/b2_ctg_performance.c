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

#define NAME dep_b_trans
#define T 1
#define J 1
#define L 0
#define D 1
#include "../b2_ctg_prevalence/b2_ctg_template.c"

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    init_lib_probe_icache();

    struct b1_config cfg = get_b1_config();
    test_icache_prepare_with_nops(cfg.nops, cfg.mfence, cfg.jump);

    uint64_t dummy_size = 4096*1000;
    char* dummy = mmap(0, dummy_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    memset(dummy, 0x42, dummy_size);
    assert(dummy != MAP_FAILED);
    void* victim = dummy + (random() % dummy_size);

    #if defined(__aarch64__)
    int iters = 500000;
    #elif defined(__riscv)
    int iters = 200000;
    #else
    int iters = 500000;
    #endif
    size_t timing_dep_b_trans_no_cache = 0, timing_dep_b_trans_cache = 0;
    for (int cache=0; cache<2; cache++) {
        for (int k = 0; k < iters; k++) {
            flush(victim);
            mfence();
            nospec();
            if (cache) {
                mfence();
                nospec();
                maccess(victim);
            }
            mfence();
            nospec();
            uint32_t* exec = test_dep_b_trans(victim, 0, 0);
            int t = measure_exec_time(exec);
            if (cache) {
                timing_dep_b_trans_cache += t;
            } else {
                timing_dep_b_trans_no_cache += t;
            }
        }
    }
    timing_dep_b_trans_no_cache /= iters;
    timing_dep_b_trans_cache /= iters;

    printf("%ld %ld\n", timing_dep_b_trans_cache, timing_dep_b_trans_no_cache);

    int threshold = (timing_dep_b_trans_cache + timing_dep_b_trans_no_cache * 2) / 3;
    printf("Using threshold: %d\n", threshold);

    char filename[128];
    snprintf(filename, sizeof(filename), "b2_ctg_performance_results_%d.csv", cpu);
    printf("Writing to %s\n\n", filename);
    fp_csv = fopen(filename, "w");
    assert(fp_csv);
    fprintf(fp_csv, "timer,throughput,error-rate,resolution,elapsed,bytes\n");

#ifdef DEBUG
    #define n_bytes (1<<15)
#else
    #define n_bytes (1<<20)
#endif

    uint8_t secret[n_bytes];
    for (unsigned i = 0; i < n_bytes; i++) {
        secret[i] = random();
    }

    // NOTE: Do not change the order, otherwise threshold calibration becomes invalid.
    for (int with_timer = 1; with_timer >= 0; with_timer--) {
        printf("\n%s\n", with_timer ? "With Timer:" : "Arch SC:");
        int skew = 0;

        int (*measure)(uint32_t*);
        if (with_timer) {
           measure = measure_exec_time;
        } else {
           measure = measure_gadget;
           threshold = 0;
        }

        uint8_t leaked[n_bytes] = {0};
        int correct = 0;

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (unsigned j = 0; j < n_bytes; j++) {
            for (unsigned k = 0; k < 8; k++) {
                int secret_bit = (secret[j] >> k) & 1;
                flush(victim);
                mfence();
                nospec();
                if (!secret_bit) {
                    mfence();
                    nospec();
                    maccess(victim);
                }
                mfence();
                nospec();
                // NOTE: ITLB priming can improve separation on some systems.
                #ifdef ITLB_PRIME
                // 3A5000-HV commonly needs itlb_cached.
                uint32_t* exec = test_dep_b_trans(victim, 1, 1);
                #else
                uint32_t* exec = test_dep_b_trans(victim, 0, 0);
                #endif
                int t = measure(exec);
                int res = t > threshold;
                leaked[j] |= res << k;
                skew += res;

                if (secret_bit == res) {
                    correct++;
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        printf("Skew: %f\n", (float)skew/(n_bytes*8));

        int correct_bits = correct;
        int correct_bits_of = n_bytes * 8;
        float correct_rate = ((float)correct_bits * 100 / correct_bits_of);
        printf("error rate: %d/%d %f%%\n", correct_bits_of-correct_bits, correct_bits_of, 100.0-correct_rate);

        double elapsed = (end.tv_sec - start.tv_sec)
                    + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Elapsed: %.9f sec\n", elapsed);

        double bps = (n_bytes*8.0) / elapsed;
        printf("Throughput: %.2f kbit/sec\n", bps/1000);

        double resolution = 1000000.0/bps;
        printf("Resolution: %.2f µs\n", resolution);

        fprintf(fp_csv, "%d,%f,%f,%f,%f,%d\n", with_timer, bps, 100.0-correct_rate, resolution, elapsed, n_bytes);
    }
}
