#include "EmuRegistryPlugin.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Mira.hpp>
#include <Plugins/PluginManager.hpp>

using namespace Mira::Plugins;

EmuRegistryPlugin::EmuRegistryPlugin() :
    m_GetIntHook(nullptr),
    m_SetIntHook(nullptr),
    m_GetBinHook(nullptr),
    m_SetBinHook(nullptr),
    m_GetStrHook(nullptr),
    m_SetStrHook(nullptr),
    m_StringEntryCount(0),
    m_StringEntrySize(0),
    m_BinaryEntryCount(0),
    m_BinaryEntrySize(0)
{
    // Initialize all of our entries
    m_IntEntrySize = ARRAYSIZE(m_IntEntries);
    for (auto i = 0; i < m_IntEntrySize; ++i)
        m_IntEntries[i] = nullptr;
    
    m_StringEntrySize = ARRAYSIZE(m_StringEntries);
    for (auto i = 0; i < m_StringEntrySize; ++i)
        m_StringEntries[i] = nullptr;
    
    m_IntEntrySize = ARRAYSIZE(m_BinaryEntries);
    for (auto i = 0; i < m_IntEntrySize; ++i)
        m_BinaryEntries[i] = nullptr;
}

EmuRegistryPlugin::~EmuRegistryPlugin()
{
    DestroyIntEntries();
    DestroyBinaryEntries();
    DestroyStringEntries();
}

void EmuRegistryPlugin::DestroyIntEntries()
{
    WriteLog(LL_Debug, "destroying int entries");
    for (auto i = 0; i < m_IntEntrySize; ++i)
    {
        if (m_IntEntries[i] != nullptr)
            delete m_IntEntries[i];
        
        m_IntEntries[i] = nullptr;
    }

    m_IntEntryCount = 0;
    m_IntEntrySize = 0;
}

void EmuRegistryPlugin::DestroyStringEntries()
{
    WriteLog(LL_Debug, "destroying string entries");
    for (auto i = 0; i < m_StringEntrySize; ++i)
    {
        if (m_StringEntries[i] != nullptr)
        {
            if (m_StringEntries[i]->str != nullptr)
                delete [] m_StringEntries[i]->str;
            
            m_StringEntries[i]->str = nullptr;
            m_StringEntries[i]->strSize = 0;

            delete m_StringEntries[i];
        }
        
        m_StringEntries[i] = nullptr;
    }

    m_StringEntryCount = 0;
    m_StringEntrySize = 0;
}

void EmuRegistryPlugin::DestroyBinaryEntries()
{
    WriteLog(LL_Debug, "destroying binary entries");

    for (auto i = 0; i < m_BinaryEntrySize; ++i)
    {
        if (m_BinaryEntries[i] != nullptr)
        {
            if (m_BinaryEntries[i]->data != nullptr)
                delete [] m_BinaryEntries[i]->data;
            
            m_BinaryEntries[i]->data = nullptr;
            m_BinaryEntries[i]->dataSize = 0;

            delete [] m_BinaryEntries[i];
        }

        m_BinaryEntries[i] = nullptr;
    }
    
    m_BinaryEntryCount = 0;
    m_BinaryEntrySize =  0;
}

bool EmuRegistryPlugin::OnLoad()
{
    /*m_GetIntHook = new Utils::Hook(kdlsym(sceRegMgrGetInt), reinterpret_cast<void*>(OnSceRegMgrGetInt));
    m_SetIntHook = new Utils::Hook(kdlsym(sceRegMgrSetInt), reinterpret_cast<void*>(OnSceRegMgrSetInt));
    m_GetBinHook = new Utils::Hook(kdlsym(sceRegMgrGetBin), reinterpret_cast<void*>(OnSceRegMgrGetBin));
    m_SetBinHook = new Utils::Hook(kdlsym(sceRegMgrSetBin), reinterpret_cast<void*>(OnSceRegMgrSetBin));
    m_GetStrHook = new Utils::Hook(kdlsym(sceRegMgrGetStr), reinterpret_cast<void*>(OnSceRegMgrGetStr));
    m_SetStrHook = new Utils::Hook(kdlsym(sceRegMgrSetStr), reinterpret_cast<void*>(OnSceRegMgrSetStr));
    */return true;
}

bool EmuRegistryPlugin::OnUnload()
{
    if (m_GetIntHook)
    {
        m_GetIntHook->Disable();
        delete m_GetIntHook;
        m_GetIntHook = nullptr;
    }

    if (m_SetIntHook)
    {
        m_SetIntHook->Disable();
        delete m_SetIntHook;
        m_SetIntHook = nullptr;
    }

    if (m_GetBinHook)
    {
        m_GetBinHook->Disable();
        delete m_GetBinHook;
        m_GetBinHook = nullptr;
    }

    if (m_SetBinHook)
    {
        m_SetBinHook->Disable();
        delete m_SetBinHook;
        m_SetBinHook = nullptr;
    }

    if (m_GetStrHook)
    {
        m_GetStrHook->Disable();
        delete m_GetStrHook;
        m_GetStrHook = nullptr;
    }

    if (m_SetStrHook)
    {
        m_SetStrHook->Disable();
        delete m_SetStrHook;
        m_SetStrHook = nullptr;
    }
    
    return true;
}

bool EmuRegistryPlugin::OnSuspend()
{
    return OnUnload();
}

bool EmuRegistryPlugin::OnResume()
{
    return OnLoad();
}

uint32_t EmuRegistryPlugin::OnSceRegMgrGetInt(uint32_t p_Id, int32_t* p_OutValue)
{
    return 0;
}

uint32_t EmuRegistryPlugin::OnSceRegMgrSetInt(uint32_t p_Id, int32_t p_Value)
{
    return 0;
}

uint32_t EmuRegistryPlugin::OnSceRegMgrGetBin(uint32_t p_Id, void* p_Data, uint32_t p_Size)
{
    return 0;
}

uint32_t EmuRegistryPlugin::OnSceRegMgrSetBin(uint32_t p_Id, void* p_Data, uint32_t p_Size)
{
    return 0;
}

uint32_t EmuRegistryPlugin::OnSceRegMgrGetStr(uint32_t p_Id, char* p_String, uint32_t p_Size)
{
    return 0;
}

uint32_t EmuRegistryPlugin::OnSceRegMgrSetStr(uint32_t p_Id, char* p_String, uint32_t p_Size)
{
    return 0;
}

EmuRegistryPlugin* EmuRegistryPlugin::GetPlugin()
{
    auto s_Framework = Mira::Framework::GetFramework();
    if (s_Framework == nullptr)
        return nullptr;
    
    auto s_PluginManager = s_Framework->GetPluginManager();
    if (s_PluginManager == nullptr)
        return nullptr;
    
    auto s_RegistryPlugin = static_cast<EmuRegistryPlugin*>(s_PluginManager->GetEmulatedRegistry());
    if (s_RegistryPlugin == nullptr)
        return nullptr;
    
    return s_RegistryPlugin;
}