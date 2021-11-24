#include "Manager.hpp"

#include <string>

#ifdef _PROTOBUF
#include "Protos/Rpc.pb.h"
#endif
#include "Connection.hpp"
#include "FileManagerListener.hpp"
#include "Status.hpp"

#include <Utils/Logger.hpp>

extern "C"
{
    #include <unistd.h>
}

using namespace Mira::Rpc;

Manager::Manager() :
    m_Socket(-1),
    m_Port(0),
    m_Address { 0 },
    m_NextConnectionId(0)
{
    // Add default listeners
    /*m_Listeners.push_back(std::make_shared<FileManagerListener>(
        #ifdef _PROTOBUF
        &m_Arena
        #endif
        ));*/

    // Make this baby prrrr
    Startup();
}

Manager::~Manager()
{

}

// Closes the socket
void Manager::Internal_CloseSocket()
{
    if (m_Socket == -1)
        return;
    
    shutdown(m_Socket, SHUT_RDWR);
    close(m_Socket);
    m_Socket = -1;
}

bool Manager::Startup()
{
    // Acquire the manager lock
    std::lock_guard<std::mutex> s_LockGuard(m_Mutex);
    
    WriteLog(LL_Debug, "Rpc Server Starting...");

    // Close the socket if we already had one (stale)
    if (m_Socket != -1)
        Internal_CloseSocket();

    // Create a new socket
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == -1)
    {
        WriteLog(LL_Error, "could not create socket (%d).", errno);
        return false;
    }

    // Find an open port
    WriteLog(LL_Info, "Finding open port...");
    for (uint16_t l_PortNumber = static_cast<uint16_t>(Options::DefaultPort); l_PortNumber < static_cast<uint16_t>(Options::MaxDefaultPort); ++l_PortNumber)
    {
        // Set our server to listen on 0.0.0.0:<port>
        memset(&m_Address, 0, sizeof(m_Address));
        m_Address.sin_family = AF_INET;
        m_Address.sin_addr.s_addr = htonl(INADDR_ANY);
        m_Address.sin_port = htons(static_cast<uint16_t>(l_PortNumber));
        //m_Address.sin_len = sizeof(m_Address);

        WriteLog(LL_Debug, "Attempting to bind on port (%d).", l_PortNumber);
        auto l_Ret = bind(m_Socket, reinterpret_cast<struct sockaddr*>(&m_Address), sizeof(m_Address));
        if (l_Ret == -1)
        {
            WriteLog(LL_Error, "could not bind socket (%d) to port (%d).", m_Socket, l_PortNumber);
            continue;
        }

        // Save the port number
        m_Port = static_cast<uint16_t>(l_PortNumber);
        WriteLog(LL_Info, "Socket (%d) bound to port (%d).", m_Socket, m_Port);
        break;
    }

    // Listen for new connections
    WriteLog(LL_Info, "Attempting to listen for new connections...");
    auto s_Ret = listen(m_Socket, static_cast<uint16_t>(Options::MaxConnections));
    if (s_Ret == -1)
    {
        Internal_CloseSocket();
        WriteLog(LL_Error, "could not listen for any connections (%d).", errno);
        return false;
    }

    // Set up the new client pump loop
    m_ServerThread = std::thread([=]()
    {
        struct timeval s_Timeout
        {
            .tv_sec = 0,
            .tv_usec = 0,
        };

        int32_t s_ClientSocket = -1;
        struct sockaddr_in s_ClientAddress = { 0 };
        socklen_t s_ClientAddressLength = sizeof(s_ClientAddress);

        // Iterate accepting the socket
        while ((s_ClientSocket = accept(m_Socket, reinterpret_cast<struct sockaddr*>(&s_ClientAddress), &s_ClientAddressLength)) > 0)
        {
            // Set the linger timeout to 0 to close sockets immediately
            auto l_Ret = setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, &s_Timeout, sizeof(s_Timeout));
            if (l_Ret == -1)
            {
                WriteLog(LL_Error, "could not set socket linger timeout (%d).", errno);
                shutdown(s_ClientSocket, SHUT_RDWR);
                close(s_ClientSocket);
                continue;
            }

            // Debug logging
            uint32_t l_Address = static_cast<uint32_t>(s_ClientAddress.sin_addr.s_addr);
            WriteLog(LL_Debug, "got new rpc connection (%d) from IP (%03d.%03d.%03d.%03d).", 
                                s_ClientSocket, 
                                (l_Address & 0xFF),
                                (l_Address >> 8) & 0xFF,
                                (l_Address >> 16) & 0xFF,
                                (l_Address >> 24) & 0xFF);
            
            // Create the connection which handles the message pump
            auto l_Connection = std::make_shared<Connection>(s_ClientSocket, s_ClientSocket, s_ClientAddress, std::bind(&Manager::OnConnectionDisconnect, this, std::placeholders::_1));
            if (l_Connection == nullptr)
            {
                WriteLog(LL_Error, "could not allocate connection.");
                shutdown(s_ClientSocket, SHUT_RDWR);
                close(s_ClientSocket);
                continue;
            }

            {
                std::lock_guard<std::mutex> s_LockGuard(m_Mutex);
                m_Connections.push_back(l_Connection);
            }

            // Create a new client thread
            std::thread l_ConnectionThread([=]()
            {
                WriteLog(LL_Info, "rpc connection thread created socket: (%d), addr: (%p)",
                                    l_Connection->GetSocket(),
                                    l_Connection.get());
                
                auto s_Socket = l_Connection->GetSocket();

                uint64_t s_IncomingMessageSize = 0;
                ssize_t s_Ret = -1;

                while ((s_Ret = recv(s_Socket, &s_IncomingMessageSize, sizeof(s_IncomingMessageSize), 0)) > 0)
                {
                    // Verify we got enough data for the incoming size
                    if (s_Ret != sizeof(s_IncomingMessageSize))
                    {
                        WriteLog(LL_Error, "could not get incoming message size.");
                        break;
                    }

                    if (s_IncomingMessageSize > static_cast<uint64_t>(Options::MaxIncomingMessageSize))
                    {
                        WriteLog(LL_Error, "Incoming message size (0x%lx) > max message size (0x%lx).", s_IncomingMessageSize, Options::MaxIncomingMessageSize);
                        break;
                    }

                    // Internal data
                    std::vector<uint8_t> s_IncomingData;
                    s_IncomingData.resize(s_IncomingMessageSize, 0);

                    // Get the data from the wire
                    s_Ret = recv(s_Socket, s_IncomingData.data(), s_IncomingMessageSize, 0);
                    if (s_Ret != s_IncomingMessageSize)
                    {
                        WriteLog(LL_Error, "did not get the correct amount of data (0x%lx) wanted (0x%lx).", s_Ret, s_IncomingMessageSize);
                        break;
                    }

                    #if _PROTOBUF

                    // Create the new request and responses
                    auto s_Request = google::protobuf::Arena::CreateMessage<RpcMessage>(&m_Arena);
                    if (s_Request == nullptr)
                    {
                        WriteLog(LL_Error, "could not allocate new request.");
                        break;
                    }

                    auto s_Response = google::protobuf::Arena::CreateMessage<RpcMessage>(&m_Arena);
                    if (s_Response == nullptr)
                    {
                        WriteLog(LL_Error, "could not allocate new response.");
                        break;
                    }

                    // Attempt to parse the incoming data
                    if (!s_Request->ParseFromArray(s_IncomingData.data(), s_IncomingData.size()))
                    {
                        WriteLog(LL_Error, "could not parse RpcMessage from incoming data, is it malformed?");
                        continue;
                    }

                    // Validate the magic
                    if (s_Request->magic() != RpcMessage_Magic::RpcMessage_Magic_V3)
                    {
                        WriteLog(LL_Error, "incompatible version of RpcMessage (%d).", s_Request->magic());
                        continue;
                    }

                    // Validate the category
                    auto s_Category = s_Request->category();
                    if (s_Category <= RpcMessage_RpcCategory_NO_CATEGORY ||
                        s_Category >= RpcMessage_RpcCategory_RPC_COUNT)
                    {
                        WriteLog(LL_Error, "invalid category (%d).", s_Category);
                        continue;
                    }

                    // Iterate through all of our listeners
                    for (auto l_Listener : m_Listeners)
                    {
                        auto l_Status = l_Listener->OnMessage(s_Request, s_Response);
                        
                        // This could mean that the listener does not support this message
                        if (l_Status == Status::SKIPPED)
                            continue;
                        
                        // This means stop processing all together and go back to listening for messages
                        if (l_Status == Status::CANCELLED)
                            break;
                        
                        // These always get forced
                        s_Response->set_magic(RpcMessage_Magic_V3);
                        
                        std::string s_OutgoingData;
                        if (!s_Response->SerializeToString(&s_OutgoingData))
                        {
                            WriteLog(LL_Error, "could not serialize outgoing data.");
                            break;
                        }

                        // Write back the response
                        uint64_t s_OutgoingDataSize = s_OutgoingData.size();
                        s_Ret = write(l_Connection->GetSocket(), &s_OutgoingDataSize, sizeof(s_OutgoingDataSize));
                        if (s_Ret == -1)
                        {
                            WriteLog(LL_Error, "could not write the outgoing data size (%d).", errno);
                            break;
                        }

                        s_Ret = write(l_Connection->GetSocket(), s_OutgoingData.data(), s_OutgoingData.size());
                        if (s_Ret == -1)
                        {
                            WriteLog(LL_Error, "could not write response back to wire (%d).", errno);
                            break;
                        }

                        // Once we get a success, then we stop processing
                        // There should only be ONE endpoint per-message type
                        break;
                    }
                    #endif
                }

                OnConnectionDisconnect(l_Connection->GetId());
            });

            l_ConnectionThread.detach();

            // Zero out the address
            memset(&s_ClientAddress, 0, sizeof(s_ClientAddress));
            
        }
    });

    // Kick the thread off
    m_ServerThread.detach();
    return true;
}

void Manager::OnConnectionDisconnect(uint64_t p_ConnectionId)
{
    WriteLog(LL_Info, "shutting down connection (%d).", p_ConnectionId);

    auto s_Found = std::find_if(m_Connections.begin(), m_Connections.end(), [&](std::shared_ptr<Connection> p_IterConnection)
    {
        return p_IterConnection->GetId() == p_ConnectionId;
    });

    if (s_Found != m_Connections.end())
    {
        auto s_Connection = (*s_Found).get();

        shutdown(s_Connection->GetSocket(), SHUT_RDWR);
        close(s_Connection->GetSocket());

        m_Connections.erase(s_Found);

        WriteLog(LL_Debug, "client removed from connections list.");
    }
}