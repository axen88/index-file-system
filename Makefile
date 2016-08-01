
CC = gcc
CXX = g++
AR = ar
LD = ld

PUBLIC_DIR := public
TOOLS_DIR  := tools
OBS_DIR    := object_system

CFLAGS = -Iinclude -Ipublic -Wall -g -Wno-unused  -D__EN_FILE_IF__

LIB_OBJS = $(OBS_DIR)/ofs_block_rw.o $(OBS_DIR)/ofs_btree.o $(OBS_DIR)/ofs_container_manager.o \
	    $(OBS_DIR)/ofs_metadata_cache.o $(OBS_DIR)/ofs_collate.o $(OBS_DIR)/ofs_extent_map.o \
	    $(OBS_DIR)/ofs_object_manager.o $(OBS_DIR)/ofs_log.o $(OBS_DIR)/ofs_space_manager.o \
	    $(PUBLIC_DIR)/avl.o $(PUBLIC_DIR)/os_cmd_ui.o \
	    $(PUBLIC_DIR)/os_log.o  $(PUBLIC_DIR)/os_utils.o $(PUBLIC_DIR)/os_file_if.o \

TOOLS_OBJS = $(TOOLS_DIR)/ofs_tools_dump.o $(TOOLS_DIR)/ofs_tools_debug.o \
	    $(TOOLS_DIR)/ofs_tools_list.o  $(TOOLS_DIR)/ofs_tools_if.o \
	    $(TOOLS_DIR)/ofs_tools_tree.o  $(TOOLS_DIR)/ofs_tools_performace.o 
	    
SERVER_OBJS = $(TOOLS_DIR)/ofs_server_main.o  
CLIENT_OBJS = $(TOOLS_DIR)/ofs_client_main.o
UI_OBJS     = $(TOOLS_DIR)/ofs_tools_main.o

OFS_LIB = libofs.a
OFS_SERVER = ofs_server
OFS_CLIENT = ofs_client
OFS_UI     = ofs_ui

TARGET_ALL = $(OFS_LIB) $(OFS_SERVER) $(OFS_CLIENT) $(OFS_UI)

all: $(TARGET_ALL)

%o:%c
	$(CC) -c $(CFLAGS) $< -o $@

%o:%cpp
	$(CXX) -c $(CFLAGS) $< -o $@

$(OFS_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(OFS_SERVER): $(LIB_OBJS) $(SERVER_OBJS) $(TOOLS_OBJS)
	$(CC) -o $@ $^ -lpthread -levent
	
$(OFS_CLIENT): $(CLIENT_OBJS)
	$(CC) -o $@ $^ -levent

$(OFS_UI): $(LIB_OBJS) $(UI_OBJS) $(TOOLS_OBJS)
	$(CC) -o $@ $^ -lpthread
	
clean:
	rm -f $(LIB_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) $(UI_OBJS) $(TOOLS_OBJS) $(TARGET_ALL)
