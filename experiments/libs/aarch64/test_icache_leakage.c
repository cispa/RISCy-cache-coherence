#define _GNU_SOURCE

#include <stdio.h>

#include "lib_arch_sc_aarch64.h"

#define MEASURE_ICACHE test_icache_is_stale
/* #define MEASURE_ICACHE test_icache_time */

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    while (1) {
        uint32_t* exec_page;

        int a = 0;
        int n = 500000;

        for (int i = 0; i < n; i++) {
            // Needed on A76, otherwise the bottom case has 50% false positives.
            exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));

            sched_yield();
            test_icache_prepare(exec_page);
            flush(exec_page);
            ifence();
            a += MEASURE_ICACHE(exec_page);
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);

        a = 0;
        for (int i = 0; i < n; i++) {
            // Needed on A76, otherwise the bottom case has 50% false positives.
            exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));

            sched_yield();
            test_icache_prepare(exec_page);
            flush(exec_page);
            ifence();
            test_icache_prime(exec_page);
            ifence();
            a += MEASURE_ICACHE(exec_page);
        }
        printf("should be high: %6.2f\n", (float)a * 100 / n);

        a = 0;
        int b = 0;
        for (int i=0; i<2*n; i++) {
            // Needed on A76, otherwise the bottom case has 50% false positives.
            exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));

            int choice = random() % 2;
            sched_yield();
            test_icache_prepare(exec_page);
            flush(exec_page);
            ifence();
            if (!choice) {
                ifence();
                test_icache_prime(exec_page);
            }
            ifence();
            int res = MEASURE_ICACHE(exec_page);
            if (choice) {
                a += res;
            } else {
                b += res;
            }
        }
        printf("should be low:  %6.2f\n", (float)a * 100 / n);
        printf("should be high: %6.2f\n", (float)b * 100 / n);
        printf("---------------\n");
    }
}
