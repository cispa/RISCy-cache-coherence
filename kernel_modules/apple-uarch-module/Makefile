obj-m += m1_uarch_module.o
ccflags-y += -Wno-unused-result

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

m2:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) ccflags-y="$(ccflags-y) -D APPLE_M2" modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
