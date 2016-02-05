WORK_PWD := $(PWD)
PUBLIC_DIR := ../public
UTILS_DIR := tools

PWD  := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build  

EXTRA_CFLAGS += -I$(WORK_PWD) -I$(WORK_PWD)/../include
EXTRA_CFLAGS += -W -Wall -g -Wno-unused -D__KERNEL__ -D__EN_FILE_IF__

obj-m := index.o
index-y := index_block_manager.o index_block_rw.o index_bitmap.o index_tree_algorithm.o index_manager.o \
	index_cache.o  index_utils.o index_attr_manager.o index_attr_rw.o index_object_manager.o \
	$(UTILS_DIR)/index_tools_verify.o $(UTILS_DIR)/index_tools_dump.o $(UTILS_DIR)/index_tools_debug.o \
	$(UTILS_DIR)/index_tools_fixup.o $(UTILS_DIR)/index_tools_list.o $(UTILS_DIR)/index_tools_if.o \
	$(UTILS_DIR)/index_tools_test.o $(UTILS_DIR)/index_tools_evaluate.o \
	$(PUBLIC_DIR)/os_utils.o $(PUBLIC_DIR)/os_list_double.o \
	$(PUBLIC_DIR)/os_cmd_ui.o $(PUBLIC_DIR)/os_threads_group.o $(PUBLIC_DIR)/os_log.o \
	$(PUBLIC_DIR)/os_collate.o $(PUBLIC_DIR)/os_queue.o $(PUBLIC_DIR)/os_file_if.o $(PUBLIC_DIR)/avl.o 

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	-rm -f *.o .*.o.cmd
	-rm -f *.mod.c
	-rm -fr .tmp_versions
	
	$(CC) -o index_tools index_block_manager.c index_block_rw.c index_bitmap.c index_tree_algorithm.c index_manager.c \
	    index_cache.c index_utils.c index_attr_manager.c  index_attr_rw.c index_object_manager.c \
	    $(UTILS_DIR)/index_tools_verify.c  $(UTILS_DIR)/index_tools_dump.c \
	    $(UTILS_DIR)/index_tools_debug.c  $(UTILS_DIR)/index_tools_fixup.c \
	    $(UTILS_DIR)/index_tools_list.c  $(UTILS_DIR)/index_tools_if.c $(UTILS_DIR)/index_tools_main.c\
	    $(UTILS_DIR)/index_tools_test.c  $(UTILS_DIR)/index_tools_evaluate.c \
	    $(PUBLIC_DIR)/os_queue.c $(PUBLIC_DIR)/os_list_double.c $(PUBLIC_DIR)/avl.c \
	    $(PUBLIC_DIR)/os_log.c  $(PUBLIC_DIR)/os_utils.c $(PUBLIC_DIR)/os_file_if.c \
	    $(PUBLIC_DIR)/os_cmd_ui.c $(PUBLIC_DIR)/os_threads_group.c  $(PUBLIC_DIR)/os_collate.c \
	    -I. -I../include -W -Wall -g -Wno-unused -D__EN_FILE_IF__ -lpthread
	
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f ./index_tools
