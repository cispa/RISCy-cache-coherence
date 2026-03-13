#ifndef MEASURE_HELPERS_H
#define MEASURE_HELPERS_H

static inline int measure_exec_time(uint32_t* inst) {
    int begin = rdtsc();
    ((int (*volatile)(void))inst)();
    int end = rdtsc();
    return end - begin;
}

static inline int measure_gadget(uint32_t* inst) {
    return !call_template(template, inst, inst);
}

#endif
