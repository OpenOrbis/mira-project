#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Threading
    {
        enum ThreadTypes
        {
            ThreadType_None,
            ThreadType_User,
            ThreadType_Kernel,
        };

        class ThreadContainer
        {
        private:
            ThreadTypes m_Type;
            int32_t m_ProcessId;
            void* m_Thread;
            void* m_OwningThread;

            // If the thread manager gets destroyed, should this thread terminate or run?
            bool m_LeaveRunning;

            void* m_Tag;

        public:
            ThreadContainer(ThreadTypes p_Type, int32_t p_ProcessId, void* p_Thread, void* p_OwningThread, void* p_Tag = nullptr);
            ~ThreadContainer();

            ThreadTypes GetType() { return m_Type; }
            int32_t GetProcessId() { return m_ProcessId; }
            void* GetThread() { return m_Thread; }

            void SetLeaveRunning(bool p_LeaveRunning);
        };
    }
}