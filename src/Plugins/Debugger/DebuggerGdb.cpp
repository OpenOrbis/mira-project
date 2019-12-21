#include "DebuggerGdb.hpp"
#include <Utils/Logger.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>
#include <Utils/SysWrappers.hpp>

#include <Mira.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/socket.h>
    #include <sys/ptrace.h>
};


using namespace Mira::Plugins;

const char DebuggerGdb::c_Hex[]  = "0123456789abcdef";

DebuggerGdb::DebuggerGdb() :
    m_GdbSocket(GDB_INVALID_HANDLE),
    m_GdbAddress { 0 },
    m_Buffer(nullptr),
    m_BufferSize(0),
    m_ExtendedMode(false),
    m_LastSignal(-1),
    m_ArgCount(0),
    m_Args(nullptr),
    m_EnableErrorStrings(false),
    m_StdInPath(GDB_DEFAULT_STDIN),
    m_StdOutPath(GDB_DEFAULT_STDOUT),
    m_StdErrPath(GDB_DEFAULT_STDERR),
    m_WorkingDirectory(""),
    m_DisableASLR(false),
    m_ListThreadsInStopReply(false),
    m_PacketSizeSupported(0),
    m_HaltReason(-1),
    m_ProcessId(-1)
{
    // The remote gdb spec says, we need 2* the size for registers and such
    m_Buffer = new uint8_t[GDB_REQUIRED_BUFFER_SIZE];
    if (m_Buffer == nullptr)
    {
        WriteLog(LL_Error, "could not allocate required buffer size (%x).", GDB_REQUIRED_BUFFER_SIZE);
        return;
    }

    // Set our values, as we won't use the constants in the future
    m_PacketSizeSupported = GDB_MAX_BUFFER_SIZE;
    m_BufferSize = GDB_MAX_BUFFER_SIZE;
}

bool DebuggerGdb::StartServer(int32_t p_ProcessId, uint16_t p_Port)
{
    if (m_Buffer == nullptr ||
        m_BufferSize == 0)
    {
        WriteLog(LL_Error, "buffer has not been allocated");
        return false;
    }

    // Check the process id
    if (p_ProcessId < 0)
    {
        WriteLog(LL_Error, "invalid process id (%d).", p_ProcessId);
        return false;
    }

    // Verify that this process is still alive
    if (!IsProcessAlive(p_ProcessId))
    {
        WriteLog(LL_Error, "process id (%d) does not exist.", p_ProcessId);
        return false;
    }
    
    // If we are using the default invalid port, set the default (2345)
    if (p_Port == (uint16_t)GDB_INVALID_HANDLE)
        p_Port = GDB_DEFAULT_PORT;

    // Get a refrence to the main mira thread
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    // Create a new socket
    m_GdbSocket = ksocket_t(AF_INET, SOCK_STREAM, 0, s_MainThread);
    WriteLog(LL_Debug, "gdb socket: %d", m_GdbSocket);
    if (m_GdbSocket <= 0)
    {
        WriteLog(LL_Error, "there was an error creating gdb socket (%d).", m_GdbSocket);
        m_GdbSocket = -1;
        return false;
    }

    // Set up the address
	memset(&m_GdbAddress, 0, sizeof(m_GdbAddress));
	m_GdbAddress.sin_family = AF_INET;
	m_GdbAddress.sin_port = htons(p_Port);
	m_GdbAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind to the port
    WriteLog(LL_Debug, "gdb socket is binding to port (%d).", p_Port);
    auto s_Result = kbind_t(m_GdbSocket, (const struct sockaddr*)&m_GdbAddress, sizeof(m_GdbAddress), s_MainThread);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not bind gdb socket");
        kclose_t(m_GdbSocket, s_MainThread);
        m_GdbSocket = -1;
        return false;
    }

    // Start listening for new clients
    s_Result = klisten_t(m_GdbSocket, 10, s_MainThread);
    if (s_Result != 0)
    {
        WriteLog(LL_Error, "could not listen on the gdb socket");
        kclose_t(m_GdbSocket, s_MainThread);
        m_GdbSocket = -1;
        return false;
    }

    return true;
}

void DebuggerGdb::ParsePacket(const char* p_Data, uint32_t p_DataLength)
{
    if (p_Data == nullptr || p_DataLength == 0)
    {
        WriteLog(LL_Error, "invalid packet data");
        return;
    }

    const char s_Command = p_Data[0];
    switch (s_Command)
    {
    case '!':
        HandleExtendedMode(p_Data, p_DataLength);
        break;
    case '?':
        HandleHaltReason(p_Data, p_DataLength);
        break;
    case 'A':
        WriteLog(LL_Info, "args passing not implemented");
        break;
    case 'b':
    case 'B':
        HandleB(p_Data, p_DataLength);
        break;
    case 'c':
        break;
    }
}

bool DebuggerGdb::PutChar(uint8_t p_Char)
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    ssize_t s_Ret = kwrite_t(m_GdbSocket, &p_Char, sizeof(p_Char), s_MainThread);
    if (s_Ret <= 0)
    {
        WriteLog(LL_Error, "could not write to gdb socket (%lld).", s_Ret);
        return false;
    }

    return true;
}

uint8_t DebuggerGdb::GetChar()
{
    if (m_GdbSocket <= 0)
    {
        WriteLog(LL_Error, "invalid gdb socket");
        return '\0';
    }

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return '\0';
    }

    uint8_t s_Buffer[] = { 0, 0 };
    
    ssize_t s_Ret = kread_t(m_GdbSocket, s_Buffer, sizeof(uint8_t), s_MainThread);
    if (s_Ret <= 0)
    {
        WriteLog(LL_Error, "could not read from gdb socket (%lld).", s_Ret);
        return '\0';
    }

    // printf
    return s_Buffer[0];
}

void DebuggerGdb::OnException(int p_ExceptionVector, uint64_t p_ErrorCode)
{
    
}

uint8_t* DebuggerGdb::ReadPacket()
{
    if (m_BufferSize == 0 ||
        m_Buffer == nullptr)
    {
        WriteLog(LL_Error, "no buffer has been allocated");
        return nullptr;
    }

    // Zero out our buffer
    memset(m_Buffer, 0, m_BufferSize);
    
    uint8_t* s_Buffer = m_Buffer;

    uint32_t s_Count = 0;
    uint8_t s_Tmp = 0;

    uint8_t s_Checksum = 0;
    uint8_t s_XmitChecksum = -1;

    // Skip uneeded data
    while ((s_Tmp = GetChar()) != '$')
        ;
    
    // Read all data until we hit a # (gdb stop marker)
    // or that we hit our max reading
    while (s_Count < m_BufferSize - 1)
    {
        s_Tmp = GetChar();
        if (s_Tmp == '$')
            continue;
        
        if (s_Tmp == '#')
            break;
        
        s_Checksum += s_Tmp;
        s_Buffer[s_Count] = s_Tmp;
        s_Count++;
    }
    s_Buffer[s_Count] = 0;

    if (s_Tmp == '#')
    {
        s_Tmp = GetChar();
        s_XmitChecksum = Hex(s_Tmp) << 4;
        s_Tmp = GetChar();
        s_XmitChecksum += Hex(s_Tmp);

        if (s_Checksum != s_XmitChecksum)
        {
            WriteLog(LL_Info, "failed checksum");

            if (!PutChar('-'))
                WriteLog(LL_Error, "could not send failed checksum");
        }
        else
        {
            if (!PutChar('+'))
                WriteLog(LL_Error, "could not send successful transfer");
            
            // Check if a sequence char is present, reply the sequence id
            if (s_Buffer[2] == ':')
            {
                if (!PutChar(s_Buffer[0]))
                    WriteLog(LL_Error, "could not send first part of sequence id");
                if (!PutChar(s_Buffer[1]))
                    WriteLog(LL_Error, "could not send second part of sequence id");

                return &s_Buffer[3];
            }

            return &s_Buffer[0];
        }
    }

    // TODO: Implement
    return nullptr;
}

bool DebuggerGdb::WritePacket(uint8_t* p_Buffer, uint32_t p_BufferLength)
{
    uint8_t s_Checksum = 0;
    uint32_t s_Count = 0;
    uint8_t s_Tmp = 0;

    // $<packet info>#<checksum>
    do
    {
        if (!PutChar('$'))
        {
            WriteLog(LL_Error, "could not write start of packet");
            return false;
        }
        s_Checksum = 0;
        s_Count = 0;

        while ((s_Tmp = p_Buffer[s_Count]))
        {
            if (!PutChar(s_Tmp))
            {
                WriteLog(LL_Error, "could not write buffer of packet");
                return false;
            }

            s_Checksum += s_Tmp;
            s_Count++;
        }

        if (!PutChar('#'))
            break;
        
        if (!PutChar(c_Hex[s_Checksum >> 4]))
            break;
        
        if (!PutChar(c_Hex[s_Checksum % 16]))
            break;

    } while (GetChar() != '+');
    
    return true;
}

bool DebuggerGdb::IsProcessAlive(int32_t p_ProcessId)
{
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto pfind = (struct proc* (*)(pid_t processId))kdlsym(pfind);
	
    struct proc* s_Process = pfind(p_ProcessId);
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find process for pid (%d).", p_ProcessId);
		return false;
	}
	PROC_UNLOCK(s_Process);
	return true;
}

int DebuggerGdb::Hex(char p_Char)
{
    if ((p_Char >= 'a') && (p_Char <= 'f'))
        return (p_Char - 'a' + 10);
    if ((p_Char >= '0') && (p_Char <= '9'))
        return (p_Char - '0');
    if ((p_Char >= 'A') && (p_Char <= 'F'))
        return (p_Char - 'A' + 10);
    return (-1);
}

bool DebuggerGdb::UpdateRegisters()
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    auto s_Ret = kptrace_t(PT_GETREGS, m_ProcessId, (caddr_t)&m_Registers, 0, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get registers (%d).", s_Ret);
        return false;
    }

    s_Ret = kptrace_t(PT_GETDBREGS, m_ProcessId, (caddr_t)&m_DebugRegisters, 0, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get debug registers (%d).", s_Ret);
        return false;
    }

    s_Ret = kptrace_t(PT_GETFPREGS, m_ProcessId, (caddr_t)&m_FloatingRegisters, 0, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not get floating point registers (%d).", s_Ret);
        return false;
    }

    return true;
}

bool DebuggerGdb::WriteError(int32_t p_ErrorCode)
{
    auto snprintf = (int(*)(char *str, size_t size, const char *format, ...))kdlsym(snprintf);
    char s_Buffer[16];
    memset(s_Buffer, 0, sizeof(s_Buffer));

    if (snprintf(s_Buffer, sizeof(s_Buffer), "E %02X", p_ErrorCode) <= 0)
    {
        WriteLog(LL_Error, "could not sprintf error");
        return false;
    }

    // TODO: Write out to the connection
    return true;
}

void DebuggerGdb::HandleExtendedMode(const char* p_Data, uint32_t p_DataLength)
{
    if (p_Data[0] != '!')
    {
        WriteLog(LL_Error, "not extended mode packet");
        WriteError(ENOMEM);
    }
}

void DebuggerGdb::HandleHaltReason(const char* p_Data, uint32_t p_DataLength)
{
    if (p_Data[0] != '?')
    {
        WriteLog(LL_Error, "not halt reason packet");
        WriteError(ENOMEM);
    }
}

void DebuggerGdb::HandleB(const char* p_Data, uint32_t p_DataLength)
{
    if (p_DataLength < 2)
    {
        WriteLog(LL_Error, "invalid HandleB packet");
        WriteError(ENOMEM);
        return;
    }

    if (p_Data[0] != 'b')
    {
        const char s_SecondPart = p_Data[1];
        
        switch (s_SecondPart)
        {
        case ' ': // Handle baud rate
            WriteLog(LL_Error, "baud rate not supported");
            WriteError(ENOMEM);
            return;
        case 'c': // handle 'bc' (backwards continue)
            WriteLog(LL_Error, "backwards continue not supported");
            WriteError(ENOSYS);
            break;
        case 's': // handle 'bs' (backwards single step)
            WriteLog(LL_Error, "backwards single step not supported");
            WriteError(ENOSYS);
            break;
        default:
            WriteLog(LL_Error, "invalid HandleB packet");
            WriteError(EPERM);
            break;
        }
        
    }
}