#pragma once
#include <Utils/New.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>

extern "C"
{
	#include <sys/param.h>
	#include <sys/lock.h>
	#include <sys/sx.h>
}

enum LogLevels
{
	LL_None,
	LL_Info,
	LL_Warn,
	LL_Error,
	LL_Debug,
	LL_All
};

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define KGRY  "\x1b[90m"

#define Logger_MaxBuffer 0x500
#ifdef _DEBUG
#define WriteLog(logLevel, format, ...) Mira::Utils::Logger::GetInstance()->WriteLog_Internal2(logLevel, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define WriteLog(x, y, ...)
#endif

namespace Mira
{
	namespace Utils
	{
		class Logger
		{
		private:
			static Mira::Utils::Logger* m_Instance;

			enum LogLevels m_LogLevel;
			char m_Buffer[Logger_MaxBuffer];
			char m_FinalBuffer[Logger_MaxBuffer];

			int32_t m_Handle;
			
			struct sx m_Mutex;

		protected:
			Logger();
			~Logger();

		public:
			static Mira::Utils::Logger* GetInstance();

			struct sx* GetMutex() { return &m_Mutex; }

			template<typename... Args>
			inline void WriteLog_Internal2(enum LogLevels p_LogLevel, const char* p_Function, int32_t p_Line, const char* p_Format, Args... p_Args)
			{
				// If the logger is set not to output anything, skip everything
				if (m_LogLevel <= LL_None)
					return;

				if (p_LogLevel > m_LogLevel)
					return;

				auto memset = (void* (*)(void *s, int c, size_t n))kdlsym(memset);
				auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
				auto printf = (void(*)(char *format, ...))kdlsym(printf);
				auto __sx_xlock = (int (*)(struct sx *sx, int opts, const char* file, int line))kdlsym(_sx_xlock);
				auto __sx_xunlock = (int (*)(struct sx *sx, const char* file, int line))kdlsym(_sx_xunlock);

				// Zero out the buffer
				__sx_xlock(&m_Mutex, 0, __FILE__, __LINE__);
				{
					memset(m_Buffer, 0, sizeof(m_Buffer));
					memset(m_FinalBuffer, 0, sizeof(m_FinalBuffer));

					snprintf(m_Buffer, sizeof(m_Buffer), p_Format, p_Args...);

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
						s_LevelColor = KGRY;
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
				__sx_xunlock(&m_Mutex, __FILE__, __LINE__);
			}
		};
	}
}