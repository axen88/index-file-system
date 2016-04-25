
# features
1. all data structure is btree or b+tree 
2. kv/object/file system are all in one system, sharing the same disk space and layout
3. the data on disk is consistent at any time
4. support snapshot management

## support key-value operations
1. support kv operations like insert/remove/search etc.
2. support variant length key or value
3. support NULL value or zero length value
4. support big value (TODO)
5. support both b tree and b+ tree

## support object system operations (TODO)
1. support object operations like read/write etc
2. object size can be up to 2^64-1 bytes

## support file system operations (TODO)
1. support all kinds of file system operations like file/directory/xattr create/delete/read/write etc.
2. support linux fs posix API
3. support fuse
4. support docking other service


# compile on linux

* run "make" to generate the ofs_tools/ofs_server/ofs_client program, you must install libevent package first


# compile on windows (vs2008 IDE is necessary)

* open win_proj/ofs_tools.sln to generate the demo program ofs_tools.exe
  
* open unit_test/ofs_test.sln to generate the unit test program ofs_test.exe
  

# demo program with libevent usage (only support linux now)

1. run ofs_server
  
2. run "telnet IP_ADDRESS 9999" command to connect to the ofs_server(to quit the telnet, please press "ctrl+]" keys, and then input "quit")

3. or run ofs_client to connect to the ofs_server
  
4. input the demo program command like the following section
  
# demo program(ofs_tools) usage example

|description|command example|
|-----------|---------------|
|create container named ct0       | create -ct ct0 |
|create object with objid 300 | create -ct ct0 -o 300|
|insert kv in object 300      | insert -ct ct0 -o 300 -k axen -v abcdjkjkj |
|dump all kv in object 300    | dump   -ct ct0 -o 300 |
|remove kv in object 300      | remove -ct ct0 -o 300 -k axen |


This project comes from https://github.com/axenhook/object-file-system


