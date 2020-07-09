#pragma once

extern "C"
{
    #include <sys/stdint.h>
};

typedef struct _TrainerEntry
{
    enum { MaxTitleLength = 32 };

    typedef enum _EntryType : uint32_t
    {
        Boolean,
        Byte_1,
        Byte_2,
        Byte_4,
        Byte_8,
    } EntryType;

    EntryType Type;
    bool Enabled;
    uint8_t Padding[3];
    char Title[MaxTitleLength];

    // Ghetto hack for conversions
    union
    {
        uint8_t Data[8];
        bool AsBool;
        int8_t AsInt8;
        uint8_t AsUInt8;
        int16_t AsInt16;
        uint16_t AsUInt16;
        int32_t AsInt32;
        uint32_t AsUInt32;
        int64_t AsInt64;
        uint64_t AsUInt64;
        float AsSingle;
        double AsDouble;
    };
} TrainerEntry;
static_assert(sizeof(TrainerEntry) == 32);

typedef struct _TrainerTitleId
{
    enum { MaxTitleIdLength = 8 };

    char TitleId[MaxTitleIdLength];
    uint8_t MajorVersion;
    uint8_t MinorVersion;
} TrainerTitleId;

typedef struct _TrainerHeaderV1
{
    enum
    {
        MaxNameLength = 64,
        MaxDescriptionLength = 1024,
        MaxAuthorLength = 256,
        MaxTitleIdCount = 8,
        MaxTitleIdLength = 8,
    };
    
    // Total header size (including entries)
    uint32_t Size;

    // Trainer major version
    uint8_t MajorVersion;

    // Trainer minor version
    uint8_t MinorVersion;

    // Is this trainer enabled (even if it's loaded)
    bool Enabled;

    // Should the trainer re-load the values
    bool NeedsUpdate;
    
    // Name of the trainer
    char Name[MaxNameLength];

    // Description of the trainer
    char Description[MaxDescriptionLength];

    // Author(s) of the trainer
    char Author[MaxAuthorLength];

    // Total supported title id's (up to MaxTitleIdCount)
    TrainerTitleId TitleIds[MaxTitleIdCount];

    // Variable length trainer entries
    TrainerEntry Entries[];

} TrainerHeaderV1;

typedef struct _TrainerHeaderV2
{
    uint32_t Size;

} TrainerHeaderV2;

typedef struct _TrainerHeader
{
    uint64_t Version;
    union
    {
        // We want to be able to support multiple versions of trainers as needs/requirements change
        TrainerHeaderV1 v1Header;
        //TrainerHeaderV2 v2Header;
    };
    
} TrainerHeader;

static_assert(sizeof(TrainerHeader) < 4096/*PAAGE_SIZE*/);