#define _GNU_SOURCE

#define PRIV_IFLUSH
#define PRIV_FLUSH

#include <stdio.h>
#include <float.h>
#include "../libs/lib_probe_icache.h"

FILE *fp_csv;

__attribute__((always_inline)) static inline int measure_exec_time(uint32_t* inst) {
    int begin = rdtsc();
    ((int (*volatile)(void))inst)();
    int end = rdtsc();
    return end-begin;
}

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    char filename[128];
    snprintf(filename, sizeof(filename), "timer_resolution_results_%d.csv", cpu);
    printf("Writing to %s\n", filename);
    fp_csv = fopen(filename, "w");
    assert(fp_csv);
    fprintf(fp_csv, "minimal-increments,elapsed-timer,elapsed-wallclock,resolution,reload-time,flush-reload-time,flush-reload-diff,flush-reload-threshold,relative-to-min-inc,i-reload-time,i-flush-reload-time,i-flush-reload-diff,i-flush-reload-threshold,i-relative-to-min-inc\n");

    uint64_t min = -1;
    for (int i = 0; i < 1000000; i++) {
        uint64_t first = rdtsc();
        uint64_t second = rdtsc();
        uint64_t diff = second-first;
        if (diff > 0 && diff < min) {
            min = diff;
        }
    }
    printf("min: %ld\n", min);

    uint64_t timer_start = rdtsc();
    uint64_t wallclock_start = time(NULL);;

    sleep(10);

    uint64_t timer_end = rdtsc();
    uint64_t wallclock_end = time(NULL);

    uint64_t elapsed_wallclock = wallclock_end-wallclock_start;
    uint64_t elapsed_timer = timer_end-timer_start;
    printf("%ld %ld\n", elapsed_wallclock, elapsed_timer);
    float res_ns = ((double)min*elapsed_wallclock*1000*1000*1000)/elapsed_timer;
    printf("resolution: %fns\n", res_ns);

    size_t reload_time = 0, flush_reload_time = 0, i, count = 1000000;
    size_t dummy[16];
    size_t *ptr = dummy + 8;

    maccess(ptr);
    for (i = 0; i < count; i++) {
        reload_time += reload_t(ptr);
    }
    for (i = 0; i < count; i++) {
        flush_reload_time += flush_reload_t(ptr);
    }
    reload_time /= count;
    flush_reload_time /= count;

    printf("reload time: %ld\n", reload_time);
    printf("F+R time: %ld\n", flush_reload_time);
    if (flush_reload_time < reload_time) {
        printf("error\n");
        exit(1);
    }
    size_t fr_diff = flush_reload_time-reload_time;
    printf("diff time: %ld\n", fr_diff);
    double fr_rel = (double)fr_diff/min;
    printf("relative to minimal timer increment: %f\n", fr_rel);

    CACHE_MISS = (flush_reload_time + reload_time * 2) / 3;
    printf("FR threshold: %ld\n", CACHE_MISS);

    size_t i_reload_time = 0, i_flush_reload_time = 0;
    uint32_t* exec_page = (uint32_t*)exec_mapping;
    test_icache_prepare(exec_page);
    for (i = 0; i < count; i++) {
        /* uint32_t* exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63))); */
        /* test_icache_prepare(exec_page); */
        flush(exec_page);
        mfence();
        nospec();
        iflush(exec_page);
        mfence();
        nospec();
        test_icache_prime(exec_page);
        mfence();
        nospec();
        i_reload_time += measure_exec_time(exec_page);
    }
    for (i = 0; i < count; i++) {
        /* uint32_t* exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63))); */
        /* test_icache_prepare(exec_page); */
        flush(exec_page);
        mfence();
        nospec();
        iflush(exec_page);
        mfence();
        nospec();
        i_flush_reload_time += measure_exec_time(exec_page);
    }
    i_flush_reload_time /= count;
    i_reload_time /= count;
    printf("i-cache reload time %ld\n", i_reload_time);
    printf("i-cache FR time %ld\n", i_flush_reload_time);
    if (i_flush_reload_time < i_reload_time) {
        printf("error\n");
        exit(1);
    }
    size_t ifr_diff = i_flush_reload_time-i_reload_time;
    printf("diff time: %ld\n", ifr_diff);
    double ifr_rel = (double)ifr_diff/min;
    printf("relative to minimal timer increment: %f\n", ifr_rel);

    size_t ICACHE_MISS = (i_flush_reload_time + i_reload_time * 2) / 3;
    printf("I-cache FR threshold: %ld\n", ICACHE_MISS);

    fprintf(fp_csv, "%ld,%ld,%ld,%f,%ld,%ld,%ld,%ld,%f,%ld,%ld,%ld,%ld,%f\n", min, elapsed_timer, elapsed_wallclock, res_ns, reload_time, flush_reload_time, fr_diff, CACHE_MISS, fr_rel, i_reload_time, i_flush_reload_time, ifr_diff, ICACHE_MISS, ifr_rel);
}
