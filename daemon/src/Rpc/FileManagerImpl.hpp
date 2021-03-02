#pragma once
#include <grpcpp/grpcpp.h>

#include "Protos/FileManager.pb.h"
#include "Protos/FileManager.grpc.pb.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

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
            Status Echo(ServerContext* context, const FileManager::EchoRequest* request, FileManager::EmptyReply* response) override
            {
                auto s_Message = request->message();
                if (s_Message.length() > 1024)
                {
                    fprintf(stderr, "message max length past.\n");
                    return Status::CANCELLED;
                }

                printf("[Echo] %s\n", request->message().c_str());

                return Status::OK;
            }

            Status Open(ServerContext* context, const FileManager::OpenRequest* request, FileManager::OpenResponse* response) override
            {
                // Open the file handle
                auto s_Descriptor = open(request->path().c_str(), request->flags(), request->mode());

                // Check for error
                if (s_Descriptor == -1)
                {
                    // Set the error to errono and clear out the fd
                    response->set_error(errno);
                    response->set_fd(-1);
                    return Status::OK;
                }

                // CLear the error and set the fd
                response->set_error(0);
                response->set_fd(s_Descriptor);

                return Status::OK;
            }

            Status Close(ServerContext* context, const FileManager::CloseRequest* request, FileManager::CloseResponse* response) override
            {
                // Get the file descriptor
                auto s_Descriptor = request->handle();
                if (s_Descriptor < 0)
                {
                    response->set_error(ENOENT);
                    return Status::OK;
                }

                // Close the file descriptor
                auto s_Ret = close(s_Descriptor);
                if (s_Ret == -1)
                {
                    response->set_error(errno);
                    return Status::CANCELLED;
                }

                // Clear the error
                response->set_error(0);

                return Status::OK;
            }

            Status Read(ServerContext* context, const FileManager::ReadRequest* request, ServerWriter< FileManager::ReadResponse>* writer) override
            {
                // Hold our current read response
                Mira::Rpc::FileManager::ReadResponse s_Response;

                // Get the descriptor handle
                auto s_Descriptor = request->handle();
                if (s_Descriptor < 0)
                {
                    s_Response.set_error(ENOENT);
                    writer->Write(s_Response);

                    return Status::CANCELLED;
                }

                // Get the requested user size (can be over the chunk size)
                auto s_RequestedSize = request->size();

                // Get how much we need to allocate, if it's greater than buffer size we cap the max and chunk it, otherwise we allocate all at once
                auto s_AllocatedSize = 0;
                if (s_RequestedSize > FileManagerImpl::MaxBufferSize)
                    s_AllocatedSize = FileManagerImpl::MaxBufferSize;
                else
                    s_AllocatedSize = s_RequestedSize;

                // Allocate data
                auto s_Data = new uint8_t[s_AllocatedSize];
                if (s_Data == nullptr)
                {
                    s_Response.set_error(ENOMEM);
                    writer->Write(s_Response);

                    return Status::CANCELLED;
                }

                // Zero out the buffer
                memset(s_Data, 0, s_AllocatedSize);

                // If the requested size > allocated size then we chunk
                if (s_RequestedSize > s_AllocatedSize)
                {
                    // Chunking
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
                            // Free the allocated memory
                            delete [] s_Data;
                            s_Data = nullptr;

                            s_Response.clear_data();
                            s_Response.clear_error();
                            s_Response.clear_offset();

                            s_Response.set_error(errno);
                            writer->Write(s_Response);

                            return Status::CANCELLED;
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
                        // Free the allocated memory
                        delete [] s_Data;
                        s_Data = nullptr;

                        s_Response.clear_data();
                        s_Response.clear_error();
                        s_Response.clear_offset();

                        s_Response.set_error(errno);
                        writer->Write(s_Response);

                        return Status::CANCELLED;
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

                        return Status::CANCELLED;
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
                
                return Status::OK;
            }

            Status List(ServerContext* context, const FileManager::ListRequest* request, ServerWriter< FileManager::ListResponse>* writer) override
            {
                FileManager::ListResponse s_Response;

                auto s_Path = request->path();
                if (s_Path.length() <= 1)
                {
                    fprintf(stderr, "invalid path.\n");
                    return Status::CANCELLED;
                }

                auto s_Dir = opendir(s_Path.c_str());
                if (s_Dir == nullptr)
                {
                    fprintf(stderr, "could not opendir() (%d).\n", errno);
                    return Status::CANCELLED;
                }

                do
                {
                    struct dirent* s_Entry = nullptr;
                    while ((s_Entry = readdir(s_Dir)) != nullptr)
                    {
                        s_Response.set_inode(s_Entry->d_ino);
                        s_Response.set_name(s_Entry->d_name);
                        s_Response.set_offset(s_Entry->d_off);

                        switch (s_Entry->d_type)
                        {
                            case DT_BLK:
                                s_Response.set_type(FileManager::ListTypes::BLOCK_DEVICE);
                                break;
                            case DT_CHR:
                                s_Response.set_type(FileManager::ListTypes::CHARACTER_DEVICE);
                                break;
                            case DT_DIR:
                                s_Response.set_type(FileManager::ListTypes::DIRECTORY);
                                break;
                            case DT_FIFO:
                                s_Response.set_type(FileManager::ListTypes::NAMED_PIPE);
                                break;
                            case DT_LNK:
                                s_Response.set_type(FileManager::ListTypes::SYMBOLIC_LINK);
                                break;
                            case DT_REG:
                                s_Response.set_type(FileManager::ListTypes::REGULAR);
                                break;
                            case DT_SOCK:
                                s_Response.set_type(FileManager::ListTypes::SOCKET);
                                break;
                            case DT_UNKNOWN:
                            default:
                                s_Response.set_type(FileManager::ListTypes::UNKNOWN);
                                break;
                        }
                        
                        writer->Write(s_Response);
                    }
                } while (false);

                auto s_Ret = closedir(s_Dir);
                if (s_Ret == -1)
                {
                    // TODO: Logging
                    return Status::CANCELLED;
                }
                
                return Status::OK;
            }

            Status Stat(ServerContext* context, const FileManager::StatRequest* request, FileManager::StatResponse* response) override
            {
                // Get the handle
                auto s_Handle = request->handle();

                struct stat s_Stat = { 0 };

                // Check to see if the handle provided is valid, then we will do a handle stat, otherwise use path
                if (s_Handle > 0)
                {
                    auto s_Ret = fstat(s_Handle, &s_Stat);
                    if (s_Ret == -1)
                    {
                        // TODO: Logging
                        return Status::CANCELLED;
                    }
                }
                else
                {
                    auto s_Path = request->path();
                    if (s_Path.length() <= 0)
                    {
                        // TODO: Logging
                        return Status::CANCELLED;
                    }

                    auto s_Ret = stat(s_Path.c_str(), &s_Stat);
                    if (s_Ret == -1)
                    {
                        // TODO: Logging
                        return Status::CANCELLED;
                    }
                }

                // Copy over all of the information into the message
                response->set_device(s_Stat.st_dev);
                response->set_inode(s_Stat.st_ino);
                response->set_protection(s_Stat.st_mode);
                response->set_num_hard_links(s_Stat.st_nlink);
                response->set_user_id(s_Stat.st_uid);
                response->set_group_id(s_Stat.st_gid);
                response->set_device_id(s_Stat.st_rdev);
                response->set_size(s_Stat.st_size);
                response->set_block_size(s_Stat.st_blksize);
                response->set_blocks(s_Stat.st_blocks);

                // TODO: Set the times once we figure out how to allocate some shit
                //response->set_allocated_access_time()
                return Status::OK;
            }

            Status Mkdir(ServerContext* context, const FileManager::MkdirRequest* request, FileManager::MkdirResponse* response) override
            {
                auto s_Path = request->path();
                if (s_Path.length() <= 1)
                {
                    fprintf(stderr, "invalid path.");
                    return Status::CANCELLED;
                }

                auto s_Mode = request->mode();

                auto s_Ret = mkdir(s_Path.c_str(), s_Mode);
                if (s_Ret == -1)
                {
                    response->set_error(errno);
                    return Status::CANCELLED;
                }

                // Set our success error code
                response->set_error(0);

                return Status::OK;
            }
        };
    }
}