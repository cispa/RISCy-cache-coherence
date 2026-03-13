#define _GNU_SOURCE

#include <stdio.h>

#include "lib_arch_sc_riscv64.h"

#define MEASURE_ICACHE test_icache_is_stale
/* #define MEASURE_ICACHE test_icache_time */

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(cpu);

    while (1) {
        uint32_t* exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~7)));

        int a = 0;
        int n = 10000;

        for (int i = 0; i < n; i++) {
            sched_yield();
            test_icache_prepare(exec_page);
                // Not needed on C910
                /* test_icache_prime(exec_page); */
                /* ifence(); */
                /* flush(exec_page); */
            ifence();
            // Very important on C910.
            maccess(exec_page);
            ifence();
            a += MEASURE_ICACHE(exec_page);
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);

        a = 0;
        for (int i = 0; i < n; i++) {
            sched_yield();
            test_icache_prepare(exec_page);
                // Not needed on C910
                /* test_icache_prime(exec_page); */
                /* ifence(); */
                /* flush(exec_page); */
            ifence();
            // Very important on C910.
            maccess(exec_page);
            ifence();
            test_icache_prime(exec_page);
            ifence();
            a += MEASURE_ICACHE(exec_page);
        }
        printf("should be high: %6.2f\n", (float)a * 100 / n);

        a = 0;
        int b = 0;
        for (int i=0; i<2*n; i++) {
            int choice = random() % 2;
            sched_yield();
            test_icache_prepare(exec_page);
                // Not needed on C910
                /* test_icache_prime(exec_page); */
                /* ifence(); */
                /* flush(exec_page); */
            ifence();
            // Very important on C910.
            maccess(exec_page);
            ifence();
            if (!choice) {
                ifence();
                test_icache_prime(exec_page);
            }
            ifence();
            if (choice) {
                a += MEASURE_ICACHE(exec_page);
            } else {
                b += MEASURE_ICACHE(exec_page);
            }
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);
        printf("should be high: %6.2f\n", (float)b * 100 / n);
        printf("---------------\n");
    }
}
