#include "tests.h"
#include <unistd.h>
#include <fcntl.h>

#include <utility>

using namespace Mira;

Tests::Tests() :
    m_Device(-1)
{

}

Tests::~Tests()
{

}

bool Tests::Initialize()
{
    // If the device is already opened close it
    if (m_Device != -1)
        close(m_Device);
    
    // Clear the error bits
    m_Log.clear();

    // Empty the stringstream
    m_Log.str(std::string());
    
    // Open the new device
    m_Device = open("/dev/mira", O_RDWR);
    if (m_Device <= 0)
    {
        AddLog("[-] could not open /dev/mira device handle, is mira running?");
        return false;
    }

    AddLog("[+] /dev/mira opened.");

    return true;
}

bool Tests::Cleanup()
{
    // Close the device if not already closed
    if (m_Device != -1)
        close(m_Device);
    
    // Reset our device descriptor
    m_Device = -1;

    AddLog("[+] /dev/mira closed.");

    return true;
}

void Tests::AddLog(std::string p_Message)
{
    if (p_Message.empty())
        return;
    
    m_Log << p_Message << std::endl;
}

bool Tests::RunAll()
{
    AddLog("[+] running all tests.");

    if (!TestThreadCredentials())
        AddLog("[-] thread credentials test FAILED!");
    
    if (!TestPidList())
        AddLog("[-] pid list test FAILED!");
    
    if (!TestProcInformation())
        AddLog("[-] proc information test FAILED!");
    
    if (!TestMountInSandbox())
        AddLog("[-] mount in sandbox test FAILED!");
    
    if (!TestTrainerShm())
        AddLog("[-] trainer shm test FAILED!");
    
    if (!TestTrainers())
        AddLog("[-] trainers test FAILED!");
    
    if (!TestMemory())
        AddLog("[-] memory test FAILED!");

    if (!TestConfig())
        AddLog("[-] config test FAILED!");
    
    if (!TestPrivCheck())
        AddLog("[-] priv check test FAILED!");
    
    AddLog("[*] all tests ran, no failures = full success.");

    return true;
}