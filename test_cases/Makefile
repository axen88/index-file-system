CC = gcc
CXX = g++
AR = ar
LD = ld

CFLAGS = -g -Wall -I./ -I../public -I../include -I../tx -I../allocator -D__EN_FILE_IF__

POBJS = 

TEST_SUITE_MAIN = test_suite_main

PUBLIC_DIR = ../public

PUBLIC_SRC = $(PUBLIC_DIR)/log.c $(PUBLIC_DIR)/file_if.c $(PUBLIC_DIR)/utils.c $(PUBLIC_DIR)/hashtab.c
TX_CACHE_SRC = test_bitmap.c ../allocator/bitmap.c ../tx/tx_cache.c $(PUBLIC_SRC)

TARGET_ALL = $(TEST_SUITE_MAIN)

all: $(TARGET_ALL)

$(TEST_SUITE_MAIN):
	gcc -o $@  $(CFLAGS)  $(TX_CACHE_SRC) test_suite_main.c -lpthread -lcunit -static
	
	
clean:
	rm -f *.o ../*.o $(TARGET_ALL)
