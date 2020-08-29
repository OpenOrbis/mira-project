#include "TrainerManager.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

using namespace Mira::Trainers;

const char* TrainerManager::c_ShmPrefix = "_shm_";

TrainerManager::TrainerManager()
{

}

TrainerManager::~TrainerManager()
{

}

bool TrainerManager::OnLoad()
{
    return true;
}

bool TrainerManager::OnUnload()
{
    return true;
}

bool TrainerManager::OnProcessExit(struct proc* p_Process)
{
    return true;
}

bool TrainerManager::OnProcessExecEnd(struct proc* p_Process)
{
    return true;
}

bool TrainerManager::GenerateShmId(char* p_OutputString, uint32_t p_OutputStringLength)
{
    // TODO: Get a random source of entropy
    auto rand = [](){ return 4; /* chosen by a fair dice roll */ };
    auto strlen = (size_t(*)(const char *str))kdlsym(strlen);
    auto s_PrefixLength = strlen(c_ShmPrefix);

    // Validate our output string
    if (p_OutputString == nullptr)
    {
        WriteLog(LL_Error, "could not generate shm id: invalid output string.");
        return false;
    }
    
    // Validate that our output string length is enough for the prefix and at least 1 characters
    if (p_OutputStringLength <= s_PrefixLength + 1)
    {
        WriteLog(LL_Error, "could not generate shm id: length (%d) < prefix + 1 length (%d).", p_OutputStringLength, s_PrefixLength + 1);
        return false;
    }

    static const char s_Dict[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    auto s_DictLength = strlen(s_Dict);

    // Zero out our buffer
    memset(p_OutputString, 0, p_OutputStringLength);

    // Copy the prefix
    memcpy(p_OutputString, c_ShmPrefix, s_PrefixLength);

    // Randomly generate the rest of it
    for (auto i = s_PrefixLength; i < p_OutputStringLength; ++i)
        p_OutputString[i] = s_Dict[rand() % (s_DictLength - 1)];
    
    return true;
}