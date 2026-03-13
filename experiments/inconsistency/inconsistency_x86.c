#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

typedef int (*stub_fn_t)(void);

// Write "mov eax, imm32 ; ret"
static void write_stub(uint8_t *buf, uint32_t imm) {
    buf[0] = 0xB8;               // MOV EAX, imm32
    *(uint32_t *)(buf + 1) = imm;
    buf[5] = 0xC3;               // RET
}

int main(int argc, char **argv) {
    size_t pagesz = sysconf(_SC_PAGESIZE);
    uint8_t *code = mmap(NULL, pagesz, PROT_READ|PROT_WRITE|PROT_EXEC,
                         MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    long stale = 0;
    for (int i = 0; i < 10000000; i++) {
        write_stub(code, 1);
        stub_fn_t fn = (stub_fn_t)code;

        (void)fn();

        *(uint32_t *)(code + 1) = 0;

        asm volatile("lfence");
        /* asm volatile("mfence"); */

        int r = fn();
        if (r) {
            exit(42);
        }
    }

    printf("no instruction/data-cache inconsistency observed\n");

    return 0;
}
