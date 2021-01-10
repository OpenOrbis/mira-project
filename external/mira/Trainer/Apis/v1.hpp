#pragma once
#include "../ApiDefs.hpp"

#if __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief v1 Default values
 * 
 */
enum
{
    Max_TrainerNameLength = 128,
    Max_TrainerDescriptionLength = 1024,
    Max_ExecutableLength = 1024,

    Max_OptionNameLength = 32,
    Max_TitleIdLength = 16,

    // https://semver.org/
    Max_SemverLength = 256,
};

/**
 * @brief Generic data container for an option
 * 
 */
struct v1Option_t
{
    char name[Max_OptionNameLength];
    enum OptionType_t optionType;
    enum DisplayType_t displayType;

    // Holds the currently set data
    union
    {
        bool asBool;
        uint8_t asUInt8;
        int8_t asInt8;
        uint16_t asUInt16;
        int16_t asInt16;
        uint32_t asUInt32;
        int32_t asInt32;
        uint64_t asUInt64;
        int64_t asInt64;
        float asFloat;
        double asDouble;
        uint8_t asBytes[8];
    } data;

    // Holds the default value (also used for resettings)
    union
    {
        bool asBool;
        uint8_t asUInt8;
        int8_t asInt8;
        uint16_t asUInt16;
        int16_t asInt16;
        uint32_t asUInt32;
        int32_t asInt32;
        uint64_t asUInt64;
        int64_t asInt64;
        float asFloat;
        double asDouble;
        uint8_t asBytes[8];
    } defaultData;
};

struct v1Trainer_t
{
    // Name of this trainer
    char name[Max_TrainerNameLength];

    // Description of this trainer
    char description[Max_TrainerDescriptionLength];

    // Title id of the game that's supposed to be supported
    char titleId[Max_TitleIdLength];

    // Executable name to check for (default: eboot.bin)
    char executable[Max_ExecutableLength];

    // Version of the game patch to check against ">= 1.00"
    char version[Max_SemverLength];

    // Number of options
    uint32_t optionsCount;

    // Array of options
    struct v1Option_t options[];
}; // NOTE: v1Option_t * optionsCount

#if __cplusplus
}
#endif // __cplusplus