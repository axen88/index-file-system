This project comes from https://github.com/axenhook/index-file-system

# compile on linux

* run "make -f make_index_tools" to generate the demo program  
  
* run "make -f make_index_server" to generate the demo program with libevent, you must install libevent first


# compile on windows

* run vs2008 IDE, open win_proj/index_tools.sln to generate the demo program
  
* run vs2008 IDE, open unit_test/index_test.sln to generate the unit test program
  

# demo program with libevent usage(only support linux now)

1. run index_server
  
2. run "telnet IP_ADDRESS 9999" command to connect to the index server, if it is on the local machine, the IP_ADDRESS may be "127.0.0.1"  (to quit the telnet, please press "ctrl+]" keys, and then input "quit")
  
3. input the demo program command like the following section
  
# demo program usage example

|description|command example|
|-----------|---------------|
|create index named i       | create -i i |
|create object with objid 1 | create -i i -o 1|
|insert kv in object 1      | insert -i i -o 1 -k 1 -v abcdjkjkj |
|dump all kv in object 1    | dump   -i i -o 1 |
|remove kv in object 1      | remove -i i -o 1 -k 1 |

# feature
1. support multi fs
2. support variant length key or value
3. support NULL value or zero length value

# TODO
1. using btree manager the space instead of bitmap
2. support snapshot
3. the data on disk is consistent at any time
4. object read/write
5. big value
6. file system AI
7. ...


