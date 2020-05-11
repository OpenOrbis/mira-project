#pragma once
#include <Utils/New.hpp>
#include <sys/param.h>
#include <sys/lock.h>
#include <sys/sx.h>

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
#define WriteLog(x, y, ...) Mira::Utils::Logger::GetInstance()->WriteLog_Internal(x, __FUNCTION__, __LINE__, y, ##__VA_ARGS__)
#define WriteKernelFileLog(y, ...) Mira::Utils::Logger::GetInstance()->WriteKernelFileLog_Internal(__FUNCTION__, __LINE__, y, ##__VA_ARGS__)
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

			void WriteLog_Internal(enum LogLevels p_LogLevel, const char* p_Function, int32_t p_Line, const char* p_Format, ...);
			void WriteKernelFileLog_Internal(const char* p_Function, int32_t p_Line, const char* p_Format, ...);
		};
	}
}