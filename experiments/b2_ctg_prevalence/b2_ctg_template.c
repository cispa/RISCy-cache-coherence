#define STR1(x) #x
#define STR(x)  STR1(x)
#define PASTE1(a,b) a##b
#define PASTE(a,b) PASTE1(a,b)

__attribute__((noinline, aligned(16384), noclone)) static uint32_t*
PASTE(test_,NAME)(void* victim, int d_cached, int itlb_cached) {
    extern void PASTE(return_to_, NAME)();

    /* void **call_a = (void**)(exec_mapping + ((random() % exec_mapping_size) & (~63))); */

    // NOTE: Cortex-A76 needs this very special condition on the address of call_a.
    // No idea yet why. It has to be a static address. Randomly picking does not work.
    // Otherwise we see ~5% hits when there shouldn't be any.
    void **call_a = (void **)(exec_mapping + (((uint64_t)victim & 4095) & (~63)));
    *call_a = (void*)&PASTE(return_to_, NAME);

    uint32_t* exec_page;
    do {
        // This needs to be randomized, otherwise the prefetcher probably
        // triggers and preloads to I-cache (we always get stale)
        exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
    } while ((void*)exec_page == call_a);

    sched_yield();

    test_icache_prepare(exec_page);
    if (itlb_cached) {
        mfence();
        nospec();
        test_icache_prime(exec_page);
    }
    mfence();
    nospec();
    flush(exec_page);
    mfence();
    nospec();
    iflush(exec_page);
    if (d_cached) {
        mfence();
        nospec();
        // NOTE: this access is required for stable behavior on C910 with mfence.
        // Very important on C910.
        maccess(exec_page);
    }
    mfence();
    nospec();

    flush(call_a);

    // Full barrier here. This brings down false hits a lot.
    mfence();
    nospec();

    asm volatile(
        ".global return_to_"STR(NAME)"\n"

#if T
        #if defined(__aarch64__)
        "bl 1f\n"
        #elif defined(__riscv)
        "call 1f\n"
        #else
        "bl 1f\n"
        #endif
#endif

#if D
        #if defined(__aarch64__)
        "ldrb w20, [%[victim]]\n"
        "eor w20, w20, w20\n"
        "add %[exec_page], %[exec_page], x20\n"
        #elif defined(__riscv)
        "lb x20, 0(%[victim])\n"
        "and x20, x20, 0\n"
        "add %[exec_page], %[exec_page], x20\n"
        #else
        "ld.b $r21, %[victim], 0\n"
        "andi $r21, $r21, 0\n"
        "add.d %[exec_page], %[exec_page], $r21\n"
        #endif
#endif
#if J
        #if defined(__aarch64__)
        "blr %[exec_page]\n"
        #elif defined(__riscv)
        "jalr x1, %[exec_page], 0\n"
        #else
        "jirl $r1, %[exec_page], 0\n"
        #endif
#endif
#if L
        #if defined(__aarch64__)
        "ldrb wzr, [%[exec_page]]\n"
        #elif defined(__riscv)
        "lb zero, 0(%[exec_page])\n"
        #else
        "ld.b $r23, %[exec_page], 0\n"
        #endif
#endif

#if T
        "1:\n"
        #if defined(__aarch64__)
        "ldr lr, [%[call_a]]\n"
        "ret\n"
        #elif defined(__riscv)
        "ld x1, 0(%[call_a])\n"
        "jalr zero, x1, 0\n"
        #else
        "ld.d $r1, %[call_a], 0\n"
        "jirl $r0,$r1,0\n"
        #endif
#endif
        "return_to_"STR(NAME)":\n"
        #if defined(__aarch64__)
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "x20", "x30", "x0", "memory");
        #elif defined(__riscv)
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "x20", "x1", "memory");
        #else
        :: [victim] "r"(victim), [exec_page] "r"(exec_page), [call_a] "r"(call_a) : "r20", "r0", "r23", "r4", "r21", "r24", "memory");
        #endif

    return exec_page;
}

#undef NAME
#undef T
#undef J
#undef L
#undef D
