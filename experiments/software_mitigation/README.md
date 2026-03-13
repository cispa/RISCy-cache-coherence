# Software Mitigation

This experiment implements the software mitigation based on blocking W+X mappings and permission changes (Section 7.1).

## Files

- `wxx_guard_kprobe.c` - kernel module implementing the mitigation
- `test.c` - simple user-space test program creating W+X mappings

## Usage

Build the kernel module:

```sh
make
```

Build the test program:

```sh
make test
```

Load in logging mode:

```sh
make log
```

Load in enforcement mode:

```sh
make enforce
```

Unload the module:

```sh
make unload
```

`log` records violations in the kernel log. `enforce` kills violating processes and logs the event.
