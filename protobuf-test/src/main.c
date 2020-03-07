
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protobuf-c.h"
#include "rpc.pb-c.h"
#include "filemanager.pb-c.h"

int main(int argc, char* argv[])
{
    RpcTransport s_Transport = RPC_TRANSPORT__INIT;
    
    RpcHeader s_Header = RPC_HEADER__INIT;
    s_Header.category = RPC_CATEGORY__FILE;
    s_Header.error = 0;
    s_Header.isrequest = 1;
    s_Header.magic = 2;
    s_Header.type = 0xEBDB1342;

    s_Transport.header = &s_Header;

    FmEchoRequest s_Request = FM_ECHO_REQUEST__INIT;
    s_Request.message = "hello world";

    int s_Size = fm_echo_request__get_packed_size(&s_Request);
    uint8_t* s_Data = malloc(s_Size);
    if (s_Data == NULL)
    {
        fprintf(stderr, "err: could not allocate data\n");
        return -1;
    }
    memset(s_Data, 0, s_Size);

    int s_Ret = fm_echo_request__pack(&s_Request, s_Data);
    if (s_Ret < 0)
    {
        fprintf(stderr, "err: pack returned (%d).\n", s_Ret);
        return -1;
    }

    s_Transport.data.data = s_Data;
    s_Transport.data.len = s_Size;
    

    s_Size = rpc_transport__get_packed_size(&s_Transport);
    int s_TotalSize = s_Size + sizeof(uint64_t);

    uint8_t* s_TransportData = malloc(s_TotalSize);
    if (s_TransportData == NULL)
    {
        fprintf(stderr, "err: could not allocate transport data\n");
        return -1;
    }
    memset(s_TransportData, 0, s_TotalSize);

    *(uint64_t*)s_TransportData = s_Size;

    uint8_t* s_TransportStart = s_TransportData + sizeof(uint64_t);
    s_Ret = rpc_transport__pack(&s_Transport, s_TransportStart);
    if (s_Ret <= 0)
    {
        fprintf(stderr, "err: could not pack transport\n");
        return -1;
    }

    FILE* s_Handle = fopen("dump.bin", "wb+");
    if (s_Handle == NULL)
    {
        fprintf(stderr, "err: could not open file handle\n");
        return -1;
    }

    fwrite(s_TransportData, s_TotalSize, 1, s_Handle);
    fclose(s_Handle);


    return 0;
}