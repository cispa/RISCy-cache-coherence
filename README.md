# Artifact – RISCy Cache Coherence: Timer-Free Architectural Cache Attacks via Instruction/Data Cache Incoherence

This repository contains the artifact for the paper *"RISCy Cache Coherence: Timer-Free Architectural Cache Attacks via Instruction/Data Cache Incoherence"*.
It is archived on [Zenodo](TODO).

> [!NOTE]
> This repository relies on [Git LFS](https://git-lfs.com/) to handle large files. Please install it **before** cloning the repository. If you cloned without having LFS installed or have other problems with LFS files run: `git lfs install && git lfs pull`.

## Abstract

This artifact accompanies "RISCy Cache Coherence: Timer-Free Architectural Cache Attacks via Instruction/Data Cache Incoherence" and provides the code and data for reproducing the paper's experiments.
It covers the instruction/data-cache inconsistency analysis, prevalence and performance evaluations of building blocks B1/B2 and ICSC covert channels, and the case studies (Spectral, AES T-tables, and shared-library user-input leakage).
The repository also includes helper tooling for cross-machine experiment orchestration, standalone table/figure generation, and software mitigation evaluation.

## Structure

The repository is organized into the following files/directories:

- [bin](./bin/): Shared tooling, including [`run-and-gather`](./bin/run-and-gather) for multi-machine execution.
- [experiments](./experiments/): All paper experiments and case studies.
- [kernel_modules](./kernel_modules/): Optional platform-specific kernel helpers.

For Artifact Evaluation instructions, see [ARTIFACT_EVALUATION.md](./ARTIFACT_EVALUATION.md).

## Set Up

The artifact uses [Nix](https://nixos.org/) to provide the required compilers, a LaTeX compiler, and python packages.
Nix is a declarative package manager, i.e., needed packages can be specified in a file, locked via a lock file and then loaded reproducibly.
All packages are declared in the top-level `flake.nix`.
Nix can be installed on a large range of Linux distributions.

1. Please install [Nix](https://nixos.org/download/).
   We recommend the multi-user installation if root privileges are available.

2. The artifacts further use an experimental Nix feature called Flakes.
   This feature needs to be enabled by running the following command:
   ``` sh
   mkdir -p ~/.config/nix && echo 'experimental-features = nix-command flakes' > ~/.config/nix/nix.conf
   ```

3. Quickly verify that Nix Flakes work by running:
   ``` sh
   nix run 'nixpkgs#hello'
   ```
   This should print "Hello, world!".

## Quick Start & Basic Tests

To enter a bash shell that has all the packages installed run the following command in the top-level directory:
``` sh
nix develop
```

> [!NOTE]
> Loading the environment can take some time, depending on the host architecture.
> This is because some of the packages have to be partially build from source.
> We recommend and support x86 machines, however others might work as well.

Please run this command in the top-level (this) directory to verify that a static (cross-)compiler for each architecture is available:
``` sh
which loongarch64-unknown-linux-musl-gcc && which aarch64-unknown-linux-musl-gcc && which riscv64-unknown-linux-musl-gcc
```

Expected output:
```
/nix/store/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-loongarch64-unknown-linux-musl-gcc-wrapper-14.3.0/bin/loongarch64-unknown-linux-musl-gcc
/nix/store/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-aarch64-unknown-linux-musl-gcc-wrapper-14.3.0/bin/aarch64-unknown-linux-musl-gcc
/nix/store/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-riscv64-unknown-linux-musl-gcc-wrapper-14.3.0/bin/riscv64-unknown-linux-musl-gcc
```

If you find any runtime or compilation errors it is likely that the Nix setup is somehow corrupted, as the artifact should provide all packages needed to run every experiment.
