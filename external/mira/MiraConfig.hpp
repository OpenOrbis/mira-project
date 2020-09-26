#pragma once
#include <Utils/Types.hpp>

// List from Al-Azif/LM's PR
typedef enum _TargetId : uint8_t
{
    Unchanged = 0xFF,
    Diagnostic = 0x80,
    Devkit = 0x81,
    TestKit = 0x82,
    Japan = 0x83,
    NorthAmerica = 0x84,
    Europe = 0x85,
    Korea = 0x86,
    UK = 0x87,
    Mexico = 0x88,
    Austrailia = 0x89,
    SingaporeMalaysia = 0x8A,
    Taiwan = 0x8B,
    Russia = 0x8C,
    China = 0x8D,
    HongKongMacao = 0x8E,
    Brazil = 0x8F,
    KratosArcade = 0xA0
} TargetId;

typedef struct _MiraConfig
{
    // ==========
    // Kernel Configuration Options
    // ==========

    // Enable the trainer manager subsystem
    bool TrainerManagerEnabled;

    // Enable the Mira device driver
    bool MiraDriverEnabled;

    // Enable the remote play enabler
    bool RemotePlayActivatorEnabled;

    // Enable PSVR
    bool MorpheusActivatorEnabled;

    // Enable Browser
    bool BrowserActivatorEnabled;

    // Override the default target ID
    TargetId OverrideTargetId;

    // ==========
    // Userland Configuration Options
    // ==========

    // Logging server port
    uint16_t LogServerPort;

    // RPC server port
    uint16_t RpcServerPort;

    // Maximum client count for RPC
    uint16_t RpcServerMaxClients;
} MiraConfig;

// Initalize the configuration block with default values
static inline void InitializeMiraConfig(MiraConfig* p_Config)
{
    // Validate the incoming pointer
    if (p_Config == nullptr)
        return;
    
    p_Config->TrainerManagerEnabled = true;
    p_Config->MiraDriverEnabled = true;
    p_Config->OverrideTargetId = TargetId::Unchanged;
}