# Makefile for wxx_guard
obj-m += wxx_guard_kprobe.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean


test: test.c
	gcc -O2 test.c -o test

log: wxx_guard_kprobe.ko
	sudo insmod ./wxx_guard_kprobe.ko action=0

enforce: wxx_guard_kprobe.ko
	sudo insmod ./wxx_guard_kprobe.ko action=1

unload:
	sudo rmmod wxx_guard_kprobe
