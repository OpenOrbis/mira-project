#include <cstdio>
#include <cstdint>

int testLibraryFunction()
{
    printf("This is a print from a test library function!");

    for (;;)
        printf("This is a print from a test library function!");

    return 0x1337;
}

extern "C" int32_t trainer_load()
{

}

extern "C" int32_t trainer_unload()
{
    
}