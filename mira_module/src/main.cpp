#include <cstdio>
#include <cstdint>

extern "C" int testLibraryFunction()
{
    printf("This is a print from a test library function!");

    for (;;)
        printf("This is a print from a test library function!");

    return 0x1337;
}

extern "C" bool trainer_load()
{
    printf("trainer_load()\n");
    return true;
}

extern "C" bool trainer_unload()
{
    printf("trainer_unload()\n");
    return true;
}