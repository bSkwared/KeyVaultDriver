ifneq ($(KERNELRELEASE),)
# call from kernel build system

kv_mod-objs := kv_kernel_mod.o

obj-m	:= kv_kernel_mod.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *.ko *.mod.c *.order *.symvers

