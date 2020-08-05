#include "Manager.hpp"
#include "Listener.hpp"

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

bool Manager::Register(RpcCategory p_Category, uint32_t p_Type, std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<Rpc::RpcHeader>)> p_Callback)
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
    m_Listeners.push_back(std::move(s_Listener));

    return true;
}

bool Manager::Unregister(RpcCategory p_Category, uint32_t p_Type, std::function<void(std::shared_ptr<Rpc::Connection>, std::shared_ptr<Rpc::RpcHeader>)>)
{
    return true;
}

void Manager::Clear()
{
    m_Listeners.clear();
}

void Manager::SendErrorResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, int32_t p_Error)
{

}

void Manager::SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, std::shared_ptr<Rpc::RpcHeader> p_Message)
{

}

void Manager::SendResponse(std::shared_ptr<Rpc::Connection> p_Connection, RpcCategory p_Category, uint32_t p_Type, int64_t p_Error, std::vector<uint8_t> p_Data)
{

}

void Manager::OnRequest(std::shared_ptr<Rpc::Connection> p_Connection, std::shared_ptr<Rpc::RpcHeader> p_Message)
{

}