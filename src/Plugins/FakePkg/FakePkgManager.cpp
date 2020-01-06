#include "FakePkgManager.hpp"
#include <Utils/Kernel.hpp>

using namespace Mira::Plugins;

FakePkgManager::FakePkgManager() :
    m_NpdrmDecryptIsolatedRifHook(nullptr),
    m_NpdrmDecryptRifNewHook(nullptr),
    m_SceSblDriverSendMsgHook(nullptr),
    m_SceSblKeymgrInvalidateKey(nullptr),
    m_SceSblPfsSetKeysHook(nullptr)
{
    memset(m_Keys, 0, sizeof(m_Keys));
}

FakePkgManager::~FakePkgManager()
{

}

bool FakePkgManager::OnLoad()
{
    return true;
}

bool FakePkgManager::OnUnload()
{
    return true;
}

bool FakePkgManager::OnSuspend()
{
    return true;
}

bool FakePkgManager::OnResume()
{
    return true;
}

bool FakePkgManager::IsKeyFilled(KeyIndex p_Index)
{
    if (p_Index <= KeyIndex::SonyRifKey || p_Index >= KeyIndex::KeyCount)
        return false;
    
    static const uint8_t c_EmptyKey[RifKeySize] = { 0 };

    return memcmp(m_Keys[p_Index].Bytes, c_EmptyKey, RifKeySize) != 0;
}