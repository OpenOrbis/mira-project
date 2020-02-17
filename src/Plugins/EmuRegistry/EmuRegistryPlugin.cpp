#include "EmuRegistryPlugin.hpp"
#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;

EmuRegistryPlugin::EmuRegistryPlugin() :
    m_GetIntHook(nullptr),
    m_SetIntHook(nullptr),
    m_GetBinHook(nullptr),
    m_SetBinHook(nullptr),
    m_GetStrHook(nullptr),
    m_SetStrHook(nullptr)
{
    
}

EmuRegistryPlugin::~EmuRegistryPlugin()
{
    
}

bool EmuRegistryPlugin::OnLoad()
{
    m_GetIntHook = new Utils::Hook(kdlsym(sceRegMgrGetInt), reinterpret_cast<void*>(OnSceRegMgrGetInt));
    m_SetIntHook = new Utils::Hook(kdlsym(sceRegMgrSetInt), reinterpret_cast<void*>(OnSceRegMgrSetInt));
    m_GetBinHook = new Utils::Hook(kdlsym(sceRegMgrGetBin), reinterpret_cast<void*>(OnSceRegMgrGetBin));
    m_SetBinHook = new Utils::Hook(kdlsym(sceRegMgrSetBin), reinterpret_cast<void*>(OnSceRegMgrSetBin));
    m_GetStrHook = new Utils::Hook(kdlsym(sceRegMgrGetStr), reinterpret_cast<void*>(OnSceRegMgrGetStr));
    m_SetStrHook = new Utils::Hook(kdlsym(sceRegMgrSetStr), reinterpret_cast<void*>(OnSceRegMgrSetStr));
    return true;
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