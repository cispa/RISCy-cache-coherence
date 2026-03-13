#define _GNU_SOURCE

#include <stdio.h>
#include <math.h>
#include <float.h>
#include "../libs/lib_probe_icache.h"

FILE *fp_csv;

float test_icache_test_with_nops(int nops, int mfence, int jump, int d_cached, int itlb_cached, int prime_itlb) {
    uint32_t* exec = test_icache_prepare_with_nops(nops, mfence, jump);

    if (jump) {
        exec = 0;
    }

    char testname[64];
    snprintf(testname, sizeof(testname), "test_icache");
    if (mfence) {
        strncat(testname, "_mfence", sizeof(testname) - strlen(testname) - 1);
    }
    if (jump) {
        strncat(testname, "_jump", sizeof(testname) - strlen(testname) - 1);
    }
    snprintf(testname + strlen(testname), sizeof(testname) - strlen(testname), "_%d", nops);

    int fp, fn, tp, tn;
    test_primitive(template, &fp, &fn, &tp, &tn, exec, d_cached, itlb_cached, prime_itlb);

    double denom = sqrt(
        (double)(tp + fp) * (double)(tp + fn) *
        (double)(tn + fp) * (double)(tn + fn));
    double mcc = (denom > 0.0)
            ? (((double)tp * (double)tn - (double)fp * (double)fn) / denom)
            : 0.0;

    printf("%40s: %s mcc=%5.2f fp=%5d fn=%5d tp=%5d tn=%5d\n", testname, (mcc > 0.8) ? "y" : " ", mcc, fp, fn, tp, tn);

    fprintf(fp_csv, "%d,%d,%d,%d,%d,%d,%d,%f\n", nops, mfence, jump, fp, fn, tp, tn, mcc);

    return mcc;
}

int main(int argc, char* argv[]) {
    int cpu = -1;
    if (argc > 1) {
        cpu = atoi(argv[1]);
    }
    init_arch_sc(&cpu);

    char filename[128];
    snprintf(filename, sizeof(filename), "b1_icache_oracle_prevalence_results_%d.csv", cpu);
    printf("Writing to %s\n", filename);
    fp_csv = fopen(filename, "w");
    assert(fp_csv);
    fprintf(fp_csv, "nops,mfence,jump,fp,fn,tp,tn,mcc\n");

    init_lib_probe_icache();

    n = 100000;

    for (int jump = 1; jump >= 0; jump--) {
        for (int mfence = 1; mfence >= 0; mfence--) {
            test_icache_test_with_nops(0, mfence, jump, 0, 0, 0);
            for (int exp = 0; exp <= 17; exp++) {
                int nops = 1<<exp;
                test_icache_test_with_nops(nops, mfence, jump, 0, 0, 0);
            }
            printf("\n");
        }
    }
}
