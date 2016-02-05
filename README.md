# index-file-system
file/object/kv system using kv(index)


创建名为i的索引         create -i i

创建名为o的对象         create -i i -o o

创建名为a的属性         create -i i -o o -a a

在属性a下插入1000个key  insert -i i -o o -a a -k 1000

查看属性a下的所有key    dump   -i i -o o -a a

删除属性a下的1000个key  remove -i i -o o -a a -k 1000





