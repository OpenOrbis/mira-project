// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FuseManager.hpp"

#include <Utils/Kdlsym.hpp>

using namespace Mira::Plugins;

FuseManager::FuseManager() :
    m_DeviceCloneHandler(nullptr)
{

}

FuseManager::~FuseManager()
{

}

bool FuseManager::OnLoad()
{
    // TODO: Initialize the mutex

    // TODO: Call clone_setup

    // Initialize the event handlers
	auto eventhandler_register = (eventhandler_tag
	(*)(struct eventhandler_list *list, const char *name,
		void *func, void *arg, int priority))kdlsym(eventhandler_register);

	// Register our event handlers
	m_DeviceCloneHandler = EVENTHANDLER_REGISTER(dev_clone, reinterpret_cast<void*>(OnDeviceClone), nullptr, EVENTHANDLER_PRI_ANY);

    // TODO: call fuse_ipc_init
    // TODO: call vfs_modevent

    return true;
}

bool FuseManager::OnUnload()
{
    // Uninitialize the device clone handler
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
	auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

	EVENTHANDLER_DEREGISTER(dev_clone, m_DeviceCloneHandler);
    return true;
}

bool FuseManager::OnSuspend()
{
    return true;
}

bool FuseManager::OnResume()
{
    return true;

}

int FuseManager::FuseLoader(struct module* p_Module, int p_What, void* p_Arg)
{
    //auto fuse_loader = (int(*)(struct module* m, int what, void* arg))kdlsym(fuse_loader);

    return 0;
}

void FuseManager::OnDeviceClone(void* p_Reserved)
{

}