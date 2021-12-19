#pragma once
#include <Utils/Stack.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Utils
    {
        class Dynlib
        {
        public:
            static int64_t LoadPrx(const char* p_PrxPath, int* p_OutModuleId);
            static int64_t UnloadPrx(int64_t p_PrxId);
            static int64_t Dlsym(int64_t p_PrxId, const char* p_FunctionName, void *p_DestinationFunctionOffset);
        };
    }
}