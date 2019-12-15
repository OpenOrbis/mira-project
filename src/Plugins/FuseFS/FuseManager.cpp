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

void FuseManager::OnDeviceClone(void* p_Reserved)
{

}