# index-file-system
file/object/kv system using kv(index)


create index named i      :         create -i i

create object with objid 1:         create -i i -o 1

insert kv in object 1     :         insert -i i -o 1 -k 1 -v abcdjkjkj

dump all kv in object 1   :         dump   -i i -o 1

remove kv in object 1     :         remove -i i -o 1 -k 1





