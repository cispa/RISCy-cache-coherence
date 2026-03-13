// wx_test.c
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(){
    size_t len = 4096;
    void *p = mmap(NULL, len, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (p==MAP_FAILED){perror("mmap"); return 1;}
    printf("mmap succeeded at %p\n", p);
    *(char*)p = 'X';

    if(mprotect(p, len, PROT_READ|PROT_WRITE|PROT_EXEC) != 0) {
        perror("mprotect");
    } else {
        printf("mprotect succeeded\n");
    }
    return 0;
}
