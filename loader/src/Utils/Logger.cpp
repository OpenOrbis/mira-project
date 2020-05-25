// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/SysWrappers.hpp>
#include <Mira.hpp>

extern "C"
{
	#include <sys/fcntl.h>
};

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

	// Initialize a mutex to prevent overlapping spam
	auto printf = (void(*)(const char *format, ...))kdlsym(printf);
	auto sx_init_flags = (void(*)(struct sx* sx, const char* description, int opts))kdlsym(_sx_init_flags);
	sx_init_flags(&m_Mutex, "logsx", 0);

	printf("post sx init flags\n");
}

Logger::~Logger()
{
	// auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);
	// mtx_destroy(&m_Mutex);
}