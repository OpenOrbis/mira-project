#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace Plugins
    {
        namespace Debugging
        {
            typedef struct _Attach
            {

            } Attach;

            typedef struct _GetProcessListRequest
            {
                uint32_t Index;
            } GetProcessListRequest;

            typedef struct _UCred
            {
                uint32_t ReferenceCount;
                int32_t EffectiveUserId;
                int32_t RealUserId;
                int32_t SavedUserId;
                int32_t GroupsCount;
                int32_t RealGroupId;
                int32_t SavedGroupId;
                uint64_t UserIdInfo;
                uint64_t RealUserIdInfo;
                uint64_t Prison;
                uint64_t LoginClass;
                uint32_t CredentialFlags;
            } UCred;

            typedef struct _FileDesc
            {

            } FileDesc;

            typedef struct _Thread
            {

            } Thread;

            typedef struct _Process
            {
                uint32_t ThreadCount;
                UCred Credentials;

            } Process;

            typedef struct _GetProcessListResponse
            {
                uint32_t ProcessCount;

            } GetProcessListResponse;

            typedef struct _Breakpoint
            {
                bool Software;
                bool Enabled;
                uint8_t Reserved;
                uint8_t BackupLength;
                uint8_t BackupData[16];
                uint64_t Address;
                uint64_t HitCount;
            } Breakpoint;

            typedef struct _Watch
            {
                uint64_t Address;
                uint32_t Size;
                uint8_t PreviousData[8];
                uint8_t Data[8];
            } Watch;
        }
    }
}