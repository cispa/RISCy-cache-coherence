#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>

#include "m1_uarch_module.h"

#define mfence() asm volatile("DMB SY\nISB\n");

static int module_fd;

static void enable_flushing() {
    ioctl(module_fd, CMD_SET_FLUSHING_ENABLED, 1);
}

static void enable_counter() {
    ioctl(module_fd, CMD_SET_COUNTER_ENABLED, 1);
}

static inline __attribute__((always_inline)) void flush(void* addr) {
    asm volatile("DC CIVAC, %0" :: "r" (addr));
}

static inline __attribute__((always_inline)) uint64_t read_counter(){
    uint64_t value;
    asm volatile("MRS %0, s3_2_c15_c0_00" : "=r" (value));
    return value;
}

static inline __attribute__((always_inline)) uint64_t probe(void* addr) {
    register uint64_t start, end;
    mfence();
    start = read_counter();
    mfence();
    asm volatile("ldr x0, [%0]" :: "r" (addr) : "x0");
    mfence();
    end = read_counter();
    mfence();
    return end - start;
}

int main(int argc, char** argv) {
    module_fd = open(M1_UARCH_MODULE_DEVICE_PATH, O_RDONLY);
    
    if(module_fd < 0) {
        fputs("unable to open module!\n", stderr);
        return -1;
    }
    
    enable_counter();
    
    char* blubb = malloc(4096);
    
    for(int i = 0; i < 100; i++)
        probe(blubb);
    
    printf("hit: %zu\n", probe(blubb));
    
    mfence();
    flush(blubb);
    mfence();
    printf("miss (before enabling flushing): %zu\n", probe(blubb));
    
    
    enable_flushing();
    
    for(int i = 0; i < 100; i++)
        probe(blubb);
    mfence();
    flush(blubb);
    mfence();
    printf("miss (after enabling flushing): %zu\n", probe(blubb));   
}
