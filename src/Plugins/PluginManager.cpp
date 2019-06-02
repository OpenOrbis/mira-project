#include "PluginManager.hpp"

using namespace Mira::Plugins;

PluginManager::PluginManager()
{
    m_Plugins.push_back(nullptr);
    m_Plugins.push_back(nullptr);
    m_Plugins.push_back(nullptr);
}

PluginManager::~PluginManager()
{

}