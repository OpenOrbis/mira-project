#pragma once
#include <Utils/Types.hpp>

extern "C"
{
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#include <cstdint>
//#include <cstdio>
//#include <errno.h>
//#include <string.h>
//#include <unistd.h>
};


#define TRAINER_SHARED_MEM_NAME    "_shm_mira"
#define TRAINER_HEADER_BLOCK_SIZE   0x100000 // 1MB

typedef enum TrainerVersion_t : uint16_t
{
    VersionNone = 0,
    Version1,
    VersionMax
} TrainerVersion;

enum
{
    MaxTitleIds = 5,
    MaxTitleIdLength = 14,

    MaxTitleVersions = 10,
    MaxTitleVersionLength = 6, // strlen("xx.yy") + sizeof("\0")

    MaxAsciiLength = 64, // sizeof(char) * 128
    MaxUnicodeLength = 32, // sizeof(wchar_t) * 128 = len 256

    MaxOptions = 1024,

    MaxOptionName = 32,
};

typedef struct TitleId_t
{
    char Id[MaxTitleIdLength];
} TitleId;

typedef struct TitleVersion_t
{
    char Version[MaxTitleVersionLength];
} TitleVersion;

typedef enum TrainerOptionType_t : uint8_t
{
    Option_None = 0,
    Option_Bool,
    Option_Byte,
    Option_2Byte,
    Option_4Byte,
    Option_8Byte,
    Option_Ascii,
    Option_Unicode,
    Option_Max
} TrainerOptionType;

typedef enum TrainerOptionDisplayType_t : uint8_t
{
    Display_None,
    Display_Checkbox,
    Display_NumBox,
    Display_Slider,
    Display_Textbox,
    Display_Label,
    Display_Max
} TrainerOptionDisplayType;

typedef union TrainerOptionData_t
{
    bool AsBool;
    uint8_t AsUInt8;
    int8_t AsInt8;
    uint16_t AsUInt16;
    int16_t AsInt16;
    uint32_t AsUInt32;
    int32_t AsInt32;
    uint64_t AsUInt64;
    int64_t AsInt64;
    float AsFloat;
    double AsDouble;
    char AsChars[MaxAsciiLength];
    wchar_t AsWChars[MaxUnicodeLength];
} TrainerOptionData;

typedef struct TrainerOption_t
{
    TrainerOptionType Type;
    TrainerOptionDisplayType DisplayType;
    char Name[MaxOptionName];    
    TrainerOptionData Data;
} TrainerOption;

typedef struct TrainerVersion1_t
{
    // All supported title id's in case that a game uses multiple for different regions
    TitleId TitleIds[MaxTitleIds];

    // All supported versions
    TitleVersion Versions[MaxTitleVersionLength];

    TrainerOption Options[MaxOptions];
} TrainerVersion1;

typedef union VersionUber_t
{
    TrainerVersion1 v1;
} TrainerUber;

typedef struct TrainerHeader_t
{
    // This must *never* change, the bottom TrainerUber will be
    // decoded using this version specified
    // This allows us to support multiple versions of trainers
    // without breaking backwards compatability for newer versions

    TrainerVersion Version;
    TrainerUber Uber;
} TrainerHeader;

static_assert(sizeof(TrainerHeader) < TRAINER_HEADER_BLOCK_SIZE, "trainer too large to fit in page size");
/*
static inline void PrintVersion1Trainer(const TrainerUber& p_Uber)
{
    // Print out all of the title id's
    for (auto i = 0; i < MaxTitleIds; ++i)
    {
        // Check the length of the titleids
        auto s_Length = strnlen(p_Uber.v1.TitleIds[i].Id, MaxTitleIdLength);

        // Stop when we hit an empty entry
        if (s_Length == 0)
            break;
        
        printf("## Supported TitleId: %s\n", p_Uber.v1.TitleIds[i].Id);
    }

    // Print out all supported title versions
    for (auto i = 0; i < MaxTitleVersions; ++i)
    {
        // Check the length of the version information
        auto s_Length = strnlen(p_Uber.v1.Versions[i].Version, MaxTitleVersionLength);

        // Stop when we hit an empty entry
        if (s_Length == 0)
            break;
        
        printf("## Supported Title Version: %s\n", p_Uber.v1.Versions[i].Version);
    }

    // Print out all options
    for (auto i = 0; i < MaxOptions; ++i)
    {
        // Get the option entry
        const TrainerOption& s_Option = p_Uber.v1.Options[i];
        
        // Wait until we hit an empty option
        if (s_Option.Type <= Option_None || s_Option.Type >= Option_Max)
            break;
        
        printf("#### Option: %s\n", s_Option.Name);
        printf("### Type: %d\n", s_Option.Type);
        printf("### Display: %d\n", s_Option.DisplayType);

        switch (s_Option.Type)
        {
        case Option_Bool:
            printf("### Value: %s\n", s_Option.Data.AsBool ? "true" : "false");
            break;
        case Option_Byte:
            printf("### Value: %x\n", s_Option.Data.AsUInt8);
            break;
        case Option_2Byte:
            printf("### Value: %x\n", s_Option.Data.AsUInt16);
        case Option_4Byte:
            printf("### Value: %x\n", s_Option.Data.AsUInt32);
            break;
        case Option_8Byte:
            printf("### Value: %lx\n", s_Option.Data.AsUInt64);
            break;
        case Option_Ascii:
            printf("### Value: %s\n", s_Option.Data.AsChars);
            break;
        case Option_Unicode:
            printf("### Value: %S\n", s_Option.Data.AsWChars);
            break;
        default:
            fprintf(stderr, "err: unsupported option type (%d)\n", s_Option.Type);
            break;
        }
    }
}

static inline void PrintTrainerOptions(const TrainerHeader& p_Trainer)
{
    printf("# Trainer Version: %d\n", p_Trainer.Version);

    switch (p_Trainer.Version)
    {
    case Version1:
        PrintVersion1Trainer(p_Trainer.Uber);
        break;
    default:
        fprintf(stderr, "err: unsupported trainer version.\n");
        break;
    }
}*/