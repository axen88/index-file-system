
CC = gcc
CXX = g++
AR = ar
LD = ld

PUBLIC_DIR := .

CFLAGS = -I. -Wall -g -Wno-unused  -D__EN_FILE_IF__

LIB_OBJS = $(PUBLIC_DIR)/avl.o $(PUBLIC_DIR)/os_cmd_ui.o \
	    $(PUBLIC_DIR)/os_log.o  $(PUBLIC_DIR)/os_utils.o $(PUBLIC_DIR)/os_file_if.o \
	    
	    

PUBLIC_LIB = libpublic.a

TARGET_ALL = $(PUBLIC_LIB)

all: $(TARGET_ALL)

%o:%c
	$(CC) -c $(CFLAGS) $< -o $@

%o:%cpp
	$(CXX) -c $(CFLAGS) $< -o $@

$(PUBLIC_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^
	
clean:
	rm -f $(LIB_OBJS) $(TARGET_ALL)
