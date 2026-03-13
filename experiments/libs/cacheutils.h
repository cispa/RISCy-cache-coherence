#ifndef _CACHEUTILS_H_
#define _CACHEUTILS_H_

#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>

#define RDPRU ".byte 0x0f, 0x01, 0xfd"
#define RDPRU_ECX_MPERF	0
#define RDPRU_ECX_APERF	1

#define ARM_PERF            1
#define ARM_CLOCK_MONOTONIC 2
#define ARM_TIMER           3

/* ============================================================
 *                    User configuration
 * ============================================================ */
size_t CACHE_MISS = 0;

#define USE_RDTSC_BEGIN_END     0

#define USE_RDTSCP              1

#define ARM_CLOCK_SOURCE        ARM_CLOCK_MONOTONIC
/* #define ARM_CLOCK_SOURCE        ARM_TIMER */

/* ============================================================
 *                  User configuration End
 * ============================================================ */

// ---------------------------------------------------------------------------
static size_t perf_fd;
void perf_init() {
  static struct perf_event_attr attr;
  attr.type = PERF_TYPE_HARDWARE;
  attr.config = PERF_COUNT_HW_CPU_CYCLES;
  attr.size = sizeof(attr);
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;
  attr.exclude_callchain_kernel = 1;

  perf_fd = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
  assert(perf_fd >= 0);

  // ioctl(perf_fd, PERF_EVENT_IOC_RESET, 0);
}

#if defined(__i386__) || defined(__x86_64__)
// ---------------------------------------------------------------------------
uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile("mfence");
#if USE_RDTSCP
  asm volatile("rdtscp" : "=a"(a), "=d"(d) :: "rcx");
#else
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
#endif
  a = (d << 32) | a;
  asm volatile("mfence");
  return a;
}

// ---------------------------------------------------------------------------
uint64_t __rdtsc_begin() {
  uint64_t a, d;
  asm volatile ("mfence\n\t"
    "CPUID\n\t"
    "RDTSCP\n\t"
    "mov %%rdx, %0\n\t"
    "mov %%rax, %1\n\t"
    "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d<<32) | a;
  return a;
}

// ---------------------------------------------------------------------------
uint64_t __rdtsc_end() {
  uint64_t a, d;
  asm volatile("mfence\n\t"
    "RDTSCP\n\t"
    "mov %%rdx, %0\n\t"
    "mov %%rax, %1\n\t"
    "CPUID\n\t"
    "mfence\n\t"
    : "=r" (d), "=r" (a)
    :
    : "%rax", "%rbx", "%rcx", "%rdx");
  a = (d<<32) | a;
  return a;
}

// ---------------------------------------------------------------------------
void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

void maccess_wr(void *p, int val) { asm volatile("movq %%rax, (%1)\n" : : "a"(val), "c"(p) : ); }

// ---------------------------------------------------------------------------
#define s_mfence "mfence\n"
void mfence() { asm volatile(s_mfence); }

// ---------------------------------------------------------------------------
void nospec() { asm volatile("lfence"); }

#define speculation_start(label) asm goto ("call %l0" : : : : label##_retp); 
#define speculation_end(label) asm goto("jmp %l0" : : : : label); label##_retp: asm goto("lea %l0(%%rip), %%rax\nmovq %%rax, (%%rsp)\nret\n" : : : "rax" : label); label: asm volatile("nop");

// example usage: asm volatile(INTELASM("clflush [rax]\n\t"));
#define INTELASM(code) ".intel_syntax noprefix\n\t" code "\n\t.att_syntax prefix\n"


#include <cpuid.h>
// ---------------------------------------------------------------------------
unsigned int xbegin() {
  unsigned status;
  asm volatile(".byte 0xc7,0xf8,0x00,0x00,0x00,0x00" : "=a"(status) : "a"(-1UL) : "memory");
  return status;
}

// ---------------------------------------------------------------------------
void xend() {
  asm volatile(".byte 0x0f; .byte 0x01; .byte 0xd5" ::: "memory");
}

// ---------------------------------------------------------------------------
int has_tsx() {
  if (__get_cpuid_max(0, NULL) >= 7) {
    unsigned a, b, c, d;
    __cpuid_count(7, 0, a, b, c, d);
    return (b & (1 << 11)) ? 1 : 0;
  } else {
    return 0;
  }
}

// ---------------------------------------------------------------------------
void maccess_tsx(void* ptr) {
    if (xbegin() == (~0u)) {
        maccess(ptr);
        xend();
    }
}

#elif defined(__aarch64__)
#if ARM_CLOCK_SOURCE == ARM_CLOCK_MONOTONIC
#include <time.h>
#endif

// ---------------------------------------------------------------------------
uint64_t rdtsc() {
#if ARM_CLOCK_SOURCE == ARM_PERF
  long long result = 0;

  asm volatile("DSB SY");
  asm volatile("ISB");

  if (read(perf_fd, &result, sizeof(result)) < (ssize_t) sizeof(result)) {
    return 0;
  }

  asm volatile("ISB");
  asm volatile("DSB SY");

  return result;
#elif ARM_CLOCK_SOURCE == ARM_CLOCK_MONOTONIC
  asm volatile("DSB SY");
  asm volatile("ISB");
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t res = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  asm volatile("ISB");
  asm volatile("DSB SY");
  return res;
#elif ARM_CLOCK_SOURCE == ARM_TIMER
  uint64_t result = 0;

  asm volatile("DSB SY");
  asm volatile("ISB");
  asm volatile("MRS %0, cntvct_el0" : "=r"(result));
  asm volatile("DSB SY");
  asm volatile("ISB");

  return result*10;
#else
#error Clock source not supported
#endif
}
// ---------------------------------------------------------------------------
uint64_t __rdtsc_begin() {
#if ARM_CLOCK_SOURCE == ARM_PERF
  long long result = 0;

  asm volatile("DSB SY");
  asm volatile("ISB");

  if (read(perf_fd, &result, sizeof(result)) < (ssize_t) sizeof(result)) {
    return 0;
  }

  asm volatile("DSB SY");

  return result;
#elif ARM_CLOCK_SOURCE == ARM_CLOCK_MONOTONIC
  asm volatile("DSB SY");
  asm volatile("ISB");
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t res = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  asm volatile("DSB SY");
  return res;
#elif ARM_CLOCK_SOURCE == ARM_TIMER
  uint64_t result = 0;

  asm volatile("DSB SY");
  asm volatile("ISB");
  asm volatile("MRS %0, PMCCNTR_EL0" : "=r"(result));
  asm volatile("ISB");

  return result;
#else
#error Clock source not supported
#endif
}


// ---------------------------------------------------------------------------
uint64_t __rdtsc_end() {
#if ARM_CLOCK_SOURCE == ARM_PERF
  long long result = 0;

  asm volatile("DSB SY");

  if (read(perf_fd, &result, sizeof(result)) < (ssize_t) sizeof(result)) {
    return 0;
  }

  asm volatile("ISB");
  asm volatile("DSB SY");

  return result;
#elif ARM_CLOCK_SOURCE == ARM_CLOCK_MONOTONIC
  asm volatile("DSB SY");
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t res = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  asm volatile("ISB");
  asm volatile("DSB SY");
  return res;
#elif ARM_CLOCK_SOURCE == ARM_TIMER
  uint64_t result = 0;

  asm volatile("DSB SY");
  asm volatile("MRS %0, PMCCNTR_EL0" : "=r"(result));
  asm volatile("DSB SY");
  asm volatile("ISB");

  return result;
#else
#error Clock source not supported
#endif
}

// ---------------------------------------------------------------------------
void flush(void *p) {
  asm volatile("DC CIVAC, %0" ::"r"(p));
  asm volatile("DSB ISH");
  asm volatile("ISB");
}

// ---------------------------------------------------------------------------
void iflush(void *p) {
  asm volatile("IC IVAU, %0" : : "r"(p));
  asm volatile("DSB ISH");
  asm volatile("ISB");
}

// ---------------------------------------------------------------------------
void maccess(void *p) {
  volatile uint32_t value;
  asm volatile("LDR %w0, [%1]\n\t" : "=r"(value) : "r"(p));
  asm volatile("DSB ISH");
  asm volatile("ISB");
}

// ---------------------------------------------------------------------------
#define s_mfence "DSB ISH\n"
void mfence() { asm volatile(s_mfence); }

#define s_ifence "ISB\n"
void ifence() { asm volatile(s_ifence); }

// ---------------------------------------------------------------------------
#define s_nospec "DSB SY\nISB\n"
void nospec() { asm volatile(s_nospec); }


#elif defined(__riscv)

static inline __attribute__((always_inline)) void maccess(void* addr) {
    asm volatile("lb zero, 0(%0)" ::"r"(addr));
}
#define s_mfence "fence rw,rw\n"
static inline __attribute__((always_inline)) void mfence() {
    asm volatile(s_mfence);
}
#define s_ifence "fence.i\n"
static inline __attribute__((always_inline)) void ifence() {
    asm volatile(s_ifence);
}
#if defined(THEAD)
    #define s_flush \
        ".option push\n" \
        ".option arch, +xtheadcmo\n" \
        /* 0278800b th.dcache.civa a7 */ \
        "th.dcache.civa %0\n" \
        ".option pop\n"
#else
    #define s_flush \
        ".option push\n" \
        ".option arch, +zicbom\n" \
        "cbo.flush 0(%0)\n" \
        ".option pop\n"
#endif
static inline __attribute__((always_inline)) void flush(void *addr)
{
    asm volatile (s_flush :: "r"(addr));
}

#if defined(THEAD)
#define s_iflush \
        ".option push\n" \
        ".option arch, +xtheadcmo\n" \
        /* 0308800b th.icache.iva a7 */ \
        "th.icache.iva %0\n" \
        ".option pop\n"
static inline __attribute__((always_inline)) void iflush(void* addr) {
    asm volatile(s_iflush :: "r"(addr));
}
#define s_nospec \
    ".option push\n" \
    ".option arch, +xtheadsync\n" \
    /* 0190000b th.sync.s */ \
    "th.sync.s\n" \
    ".option pop\n"
static inline __attribute__((always_inline)) void nospec() {
    asm volatile(s_nospec);
}
#else
#define s_iflush "no iflush on general riscv64"
#define s_nospec "no nospec on general riscv64"
static inline __attribute__((always_inline,warning(s_iflush))) void iflush(void* addr) {}
static inline __attribute__((always_inline,warning(s_nospec))) void nospec() {}
#endif

static inline __attribute__((always_inline)) uint64_t rdtsc() {
  uint64_t val;
  mfence();
#if defined(RDCYCLE)
  asm volatile ("rdcycle %0" : "=r"(val));
#else
  asm volatile ("rdtime %0" : "=r"(val));
  val*=100;
  /* struct timespec t1; */
  /* clock_gettime(CLOCK_MONOTONIC, &t1); */
  /* val = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec; */
#endif
  mfence();
  return val;
}

/* static inline uint64_t rdtsc() { */
/*   uint64_t val; */
/*   asm volatile("fence rw,rw" : : : "memory"); */
/*   asm volatile ("rdcycle %0" : "=r"(val)); */
/*   asm volatile("fence rw,rw" : : : "memory"); */
/*   return val; */
/* } */

//Implementation just for legacy 
// ---------------------------------------------------------------------------
static inline uint64_t __rdtsc_begin() {
  return rdtsc();
}

//Implementation just for legacy 
// ---------------------------------------------------------------------------
static inline uint64_t __rdtsc_end() {
  return rdtsc(); 
}

#elif defined(__loongarch64)

#include "../../kernel_modules/loongarch64_privileged_module/module/loongarch64_privileged.h"

#define maccess(p) asm volatile("ld.b $r0, %0, 0"::"r"(p));

#define s_mfence "dbar 0\n"
#define mfence() asm volatile(s_mfence);
#define s_nospec "ibar 0\n"
static inline __attribute__((always_inline)) void nospec() {
    asm volatile(s_nospec);
}

static uint64_t rdtsc() {
    uint64_t val;
    mfence();
    nospec();
    asm volatile ("rdtime.d %0, $r0" : "=r"(val));
    val *= 100;
    /* struct timespec t1; */
    /* clock_gettime(CLOCK_MONOTONIC, &t1); */
    /* val = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec; */
    nospec();
    mfence();
    return val;
}

/* #define PRIV_IFLUSH */
/* #define PRIV_FLUSH */

#if defined(PRIV_FLUSH) || defined(PRIV_IFLUSH)
#define PRIV
static int loongarch64_privileged_fd = -1;
#else
#define EVICTION
#endif

#ifdef EVICTION
char* eviction_dummy;
const int eviction_dummy_size = 2<<20;
#endif

#if !defined(PRIV_FLUSH)
/* static __attribute__((aligned(4096))) char eviction_dummy[2<<20]; */
/* static void flush_cache_last_level() */
/* { */
/*     int ways = 4; */
/*     int sets = 256; */
/*     int linesz = 64; */

/*     // https://patchew.org/linux/20240507074357.2156083-1-lijun01@kylinos.cn/ */
/*     uint64_t addr = (uint64_t)eviction_dummy; */
/*     for (int i = 0; i < (ways * 3); i++) { */
/*         for (int j = 0; j < (sets); j++) { */
/*             *(volatile uint32_t *)addr; */
/*             addr += linesz; */
/*         } */
/*     } */
/* } */

static void flush(void* ptr) {
    // works in place
    const size_t ways = 4;
    const size_t size = 64<<10;
    const size_t stride = size / ways;
    // In-place
    /* volatile uint8_t *p = (volatile uint8_t *)((uintptr_t)ptr & ~(uintptr_t)63);*/
    // Out-of-place
    volatile uint8_t *p = (volatile uint8_t *)eviction_dummy + (size - ((uintptr_t)eviction_dummy % size)) + (((uintptr_t)ptr & (size-1)) & ~(uintptr_t)63);
    /* for (int i = 1; i <= ways; i++) { */
    for (int i = 0; i < ways; i++) {
        /* printf("ptr=%p, eviction_dummy=%p, aligned=%p, p=%p\n", ptr, eviction_dummy, eviction_dummy + (size - ((uintptr_t)eviction_dummy % size)), p+i*stride); */
        maccess(p+i*stride);
    }
    // Very important
    mfence();
}
#else
static void flush(void* p) {
    assert(loongarch64_privileged_fd != -1);
    int ret = ioctl(loongarch64_privileged_fd, MODULE_IOCTL_CMD_FLUSH, p);
    assert(ret == 0);
}
#endif

#if !defined(PRIV_IFLUSH)

__attribute__((always_inline)) static inline int icache_prime(uint32_t* inst) {
    return ((int (*volatile)(void))inst)();
}

static void iflush(void* ptr) {
    // works in place
    /* const size_t ways = 8; */
    /* const size_t size = (64<<10)*4; */
    /* const size_t ways = 8; */
    /* const size_t size = (64<<10); */
    const size_t ways = 32;
    const size_t size = (64<<10);
    const size_t stride = size / ways;
    /* printf("%d\n", stride); */
    /* volatile uint8_t *ptr = (volatile uint8_t *)((uintptr_t)p & ~(uintptr_t)63); */
    /* volatile uint8_t *ptr = (volatile uint8_t *)eviction_dummy + (size - ((uintptr_t)eviction_dummy % size)) + (((uintptr_t)p % size) & ~(uintptr_t)63); */
    volatile uint8_t *p = (volatile uint8_t *)eviction_dummy + (size - ((uintptr_t)eviction_dummy % size)) + (((uintptr_t)ptr & (size-1)) & ~(uintptr_t)63);
    for (int i = 1; i <= 32; i++) {
    /* for (int i = 0; i < ways; i++) { */
        /* printf("ptr=%p, eviction_dummy=%p, aligned=%p, p=%p\n", ptr, eviction_dummy, eviction_dummy + (size - ((uintptr_t)eviction_dummy % size)), p+i*stride); */
        mfence();
        nospec();
        /* test_icache_prime((uint32_t*)(p+i*stride)); */
        icache_prime((uint32_t*)p+i*stride);
        mfence();
        nospec();
    }

    mfence();
    nospec();
}
#else
static void iflush(void* p) {
    assert(loongarch64_privileged_fd != -1);
    int ret = ioctl(loongarch64_privileged_fd, MODULE_IOCTL_CMD_IFLUSH, p);
    assert(ret == 0);
}
#endif

#define _DEFINE_ASM_BLOB(name, code) \
    extern uint32_t name; \
    void name##_func(void) { \
        asm volatile( \
            ".global " #name "\n" \
            #name ":\n" \
            "    " code "\n" \
        ); \
    }
_DEFINE_ASM_BLOB(_ret, "jirl $r0, $r1, 0");
static void init_cacheutils_loongarch64() {
#ifdef PRIV
    // open the module
    loongarch64_privileged_fd = open(MODULE_DEVICE_PATH, O_RDONLY);
    if (loongarch64_privileged_fd < 0) {
        fprintf(stderr, "Error: Could not open module: %s\n", MODULE_DEVICE_PATH);
        exit(EXIT_FAILURE);
    }
#endif

#ifdef EVICTION
    eviction_dummy = (char*)mmap(0, eviction_dummy_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    for (int i = 0; i < eviction_dummy_size/4; i++) {
        ((uint32_t*)eviction_dummy)[i] = _ret;
    }
    __builtin___clear_cache (eviction_dummy, eviction_dummy+eviction_dummy_size);
#endif
}

#endif

// ---------------------------------------------------------------------------
int flush_reload(void *ptr) {
  uint64_t start = 0, end = 0;

#if USE_RDTSC_BEGIN_END
  start = __rdtsc_begin();
#else
  start = rdtsc();
#endif
  maccess(ptr);
#if USE_RDTSC_BEGIN_END
  end = __rdtsc_end();
#else
  end = rdtsc();
#endif

  mfence();

  flush(ptr);

  if (end - start < CACHE_MISS) {
    return 1;
  }
  return 0;
}

// ---------------------------------------------------------------------------
int flush_reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

#if USE_RDTSC_BEGIN_END
  start = __rdtsc_begin();
#else
  start = rdtsc();
#endif
  maccess(ptr);
#if USE_RDTSC_BEGIN_END
  end = __rdtsc_end();
#else
  end = rdtsc();
#endif

  mfence();

  flush(ptr);

  return (int)(end - start);
}

// ---------------------------------------------------------------------------
int reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

#if USE_RDTSC_BEGIN_END
  start = __rdtsc_begin();
#else
  start = rdtsc();
#endif
  maccess(ptr);
#if USE_RDTSC_BEGIN_END
  end = __rdtsc_end();
#else
  end = rdtsc();
#endif

  mfence();

  return (int)(end - start);
}


// ---------------------------------------------------------------------------
size_t detect_flush_reload_threshold() {
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

  return (flush_reload_time + reload_time * 2) / 3;
}

// ---------------------------------------------------------------------------
void maccess_speculative(void* ptr) {
    int i;
    size_t dummy = 0;
    void* addr;

    for(i = 0; i < 50; i++) {
        size_t c = ((i * 167) + 13) & 1;
        addr = (void*)(((size_t)&dummy) * c + ((size_t)ptr) * (1 - c));
        flush(&c);
        mfence();
        if(c / 0.5 > 1.1) maccess(addr);
    }
}


// ---------------------------------------------------------------------------
static jmp_buf trycatch_buf;

// ---------------------------------------------------------------------------
void unblock_signal(int signum __attribute__((__unused__))) {
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, signum);
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

// ---------------------------------------------------------------------------
void trycatch_segfault_handler(int signum) {
  (void)signum;
  unblock_signal(SIGSEGV);
  unblock_signal(SIGFPE);
  longjmp(trycatch_buf, 1);
}

// ---------------------------------------------------------------------------
int try_start() {
#if defined(__i386__) || defined(__x86_64__)
    if(has_tsx()) {
        unsigned status;
        // tsx begin
        asm volatile(".byte 0xc7,0xf8,0x00,0x00,0x00,0x00"
                 : "=a"(status)
                 : "a"(-1UL)
                 : "memory");
        return status == (~0u);
    } else 
#endif
    {
        signal(SIGSEGV, trycatch_segfault_handler); 
        signal(SIGFPE, trycatch_segfault_handler); 
        return !setjmp(trycatch_buf);
    }
}

// ---------------------------------------------------------------------------
void try_end() {
#if defined(__i386__) || defined(__x86_64__)
    if(!has_tsx()) 
#endif
    {
        signal(SIGSEGV, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
    }
}

// ---------------------------------------------------------------------------
void try_abort() {
#if defined(__i386__) || defined(__x86_64__)
    if(has_tsx()) {
        asm volatile(".byte 0x0f; .byte 0x01; .byte 0xd5" ::: "memory");
    } else 
#endif
    {
        maccess(0);
    }
}

// ---------------------------------------------------------------------------
size_t get_physical_address(size_t vaddr) {
    int fd = open("/proc/self/pagemap", O_RDONLY);
    uint64_t virtual_addr = (uint64_t)vaddr;
    size_t value = 0;
    off_t offset = (virtual_addr / 4096) * sizeof(value);
    int got = pread(fd, &value, sizeof(value), offset);
    if(got != sizeof(value)) {
        return 0;
    }
    close(fd);
    return (value << 12) | ((size_t)vaddr & 0xFFFULL);
}

#endif
