/**
 * @file ApiDefs.hpp
 * @author kd (@kd_tech_)
 * @brief This file is used for structures that won't change based on API version
 * @version 0.1
 * @date 2021-01-10
 * 
 * @copyright Copyright (c) 2021 OpenOrbis
 * 
 */
#pragma once
#include <stdint.h>

#if __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Describes how the underlying data type should be interpreted
 * 
 */
enum OptionType_t
{
    OT_None,
    OT_Boolean,
    OT_UInt8,
    OT_Int8,
    OT_UInt16,
    OT_Int16,
    OT_UInt32,
    OT_Int32,
    OT_UInt64,
    OT_Int64,
    OT_Float,
    OT_Double,
    OT_COUNT
};

/**
 * @brief How the underlying data type should be shown to the end-user
 * 
 */
enum DisplayType_t
{
    DT_None,
    DT_Checkbox,
    DT_Slider,
    DT_Textbox,
    DT_ProgressBar,
    DT_Label,
    DT_NumberBox,
    DT_COUNT
};

#if __cplusplus
}
#endif // __cplusplus