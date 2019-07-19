#include "MiraLoader.hpp"

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Dynlib.hpp>

#include <sys/mman.h>
#include <sys/malloc.h>
#include <stdarg.h>

using namespace MiraLoader;
using namespace Mira::Utils;

Loader::Loader(uint8_t* p_ElfData, uint64_t p_ElfDataLength, bool p_IsInKernel) :
    m_Data(p_ElfData),
    m_DataLength(p_ElfDataLength),
    m_ElfSize(0),
    m_AllocatedData(nullptr),
    m_AllocatedDataSize(0),
    m_EntryPoint(nullptr),
    m_IsInKernel(p_IsInKernel)
{

}

Loader::~Loader()
{
    // Free all of the elf resources and terminate the process
}

uint64_t Loader::RoundUp(uint64_t p_Number, uint64_t p_Multiple)
{
    if (p_Multiple == 0)
		return p_Number;

	uint64_t s_Remainder = p_Number % p_Multiple;
	if (s_Remainder == 0)
		return p_Number;

	return p_Number + p_Multiple - s_Remainder;
}

int32_t Loader::Strcmp(const char* p_First, const char* p_Second)
{
    	while (*p_First == *p_Second++)
		if (*p_First++ == '\0')
			return (0);
	return (*(const unsigned char *)p_First - *(const unsigned char *)(p_Second - 1));
}

void Loader::Memcpy(void* p_Destination, void* p_Source, uint64_t p_Size)
{
    for (uint64_t i = 0; i < p_Size; ++i)
        ((uint8_t*)p_Destination)[i] = ((uint8_t*)p_Source)[i];
}

void Loader::Memset(void* p_Address, uint8_t p_Value, uint64_t p_Length)
{
    volatile uint8_t c = (uint8_t)p_Value;

    for (uint64_t i = 0; i < p_Length; ++i)
        *(((uint8_t*)p_Address) + i) = c;
}

void* Loader::Allocate(uint32_t p_Size)
{
    void* s_AllocationData = nullptr;

#ifdef _WIN32
    s_AllocationData = Win32Allocate(p_Size);
#else
    if (m_IsInKernel)
    {
        auto kmem_alloc = (vm_offset_t(*)(vm_map_t map, vm_size_t size))kdlsym(kmem_alloc);
		vm_map_t map = (vm_map_t)(*(uint64_t *)(kdlsym(kernel_map)));

		s_AllocationData = (void*)kmem_alloc(map, p_Size);
    }
    else
    {
        s_AllocationData = _mmap(NULL, p_Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    }    
#endif

    if (s_AllocationData != nullptr)
        Memset(s_AllocationData, 0, p_Size);

    return s_AllocationData;
}

#ifdef _WIN32
void* Loader::Win32Allocate(uint32_t p_Size)
{
	return malloc(p_Size);
}
#endif

bool Loader::SetProtection(void* p_Address, uint64_t p_Size, int32_t p_Protection)
{
    if (p_Address == nullptr)
        return false;
    
#ifdef _WIN32
    DWORD oldProtection = 0;
	if (!VirtualProtect(data, dataSize, protection, &oldProtection))
		return false;
#else
    if (m_IsInKernel)
    {
        // TODO: pmap_protect
    }
    else
    {
        if ((int64_t)syscall3(SYS_MPROTECT, p_Address, (void*)p_Size, (void*)(int64_t)p_Protection) < 0)
            return false;     
    }
#endif

    return true;
}

void Loader::WriteLog(const char* p_Function, int32_t p_Line, const char* p_Format, ...)
{
    if (m_IsInKernel)
    {
        char s_Buffer[256];
        char s_FinalBuffer[256];

        auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
        auto vsnprintf = (int(*)(char *str, size_t size, const char *format, va_list ap))kdlsym(vsnprintf);
        auto printf = (void(*)(char *format, ...))kdlsym(printf);
        //auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
        //auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

        //_mtx_lock_flags(&m_Mutex, 0, __FILE__, __LINE__);

        // Zero out the buffer
        Memset(s_Buffer, 0, sizeof(s_Buffer));
        Memset(s_FinalBuffer, 0, sizeof(s_FinalBuffer));

        va_list args;
        va_start(args, p_Format);
        vsnprintf(s_Buffer, sizeof(s_Buffer), p_Format, args);
        va_end(args);

        snprintf(s_FinalBuffer, sizeof(s_FinalBuffer), "[%s] %s:%d : %s\n", "DBG", p_Function, p_Line, s_Buffer);
	    printf(s_FinalBuffer);
    }
    else
    {
        int(*snprintf)(char *str, size_t size, const char *format, ...) = NULL;
        int32_t libcModuleId = -1;
        Dynlib::LoadPrx("libSceLibcInternal.sprx", &libcModuleId);

        Dynlib::Dlsym(libcModuleId, "snprintf", &snprintf);
    }
    
    
}