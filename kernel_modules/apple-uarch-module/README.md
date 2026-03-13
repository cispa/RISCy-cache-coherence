# apple-uarch-module

This helper provides a Linux kernel module for Apple Silicon (Asahi Linux) that enables userspace microarchitectural features used by the artifact.

## Features

- enables userspace cycle-counter access (`s3_2_c15_c0_00`)
- enables userspace cache flush (`DC CIVAC`)

## Usage

Build the module:

```sh
make
```

For Apple M2, build with:

```sh
make m2
```

Insert the module:

```sh
sudo insmod m1_uarch_module.ko
```

Enable flushing/counter via ioctl from userspace as needed (see `test.c` for an example).

To disable these features, reboot the system.
