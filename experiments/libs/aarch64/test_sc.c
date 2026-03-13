#define _GNU_SOURCE

#include <stdio.h>

#include "lib_arch_sc_aarch64.h"

#define MEASURE measure_arch_sc

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    uint64_t dummy_size = 4096*1000;
    char* dummy = mmap(0, dummy_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    memset(dummy, 0x42, dummy_size);
    assert(dummy != MAP_FAILED);
    void* victim = dummy + (random() % dummy_size);

    /* maccess(victim); */
    /* assert(MEASURE(victim) == 1); */
    /* flush(victim); */
    /* assert(MEASURE(victim) == 0); */

    while (1) {
        int a = 0;
        int n = 200000;
        for (int i = 0; i < n; i++) {
            flush(victim);
            asm volatile("DSB SY\nISB\n");
            a += MEASURE(victim);
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);

        a = 0;
        for (int i = 0; i < n; i++) {
            flush(victim);
            asm volatile("DSB SY\nISB\n");
            maccess(victim);
            asm volatile("DSB SY\nISB\n");
            a += MEASURE(victim);
        }
        printf("should be high: %6.2f\n", (float)a * 100 / n);

        a = 0;
        int b = 0;
        for (int i=0; i<2*n; i++) {
            int choice = random() % 2;
            flush(victim);
            if (!choice) {
                asm volatile("DSB SY\nISB\n");
                maccess(victim);
            }
            asm volatile("DSB SY\nISB\n");
            if (choice) {
                a += MEASURE(victim);
            } else {
                b += MEASURE(victim);
            }
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);
        printf("should be high: %6.2f\n", (float)b * 100 / n);
        printf("---------------\n");
    }
}
