#define _GNU_SOURCE
#include "math.h"

#if defined(__aarch64__)
#include "../libs/aarch64/lib_arch_sc_aarch64.h"
#elif defined(__riscv)
#include "../libs/riscv64/lib_arch_sc_riscv64.h"
#elif defined(__loongarch64)
#include "../libs/loongarch64/lib_arch_sc_loongarch64.h"
#else
#error "unsupported arch"
#endif

#ifdef DEBUG
#define n_bytes 1000
#else
#define n_bytes (1<<20)
#endif

int main(int argc, char *argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    char filename[128];
    FILE *fp_csv2;
    snprintf(filename, sizeof(filename), "icsc_dcache_covert_channel_performance_results_%d.csv", cpu);
    printf("Writing to %s\n", filename);
    fp_csv2 = fopen(filename, "w");
    assert(fp_csv2);
    fprintf(fp_csv2, "arch,throughput,error-rate,resolution,elapsed,capacity,bytes\n");

    uint64_t dummy_size = 4096*1000;
    char* dummy = mmap(0, dummy_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    memset(dummy, 0x42, dummy_size);
    assert(dummy != MAP_FAILED);
    void* victim = dummy + (random() % dummy_size);

    int threshold = detect_flush_reload_threshold();
    printf("Threshold: %d\n", threshold);

    for (int a = 0; a < 2; a++) {
        int skew = 0;

        int (*measure)(void*);
        if (a) {
           measure = measure_arch_sc;
           threshold = 1;
        } else {
           measure = reload_t;
        }

        uint8_t secret[n_bytes];
        for (unsigned i = 0; i < n_bytes; i++) {
            secret[i] = random();
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
                if (secret_bit) {
                    nospec();
                    maccess(victim);
                }
                mfence();
                nospec();

                int t = measure(victim);
                if (measure == measure_arch_sc) {
                    t = !t;
                }
                int res = t < threshold;
                leaked[j] |= res << k;

                if (secret_bit == res) {
                    correct++;
                }

                sched_yield();
                mfence();
                nospec();
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

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

        double p = (double)(correct_bits_of - correct_bits) / correct_bits_of;
        double C = bps;
        double capacity = C * (1 + ((1 - p) * (log(1 - p)/log(2)) + p * (log(p)/log(2))));

        printf("Capacity: %.2f kbit/sec\n", capacity/1000);

        fprintf(fp_csv2, "%d,%f,%f,%f,%f,%f,%d\n", a, bps, 100.0-correct_rate, resolution, elapsed, capacity, n_bytes);
    }
}
