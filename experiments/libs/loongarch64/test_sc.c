#define _GNU_SOURCE

#include <stdio.h>

#include "lib_arch_sc_loongarch64.h"

#define MEASURE measure_arch_sc

static __attribute__((aligned(16384))) char dummy[4096*100];

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    memset(dummy, 0x42, sizeof(dummy));
    /* void* victim = (void*)&dummy[0 % sizeof(dummy)]; */
    /* void* victim = (void*)&dummy[random() % sizeof(dummy)]; */
    /* void* victim = (void*)&dummy[(4096)]; */
    // NOTE: Without privileged mode, only some offsets are stable.
    void* victim = (void*)&dummy[(0)];
    /* void* victim = (void*)&dummy[(312)]; */
    // [0,64) [2048,16384)

    /* maccess(victim); */
    /* assert(MEASURE(victim) == 1); */
    /* flush(victim); */
    /* assert(MEASURE(victim) == 0); */

    CACHE_MISS=detect_flush_reload_threshold();
    printf("detect %d\n", CACHE_MISS);

    /* go = 1; */
    while (1) {
        int a = 0;
        int n = 500000;
        for (int i = 0; i < n; i++) {
            /* printf("vic: %p\n", victim); */
            flush(victim);
            /* maccess(victim); */
            /* exit( 0                ); */
            mfence();
            nospec();
            a += MEASURE(victim);
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);

        a = 0;
        for (int i = 0; i < n; i++) {
            flush(victim);
            mfence();
            nospec();
            maccess(victim);
            mfence();
            nospec();
            a += MEASURE(victim);
        }
        printf("should be high: %6.2f\n", (float)a * 100 / n);

        a = 0;
        int b = 0;
        for (int i=0; i<2*n; i++) {
            int choice = random() % 2;
            flush(victim);
            mfence();
            nospec();
            if (!choice) {
                mfence();
                nospec();
                maccess(victim);
            }
            if (choice) {
                mfence();
                nospec();
                a += MEASURE(victim);
            } else {
                mfence();
                nospec();
                b += MEASURE(victim);
            }
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);
        printf("should be high: %6.2f\n", (float)b * 100 / n);
        printf("---------------\n");
    }
}
