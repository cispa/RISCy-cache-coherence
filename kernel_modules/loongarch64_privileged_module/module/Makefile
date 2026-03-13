obj-m += loongarch64_privileged.o
ccflags-y += -Wno-unused-result
insert: all
	sudo rmmod loongarch64_privileged || true
	sudo insmod loongarch64_privileged.ko
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

