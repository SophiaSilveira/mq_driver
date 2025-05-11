
obj-m := mq_driver.o
mq_driver-objs := main_driver.o list_driver.o
BUILDROOT_DIR := ../..
KDIR := $(BUILDROOT_DIR)/output/build/linux-custom
COMPILER := $(BUILDROOT_DIR)/output/host/bin/i686-buildroot-linux-gnu-gcc

all:
	$(MAKE) -C $(KDIR) M=$$PWD
	$(MAKE) -C $(KDIR) M=$$PWD modules_install INSTALL_MOD_PATH=../../target
	$(COMPILER) -o test_mq_driver test_mq_driver.c
	cp test_mq_driver $(BUILDROOT_DIR)/output/target/bin
	
	
clean:
	rm -f *.o *.ko .*.cmd
	rm -f modules.order
	rm -f Module.symvers
	rm -f mq_driver.mod.c
	rm -f test_mq_driver