// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Debugger.hpp"
#include <Utils/Kdlsym.hpp>

#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>


using namespace Mira::Plugins;


Debugger::Debugger() :
    m_Buffer { 0 }
{
    auto mtx_init = (void(*)(struct mtx *m, const char *name, const char *type, int opts))kdlsym(mtx_init);

    // Initialize our buffer mutex
    mtx_init(&m_Mutex, "DbgMtx", nullptr, 0);
}

Debugger::~Debugger()
{
    auto mtx_destroy = (void(*)(struct mtx* mutex))kdlsym(mtx_destroy);

    // Zero out our buffer
    memset(m_Buffer, 0, sizeof(m_Buffer));

    mtx_destroy(&m_Mutex);
}

bool Debugger::OnLoad()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
	// Create the trap fatal hook
    WriteLog(LL_Info, "creating trap_fatal hook");
    m_TrapFatalHook = new Utils::Hook(kdlsym(trap_fatal), reinterpret_cast<void*>(OnTrapFatal));
    
    if (m_TrapFatalHook != nullptr)
    {
        WriteLog(LL_Info, "enabling trap_fatal hook");
        m_TrapFatalHook->Enable();
    }
#endif
    return true;
}

bool Debugger::OnUnload()
{
#if MIRA_PLATFORM >= MIRA_PLATFORM_ORBIS_BSD_500
    WriteLog(LL_Info, "deleting trap fatal hook");
	if (m_TrapFatalHook != nullptr)
    {
        if (m_TrapFatalHook->IsEnabled())
            m_TrapFatalHook->Disable();
        
        delete m_TrapFatalHook;
        m_TrapFatalHook = nullptr;
    }
#endif

    return true;
}

int32_t Debugger::OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread)
{
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);
    auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);

    auto s_Debugger = static_cast<Debugger*>(Mira::Framework::GetFramework()->GetPluginManager()->GetDebugger());
    if (s_Debugger == nullptr)
        return ENOMEM;
    
    switch (p_Command)
    {
        case MIRA_READ_PROCESS_MEMORY:
        {
            MiraReadProcessMemory s_ReadProcessMemory;

            auto s_Ret = copyin(p_Data, &s_ReadProcessMemory, sizeof(s_ReadProcessMemory));
            if (s_Ret != 0)
            {
                WriteLog(LL_Error, "copyin failed (%d).", s_Ret);
                return ENOMEM;
            }

            // Validate that the incoming structure size is enough to hold the output
            if (s_ReadProcessMemory.StructureSize <= sizeof(s_ReadProcessMemory))
            {
                WriteLog(LL_Error, "structure size < needed size.");
                return EINVAL;
            }

            auto s_Size = s_ReadProcessMemory.StructureSize - sizeof(s_ReadProcessMemory);
            if (s_Size > sizeof(s_Debugger->m_Buffer))
            {
                WriteLog(LL_Error, "requested too large of size (%x).", s_Size);
                return EINVAL;
            }

            // Lock the debugger state
            _mtx_lock_flags(&s_Debugger->m_Mutex, 0);

            do
            {
                // Get the proc, the proc is locked at this point
                auto s_Proc = pfind(s_ReadProcessMemory.ProcessId);
                if (s_Proc == nullptr)
                {
                    WriteLog(LL_Error, "could not find pid (%d).", s_ReadProcessMemory.ProcessId);
                    s_Ret = ESRCH;
                    break;
                }

                // Zero out our buffer
                memset(s_Debugger->m_Buffer, 0, sizeof(s_Debugger->m_Buffer));

                // Read the process memory
                s_Ret = s_Debugger->ReadProcessMemory(p_Thread->td_proc, s_ReadProcessMemory.Address, s_Debugger->m_Buffer, s_Size);
                if (s_Ret != 0)
                {
                    WriteLog(LL_Error, "could not read process memory (%d).", s_Ret);
                    s_Ret = ENOMEM;
                    break;
                }

                // Write out the data to userland
                s_Ret = copyout(s_Debugger->m_Buffer, reinterpret_cast<uint8_t*>(p_Data) + offsetof(MiraReadProcessMemory, Data), s_Size);
                if (s_Ret != 0)
                {
                    WriteLog(LL_Error, "could not copyout (%d).", s_Ret);
                    s_Ret = EACCES;
                    break;
                }

                // Unlock the proc
                _mtx_unlock_flags(&s_Proc->p_mtx, 0);
            } while(false);

            // Unlock the debugger state
            _mtx_unlock_flags(&s_Debugger->m_Mutex, 0);

            return s_Ret;
        }
        case MIRA_WRITE_PROCESS_MEMORY:
        {
            MiraWriteProcessMemory s_WriteProcessMemory;

            // Copy in the structure from userland
            auto s_Ret = copyin(p_Data, &s_WriteProcessMemory, sizeof(s_WriteProcessMemory));
            if (s_Ret != 0)
            {
                WriteLog(LL_Error, "copyin failed (%d).", s_Ret);
                return ENOMEM;
            }

            // Check to make sure thata the structure size is properly set
            if (s_WriteProcessMemory.StructureSize <= sizeof(s_WriteProcessMemory))
            {
                WriteLog(LL_Error, "structure size <= needed size.");
                return EINVAL;
            }

            auto s_Size = s_WriteProcessMemory.StructureSize - sizeof(s_WriteProcessMemory);

            // Emit a warning if this seems like an abstractly large size
            if (s_Size > sizeof(s_Debugger->m_Buffer))
                WriteLog(LL_Warn, "write process memory size > buffer size.");
            
            WriteLog(LL_Debug, "reading (%x) from (%p).", s_Size, reinterpret_cast<const uint8_t*>(p_Data) + sizeof(MiraWriteProcessMemory));

            _mtx_lock_flags(&s_Debugger->m_Mutex, 0);
            do
            {
                s_Ret = copyin(reinterpret_cast<const uint8_t*>(p_Data) + sizeof(MiraWriteProcessMemory), s_Debugger->m_Buffer, s_Size);
                if (s_Ret != 0)
                {
                    WriteLog(LL_Error, "could not copy in data to write (%d) from (%p).", s_Ret, reinterpret_cast<const uint8_t*>(p_Data) + sizeof(MiraWriteProcessMemory));
                    s_Ret = ENOMEM;
                    break;
                }

                auto s_Proc = pfind(s_WriteProcessMemory.ProcessId);
                if (s_Proc == nullptr)
                {
                    WriteLog(LL_Error, "could not find process for pid (%d).", s_WriteProcessMemory.ProcessId);
                    s_Ret = ESRCH;
                    break;
                }

                s_Ret = s_Debugger->WriteProcessMemory(s_Proc, s_WriteProcessMemory.Address, s_Debugger->m_Buffer, s_Size);

                _mtx_unlock_flags(&s_Proc->p_mtx, 0);
            } while (false);
            _mtx_unlock_flags(&s_Debugger->m_Mutex, 0);

            return s_Ret;
        }
    }

    return 0;
}

int32_t Debugger::ReadProcessMemory(struct proc* p_Process, void* p_Address, uint8_t* p_Data, uint32_t p_DataLength)
{
    // auto copyout = (int(*)(const void *kaddr, void *udaddr, size_t len))kdlsym(copyout);
    // auto copyin = (int(*)(const void* uaddr, void* kaddr, size_t len))kdlsym(copyin);
    auto _mtx_lock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_lock_flags);
	auto _mtx_unlock_flags = (void(*)(struct mtx *mutex, int flags))kdlsym(_mtx_unlock_flags);

    // Validate the process
    if (p_Process == nullptr)
    {
        WriteLog(LL_Error, "invalid process.");
        return EPROCUNAVAIL;
    }

    // Validate the user mode address is not null
    if (p_Address == nullptr)
    {
        WriteLog(LL_Error, "invalid address.");
        return EINVAL;
    }

    if (p_Data == nullptr)
    {
        WriteLog(LL_Error, "invalid data.");
        return EINVAL;
    }

    if (p_DataLength == 0 || p_DataLength > Dbg_BufferSize)
    {
        WriteLog(LL_Error, "invalid data length.");
        return EINVAL;
    }

    size_t s_DataLength = p_DataLength;

    _mtx_lock_flags(&m_Mutex, 0);
    do
    {
        static_assert(Dbg_BufferSize == sizeof(m_Buffer), "invalid buffer size");
        
        // Zero out our temporary buffer
        memset(m_Buffer, 0, Dbg_BufferSize);

    } while (false);
    _mtx_unlock_flags(&m_Mutex, 0);
    

    return OrbisOS::Utilities::ProcessReadWriteMemory(p_Process, p_Address, s_DataLength, p_Data, &s_DataLength, false);
}

int32_t Debugger::WriteProcessMemory(struct proc* p_Process, void* p_Address, uint8_t* p_Data, uint32_t p_DataLength)
{
    if (p_Process == nullptr)
    {
        WriteLog(LL_Error, "invalid process.");
        return EPROCUNAVAIL;
    }

    if (p_Address == nullptr)
    {
        WriteLog(LL_Error, "invalid address.");
        return EINVAL;
    }

    if (p_Data == nullptr)
    {
        WriteLog(LL_Error, "invalid data.");
        return EINVAL;
    }

    if (p_DataLength == 0)
    {
        WriteLog(LL_Error, "invalid data length.");
        return EINVAL;
    }

    size_t s_DataLength = p_DataLength;
    return OrbisOS::Utilities::ProcessReadWriteMemory(p_Process, p_Address, s_DataLength, p_Data, &s_DataLength, true);
}