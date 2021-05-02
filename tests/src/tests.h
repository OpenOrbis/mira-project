#pragma once
#include <cstdint>
#include <sstream>

namespace Mira
{
    class Tests
    {
    private:
        int32_t m_Device;
        std::stringstream m_Log;

    protected:
        bool Initialize();
        bool Cleanup();

        void AddLog(std::string p_Message);

        // TESTS START
        bool TestThreadCredentials();
        bool TestPidList();
        bool TestProcInformation();
        bool TestMountInSandbox();
        bool TestTrainerShm();
        bool TestTrainers();
        bool TestMemory();
        bool TestConfig();
        bool TestPrivCheck();
        // TESTS END
        
    public:
        Tests();
        ~Tests();

        bool RunAll();

        std::string GetLog() { return m_Log.str(); }
    };
}