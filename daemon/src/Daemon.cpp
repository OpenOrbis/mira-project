#include "Daemon.hpp"

#include <flatbuffers/flatbuffers.h>
#include <External/flatbuffers/rpc_generated.h>

#include <cstdio>

using namespace Mira;

Daemon::Daemon()
{
    flatbuffers::FlatBufferBuilder s_Builder;
    Rpc::RpcHeaderBuilder s_HeaderBuilder(s_Builder);
    s_HeaderBuilder.add_magic(Rpc::RpcMagics_Version2);
    s_HeaderBuilder.add_category(Rpc::RpcCategory_SYSTEM);
    auto s_Data = s_Builder.CreateVector((uint8_t*)nullptr, 0);
    s_HeaderBuilder.add_data(s_Data);

    auto s_Size = s_Builder.GetSize();
    auto s_Buffer = s_Builder.GetBufferPointer();

    const auto s_FileName = "daemon_test.bin";
    FILE* s_File = fopen(s_FileName, "wb");
    if (s_File == nullptr)
    {
        fprintf(stderr, "could not open file (%s) for writing (%d).\n", s_FileName, errno);
        return;
    }

    auto s_Written = fwrite(s_Buffer, sizeof(uint8_t), s_Size, s_File);
    if (s_Written <= 0)
    {
        fprintf(stderr, "could not write to file (%s) (%lu).", s_FileName, s_Written);
        return;
    }

    auto s_Result = fclose(s_File);
    if (s_Result < 0)
    {
        fprintf(stderr, "could not close file (%s) (%d).", s_FileName, s_Result);
        return;
    }
}

Daemon::~Daemon()
{
    if (m_RpcServer)
        m_RpcServer.reset();
    
    if (m_Debugger)
        m_Debugger.reset();
    
    if (m_FtpServer)
        m_FtpServer.reset();
}

bool Daemon::OnLoad()
{
    if (m_Debugger)
    {
        if (!m_Debugger->OnLoad())
        {
            fprintf(stderr, "err: could not load debugger.\n");
            return false;
        }
    }
    if (m_RpcServer)
    {
        if (!m_RpcServer->OnLoad())
        {
            fprintf(stderr, "err: could not load rpc server.\n");
            return false;
        }
    }

    return true;
}