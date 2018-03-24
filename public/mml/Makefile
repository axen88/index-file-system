WORK_PWD := $(PWD)
PUBLIC_DIR := ../public

PWD  := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS += -I$(WORK_PWD)/ -I$(WORK_PWD)/../include
EXTRA_CFLAGS += -W -Wall -g -Wno-unused

obj-m = os_mml.o
os_mml-y += os_debug.o mml_kernel.o $(PUBLIC_DIR)/os_log.o $(PUBLIC_DIR)/os_utils.o $(PUBLIC_DIR)/os_file_if.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	$(CC) -o mml mml_user.c -Wall -g -I../include -I.
	
clean:
	rm -rf .tmp_versions *.ko *.o *.mod.c .*.cmd *.symvers

