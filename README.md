
# features
## key-value system
1. support key-value operations like insert/remove/search etc.
4. support variant length key or value
5. support NULL value or zero length value
## object system
1. support object read/write if the object is data stream type
2. support object size to 2^64-1
## file system
1. support all kinds of file system operations like file/directory/xattr create/delete/read/write etc.

# TODO
1. using btree manager the space instead of bitmap
2. support both b tree and b+ tree
3. support snapshot management
4. support the data on disk is consistent at any time
5. support object read/write
6. support big value
7. support file system operation

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


