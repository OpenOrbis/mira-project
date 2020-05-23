// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Utils/Logger.hpp>
#include <stdarg.h>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Utils/Kernel.hpp>

using namespace Mira::Utils;

Logger* Logger::m_Instance = nullptr;
Logger* Logger::GetInstance()
{
	if (m_Instance == nullptr)
		m_Instance = new Logger();
	
	return m_Instance;
}

Logger::Logger() :
	m_LogLevel(LL_None),
	m_Buffer{0},
	m_FinalBuffer{0},
	m_Handle(-1)
{
#ifdef _DEBUG
	m_LogLevel = LL_Debug;
#else
	m_LogLevel = LL_Error;
#endif
	m_Handle = -1;

	memset(m_Buffer, 0, sizeof(m_Buffer));
	memset(m_FinalBuffer, 0, sizeof(m_FinalBuffer));
}

Logger::~Logger()
{

}

void Logger::WriteLog_Internal(enum LogLevels p_LogLevel, const char* p_Function, int32_t p_Line, const char* p_Format, ...)
{
	// If the logger is set not to output anything, skip everything
	if (m_LogLevel <= LL_None)
		return;

	if (p_LogLevel > m_LogLevel)
		return;

	auto memset = (void* (*)(void *s, int c, size_t n))kdlsym(memset);
	auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
	auto vsnprintf = (int(*)(char *str, size_t size, const char *format, va_list ap))kdlsym(vsnprintf);
	auto printf = (void(*)(char *format, ...))kdlsym(printf);

	// Zero out the buffer
	memset(m_Buffer, 0, sizeof(m_Buffer));
	memset(m_FinalBuffer, 0, sizeof(m_FinalBuffer));

	va_list args;
	va_start(args, p_Format);
	vsnprintf(m_Buffer, sizeof(m_Buffer), p_Format, args);
	va_end(args);

	const char* s_LevelString = "None";
	const char* s_LevelColor = KNRM;
	switch (p_LogLevel)
	{
	case LL_Info:
		s_LevelString = "Info";
		s_LevelColor = KGRN;
		break;
	case LL_Warn:
		s_LevelString = "Warn";
		s_LevelColor = KYEL;
		break;
	case LL_Error:
		s_LevelString = "Error";
		s_LevelColor = KRED;
		break;
	case LL_Debug:
		s_LevelString = "Debug";
		s_LevelColor = KMAG;
		break;
	case LL_None:
	default:
		s_LevelString = "None";
		s_LevelColor = KNRM;
		break;
	}

	snprintf(m_FinalBuffer, sizeof(m_FinalBuffer), "%s[%s] %s:%d : %s %s\n", s_LevelColor, s_LevelString, p_Function, p_Line, m_Buffer, KNRM);
	printf(m_FinalBuffer);
}