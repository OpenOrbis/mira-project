/**
 * @file Api.hpp
 * @author kd (@kd_tech_)
 * @brief 
 * @version 0.1
 * @date 2021-01-10
 * 
 * @copyright Copyright (c) 2021 OpenOrbis
 * 
 */
#pragma once
#include "Apis/v1.hpp"

#if __cplusplus
extern "C" {
#endif // __cplusplus

struct Trainer_t
{
    // Version of this trainer, this is so we can maintain compatibility with old trainers
    uint32_t ApiVersion;

    union
    {
        struct v1Trainer_t v1;
    };
};

/**
 * @brief Entrypoint called for when a trainer loads
 * 
 * @return enum SubstituteError return error
 */
extern int32_t trainer_load();

/**
 * @brief Entrypoint called for when a trainer unloads
 * 
 * @return enum SubstituteError return error
 */
extern int32_t trainer_unload();

extern int32_t trainer_hookiat();
extern int32_t trainer_hook();

#if __cplusplus
}
#endif // __cplusplus