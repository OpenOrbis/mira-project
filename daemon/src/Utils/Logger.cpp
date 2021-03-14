// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "Logger.hpp"

using namespace Mira::Utils;

Logger* Logger::m_Instance = nullptr;
Logger* Logger::GetInstance()
{
	if (m_Instance == nullptr)
		m_Instance = new Logger();
	
	return m_Instance;
}

Logger::Logger() :
	m_LogLevel(LL_None)
{
#ifdef _DEBUG
	m_LogLevel = LL_Debug;
#else
	m_LogLevel = LL_Error;
#endif
}

Logger::~Logger()
{

}