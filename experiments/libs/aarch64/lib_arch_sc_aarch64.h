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

int n = 500000;

#define DEFINE_ASM_BLOB(name, code) \
    extern uint32_t name; \
    void name##_func(void) { \
        asm volatile( \
            ".global " #name "\n" \
            #name ":\n" \
            "    " code "\n" \
        ); \
    }

DEFINE_ASM_BLOB(mov_0, "mov x0, #0");
DEFINE_ASM_BLOB(mov_1, "mov x0, #1");
DEFINE_ASM_BLOB(mov_2, "mov x0, #2");
DEFINE_ASM_BLOB(mov_3, "mov x0, #3");
DEFINE_ASM_BLOB(ret, "ret");
DEFINE_ASM_BLOB(NOP, "nop");

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
    __builtin___clear_cache ((char*)inst, (char*)(inst+1));
}

// 1 is stale data (in I-CACHE)
__attribute__((always_inline)) static inline int test_icache_is_stale(uint32_t* rwx) {
    int res;
    asm volatile(
        "str %w[mov_0], [%[rwx]]\n"
        s_mfence
        /* ".rept 200\nnop\n.endr\n" */
        "blr %[rwx]\n"
        "mov %w[res], w0\n"
        : [res] "=r"(res)
        : [rwx] "r"(rwx),
          [mov_0] "r"(mov_0)
        : "x0", "x30"
    );
    return res;
}

#define S_STORE "str w20, [x21]\n"
#define S_FLUSH "dc civac, x21\n"
#define S_JUMP "br x0\n"
int call_template(uint32_t* func, uint32_t* w, uint32_t* x) {
    int res;
    asm volatile(
        "mov w20, %w[mov_0]\n"
        "mov x0,  %[x]\n"
        "mov x21, %[w]\n"
        s_mfence
        s_nospec
        "blr %[call]\n"
        "mov %w[res], w0\n"
        : [res] "=r"(res)
        : [x] "r"(x), [w] "r"(w), [mov_0] "r"(mov_0), [call] "r"(func)
        : "x0", "x30", "x20", "x21", "memory"
        );
    return res;
}

const uint64_t exec_mapping_size = 1<<28;
char *exec_mapping;
__attribute__((noinline, aligned(4096))) static int
measure_arch_sc(void *victim) {
    extern void return_to();

    /* sched_yield(); */

    // NOTE: Cortex-A76 needs this very special condition on the address of call_a.
    // No idea yet why. It has to be a static address. Randomly picking does not work.
    // Otherwise we see ~5% hits when there shouldn't be any.
    void **call_a = (void **)(exec_mapping + (((uint64_t)victim & 4095) & (~63)));
    *call_a = (void*)&return_to;

    uint32_t* exec_page;
    do {
        // Randomization reduces prefetcher-driven I-cache preloads.
        exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    } while ((void*)exec_page == call_a);

    test_icache_prepare(exec_page);
    // No need to prime ITLB on ARM.
    /* nospec(); */
    // This flush is very important on ARM.
    flush(exec_page);
    /* nospec(); */
    // This access brings instructions into I-cache on ARM.
    /* maccess(exec_page); */

    flush(call_a);

    // Full barrier here. This brings down false hits a lot.
    /* mfence(); */
    /* nospec(); */

    #define TRANSIENT
    #define BRANCH
    /* #define LOAD */
        #define DEPENDENT

    asm volatile(
        ".global return_to\n"

#ifdef TRANSIENT
        "bl 1f\n"
#endif

#ifdef DEPENDENT
        "ldrb w20, [%[victim]]\n"
        "eor w20, w20, w20\n"
        "add %[exec_page], %[exec_page], x20\n"
#endif
#ifdef BRANCH
        "blr %[exec_page]\n"
#endif
#ifdef LOAD
        "ldrb wzr, [%[exec_page]]\n"
#endif

#ifdef TRANSIENT
        "1:\n"
        "ldr lr, [%[call_a]]\n"
        "ret\n"
#endif
        "return_to:\n"
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "x20", "x30", "x0");

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
        *cpu = 4;
    }
    printf("Pinning to core %d...\n", *cpu);
    pin_to_cpu(*cpu);
}
