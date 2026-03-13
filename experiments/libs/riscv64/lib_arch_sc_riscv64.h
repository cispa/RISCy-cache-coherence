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

#include "../cacheutils.h"

int n = 20000;

#define DEFINE_ASM_BLOB(name, code) \
    extern uint32_t name; \
    void name##_func(void) { \
        asm volatile( \
            ".global " #name "\n" \
            #name ":\n" \
            "    " code "\n" \
        ); \
    }

// Twice for when instructions compile to compressed ones
DEFINE_ASM_BLOB(mov_0, "li a0, 0\nli a0, 0\n");
DEFINE_ASM_BLOB(mov_1, "li a0, 1\nli a0, 1\n");
DEFINE_ASM_BLOB(mov_2, "li a0, 2\nli a0, 2\n");
DEFINE_ASM_BLOB(mov_3, "li a0, 3\nli a0, 3\n");
DEFINE_ASM_BLOB(ret, "ret\n");
#define NOP 0x13

__attribute__((always_inline)) static inline int test_icache_prime(uint32_t* inst) {
    return ((int (*volatile)(void))inst)();
}

__attribute__((always_inline)) static inline int test_icache_time(uint32_t* inst) {
    int begin = rdtsc();
    ((int (*volatile)(void))inst)();
    int end = rdtsc();
    return end-begin;
}

__attribute__((always_inline)) static inline void test_icache_prepare(uint32_t* inst) {
    inst[0] = mov_1;
    inst[1] = ret;
    __builtin___clear_cache (inst, inst+1);
}

// 1 is stale data (in I-CACHE)
__attribute__((always_inline)) static inline int test_icache_is_stale(uint32_t* rwx) {
    int res;
    asm volatile(
        "sw %[mov_0], 0(%[rwx])\n"
        s_mfence
        "jalr %[rwx]\n"
        "mv %[res], a0\n"
        : [res] "=r"(res)
        : [rwx] "r"(rwx),
          [mov_0] "r"(mov_0)
        : "a0", "x1"
    );
    return res;
}

#define S_STORE "sw x20, 0(x21)\n"
#if defined(THEAD)
#define S_FLUSH ".option push\n.option arch, +xtheadcmo\nth.dcache.civa x21\n.option pop\n"
#else
#define S_FLUSH ".option push\n.option arch, +zicbom\ncbo.flush 0(x21)\n.option pop\n"
#endif
#define S_JUMP "jalr x0, a0\n"
int call_template(uint32_t* func, uint32_t* w, uint32_t* x) {
    int res;
    asm volatile(
        "mv x20, %[mov]\n"
        "mv  a0, %[x]\n"
        "mv x21, %[w]\n"
        s_mfence
    #ifdef THEAD
        s_nospec
    #endif
        "jalr %[call]\n"
        "mv %[res], a0\n"
        : [res] "=r"(res)
        : [x] "r"(x), [w] "r"(w), [mov] "r"(mov_0), [call] "r"(func)
        : "a0", "x1", "x20", "x21", "memory"
        );
    return res;
}

const uint64_t exec_mapping_size = 1<<26;
char *exec_mapping;
__attribute__((noinline, aligned(4096))) static int
measure_arch_sc(void *victim) {
    extern void return_to();

    /* sched_yield(); */

    // NOTE: C910 requires this call_a address constraint for stable behavior.
    // No idea yet why. Otherwise we see ~5% hits when there shouldn't be any.
    void **call_a = (void **)(exec_mapping + (((uint64_t)victim & 4095) & (~63)));
    *call_a = (void*)&return_to;

    uint32_t* exec_page;
    do {
        // Randomization reduces prefetcher-driven I-cache preloads.
        exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    } while ((void*)exec_page == call_a);

    test_icache_prepare(exec_page);
    // Seems to not be needed on C910
    /* test_icache_prime(exec_page); */
    /* iflush(exec_page); */
    /* mfence(); */
    /* nospec(); */
    // Very important on C910.
    /* maccess(exec_page); */

    flush(call_a);

    #define TRANSIENT
    #define BRANCH
    /* #define LOAD */
        #define DEPENDENT

    asm volatile(
        ".global return_to\n"

#ifdef TRANSIENT
        "call 1f\n"
#endif

#ifdef DEPENDENT
        "lb x20, 0(%[victim])\n"
        "and x20, x20, 0\n"
        "add %[exec_page], %[exec_page], x20\n"
#endif
#ifdef BRANCH
        "jalr x1, %[exec_page], 0\n"
#endif
#ifdef LOAD
        "lb zero, 0(%[exec_page])\n"
#endif

#ifdef TRANSIENT
        "1:\n"
        "ld x1, 0(%[call_a])\n"
        "jalr zero, x1, 0\n"
#endif
        "return_to:\n"
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "x20", "x1", "a0");

    return test_icache_is_stale(exec_page);
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

void init_arch_sc(int* cpu) {
    srandom(time(NULL));

    exec_mapping = (char*)mmap(0, exec_mapping_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    assert(exec_mapping != MAP_FAILED);

    if (*cpu == -1) {
        *cpu = 0;
    }
    printf("Pinning to core %d...\n", *cpu);
    pin_to_cpu(*cpu);
}
