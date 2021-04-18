#include "Utils.hpp"
#include <mira/Driver/DriverStructs.hpp>
#include <mira/Driver/DriverCmds.hpp>

#include <Utils/Logger.hpp>

#include <unistd.h>

using namespace Mira::Memory;

void* Utils::FindPattern(int32_t p_ProcessId, const char* p_Pattern, const char* p_Mask)
{
    return nullptr;
}

void Utils::IterateMemoryRanges(std::function<void(MemoryRange&)> p_Callback)
{
    
}

void* Utils::AllocateProcessMemory(int32_t p_ProcessId, uint32_t p_Size, int32_t p_Protection)
{
    uint64_t s_AllocationSize = sizeof(uint64_t) + p_Size;

    MiraAllocateMemory s_Memory
    {
        .Pointer = nullptr,
        .ProcessId = p_ProcessId,
        .Protection = p_Protection,
        .Size = s_AllocationSize
    };

    auto s_Fd = open("/dev/mira", O_RDWR);
    if (s_Fd <= 0)
    {
        WriteLog(LL_Error, "could not open mira device (%d).", s_Fd);
        return nullptr;
    }

    do
    {
        // Call to allocate the memory we need
        auto s_Ret = ioctl(s_Fd, MIRA_ALLOCATE_PROCESS_MEMORY, &s_Memory);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not ioctl mira device (%d).", s_Ret);
            break;
        }

        // Validate the pointer we got back is valid
        if (s_Memory.Pointer == nullptr)
        {
            WriteLog(LL_Error, "memory allocation failed.");
            break;
        }

        // Write the allocation size to the start of the buffer
        if (!WriteProcessMemory(p_ProcessId, s_Memory.Pointer, &s_AllocationSize, sizeof(s_AllocationSize)))
        {
            WriteLog(LL_Error, "could not write allocation size.");
            break;
        }

    } while (false);

    
    if (close(s_Fd) != 0)
        WriteLog(LL_Error, "could not close.");
    
    // the "user requested" buffer starts at +8 bytes because we store the "raw" allocation size in the first 8
    return (void*)((uint64_t)s_Memory.Pointer + sizeof(uint64_t));        
}

void Utils::FreeProcessMemory(int32_t p_ProcessId, void* p_Pointer)
{
    // Get the original allocation size
    uint64_t s_AllocationSize = 0;
    if (!ReadProcessMemory(p_ProcessId, (void*)(((uint64_t)p_Pointer) - sizeof(uint64_t)), &s_AllocationSize, sizeof(s_AllocationSize)))
    {
        WriteLog(LL_Error, "could not get the allocation size.");
        return;
    }

    // Set up the structure we need
    MiraFreeMemory s_Memory
    {
        .ProcessId = p_ProcessId,
        .Pointer = p_Pointer,
        .Size = s_AllocationSize
    };

    // Open up the device driver
    auto s_Fd = open("/dev/mira", O_RDWR);
    if (s_Fd <= 0)
    {
        WriteLog(LL_Error, "could not open device driver (%d).", s_Fd);
        return;
    }

    // ioctl to free the process memory
    auto s_Ret = ioctl(s_Fd, MIRA_FREE_PROCESS_MEMORY, &s_Memory);
    if (s_Ret != 0)
    {
        WriteLog(LL_Error, "could not free process memory (%d).", s_Ret);
        return;
    }

    // Memory should be freed by this point
}

bool Utils::WriteProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_InputData, uint32_t p_Size)
{
    // Validate process address
    if (p_ProcessAddress == nullptr)
        return false;
    
    // Valiudate write size
    if (p_Size == 0)
        return false;
    
    // Open the mira device
    auto s_Fd = open("/dev/mira", O_RDWR);
    if (s_Fd <= 0)
    {
        WriteLog(LL_Error, "could not open mira device (%d).", s_Fd);
        return false;
    }

    // Allocate enough space for the request + the buffer at the end
    auto s_WriteRequestSize = sizeof(MiraWriteProcessMemory) + p_Size;
    auto s_WriteRequestData = new uint8_t[s_WriteRequestSize];
    if (s_WriteRequestData == nullptr)
    {
        // Close the descriptor
        close(s_Fd);
        return false;
    }
    memset(s_WriteRequestData, 0, s_WriteRequestSize);

    do
    {
        auto s_WriteRequest = reinterpret_cast<MiraWriteProcessMemory*>(s_WriteRequestData);
        s_WriteRequest->Address = p_ProcessAddress;
        s_WriteRequest->ProcessId = p_ProcessId;
        s_WriteRequest->StructureSize = s_WriteRequestSize;
        memcpy(s_WriteRequest->Data, p_InputData, p_Size);

        auto s_Ret = ioctl(s_Fd, MIRA_WRITE_PROCESS_MEMORY, s_WriteRequest);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not write process memory (%d).", s_Ret);
            break;
        }

    } while (false);

    // Free the allocated write request
    delete [] s_WriteRequestData;

    if (close(s_Fd) != 0)
        WriteLog(LL_Error, "could not close fd.");
    
    return true;
}

bool Utils::ReadProcessMemory(int32_t p_ProcessId, void* p_ProcessAddress, void* p_OutputData, uint32_t p_Size)
{
    // Validate the process address
    if (p_ProcessAddress == nullptr)
        return false;

    // Validate size to read
    if (p_Size == 0)
        return false;

    // Open up the device driver
    auto s_Fd = open("/dev/mira", O_RDWR);
    if (s_Fd <= 0)
    {
        WriteLog(LL_Error, "could not open mira device (%d).", s_Fd);
        return false;
    }
    
    // Calculate the read request size and allocate it
    auto s_ReadRequestSize = sizeof(MiraReadProcessMemory) + p_Size;
    auto s_ReadRequestData = new uint8_t[s_ReadRequestSize];
    if (s_ReadRequestData == nullptr)
    {
        WriteLog(LL_Error, "read request data is null.");
        return false;
    }
    memset(s_ReadRequestData, 0, s_ReadRequestSize);

    do
    {
        // Set all of the fields and write the ioctl
        auto s_ReadRequest = reinterpret_cast<MiraReadProcessMemory*>(s_ReadRequestData);
        s_ReadRequest->Address = p_ProcessAddress;
        s_ReadRequest->ProcessId = p_ProcessId;
        s_ReadRequest->StructureSize = s_ReadRequestSize;
        
        // Read the process memory
        auto s_Ret = ioctl(s_Fd, MIRA_READ_PROCESS_MEMORY, s_ReadRequest);
        if (s_Ret != 0)
        {
            WriteLog(LL_Error, "could not read process memory (%d).", s_Ret);
            break;
        }

        // Copy the data out to our output buffer
        memcpy(p_OutputData, s_ReadRequest->Data, p_Size);
    } while (false);

    // Free the data we allocated
    delete [] s_ReadRequestData;

    // Close the device driver handle
    if (close(s_Fd) != 0)
        WriteLog(LL_Error, "could not close fd.");
    
    // Success bitches
    return true;
}