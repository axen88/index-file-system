
CC = gcc
CXX = g++
AR = ar
LD = ld

PUBLIC_DIR := .

CFLAGS = -I. -Wall -g -Wno-unused -fPIC  -D__EN_FILE_IF__

LIB_OBJS = $(PUBLIC_DIR)/avl.o $(PUBLIC_DIR)/cmd_ui.o \
	    $(PUBLIC_DIR)/log.o  $(PUBLIC_DIR)/utils.o $(PUBLIC_DIR)/file_if.o \
	    
	    

PUBLIC_LIB = libpublic.a

PUBLIC_SO = libpublic.so

TARGET_ALL = $(PUBLIC_LIB) $(PUBLIC_SO)

all: $(TARGET_ALL)

%o:%c
	$(CC) -c $(CFLAGS) $< -o $@

%o:%cpp
	$(CXX) -c $(CFLAGS) $< -o $@

$(PUBLIC_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^
	
$(PUBLIC_SO): $(LIB_OBJS)
	$(LD) -shared -soname $@ -o $@ $^
	
clean:
	rm -f $(LIB_OBJS) $(TARGET_ALL)
