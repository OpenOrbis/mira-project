#include <Mira.hpp>
#include <string>

int main(int p_Argc, char* p_Argv[])
{
    std::string s_Buffer = "rekt";
    if (s_Buffer.length())
        s_Buffer = "dsadas";
    
    auto s_Framework = Mira::Framework::GetFramework();
    
    s_Framework->Initialize();

    for (;;)
        __asm__("nop");
        
    return 0;
}