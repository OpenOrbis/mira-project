#pragma once
#include <Utils/Types.hpp>
#include <Boot/Config.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/lock.h>
    #include <sys/mutex.h>
};

namespace Mira
{
    namespace OrbisOS
    {
        typedef enum _SelfFormat
        {
            None,
            Elf,
            Self,
            Count
        } SelfFormat;

        enum
        {
            EKPFS_SIZE = 0x20,
            EEKPFS_SIZE = 0x100,
            PFS_SEED_SIZE = 0x10,
            PFS_FINAL_KEY_SIZE = 0x20,
            SIZEOF_PFS_KEY_BLOB = 0x140,
            CONTENT_KEY_SEED_SIZE = 0x10,
            SELF_KEY_SEED_SIZE = 0x10,
            EEKC_SIZE = 0x20,
            MAX_FAKE_KEYS = 32,
            SIZEOF_RSA_KEY = 0x48,
            PFS_FAKE_OBF_KEY_ID = 0x1337,
            SIZEOF_PFS_HEADER = 0x5A0,

            // RIF
            RIF_DATA_SIZE = 0x90,
            RIF_DIGEST_SIZE = 0x10,
            RIF_KEY_TABLE_SIZE = 0x230,
            RIF_MAX_KEY_SIZE = 0x20,
            SIZEOF_ACTDAT = 0x200,
            SIZEOF_RIF = 0x400,
            RIF_PAYLOAD_SIZE = (RIF_DIGEST_SIZE + RIF_DATA_SIZE),

            
            #define CCP_OP(cmd) (cmd >> 24)
            CCP_MAX_PAYLOAD_SIZE = 0x88,
            CCP_OP_AES =  0,
            CCP_OP_XTS = 2,
            CCP_OP_HMAC= 9,
            CCP_USE_KEY_FROM_SLOT   = (1 << 18),
            CCP_GENERATE_KEY_AT_SLOT= (1 << 19),
            CCP_USE_KEY_HANDLE      = (1 << 20),

            SCE_SBL_ERROR_NPDRM_ENOTSUP = 0x800F0A25,
            SIZEOF_SBL_KEY_RBTREE_ENTRY = 0xA8, // sceSblKeymgrSetKey
            SIZEOF_SBL_MAP_LIST_ENTRY = 0x50, // sceSblDriverMapPages
            TYPE_SBL_KEY_RBTREE_ENTRY_DESC_OFFSET = 0x04,
            TYPE_SBL_KEY_RBTREE_ENTRY_LOCKED_OFFSET = 0x80,
            SIZEOF_SBL_KEY_DESC = 0x7C, // sceSblKeymgrSetKey
            SBL_MSG_SERVICE_MAILBOX_MAX_SIZE = 0x80,
            SBL_MSG_CCP = 0x8,

            #define SWAP_16(x) ((((uint16_t)(x) & 0xff) << 8) | ((uint16_t)(x) >> 8))
            #define BE16(val) SWAP_16(val)
            #define LE32(val) (val)
        };

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

        typedef struct _SelfContext
        {
            SelfFormat format;          // 0x00
            int32_t elfAuthType;        // 0x04
            uint32_t totalHeaderSize;   // 0x08
            uint32_t unk12;             // 0x0C
            void* segment;              // 0x10
            uint32_t unk24;             // 0x18
            int32_t contextId;          // 0x1C
            uint64_t serviceId;         // 0x20
            uint64_t unk40;             // 0x28
            int32_t bufferId;           // 0x30
            uint32_t unk52;             // 0x34
            _SelfHeader* header;        // 0x38
            struct mtx lock;            // 0x40
        } SelfContext;
        static_assert(offsetof(SelfContext, format) == 0x00);
        static_assert(offsetof(SelfContext, elfAuthType) == 0x04);
        static_assert(offsetof(SelfContext, totalHeaderSize) == 0x08);
        static_assert(offsetof(SelfContext, unk12) == 0x0C);
        static_assert(offsetof(SelfContext, segment) == 0x10);
        static_assert(offsetof(SelfContext, contextId) == 0x1C);
        static_assert(offsetof(SelfContext, serviceId) == 0x20);
        static_assert(offsetof(SelfContext, bufferId) == 0x30);
        static_assert(offsetof(SelfContext, unk52) == 0x34);
        static_assert(offsetof(SelfContext, header) == 0x38);
        static_assert(offsetof(SelfContext, lock) == 0x40);
        static_assert(sizeof(SelfContext) == 0x60);

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
            uint64_t size;
            SelfAuthInfo info;
        } SelfFakeAuthInfo;
        static_assert(offsetof(SelfFakeAuthInfo, size) == 0x00);
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
            struct _SblKeyRbtreeEntry* left;
            struct _SblKeyRbtreeEntry* right;
            struct _SblKeyRbtreeEntry* parent;
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
            struct _SblMapListEntry* next;
            struct _SblMapListEntry* prev;
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

        typedef struct _FakeKeyDesc
        {
            uint8_t key[0x20];
            bool occupied;
        } FakeKeyDesc;

        typedef struct _FakeKeyD
        {
            uint32_t index;
            uint8_t seed[PFS_SEED_SIZE];
        } FakeKeyD;

        typedef struct _Ekc
        {
            uint8_t contentKeySeed[CONTENT_KEY_SEED_SIZE];
            uint8_t selfKeySeed[SELF_KEY_SEED_SIZE];
        } Ekc;

        struct ccp_link
        {
            void* p;
        };

        union ccp_op
        {
            struct
            {
                uint32_t cmd;
                uint32_t status;
            } common;
            struct 
            {
                uint32_t cmd;
                uint32_t status;
                uint64_t data_size;
                uint64_t in_data;
                uint64_t out_data;
                union 
                {
                    uint32_t key_index;
                    uint8_t key[0x20];
                };
                uint8_t iv[0x10];
            } aes;
            uint8_t buf[CCP_MAX_PAYLOAD_SIZE];
        };

        struct ccp_msg
        {
            union ccp_op op;
            uint32_t index;
            uint32_t result;
            TAILQ_ENTRY(ccp_msg) next;
            uint64_t message_id;
            LIST_ENTRY(ccp_link) links;
        };

        struct ccp_req
        {
            TAILQ_HEAD(, ccp_msg) msgs;
            void (*cb)(void* arg, int result);
            void* arg;
            uint64_t message_id;
            LIST_ENTRY(ccp_link) links;
        };

        typedef union _SblMsgService
        {
            struct
            {
                union ccp_op op;
            } ccp;
            
        } SblMsgService;

        typedef struct _SblMsgHeader
        {
            uint32_t cmd;
            uint32_t status;
            uint64_t message_id;
            uint64_t extended_msgs;
        } _SblMsgHeader;

        typedef struct _SblMsg
        {
            _SblMsgHeader hdr;
            union 
            {
                SblMsgService service;
                uint8_t raw[0x1000];
            };
            
        } SblMsg;

        typedef struct _RifKeyBlob
        {
            Ekc eekc;
            uint8_t entitlementKey[0x10];
        } RifKeyBlob;

        typedef union _PfsKeyBlob
        {
            struct _In
            {
                uint8_t eekpfs[EEKPFS_SIZE];
                Ekc eekc;
                uint32_t pubkeyVer; /* 0x1/0x80000001/0xC0000001 */
                uint32_t keyVer;    /* 1 (if (rif_ver_major & 0x1) != 0, then pfs_key_ver=1, otherwise pfs_key_ver=0) */
                uint64_t headerGva;
                uint32_t headerSize;
                uint32_t type;
                uint32_t finalized;
                uint32_t isDisc;
            } In;
            struct _Out
            {
                uint8_t escrowedKeys[0x40];
            } Out;
            
        } PfsKeyBlob;
        static_assert(sizeof(_PfsKeyBlob) == SIZEOF_PFS_KEY_BLOB);

        typedef union _KeymgrPayload
        {
            struct
            {
                uint32_t cmd;
                uint32_t status;
                uint64_t data;
            };
            uint8_t buf[0x80];
        } KeymgrPayload;

        typedef struct _RsaKey
        {
            uint8_t _pad00[0x20];
            uint8_t* p;
            uint8_t* q;
            uint8_t* dmp1;
            uint8_t* dmq1;
            uint8_t* iqmp;
        } RsaKey;
        static_assert(offsetof(_RsaKey, p) == 0x20);
        static_assert(offsetof(_RsaKey, q) == 0x28);
        static_assert(offsetof(_RsaKey, dmp1) == 0x30);
        static_assert(offsetof(_RsaKey, dmq1) == 0x38);
        static_assert(offsetof(_RsaKey, iqmp) == 0x40);
        static_assert(sizeof(_RsaKey) == SIZEOF_RSA_KEY);

        typedef struct _ActDat
        {
            uint32_t magic;
            uint16_t versionMajor;
            uint16_t versionMinor;
            uint64_t accountId;
            uint64_t startTime;
            uint64_t endTime;
            uint64_t flags;
            uint32_t unk3;
            uint32_t unk4;
            uint8_t _pad30[0x30];
            uint8_t openPsidHash[0x20];
            uint8_t staticPerConsoleData1[0x20];
            uint8_t digest[0x10];
            uint8_t keyTable[0x20];
            uint8_t staticPerConsoleData2[0x10];
            uint8_t staticPerConsoleData3[0x20];
            uint8_t signature[0x100];
        } ActDat;
        static_assert(offsetof(_ActDat, magic) == 0x00);
        static_assert(offsetof(_ActDat, versionMajor) == 0x04);
        static_assert(offsetof(_ActDat, versionMinor) == 0x06);
        static_assert(offsetof(_ActDat, accountId) == 0x08);
        static_assert(offsetof(_ActDat, startTime) == 0x10);
        static_assert(offsetof(_ActDat, endTime) == 0x18);
        static_assert(offsetof(_ActDat, flags) == 0x20);
        static_assert(offsetof(_ActDat, unk3) == 0x28);
        static_assert(offsetof(_ActDat, unk4) == 0x2C);
        static_assert(offsetof(_ActDat, openPsidHash) == 0x60);
        static_assert(offsetof(_ActDat, staticPerConsoleData1) == 0x80);
        static_assert(offsetof(_ActDat, digest) == 0xA0);
        static_assert(offsetof(_ActDat, keyTable) == 0xB0);
        static_assert(offsetof(_ActDat, staticPerConsoleData2) == 0xD0);
        static_assert(offsetof(_ActDat, staticPerConsoleData3) == 0xE0);
        static_assert(offsetof(_ActDat, signature) == 0x100);
        static_assert(sizeof(_ActDat) == SIZEOF_ACTDAT);

        typedef struct _Rif
        {
            uint32_t magic;
            uint16_t versionMajor;
            uint16_t versionMinor;
            uint64_t accountId;
            uint64_t startTime;
            uint64_t endTime;
            char contentId[0x30];
            uint16_t format;
            uint16_t drmType;
            uint16_t contentType;
            uint16_t skuFlag;
            uint64_t contentFlags;
            uint32_t iroTag;
            uint32_t ekcVersion;
            uint8_t _pad6A[2];
            uint16_t unk3;
            uint16_t unk4;
            uint8_t _pad6E[0x1F2];
            uint8_t digest[0x10];
            uint8_t data[RIF_DATA_SIZE];
            uint8_t signature[0x100];
        } Rif;
        static_assert(offsetof(_Rif, magic) == 0x00);
        static_assert(offsetof(_Rif, versionMajor) == 0x04);
        static_assert(offsetof(_Rif, versionMinor) == 0x06);
        static_assert(offsetof(_Rif, accountId) == 0x08);
        static_assert(offsetof(_Rif, startTime) == 0x10);
        static_assert(offsetof(_Rif, endTime) == 0x18);
        static_assert(offsetof(_Rif, contentId) == 0x20);
        static_assert(offsetof(_Rif, format) == 0x50);
        static_assert(offsetof(_Rif, drmType) == 0x52);
        static_assert(offsetof(_Rif, contentType) == 0x54);
        static_assert(offsetof(_Rif, skuFlag) == 0x56);
        static_assert(offsetof(_Rif, contentFlags) == 0x58);
        static_assert(offsetof(_Rif, iroTag) == 0x60);
        static_assert(offsetof(_Rif, ekcVersion) == 0x64);
        static_assert(offsetof(_Rif, unk3) == 0x6A);
        static_assert(offsetof(_Rif, unk4) == 0x6C);
        static_assert(offsetof(_Rif, digest) == 0x260);
        static_assert(offsetof(_Rif, data) == 0x270);
        static_assert(offsetof(_Rif, signature) == 0x300);
        static_assert(sizeof(_Rif) == SIZEOF_RIF);

        typedef struct _RsaBuffer
        {
            uint8_t* ptr;
            size_t size;
        } RsaBuffer;

        typedef struct _PfsHeader
        {
            uint8_t _pad00[0x370];
            uint8_t cryptSeed[0x10];
            uint8_t _pad380[0x220];
        } PfsHeader;
        static_assert(offsetof(_PfsHeader, cryptSeed) == 0x370);
        static_assert(sizeof(_PfsHeader) == SIZEOF_PFS_HEADER);

                typedef union _KeymgrResponse
        {
            struct
            {
                uint32_t type;
                uint8_t key[RIF_MAX_KEY_SIZE];
                uint8_t data[RIF_DIGEST_SIZE + RIF_DATA_SIZE];
            } DecryptRif;
            struct
            {
                uint8_t raw[SIZEOF_RIF];
            } DecryptEntireRif;
        } KeymgrResponse;

        typedef union _KeymgrRequest
        {
            struct
            {
                uint32_t type;
                uint8_t key[RIF_MAX_KEY_SIZE];
                uint8_t data[RIF_DIGEST_SIZE + RIF_DATA_SIZE];
            } DecryptRif;

//#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_455 || MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_501 || MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_555
            struct 
            {
                _Rif rif;
                uint8_t keyTable[RIF_KEY_TABLE_SIZE];
                uint64_t timestamp;
                int status;
            } DecryptEntireRif;
//# endif
        } KeymgrRequest;
    }
}