#pragma once
#include <grpcpp/grpcpp.h>

#include "Protos/FileManager.pb.h"
#include "Protos/FileManager.grpc.pb.h"

#include <unistd.h>
#include <fcntl.h>

using namespace grpc;

namespace Mira
{
    namespace Rpc
    {
        class FileManagerImpl final :
            public FileManager::FileManagerService::Service
        {
        public:
            // Defaults
            enum 
            { 
                // Maximum buffer transfer size (4MB)
                MaxBufferSize = 0x4000000
            };

        public:
            Status Echo(ServerContext* context, const ::Mira::Rpc::FileManager::EchoRequest* request, ::Mira::Rpc::FileManager::EmptyReply* response) override
            {
                printf("asdasdasdas echo ech ehco\n");
                auto s_Message = request->message();
                if (s_Message.length() > 1024)
                {
                    fprintf(stderr, "message max length past.\n");
                    return ::grpc::Status::CANCELLED;
                }

                printf("[Echo] %s\n", request->message().c_str());

                return ::grpc::Status::OK;
            }

            ::grpc::Status Open(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::OpenRequest* request, ::Mira::Rpc::FileManager::OpenResponse* response) override
            {
                // Open the file handle
                auto s_Descriptor = open(request->path().c_str(), request->flags(), request->mode());

                // Check for error
                if (s_Descriptor == -1)
                {
                    // Set the error to errono and clear out the fd
                    response->set_error(errno);
                    response->set_fd(-1);
                    return ::grpc::Status::OK;
                }

                // CLear the error and set the fd
                response->set_error(0);
                response->set_fd(s_Descriptor);

                return ::grpc::Status::OK;
            }

            ::grpc::Status Close(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::CloseRequest* request, ::Mira::Rpc::FileManager::CloseResponse* response) override
            {
                auto s_Descriptor = request->handle();
                if (s_Descriptor < 0)
                {
                    response->set_error(ENOENT);
                    return ::grpc::Status::OK;
                }
                return ::grpc::Status::OK;
            }

            ::grpc::Status Read(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::ReadRequest* request, ::grpc::ServerWriter< ::Mira::Rpc::FileManager::ReadResponse>* writer) override
            {
                Mira::Rpc::FileManager::ReadResponse s_Response;

                auto s_Descriptor = request->handle();
                if (s_Descriptor < 0)
                {
                    s_Response.set_error(ENOENT);
                    writer->Write(s_Response);

                    return ::grpc::Status::CANCELLED;
                }

                auto s_RequestedSize = request->size();
                auto s_AllocatedSize = 0;
                if (s_RequestedSize > FileManagerImpl::MaxBufferSize)
                    s_AllocatedSize = FileManagerImpl::MaxBufferSize;
                else
                    s_AllocatedSize = s_RequestedSize;

                // Allocate data
                auto s_Data = new uint8_t[s_AllocatedSize];
                if (s_Data == nullptr)
                {
                    // TODO: Send Error
                    return ::grpc::Status::CANCELLED;
                }

                // Zero out the buffer
                memset(s_Data, 0, s_AllocatedSize);

                if (s_RequestedSize > s_AllocatedSize)
                {
                    // TODO: Chunking
                    uint64_t s_LeftoverBytes = s_RequestedSize % s_AllocatedSize;
                    uint64_t s_Chunks = s_RequestedSize / s_AllocatedSize;

                    // Iterate all of our chunks
                    for (uint64_t l_ChunkIndex = 0; l_ChunkIndex < s_Chunks; ++l_ChunkIndex)
                    {
                        // Zero the buffer to clear out previous data
                        memset(s_Data, 0, s_AllocatedSize);

                        // Read the data out
                        auto l_Ret = read(request->handle(), s_Data, s_AllocatedSize);
                        if (l_Ret == -1)
                        {
                            delete [] s_Data;
                            s_Data = nullptr;

                            s_Response.clear_data();
                            s_Response.clear_error();
                            s_Response.clear_offset();

                            s_Response.set_error(errno);
                            writer->Write(s_Response);

                            return ::grpc::Status::CANCELLED;
                        }

                        // Clear the error
                        s_Response.set_error(0);

                        // Set the data
                        s_Response.clear_data();
                        s_Response.set_data(s_Data, s_AllocatedSize);
                        writer->Write(s_Response);
                    }

                    // Finish with the leftovers
                    memset(s_Data, 0, s_AllocatedSize);
                    auto s_Ret = read(request->handle(), s_Data, s_LeftoverBytes);
                    if (s_Ret == -1)
                    {
                        delete [] s_Data;
                        s_Data = nullptr;

                        s_Response.clear_data();
                        s_Response.clear_error();
                        s_Response.clear_offset();

                        s_Response.set_error(errno);
                        writer->Write(s_Response);

                        return ::grpc::Status::CANCELLED;
                    }

                    s_Response.clear_data();
                    s_Response.set_error(0);
                    s_Response.set_data(s_Data, s_LeftoverBytes);
                    writer->Write(s_Response);
                }
                else
                {
                    // Send one whole blob
                    memset(s_Data, 0, s_AllocatedSize);
                    auto s_Ret = read(request->handle(), s_Data, s_AllocatedSize);
                    if (s_Ret == -1)
                    {
                        // Free the allocated memory
                        delete [] s_Data;
                        s_Data = nullptr;

                        s_Response.clear_data();
                        s_Response.set_error(errno);
                        writer->Write(s_Response);

                        return ::grpc::Status::CANCELLED;
                    }

                    s_Response.set_error(0);
                    s_Response.set_data(s_Data, s_AllocatedSize);

                    writer->Write(s_Response);
                }

                // Free the memory
                if (s_Data != nullptr)
                {
                    delete [] s_Data;
                    s_Data = nullptr;
                }
                
                return ::grpc::Status::OK;
            }

            ::grpc::Status List(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::ListRequest* request, ::grpc::ServerWriter< ::Mira::Rpc::FileManager::ListResponse>* writer) override
            {
                return ::grpc::Status::OK;
            }

            ::grpc::Status Stat(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::StatRequest* request, ::Mira::Rpc::FileManager::StatResponse* response) override
            {
                return ::grpc::Status::OK;
            }
            ::grpc::Status Mkdir(::grpc::ServerContext* context, const ::Mira::Rpc::FileManager::MkdirRequest* request, ::Mira::Rpc::FileManager::MkdirResponse* response) override
            {
                return ::grpc::Status::OK;
            }
        };
    }
}