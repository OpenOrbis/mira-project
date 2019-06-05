#include "FileManager.hpp"
#include <Utils/Kernel.hpp>

using namespace Mira::Plugins;

FileManager::FileManager() :
    m_Buffer{0}
{
    m_Buffer[0] = '\0';
}

FileManager::~FileManager()
{
    // Delete all of the resources

    // Zero the buffer
    memset(m_Buffer, 0, sizeof(m_Buffer));
}