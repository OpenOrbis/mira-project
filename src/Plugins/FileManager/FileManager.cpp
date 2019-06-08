#include "FileManager.hpp"
#include <Utils/Kernel.hpp>

#include <Messaging/Message.hpp>
#include <Messaging/MessageManager.hpp>

#include <Mira.hpp>

#include "FileManagerMessages.hpp"

using namespace Mira::Plugins;

FileManager::FileManager() :
    m_Buffer(0x10000)
{
}

FileManager::~FileManager()
{
    // Delete all of the resources

    // Zero the buffer
    m_Buffer.zero();
}

bool FileManager::OnLoad()
{
    Mira::Framework::GetFramework()->GetMessageManager()->RegisterCallback(Messaging::MessageCategory_File, FileManager_Echo, OnEcho);
    return true;
}

bool FileManager::OnUnload()
{
    Mira::Framework::GetFramework()->GetMessageManager()->UnregisterCallback(Messaging::MessageCategory_File, FileManager_Echo, OnEcho);
    return true;
}

void FileManager::OnEcho(shared_ptr<Messaging::Message> p_Message)
{
    if (!p_Message)
    {
        WriteLog(LL_Error, "invalid message");
        return;
    }

    if (p_Message->GetPayloadLength() == 0)
    {
        WriteLog(LL_Error, "no message to echo");
        return;
    }

    WriteLog(LL_Error, "echo: (%s).", reinterpret_cast<char*>(p_Message->GetPayloadData()));
}