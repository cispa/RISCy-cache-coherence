#ifndef B1_CONFIG_H
#define B1_CONFIG_H

struct b1_config {
    int mfence;
    int ifence;
    int jump;
    int nops;
};

static inline struct b1_config get_b1_config(void) {
    struct b1_config cfg;

    // Default B1 configuration used in the artifact.
    cfg.mfence = 1;
    cfg.ifence = 0;
    cfg.jump = 1;

#ifdef MFENCE_8
    // X60, C908, Kunpeng Pro: mfence + 8 nops
    cfg.nops = 8;
#elif defined(MFENCE_64)
    // A78: mfence + 64 nops
    cfg.nops = 64;
#else
    // all others: mfence + 0 nops
    cfg.nops = 0;
#endif

    // not working at all: A72, C906

    return cfg;
}

#endif
