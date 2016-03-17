# compile on linux

* run "make -f make_index_tools" to generate demo program  
  
* run "make -f make_index_server" to generate demo program with libevent, you master install libevent first


# compile on windows

* run vs2008 IDE, open win_proj/index_tools.sln to generate demo program
  
* run unit_test/index_test.sln to generate unit test program
  

# demo program with libevent usage

1. run index_server
  
2. run "telnet 9999" command to connect to the index server
  
3. input the demo program command like the flowing section
  
# demo program usage example

|description|command example|
|-----------|---------------|
|create index named i       | create -i i |
|create object with objid 1 | create -i i -o 1|
|insert kv in object 1      | insert -i i -o 1 -k 1 -v abcdjkjkj |
|dump all kv in object 1    | dump   -i i -o 1 |
|remove kv in object 1      | remove -i i -o 1 -k 1 |



