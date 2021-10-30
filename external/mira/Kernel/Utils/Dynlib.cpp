// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

extern "C"
{
    #include <Utils/_Syscall.hpp>
};

#include <Utils/New.hpp>

#include "Dynlib.hpp"

using namespace Mira::Utils;

int64_t Dynlib::LoadPrx(const char* p_PrxPath, int* p_OutModuleId)
{
    return (int64_t)syscall4(594, reinterpret_cast<void*>(const_cast<char*>(p_PrxPath)), 0, p_OutModuleId, 0);
}

int64_t Dynlib::UnloadPrx(int64_t p_PrxId)
{
    return (int64_t)syscall1(595, (void*)p_PrxId);
}

int64_t Dynlib::Dlsym(int64_t p_PrxId, const char* p_FunctionName, void* p_DestinationFunctionOffset)
{
    return (int64_t)syscall3(591, (void*)p_PrxId, (void*)p_FunctionName, p_DestinationFunctionOffset);
}