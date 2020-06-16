#include <Mira.hpp>

int main(int p_Argc, char* p_Argv[])
{
    auto s_Framework = Mira::Framework::GetFramework();
    
    s_Framework->Initialize();

    for (;;)
        __asm__("nop");
        
    return 0;
}