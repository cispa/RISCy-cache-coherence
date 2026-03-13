# LoongArch64 Privileged Module

This helper provides a Linux kernel module for Loongson systems to expose privileged cache-maintenance operations needed on LoongArch.

## Files

- [`module/`](./module/) - kernel module source and module build helpers
- [`example.c`](./example.c) - minimal userspace example using the module device/ioctl interface

## Usage

Build and insert the kernel module by following [`module/Makefile`](./module/Makefile):

```sh
make -C module
make -C module insert
```

Remove the module when done:

```sh
sudo rmmod loongarch64_privileged
```

If needed, see `example.c` for a minimal userspace interaction with the module.
