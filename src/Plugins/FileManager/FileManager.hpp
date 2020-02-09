#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

#include <sys/elf64.h>

extern "C"
{
    #include <Messaging/Rpc/rpc.pb-c.h>
};

namespace Mira
{
    namespace Messaging
    {
        namespace Rpc
        {
            class Connection;
        }
    }
    
    namespace Plugins
    {
        namespace FileManagerExtent
        {
            class FileManager : public Mira::Utils::IModule
            {
            public:
                FileManager();
                virtual ~FileManager();

                virtual bool OnLoad() override;
                virtual bool OnUnload() override;

                virtual const char* GetName() override { return "FileManager"; }
                virtual const char* GetDescription() override { return "replaces ftp"; }

            private:
                static void OnEcho(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnOpen(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnClose(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnRead(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnWrite(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnGetDents(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnStat(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnMkDir(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnRmDir(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnUnlink(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);
                static void OnDecryptSelf(Messaging::Rpc::Connection* p_Connection, const RpcTransport& p_Message);

                static uint8_t* DecryptSelfFd(int p_SelfFd, size_t* p_OutElfSize);
                static uint8_t* DecryptSelf(uint8_t* p_SelfData, size_t p_SelfSize, int p_SelfFd, size_t* p_OutElfSize);

                static bool IsValidElf(Elf64_Ehdr* p_Header)
                {
                    if (p_Header == nullptr) return false;

                    return (p_Header->e_ident[EI_MAG0] == ELFMAG0 &&
                            p_Header->e_ident[EI_MAG1] == ELFMAG1 &&
                            p_Header->e_ident[EI_MAG2] == ELFMAG2 &&
                            p_Header->e_ident[EI_MAG3] == ELFMAG3);
                }

                static uint64_t GetDentCount(const char* p_Path);
            };
        }
    }
}