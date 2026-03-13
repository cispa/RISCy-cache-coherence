# Artifact Evaluation

This document defines the artifact-evaluation scope for IEEE S&P 2026.

> [!NOTE]
> The Nix setup which provides a reproducible building environment for the entire artifact is not required for this evaluation.
> All compilations can be done on the machine itself.

## Claim 1

The ICSC side channel leaks D-cache state without timers.

- Machine: lab88 (Qualcomm Oryon)
- Experiment: [AArch64 Oryon ICSC Library](./experiments/libs/aarch64/oryon/README.md)
- Expected outcome: repeated measurements show `should be low` close to `0%` and `should be high` close to `100%`.

### Claim 1 Instructions

Copy the sources to the remote machine, compile and run:

```sh
rsync -r experiments/libs/ lab88:libs
ssh lab88
# On remote:
cd libs/aarch64/oryon
timeout 2s make run
```

Expected output:

```text
Pinning to core 5...
should be low:    0.01
should be high: 100.00
should be low:    0.02
should be high:  99.80
---------------
```

## Claim 2

ICSC can be used to mount attacks, including practical timer-free Spectral attacks.

- Machine: lab88 (Qualcomm Oryon)
- Experiment: [Spectral case study](./experiments/spectral/README.md).
- Expected outcome: successful secret leakage without relying on high-resolution timers.

### Claim 2 Instructions

Copy the sources to the remote machine, compile and run:

```sh
rsync -r experiments/libs/ experiments/spectral/ lab88:
ssh lab88
# On remote:
cd spectral
make poc_oryon
./poc_oryon
```

Expected output:

```text
[ ]  SECRET

[>] Done
Leaked 6 bytes in 4447 us -> 1349 bytes/s
```
