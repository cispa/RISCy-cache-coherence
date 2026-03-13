#if defined(__aarch64__)
#include "aarch64/lib_arch_sc_aarch64.h"
#elif defined(__riscv)
#include "riscv64/lib_arch_sc_riscv64.h"
#else
#include "loongarch64/lib_arch_sc_loongarch64.h"
#endif

int call_template(uint32_t* func, uint32_t* w, uint32_t* x);
void test_primitive(uint32_t* func, int* fp, int* fn, int* tp, int* tn, uint32_t* exec, int d_cached, int itlb_cached, int prime_itlb) {
    uint32_t* exec_page;

    *fp = 0;
    *fn = 0;
    *tp = 0;
    *tn = 0;

    for (int i=0; i<2*n; i++) {
        if (exec) {
            exec_page = exec;
        } else {
            // NOTE: Required on A76; otherwise the negative case shows ~50% false positives.
            exec_page = (uint32_t*)(exec_mapping + ((random() % exec_mapping_size) & (~63)));
        }
        sched_yield();

        int choice = random() % 2;
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
        if (!choice) {
            mfence();
            nospec();
            test_icache_prime(exec_page);
            if (prime_itlb) {
                mfence();
                nospec();
                iflush(exec_page);
            }
        }
        mfence();
        nospec();
        int res = call_template(func, exec_page, exec_page);
        if (!choice) {
            if (res) {
                (*tp)++;
            } else {
                (*fn)++;
            }
        } else {
            if (res) {
                (*fp)++;
            } else {
                (*tn)++;
            }
        }
    }
}

DEFINE_ASM_BLOB(STORE, S_STORE);
DEFINE_ASM_BLOB(FLUSH, S_FLUSH);
DEFINE_ASM_BLOB(JUMP, S_JUMP);
DEFINE_ASM_BLOB(MFENCE, s_mfence);

uint32_t* template;

uint32_t* test_icache_prepare_with_nops(int nops, int mfence, int jump) {
    int i = 0;
    template[i++] = STORE;
    if (mfence) {
        template[i++] = MFENCE;
    }
    for (int j = 0; j < nops; j++) {
        template[i++] = NOP;
    }
    /* template[i++] = MFENCE; */
    /* template[i++] = FLUSH; */
    if (jump) {
        template[i++] = JUMP;
    }

    __builtin___clear_cache ((char*)&template[0], (char*)&template[i+1]);

    return &template[i];
}

void init_lib_probe_icache() {
    template = (uint32_t*)mmap(0, (1<<17)*4+4*4, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    assert(template != MAP_FAILED);
}
