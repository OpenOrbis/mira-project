#include "Manager.hpp"
#include "Listener.hpp"
#include "Rpc/Connection.hpp"

using namespace Mira::Rpc;

Manager::Manager()
{

}

Manager::~Manager()
{

}

// idk: https://stackoverflow.com/questions/20833453/comparing-stdfunctions-for-equality
template<typename T, typename... U>
size_t getAddress(std::function<T(U...)> f)
{
    typedef T(fnType)(U...);
    fnType** fnPointer = f.template target<fnType*>();
    return (size_t)*fnPointer;
}

bool Manager::Register(RpcCategory p_Category, uint32_t p_Type, std::function<void(Rpc::Connection*, const Rpc::RpcHeader*)> p_Callback)
{
    // Check if we have reached our max
    if (m_Listeners.size() >= Manager_MaxListeners)
    {
        fprintf(stderr, "err: reached maximum listener count (%lu)/(%d).\n", m_Listeners.size(), Manager_MaxListeners);
        return false;
    }

    // Check for duplicates
    auto s_It = std::find_if(m_Listeners.begin(), m_Listeners.end(), [&] (std::shared_ptr<Rpc::Listener>& p_Listener)
    {
        if (p_Listener == nullptr)
            return false;

        auto s_CallbackAddress = getAddress(p_Listener->GetCallback());
        auto s_TargetAddress = getAddress(p_Callback);

        if (s_CallbackAddress == s_TargetAddress &&
            p_Listener->GetCategory() == p_Category &&
            p_Listener->GetType() == p_Type)
        {
            return true;
        }

        return false;
    });

    // If there is a duplicate reject it
    if (s_It != m_Listeners.end())
    {
        fprintf(stderr, "err: duplicate message listener is attempting to be registered cat (%d) type (%d).\n", p_Category, p_Type);
        return false;
    }

    // Create a new listener
    auto s_Listener = std::make_shared<Rpc::Listener>(p_Category, p_Type, p_Callback);
    if (s_Listener == nullptr)
    {
        fprintf(stderr, "err: could not allocate message listener for cat (%d) type (%d).\n", p_Category, p_Type);
        return false;
    }

    // Add it to our listeners
    std::lock_guard<std::mutex> s_Guard(m_Lock);
    m_Listeners.push_back(std::move(s_Listener));

    return true;
}

bool Manager::Unregister(RpcCategory p_Category, uint32_t p_Type, std::function<void(Rpc::Connection*, const Rpc::RpcHeader*)> p_Callback)
{
    std::lock_guard<std::mutex> s_Guard(m_Lock);

    auto s_Item = std::remove_if(m_Listeners.begin(), m_Listeners.end(), [&](const std::shared_ptr<Listener> p_Listener)
    {
        if (!p_Listener)
            return false;
        
        return p_Listener->GetCategory() == p_Category &&
                p_Listener->GetType() == p_Type &&
                getAddress(p_Listener->GetCallback()) == getAddress(p_Callback);
    });

    if (s_Item == m_Listeners.end())
        return false;
    
    // Remove from the list
    m_Listeners.erase(s_Item);

    return true;
}

void Manager::Clear()
{
    std::lock_guard<std::mutex> s_Guard(m_Lock);
    m_Listeners.clear();
}

void Manager::SendErrorResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, int32_t p_Error)
{
    if (!p_Connection)
        return;
    
    SendResponse(p_Connection, p_Category, static_cast<uint32_t>(0), p_Error < 0 ? p_Error : (-p_Error), std::vector<uint8_t>());
}

void Manager::SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, std::vector<uint8_t> p_Data)
{
    if (p_Connection == nullptr)
        return;

    auto s_Socket = p_Connection->GetSocket();
    if (s_Socket < 0)
        return;
    
    flatbuffers::FlatBufferBuilder s_Builder;

    // Create a new vector
    auto s_DataVector = s_Builder.CreateVector(p_Data.data(), p_Data.size());

    // Create a new header
    auto s_Header = CreateRpcHeader(s_Builder, RpcMagics::RpcMagics_Version2, p_Category, p_Type, 0, p_Error, s_DataVector);

    // Finalize the header
    s_Builder.Finish(s_Header);

    // Get the buffer pointer and size
    auto s_BufferPointer = s_Builder.GetBufferPointer();
    auto s_BufferSize = s_Builder.GetSize();

    printf("info: buf: (%p), size: (%x).\n", s_BufferPointer, s_BufferSize);

    auto s_Ret = write(s_Socket, s_BufferPointer, s_BufferSize);
    if (s_Ret <= 0)
    {
        fprintf(stderr, "could not write to socket (%ld).\n", s_Ret);
        return;
    }
}

void Manager::OnRequest(Rpc::Connection* p_Connection, const Mira::Rpc::RpcHeader* p_Message)
{
    if (p_Connection == nullptr)
        return;
    
    if (p_Message == nullptr)
        return;
    
    std::lock_guard<std::mutex> s_Guard(m_Lock);
    std::for_each(m_Listeners.begin(), m_Listeners.end(), [&](std::shared_ptr<Listener> p_Listener)
    {
        // Validate the listener
        if (p_Listener == nullptr)
            return;
        
        // Validate the category
        if (p_Listener->GetCategory() != p_Message->category())
            return;
        
        // Validate the type
        if (p_Listener->GetType() != p_Message->type())
            return;
        
        // Call the callback
        auto s_Callback = p_Listener->GetCallback();
        if (s_Callback == nullptr)
            return;
        
        // Call the callback
        s_Callback(p_Connection, p_Message);
    });
}