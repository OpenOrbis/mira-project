#pragma once
#include <Utils/Types.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/mutex.h>
};

namespace Mira
{
    namespace Plugins
    {
        typedef enum _SelfFormat
        {
            None,
            Elf,
            Self,
            Count
        } SelfFormat;

        typedef struct _SelfContext
        {
            SelfFormat format;          // 0x00
            int32_t elfAuthType;        // 0x04
            uint32_t totalHeaderSize;   // 0x08
            uint8_t unk12[0x10];        // 0x0C
            int32_t contextId;          // 0x1C
            uint64_t serviceId;         // 0x20
            uint8_t unk40[8];           // 0x28
            int32_t bufferId;           // 0x30
            uint8_t unk52[4];          // 0x34
            uint8_t* header;            // 0x38
            struct mtx lock;            // 0x40
        } SelfContext;
        static_assert(offsetof(SelfContext, format) == 0x00);
        static_assert(offsetof(SelfContext, elfAuthType) == 0x04);
        static_assert(offsetof(SelfContext, totalHeaderSize) == 0x08);
        static_assert(offsetof(SelfContext, unk12) == 0x0C);
        static_assert(offsetof(SelfContext, contextId) == 0x1C);
        static_assert(offsetof(SelfContext, serviceId) == 0x20);
        static_assert(offsetof(SelfContext, bufferId) == 0x30);
        static_assert(offsetof(SelfContext, unk52) == 0x34);
        static_assert(offsetof(SelfContext, header) == 0x38);
        static_assert(offsetof(SelfContext, lock) == 0x40);
        static_assert(sizeof(SelfContext) == 0x60);

        typedef struct _SelfHeader
        {
            uint32_t magic;
            uint8_t version;
            uint8_t mode;
            uint8_t endian;
            uint8_t attr;
            uint32_t keyType;
            uint16_t headerSize;
            uint16_t metaSize;
            uint64_t fileSize;
            uint16_t numEntries;
            uint16_t flags;
        } SelfHeader;
        static_assert(offsetof(SelfHeader, magic) == 0x00);
        static_assert(offsetof(SelfHeader, version) == 0x04);
        static_assert(offsetof(SelfHeader, mode) == 0x05);
        static_assert(offsetof(SelfHeader, endian) == 0x06);
        static_assert(offsetof(SelfHeader, attr) == 0x07);
        static_assert(offsetof(SelfHeader, keyType) == 0x08);
        static_assert(offsetof(SelfHeader, headerSize) == 0x0C);
        static_assert(offsetof(SelfHeader, metaSize) == 0x0E);
        static_assert(offsetof(SelfHeader, fileSize) == 0x10);
        static_assert(offsetof(SelfHeader, numEntries) == 0x18);
        static_assert(offsetof(SelfHeader, flags) == 0x1A);
        static_assert(sizeof(SelfHeader) == 0x20);

        typedef struct _SelfEntry
        {
            uint64_t props;
            uint64_t offset;
            uint64_t fileSize;
            uint64_t memorySize;
        } SelfEntry;
        static_assert(offsetof(SelfEntry, props) == 0x00);
        static_assert(offsetof(SelfEntry, offset) == 0x08);
        static_assert(offsetof(SelfEntry, fileSize) == 0x10);
        static_assert(offsetof(SelfEntry, memorySize) == 0x18);
        static_assert(sizeof(SelfEntry) == 0x20);

        typedef struct _SelfExInfo
        {
            uint64_t paid;
            uint64_t ptype;
            uint64_t appVersion;
            uint64_t firmwareVersion;
            uint8_t digest[0x20];
        } SelfExInfo;
        static_assert(offsetof(SelfExInfo, paid) == 0x00);
        static_assert(offsetof(SelfExInfo, ptype) == 0x08);
        static_assert(offsetof(SelfExInfo, appVersion) == 0x10);
        static_assert(offsetof(SelfExInfo, firmwareVersion) == 0x18);
        static_assert(offsetof(SelfExInfo, digest) == 0x20);
        static_assert(sizeof(SelfExInfo) == 0x40);

        typedef struct _SelfAuthInfo
        {
            uint64_t paid;
            uint64_t caps[4];
            uint64_t attrs[4];
            uint8_t unk[0x40];
        } SelfAuthInfo;
        static_assert(offsetof(SelfAuthInfo, paid) == 0x00);
        static_assert(offsetof(SelfAuthInfo, caps) == 0x08);
        static_assert(offsetof(SelfAuthInfo, attrs) == 0x28);
        static_assert(offsetof(SelfAuthInfo, unk) == 0x48);
        static_assert(sizeof(SelfAuthInfo) == 0x88);

        typedef struct _SelfFakeAuthInfo
        {
            uint64_t paid;
            SelfAuthInfo info;
        } SelfFakeAuthInfo;
        static_assert(offsetof(SelfFakeAuthInfo, paid) == 0x00);
        static_assert(offsetof(SelfFakeAuthInfo, info) == 0x08);
        static_assert(sizeof(SelfFakeAuthInfo) == sizeof(uint64_t) + sizeof(SelfAuthInfo));

        typedef struct _MailboxMessage
        {
            int16_t funcId; // 2
            char pad02[2];
            int32_t retVal; // Return Value
            uint64_t unk08;
            uint32_t unk16;
            uint32_t unk20;
            uint64_t unk24;
            uint64_t unk32;
            uint64_t unk40;
            uint32_t unk48;
            char unk52[76];
        } MailboxMessage;
        static_assert(offsetof(MailboxMessage, unk08) == 8);
        static_assert(offsetof(MailboxMessage, retVal) == 4);
        static_assert(offsetof(MailboxMessage, unk24) == 24);
        static_assert(offsetof(MailboxMessage, unk48) == 48);
        static_assert(sizeof(MailboxMessage) == 0x80);

        typedef union _SblKeyDesc
        {
            struct _Pfs
            {
                uint16_t obfuscatedKeyId;
                uint16_t keySize;
                uint8_t escrowedKey[0x20];
            } Pfs;
            struct _Portability
            {
                uint16_t command;
                uint16_t unk02;
                uint16_t keyId;
            } Portability;
            uint8_t raw[0x7C];
        } SblKeyDesc;
        static_assert(sizeof(SblKeyDesc) == 0x7C);

        typedef struct _SblKeySlotDesc
        {
            uint32_t keyId;
            uint32_t unk04;
            uint32_t keyHandle; // -1 if freed
            uint32_t unk12;
            TAILQ_ENTRY(_SblKeySlotDesc) list;
        } SblKeySlotDesc;
        static_assert(sizeof(SblKeySlotDesc) == 0x20);

        TAILQ_HEAD(_SblKeySlotQueue, _SblKeySlotDesc);

        typedef struct _SblKeyRbtreeEntry
        {
            uint32_t handle;
            uint32_t occupied;
            SblKeyDesc desc;
            uint8_t pad[0x4];
            //uint32_t locked; // this seems wrong, it says 0x80, but that's in the SblKeyDesc??
            SblKeyRbtreeEntry* left;
            SblKeyRbtreeEntry* right;
            SblKeyRbtreeEntry* parent;
            uint32_t set;
        } SblKeyRbtreeEntry;
        static_assert(offsetof(SblKeyRbtreeEntry, handle) == 0x00);
        static_assert(offsetof(SblKeyRbtreeEntry, occupied) == 0x04);
        static_assert(offsetof(SblKeyRbtreeEntry, desc) == 0x08);
        //static_assert(offsetof(SblKeyRbtreeEntry, locked) == 0x80);
        static_assert(offsetof(SblKeyRbtreeEntry, left) == 0x88);
        static_assert(offsetof(SblKeyRbtreeEntry, right) == 0x90);
        static_assert(offsetof(SblKeyRbtreeEntry, parent) == 0x98);
        static_assert(offsetof(SblKeyRbtreeEntry, set) == 0xA0);
        static_assert(sizeof(SblKeyRbtreeEntry) == 0xA8);

        typedef struct _SblMapListEntry
        {
            SblMapListEntry* next;
            SblMapListEntry* prev;
            uint64_t cpuVa;
            uint32_t numPageGroups;
            uint64_t gpuVa;
            void* pageGroups;
            uint32_t numPages;
            uint64_t flags;
            struct proc* proc;
            void* vmPage;
        } SblMapListEntry;
        static_assert(offsetof(SblMapListEntry, next) == 0x00);
        static_assert(offsetof(SblMapListEntry, prev) == 0x08);
        static_assert(offsetof(SblMapListEntry, cpuVa) == 0x10);
        static_assert(offsetof(SblMapListEntry, numPageGroups) == 0x18);
        static_assert(offsetof(SblMapListEntry, gpuVa) == 0x20);
        static_assert(offsetof(SblMapListEntry, pageGroups) == 0x28);
        static_assert(offsetof(SblMapListEntry, numPages) == 0x30);
        static_assert(offsetof(SblMapListEntry, flags) == 0x38);
        static_assert(offsetof(SblMapListEntry, proc) == 0x40);
        static_assert(offsetof(SblMapListEntry, vmPage) == 0x48);
        static_assert(sizeof(SblMapListEntry) == 0x50);
    }
}