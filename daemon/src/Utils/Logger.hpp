#pragma once
#include <mutex>
#include <cstdio>

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
			std::mutex m_Mutex;

		protected:
			Logger();
			~Logger();

		public:
			static Mira::Utils::Logger* GetInstance();

			template<typename... Args>
			inline void WriteLog_Internal2(enum LogLevels p_LogLevel, const char* p_Function, int32_t p_Line, const char* p_Format, Args... p_Args)
			{
				std::lock_guard<std::mutex> s_LockGuard(m_Mutex);

				// If the logger is set not to output anything, skip everything
				if (m_LogLevel <= LL_None)
					return;

				if (p_LogLevel > m_LogLevel)
					return;
				
				std::string s_Buffer(Logger_MaxBuffer, '\0');
				std::string s_FinalBuffer(Logger_MaxBuffer, '\0');

				#pragma clang diagnostic push
				#pragma clang diagnostic ignored "-Wformat-security"
				std::snprintf(const_cast<char*>(s_Buffer.data()), s_Buffer.capacity(), p_Format, p_Args...);
				
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

				std::snprintf(const_cast<char*>(s_FinalBuffer.data()), s_FinalBuffer.capacity(), "%s[%s] %s:%d : %s %s\n", s_LevelColor, s_LevelString, p_Function, p_Line, s_Buffer.c_str(), KNRM);
				std::printf("%s", s_FinalBuffer.c_str());
				#pragma clang diagnostic pop
			}
		};
	}
}