SCE_REGMGR_ENT_KEY_DEVENV_TOOL_boot_param


=== Default uint32 Values ===

bootParam_uint_0_31    = 2863311530
bootParam_uint_32_63   = 65535
bootParam_uint_64_95   = 13
bootParam_uint_96_127  = 3
bootParam_uint_128_159 = 0


=== Binary Form ===

10101010 10101010 10101010 10101010
00000000 00000000 11111111 11111111
00000000 00000000 00000000 00001101
00000000 00000000 00000000 00000011
00000000 00000000 00000000 00000000


=== SET 1 ===

BOOTPARAM | 1 << BIT


=== SET 0 ===

BOOTPARAM & ~(1 << BIT)


=== Bytes 0 - 4 ===

private const int Bit_No_Release_Check_Mode = 0;

private const int Bit_No_Testkit_Mode = 1;

private const int Bit_No_Release_Mode_Console = 4;

private const int Bit_No_Disable_Qaf = 8;

private const int Bit_No_Disable_Utoken = 9;

private const int Bit_No_Disable_Razor = 24;


=== Bytes 4 - 8 === // (32 - Bit)

private const int Bit_No_Fs_Reverted = 32; // 0

private const int Bit_No_Fs_Advanced = 33; // 1

private const int Bit_No_Fs_Trial = 34; // 2

private const int Bit_No_Slow_Hdd_Mode = 40; // 8

private const int Bit_No_Repeat_Kernel_System_Suspend = 62; // 30


=== Bytes 8 - 12 === // (64 - Bit)

private const int Bit_No_Cpu_Restrict = 64; // 0

private const int Bit_No_Cpu_Only_One = 65; // 1

private const int Bit_No_Disable_Unapproved_Sysctl = 70; // 6

private const int Bit_No_Vm_Compressor_Mode0 = 72; // 8


=== Bytes 12 - 16 === // (96 - Bit)

private const int Bit_No_Kratos_Swap_Off = 96; // 0

private const int Bit_No_Kratos_Nfs_Mode = 97; // 1

private const int Bit_No_Kratos_Wlan_Off = 100; // 4


=== Bytes 16 - 20 === // (128 - Bit)

private const int Bit_No_Vsh_4k_Mode = 128; // 0

private const int Bit_No_Base_PS4_Emulation = 146; // 18

