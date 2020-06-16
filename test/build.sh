cd src;
clang protobuf-c.c rpc.pb-c.c filemanager.pb-c.c main.c -o test.elf -g -static