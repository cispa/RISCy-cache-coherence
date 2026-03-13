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

DEFINE_ASM_BLOB(mov_0, "addi.w $r4, $r0, 0");
DEFINE_ASM_BLOB(mov_1, "addi.w $r4, $r0, 1");
DEFINE_ASM_BLOB(mov_2, "addi.w $r4, $r0, 2");
DEFINE_ASM_BLOB(mov_3, "addi.w $r4, $r0, 3");
DEFINE_ASM_BLOB(ret, "jirl $r0, $r1, 0");
DEFINE_ASM_BLOB(NOP, "nop");

__attribute__((always_inline)) static inline int test_icache_prime(uint32_t* inst) {
    return ((int (*volatile)(void))inst)();
}

__attribute__((always_inline)) static inline void test_icache_prepare(uint32_t* inst) {
    inst[0] = mov_1;
    inst[1] = ret;
    __builtin___clear_cache (inst, inst+1);
}

__attribute__((always_inline)) static inline uint64_t test_icache_time(uint32_t* inst) {
    uint64_t begin = rdtsc();
    ((int (*volatile)(void))inst)();
    uint64_t end = rdtsc();
    return end-begin;
}

// 1 is stale data (in I-CACHE)
__attribute__((always_inline)) static inline int test_icache_is_stale(uint32_t* rwx) {
    int res;
    asm volatile(
        "st.w %[mov_0], %[rwx], 0\n"
        s_mfence
        /* "ibar 0\n" */
        "jirl $r1, %[rwx], 0\n"
        "move %[res], $r4\n"
        : [res] "=r"(res)
        : [rwx] "r"(rwx),
          [mov_0] "r"(mov_0)
        : "r4", "r1"
    );
    return res;
}

#define S_STORE "st.w $r20, $r21, 0\n"
#define S_FLUSH ".long 0\n"
#define S_JUMP "jirl $r0, $r4, 0\n"
int call_template(uint32_t* func, uint32_t* w, uint32_t* x) {
    int res;
    asm volatile(
        "move $r20, %[mov_0]\n"
        "move $r4,  %[x]\n"
        "move $r21, %[w]\n"
        s_mfence
        s_nospec
        "jirl $r1, %[call], 0\n"
        "move %[res], $r4\n"
        : [res] "=r"(res)
        : [w] "r"(w), [x] "r"(x), [mov_0] "r"(mov_0), [call] "r"(func)
        : "r4", "r1", "r20", "r21", "memory"
        );
    return res;
}

const uint64_t exec_mapping_size = 1<<28;
char *exec_mapping;
__attribute__((noinline, aligned(4096))) static int
measure_arch_sc(void *victim) {
    extern void return_to();

    /* sched_yield(); */

    // NOTE: 3A5000 requires this call_a placement for stable behavior.
    /* void **call_a = (void **)(exec_mapping + (((uint64_t)victim & 4095) & (~63))); */
    // NOTE: keep call_a distinct from exec_page to avoid accidental aliasing.
    #ifdef PRIV
    void **call_a = (void **)(exec_mapping + 64*1337);
    #else
    void **call_a = (void **)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    #endif
    *call_a = (void*)&return_to;

    uint32_t* exec_page;
    do {
        // Randomization reduces prefetcher-driven I-cache preloads.
        exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    } while ((void*)exec_page == call_a);

    test_icache_prepare(exec_page);
    // NOTE: very important for ITLB, otherwise we only get 80% copy to I-cache
    test_icache_prime(exec_page);
    mfence();
    nospec();
    flush(exec_page);
    mfence();
    nospec();

#if !defined(PRIV_FLUSH)
    // If no privileged flush, we need to evict/flush I-cache as well
    iflush(exec_page);
    mfence();
    nospec();
#endif

    maccess(exec_page);

    flush(call_a);

    mfence();
    nospec();

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
        "ld.b $r21, %[victim], 0\n"
        "andi $r21, $r21, 0\n"
        "add.d %[exec_page], %[exec_page], $r21\n"
#endif

#ifdef BRANCH
        "jirl $r1, %[exec_page], 0\n"
#endif
#ifdef LOAD
        "ld.b $r23, %[exec_page], 0\n"
#endif

#ifdef TRANSIENT
        "1:\n"
        "ld.d $r1, %[call_a], 0\n"
        "jirl $r0,$r1,0\n"
#endif
        "return_to:\n"
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "r20", "r0", "r23", "r4", "r21", "r24");

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
        *cpu = 3;
    }
    printf("Pinning to core %d...\n", *cpu);
    pin_to_cpu(*cpu);

    init_cacheutils_loongarch64();
}
