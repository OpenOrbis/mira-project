#include "LogManager.hpp"
#include <Messaging/Rpc/Connection.hpp>
#include <Messaging/MessageManager.hpp>

#include <Utils/Kdlsym.hpp>
#include <Utils/Kernel.hpp>

#include <Utils/SysWrappers.hpp>

#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/filedesc.h>

#include <Mira.hpp>

using namespace Mira::Plugins;
using namespace Mira::Plugins::LogManagerExtent;


LogManager::LogManager(uint16_t p_Port) :
    m_Socket(-1),
    m_Port(p_Port),
    m_Thread(nullptr),
    m_Running(false)
{
    // Zero out the address
    memset(&m_Address, 0, sizeof(m_Address));
}

LogManager::~LogManager()
{
    
}

bool LogManager::OnLoad()
{
    WriteLog(LL_Info, "starting log server");
    return Startup();
}

bool LogManager::OnUnload()
{
    WriteLog(LL_Error, "unloading log server");
    return Teardown();
}

bool LogManager::OnSuspend()
{
    WriteLog(LL_Error, "suspending log server");
    return Teardown();
}

bool LogManager::OnResume()
{
    WriteLog(LL_Error, "resuming log server");
    return Startup();
}

bool LogManager::Startup()
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    auto kthread_add = (int(*)(void(*func)(void*), void* arg, struct proc* procptr, struct thread** tdptr, int flags, int pages, const char* fmt, ...))kdlsym(kthread_add);
    // Create a socket
    m_Socket = ksocket_t(AF_INET, SOCK_STREAM, 0, s_MainThread);
    if (m_Socket < 0)
    {
        WriteLog(LL_Error, "could not initialize socket (%d).", m_Socket);
        return false;
    }
    WriteLog(LL_Info, "socket created: (%d).", m_Socket);

    // Set our server to listen on 0.0.0.0:<port>
    memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
    m_Address.sin_addr.s_addr = htonl(INADDR_ANY);
    m_Address.sin_port = htons(m_Port);
    m_Address.sin_len = sizeof(m_Address);

    // Bind to port
    auto s_Ret = kbind_t(m_Socket, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address), s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not bind socket (%d).", s_Ret);
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
        m_Socket = -1;
        return false;
    }
    WriteLog(LL_Info, "socket (%d) bound to port (%d).", m_Socket, m_Port);

    // Listen on the port for new connections
    s_Ret = klisten_t(m_Socket, 1, s_MainThread);
    if (s_Ret < 0)
    {
        WriteLog(LL_Error, "could not listen on socket (%d).", s_Ret);
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
		m_Socket = -1;
		return false;
    }

    // Create the new server processing thread, 8MiB stack
    s_Ret = kthread_add(LogManager::ServerThread, this, Mira::Framework::GetFramework()->GetInitParams()->process, reinterpret_cast<thread**>(&m_Thread), 0, 200, "RpcServer");
    WriteLog(LL_Debug, "rpcserver kthread_add returned (%d).", s_Ret);

    return s_Ret == 0;
}

bool LogManager::Teardown()
{
    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main thread");
        return false;
    }

    // Set that we no longer want to run this server instance
    m_Running = false;
    WriteLog(LL_Debug, "we are no longer running...");

    // Close the server socket
    if (m_Socket > 0)
    {
        kshutdown_t(m_Socket, SHUT_RDWR, s_MainThread);
		kclose_t(m_Socket, s_MainThread);
        m_Socket = -1;
    }

    WriteLog(LL_Debug, "socket is killed");

    return true;
}

void LogManager::ServerThread(void* p_UserArgs)
{
    auto kthread_exit = (void(*)(void))kdlsym(kthread_exit);

    auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "no main thread");
        kthread_exit();
        return;
    }

    //auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
	//auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);

    // Check for invalid usage
    LogManager* s_Server = static_cast<LogManager*>(p_UserArgs);
    if (s_Server == nullptr)
    {
        WriteLog(LL_Error, "invalid usage, userargs are null.");
        kthread_exit();
        return;
    }

    // Set our running state
    s_Server->m_Running = true;

    // Create our timeout
    struct timeval s_Timeout
    {
        .tv_sec = 3,
        .tv_usec = 0
    };

    int s_ClientSocket = -1;
    struct sockaddr_in s_ClientAddress = { 0 };
    size_t s_ClientAddressLen = sizeof(s_ClientAddress);
    memset(&s_ClientAddress, 0, s_ClientAddressLen);
    s_ClientAddress.sin_len = s_ClientAddressLen;
	char buffer[2];

	WriteLog(LL_Info, "Opening klog");
	auto klog = kopen_t("/dev/klog", 0x00, 0, s_MainThread);
	if (klog < 0)
	{
		WriteLog(LL_Error, "could not open klog for reading (%d).", klog);
		kclose_t(s_ClientSocket, s_MainThread);
	}
    while ((s_ClientSocket = kaccept_t(s_Server->m_Socket, reinterpret_cast<struct sockaddr*>(&s_ClientAddress), &s_ClientAddressLen, s_MainThread)) > 0)
    {
        // Set the send recv and linger timeouts
        /*auto result = ksetsockopt_t(s_ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (caddr_t)&s_Timeout, sizeof(s_Timeout), s_MainThread);
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set recv timeout (%d).", result);
            kclose_t(s_ClientSocket, s_MainThread);
            continue;
        }

        result = ksetsockopt_t(s_ClientSocket, SOL_SOCKET, SO_SNDTIMEO, (caddr_t)&s_Timeout, sizeof(s_Timeout), s_MainThread);
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set send timeout (%d).", result);
            kclose_t(s_ClientSocket, s_MainThread);
            continue;
        }*/

        // SO_LINGER
        s_Timeout.tv_sec = 0;
        auto result = ksetsockopt_t(s_ClientSocket, SOL_SOCKET, SO_LINGER, (caddr_t)&s_Timeout, sizeof(s_Timeout), s_MainThread);
        if (result < 0)
        {
            WriteLog(LL_Error, "could not set send timeout (%d).", result);
            kshutdown_t(s_ClientSocket, SHUT_RDWR, s_MainThread);
            kclose_t(s_ClientSocket, s_MainThread);
            continue;
        }

        uint32_t l_Addr = (uint32_t)s_ClientAddress.sin_addr.s_addr;

		WriteLog(LL_Debug, "got new log connection (%d) from IP (%03d.%03d.%03d.%03d).", s_ClientSocket, 
			(l_Addr & 0xFF),
			(l_Addr >> 8) & 0xFF,
			(l_Addr >> 16) & 0xFF,
			(l_Addr >> 24) & 0xFF);

        auto bytesRead = 0;
		while ((bytesRead = kread_t(klog, buffer, 1, s_MainThread)) > 0)
		{
			if (kwrite_t(s_ClientSocket, buffer, 1, s_MainThread) <= 0)
				break;

			memset(buffer, 0, sizeof(buffer));
		}

    }

    // Disconnects all clients, set running to false
    WriteLog(LL_Debug, "logserver tearing down");
    s_Server->Teardown();

    WriteLog(LL_Debug, "logserver exiting cleanly");
    kthread_exit();
}