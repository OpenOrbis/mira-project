// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

/*
    Implemented from: https://github.com/xvortex/ps4-hen-vtx
    Ported by: kiwidog (@kd_tech_)

    Bugfixes: SiSTRo (https://github.com/SiSTR0), SocraticBliss (https://github.com/SocraticBliss)
*/

#include "FakePkgManager.hpp"
#include <Utils/Kernel.hpp>
#include <Utils/Kdlsym.hpp>
#include <Utils/_Syscall.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>
#include <Boot/Config.hpp>

#include <Utils/System.hpp>
#include <OrbisOS/Utilities.hpp>

#include <Mira.hpp>
#include <Boot/Config.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/sysent.h>
    #include <sys/proc.h>
    #include <sys/mman.h>
    #include <sys/ptrace.h>
    #include <sys/wait.h>
    #include <sys/signal.h>
};

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

#pragma region "Fake-Self-Keys"
const uint8_t FakePkgManager::g_ypkg_p[] =
{
    0x2D, 0xE8, 0xB4, 0x65, 0xBE, 0x05, 0x78, 0x6A, 0x89, 0x31, 0xC9, 0x5A, 0x44, 0xDE, 0x50, 0xC1,
    0xC7, 0xFD, 0x9D, 0x3E, 0x21, 0x42, 0x17, 0x40, 0x79, 0xF9, 0xC9, 0x41, 0xC1, 0xFC, 0xD7, 0x0F,
    0x34, 0x76, 0xA3, 0xE2, 0xC0, 0x1B, 0x5A, 0x20, 0x0F, 0xAF, 0x2F, 0x52, 0xCD, 0x83, 0x34, 0x72,
    0xAF, 0xB3, 0x12, 0x33, 0x21, 0x2C, 0x20, 0xB0, 0xC6, 0xA0, 0x2D, 0xB1, 0x59, 0xE3, 0xA7, 0xB0,
    0x4E, 0x1C, 0x4C, 0x5B, 0x5F, 0x10, 0x9A, 0x50, 0x18, 0xCC, 0x86, 0x79, 0x25, 0xFF, 0x10, 0x02,
    0x8F, 0x90, 0x03, 0xA9, 0x37, 0xBA, 0xF2, 0x1C, 0x13, 0xCC, 0x09, 0x45, 0x15, 0xB8, 0x55, 0x74,
    0x0A, 0x28, 0x24, 0x04, 0xD1, 0x19, 0xAB, 0xB3, 0xCA, 0x44, 0xB6, 0xF8, 0x3D, 0xB1, 0x2A, 0x72,
    0x88, 0x35, 0xE4, 0x86, 0x6B, 0x55, 0x47, 0x08, 0x25, 0x16, 0xAB, 0x69, 0x1D, 0xBF, 0xF6, 0xFE,
};

const uint8_t FakePkgManager::g_ypkg_q[] =
{
    0x23, 0x80, 0x77, 0x84, 0x4D, 0x6F, 0x9B, 0x24, 0x51, 0xFE, 0x2A, 0x6B, 0x28, 0x80, 0xA1, 0x9E,
    0xBD, 0x6D, 0x18, 0xCA, 0x8D, 0x7D, 0x9E, 0x79, 0x5A, 0xE0, 0xB8, 0xEB, 0xD1, 0x3D, 0xF3, 0xD9,
    0x02, 0x90, 0x2A, 0xA7, 0xB5, 0x7E, 0x9A, 0xA2, 0xD7, 0x2F, 0x21, 0xA8, 0x50, 0x7D, 0x8C, 0xA1,
    0x91, 0x2F, 0xBF, 0x97, 0xBE, 0x92, 0xC2, 0xC1, 0x0D, 0x8C, 0x0C, 0x1F, 0xDE, 0x31, 0x35, 0x15,
    0x39, 0x90, 0xCC, 0x97, 0x47, 0x2E, 0x7F, 0x09, 0xE9, 0xC3, 0x9C, 0xCE, 0x91, 0xB2, 0xC8, 0x58,
    0x76, 0xE8, 0x70, 0x1D, 0x72, 0x5F, 0x4A, 0xE6, 0xAA, 0x36, 0x22, 0x94, 0xC6, 0x52, 0x90, 0xB3,
    0x9F, 0x9B, 0xF0, 0xEF, 0x57, 0x8E, 0x53, 0xC3, 0xE3, 0x30, 0xC9, 0xD7, 0xB0, 0x3A, 0x0C, 0x79,
    0x1B, 0x97, 0xA8, 0xD4, 0x81, 0x22, 0xD2, 0xB0, 0x82, 0x62, 0x7D, 0x00, 0x58, 0x47, 0x9E, 0xC7,
};

const uint8_t FakePkgManager::g_ypkg_dmp1[] =
{
    0x25, 0x54, 0xDB, 0xFD, 0x86, 0x45, 0x97, 0x9A, 0x1E, 0x17, 0xF0, 0xE3, 0xA5, 0x92, 0x0F, 0x12,
    0x2A, 0x5C, 0x4C, 0xA6, 0xA5, 0xCF, 0x7F, 0xE8, 0x5B, 0xF3, 0x65, 0x1A, 0xC8, 0xCF, 0x9B, 0xB9,
    0x2A, 0xC9, 0x90, 0x5D, 0xD4, 0x08, 0xCF, 0xF6, 0x03, 0x5A, 0x5A, 0xFC, 0x9E, 0xB6, 0xDB, 0x11,
    0xED, 0xE2, 0x3D, 0x62, 0xC1, 0xFC, 0x88, 0x5D, 0x97, 0xAC, 0x31, 0x2D, 0xC3, 0x15, 0xAD, 0x70,
    0x05, 0xBE, 0xA0, 0x5A, 0xE6, 0x34, 0x9C, 0x44, 0x78, 0x2B, 0xE5, 0xFE, 0x38, 0x56, 0xD4, 0x68,
    0x83, 0x13, 0xA4, 0xE6, 0xFA, 0xD2, 0x9C, 0xAB, 0xAC, 0x89, 0x5F, 0x10, 0x8F, 0x75, 0x6F, 0x04,
    0xBC, 0xAE, 0xB9, 0xBC, 0xB7, 0x1D, 0x42, 0xFA, 0x4E, 0x94, 0x1F, 0xB4, 0x0A, 0x27, 0x9C, 0x6B,
    0xAB, 0xC7, 0xD2, 0xEB, 0x27, 0x42, 0x52, 0x29, 0x41, 0xC8, 0x25, 0x40, 0x54, 0xE0, 0x48, 0x6D,
};

const uint8_t FakePkgManager::g_ypkg_dmq1[] =
{
    0x4D, 0x35, 0x67, 0x38, 0xBC, 0x90, 0x3E, 0x3B, 0xAA, 0x6C, 0xBC, 0xF2, 0xEB, 0x9E, 0x45, 0xD2,
    0x09, 0x2F, 0xCA, 0x3A, 0x9C, 0x02, 0x36, 0xAD, 0x2E, 0xC1, 0xB1, 0xB2, 0x6D, 0x7C, 0x1F, 0x6B,
    0xA1, 0x8F, 0x62, 0x20, 0x8C, 0xD6, 0x6C, 0x36, 0xD6, 0x5A, 0x54, 0x9E, 0x30, 0xA9, 0xA8, 0x25,
    0x3D, 0x94, 0x12, 0x3E, 0x0D, 0x16, 0x1B, 0xF0, 0x86, 0x42, 0x72, 0xE0, 0xD6, 0x9C, 0x39, 0x68,
    0xDB, 0x11, 0x80, 0x96, 0x18, 0x2B, 0x71, 0x41, 0x48, 0x78, 0xE8, 0x17, 0x8B, 0x7D, 0x00, 0x1F,
    0x16, 0x68, 0xD2, 0x75, 0x97, 0xB5, 0xE0, 0xF2, 0x6D, 0x0C, 0x75, 0xAC, 0x16, 0xD9, 0xD5, 0xB1,
    0xB5, 0x8B, 0xE8, 0xD0, 0xBF, 0xA7, 0x1F, 0x61, 0x5B, 0x08, 0xF8, 0x68, 0xE7, 0xF0, 0xD1, 0xBC,
    0x39, 0x60, 0xBF, 0x55, 0x9C, 0x7C, 0x20, 0x30, 0xE8, 0x50, 0x28, 0x44, 0x02, 0xCE, 0x51, 0x2A,
};

const uint8_t FakePkgManager::g_ypkg_iqmp[] =
{
    0xF5, 0x73, 0xB8, 0x7E, 0x5C, 0x98, 0x7C, 0x87, 0x67, 0xF1, 0xDA, 0xAE, 0xA0, 0xF9, 0x4B, 0xAB,
    0x77, 0xD8, 0xCE, 0x64, 0x6A, 0xC1, 0x4F, 0xA6, 0x9B, 0xB9, 0xAA, 0xCC, 0x76, 0x09, 0xA4, 0x3F,
    0xB9, 0xFA, 0xF5, 0x62, 0x84, 0x0A, 0xB8, 0x49, 0x02, 0xDF, 0x9E, 0xC4, 0x1A, 0x37, 0xD3, 0x56,
    0x0D, 0xA4, 0x6E, 0x15, 0x07, 0x15, 0xA0, 0x8D, 0x97, 0x9D, 0x92, 0x20, 0x43, 0x52, 0xC3, 0xB2,
    0xFD, 0xF7, 0xD3, 0xF3, 0x69, 0xA2, 0x28, 0x4F, 0x62, 0x6F, 0x80, 0x40, 0x5F, 0x3B, 0x80, 0x1E,
    0x5E, 0x38, 0x0D, 0x8B, 0x56, 0xA8, 0x56, 0x58, 0xD8, 0xD9, 0x6F, 0xEA, 0x12, 0x2A, 0x40, 0x16,
    0xC1, 0xED, 0x3D, 0x27, 0x16, 0xA0, 0x63, 0x97, 0x61, 0x39, 0x55, 0xCC, 0x8A, 0x05, 0xFA, 0x08,
    0x28, 0xFD, 0x55, 0x56, 0x31, 0x94, 0x65, 0x05, 0xE7, 0xD3, 0x57, 0x6C, 0x0D, 0x1C, 0x67, 0x0B,
};

const uint8_t FakePkgManager::g_RifDebugKey[] =
{
    0x96, 0xC2, 0x26, 0x8D, 0x69, 0x26, 0x1C, 0x8B, 0x1E, 0x3B, 0x6B, 0xFF, 0x2F, 0xE0, 0x4E, 0x12
};

const uint8_t FakePkgManager::g_FakeKeySeed[] =
{
    0x46, 0x41, 0x4B, 0x45, 0x46, 0x41, 0x4B, 0x45, 0x46, 0x41, 0x4B, 0x45, 0x46, 0x41, 0x4B, 0x45,
};

#pragma endregion

FakePkgManager::FakePkgManager() :
    m_NpdrmDecryptIsolatedRifHook(nullptr),
    m_NpdrmDecryptRifNewHook(nullptr),
    m_SceSblDriverSendMsgHook(nullptr),
    m_SceSblKeymgrInvalidateKey(nullptr),
    m_SceSblPfsSetKeysHook(nullptr)
{
    auto sv = (struct sysentvec*)kdlsym(self_orbis_sysvec);
    struct sysent* sysents = sv->sv_table;
    uint8_t* s_TrampolineF = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_GET_LINK].sy_call); // syscall #410
    uint8_t* s_TrampolineG = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_SET_FD].sy_call); // syscall #388
    uint8_t* s_TrampolineH = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_SET_FILE].sy_call); // syscall #389
    uint8_t* s_TrampolineI = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_SET_LINK].sy_call); // syscall #411
    uint8_t* s_TrampolineJ = reinterpret_cast<uint8_t*>(sysents[SYS_MAC_SYSCALL].sy_call); // syscall #394
    uint8_t* s_TrampolineK = reinterpret_cast<uint8_t*>(sysents[SYS___MAC_EXECVE].sy_call); // syscall #415

    Utilities::HookFunctionCall(s_TrampolineF, reinterpret_cast<void*>(OnNpdrmDecryptIsolatedRif), kdlsym(npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook));
    Utilities::HookFunctionCall(s_TrampolineG, reinterpret_cast<void*>(OnNpdrmDecryptRifNew), kdlsym(npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook));
    Utilities::HookFunctionCall(s_TrampolineH, reinterpret_cast<void*>(OnSceSblPfsSetKeys), kdlsym(mountpfs__sceSblPfsSetKeys_hookA));
    Utilities::HookFunctionCall(s_TrampolineI, reinterpret_cast<void*>(OnSceSblPfsSetKeys), kdlsym(mountpfs__sceSblPfsSetKeys_hookB));
    Utilities::HookFunctionCall(s_TrampolineJ, reinterpret_cast<void*>(OnSceSblDriverSendMsg), kdlsym(sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook));
    Utilities::HookFunctionCall(s_TrampolineK, reinterpret_cast<void*>(OnSceSblKeymgrInvalidateKeySxXlock), kdlsym(sceSblKeymgrInvalidateKey__sx_xlock_hook));

    WriteLog(LL_Debug, "Installed fpkg hooks");
}

FakePkgManager::~FakePkgManager()
{

}

void FakePkgManager::GenPfsCryptoKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[PFS_SEED_SIZE], uint32_t p_Index, uint8_t p_Key[PFS_FINAL_KEY_SIZE])
{
    if (p_EncryptionKeyPFS == nullptr)
    {
        WriteLog(LL_Error, "p_EncryptinoKeyPFS is nullptr.");
        return;
    }

    auto s_Thread = curthread;
    FakeKeyD s_D;
    memset(&s_D, 0, sizeof(s_D));

    s_D.index = p_Index;
    memcpy(s_D.seed, p_Seed, PFS_SEED_SIZE);

    // fpu_kern_enter
    auto fpu_kern_enter = (int(*)(struct thread *td, struct fpu_kern_ctx *ctx, u_int flags))kdlsym(fpu_kern_enter);
    auto fpu_kern_leave = (int (*)(struct thread *td, struct fpu_kern_ctx *ctx))kdlsym(fpu_kern_leave);

    auto fpu_ctx = (fpu_kern_ctx*)kdlsym(fpu_kern_ctx);
    auto Sha256Hmac = (void (*)(uint8_t hash[0x20], const uint8_t* data, size_t data_size, const uint8_t* key, int key_size))kdlsym(Sha256Hmac);

    fpu_kern_enter(s_Thread, fpu_ctx, 0);
    Sha256Hmac(p_Key, (const uint8_t*)&s_D, sizeof(s_D), p_EncryptionKeyPFS, EKPFS_SIZE);
    fpu_kern_leave(s_Thread, fpu_ctx);
}

void FakePkgManager::GenPfsEncKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[PFS_SEED_SIZE], uint8_t p_Key[PFS_FINAL_KEY_SIZE])
{
    GenPfsCryptoKey(p_EncryptionKeyPFS, p_Seed, 1, p_Key);
}

void FakePkgManager::GenPfsSignKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[PFS_SEED_SIZE], uint8_t p_Key[PFS_FINAL_KEY_SIZE])
{
    GenPfsCryptoKey(p_EncryptionKeyPFS, p_Seed, 2, p_Key);
}

int FakePkgManager::DecryptNpdrmDebugRif(uint32_t p_Type, uint8_t* p_Data)
{
    auto s_Thread = __curthread();
    if (s_Thread == nullptr)
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;

    if (p_Data == nullptr)
    {
        WriteLog(LL_Error, "p_Data is nullptr.");
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;
    }
    
    auto fpu_kern_enter = (int(*)(struct thread *td, struct fpu_kern_ctx *ctx, u_int flags))kdlsym(fpu_kern_enter);
    auto fpu_kern_leave = (int (*)(struct thread *td, struct fpu_kern_ctx *ctx))kdlsym(fpu_kern_leave);
    auto fpu_ctx = (fpu_kern_ctx*)kdlsym(fpu_kern_ctx);
    //auto AesCbcCfb128Encrypt = (int (*)(uint8_t* out, const uint8_t* in, size_t data_size, const uint8_t* key, int key_size, uint8_t* iv))kdlsym(AesCbcCfb128Encrypt);
    auto AesCbcCfb128Decrypt = (int (*)(uint8_t* out, const uint8_t* in, size_t data_size, const uint8_t* key, int key_size, uint8_t* iv))kdlsym(AesCbcCfb128Decrypt);
    
    auto s_Ret = 0;
    fpu_kern_enter(s_Thread, fpu_ctx, 0);
    s_Ret = AesCbcCfb128Decrypt(p_Data + RIF_DIGEST_SIZE, p_Data + RIF_DIGEST_SIZE, RIF_DATA_SIZE, g_RifDebugKey, sizeof(g_RifDebugKey) * 8, p_Data);
    fpu_kern_leave(s_Thread, fpu_ctx);
    if (s_Ret)
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;

    return s_Ret;
}

SblMapListEntry* FakePkgManager::SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa)
{
    if (p_GpuVa == 0)
    {
        WriteLog(LL_Error, "invalid gpu va");
        return nullptr;
    }

    auto _mtx_lock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_lock_flags);
    auto _mtx_unlock_flags = (void(*)(struct mtx *m, int opts, const char *file, int line))kdlsym(_mtx_unlock_flags);
    auto s_SblDrvMsgMtx = (struct mtx*)kdlsym(sbl_drv_msg_mtx);

    SblMapListEntry* s_Entry = *(SblMapListEntry**)kdlsym(gpu_va_page_list);
    SblMapListEntry* s_FinalEntry = nullptr;

    // Lock before we iterate this list, because other paths can absolutely use it concurrently
    _mtx_lock_flags(s_SblDrvMsgMtx, 0, __FILE__, __LINE__);

    while (s_Entry)
    {
        if (s_Entry->gpuVa == p_GpuVa)
        {
            s_FinalEntry = s_Entry;
            break;
        }

        s_Entry = s_Entry->next;
    }

    _mtx_unlock_flags(s_SblDrvMsgMtx, 0, __FILE__, __LINE__);
    return s_FinalEntry;
}

vm_offset_t FakePkgManager::SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups)
{
    if (p_GpuVa == 0)
    {
        WriteLog(LL_Error, "p_GpuVa is nullptr.");
        return 0;
    }

    auto s_Entry = SceSblDriverFindMappedPageListByGpuVa(p_GpuVa);
    if (s_Entry == nullptr)
        return 0;
    
    if (p_NumPageGroups != nullptr)
        *p_NumPageGroups = s_Entry->numPageGroups;
    
    return s_Entry->cpuVa;
}


int FakePkgManager::OnSceSblDriverSendMsg(SblMsg* p_Message, size_t p_Size)
{
    if (p_Message == nullptr)
    {
        WriteLog(LL_Error, "p_Message is nullptr.");
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;
    }

    auto sceSblDriverSendMsg = (int (*)(SblMsg* msg, size_t size))kdlsym(sceSblDriverSendMsg);
    if (p_Message->hdr.cmd != SBL_MSG_CCP)
        return sceSblDriverSendMsg(p_Message, p_Size);
    
    union ccp_op* s_Op = &p_Message->service.ccp.op;
    if (CCP_OP(s_Op->common.cmd) != CCP_OP_AES)
        return sceSblDriverSendMsg(p_Message, p_Size);
    
    uint32_t s_Mask = CCP_USE_KEY_FROM_SLOT | CCP_GENERATE_KEY_AT_SLOT;
    if ((s_Op->aes.cmd & s_Mask) != s_Mask || (s_Op->aes.key_index != PFS_FAKE_OBF_KEY_ID))
        return sceSblDriverSendMsg(p_Message, p_Size);

    s_Op->aes.cmd &= ~CCP_USE_KEY_FROM_SLOT;

    size_t key_len = 16;

    /* reverse key bytes */
    //WriteLog(LL_Debug, "before");
    for (auto i = 0; i < key_len; ++i)
        s_Op->aes.key[i] = g_FakeKeySeed[key_len - i - 1];
    //WriteLog(LL_Debug, "after");

    return sceSblDriverSendMsg(p_Message, p_Size);
}

int FakePkgManager::OnSceSblPfsSetKeys(uint32_t* ekh, uint32_t* skh, uint8_t* eekpfs, Ekc* eekc, uint32_t pubkey_ver, uint32_t key_ver, PfsHeader* hdr, size_t hdr_size, uint32_t type, uint32_t finalized, uint32_t is_disc)
{
    auto sceSblPfsSetKeys = (int(*)(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_Eekpfs, Ekc* p_Eekc, unsigned int p_PubkeyVer, unsigned int p_KeyVer, PfsHeader* p_Header, size_t p_HeaderSize, unsigned int p_Type, unsigned int p_Finalized, unsigned int p_IsDisc))kdlsym(sceSblPfsSetKeys);
    auto RsaesPkcs1v15Dec2048CRT = (int (*)(RsaBuffer* out, RsaBuffer* in, RsaKey* key))kdlsym(RsaesPkcs1v15Dec2048CRT);
    auto fpu_kern_enter = (int(*)(struct thread *td, struct fpu_kern_ctx *ctx, u_int flags))kdlsym(fpu_kern_enter);
    auto fpu_kern_leave = (int (*)(struct thread *td, struct fpu_kern_ctx *ctx))kdlsym(fpu_kern_leave);
    auto sbl_pfs_sx = (struct sx*)kdlsym(sbl_pfs_sx);
    auto fpu_kern_ctx = (struct fpu_kern_ctx*)kdlsym(fpu_kern_ctx);
    //int	(*A_sx_xlock_hard)(struct sx *sx, uintptr_t tid, int opts, const char *file, int line) = kdlsym(_sx_xlock);
	//void (*A_sx_xunlock_hard)(struct sx *sx, uintptr_t tid, const char *file, int line) = kdlsym(_sx_xunlock);
    auto A_sx_xlock_hard = (int (*)(struct sx *sx, int opts))kdlsym(_sx_xlock);
    auto A_sx_xunlock_hard = (int (*)(struct sx *sx))kdlsym(_sx_xunlock);
    auto AesCbcCfb128Encrypt = (int (*)(uint8_t* out, const uint8_t* in, size_t data_size, const uint8_t* key, int key_size, uint8_t* iv))kdlsym(AesCbcCfb128Encrypt);
    auto sceSblKeymgrSetKeyForPfs = (int (*)(SblKeyDesc* key, unsigned int* handle))kdlsym(sceSblKeymgrSetKeyForPfs);
    auto sceSblKeymgrClearKey = (int (*)(uint32_t kh))kdlsym(sceSblKeymgrClearKey);

    struct thread* td;
    RsaBuffer in_data;
    RsaBuffer out_data;
    RsaKey key;
    uint8_t ekpfs[EKPFS_SIZE];
    uint8_t iv[16];
    SblKeyDesc enc_key_desc;
    SblKeyDesc sign_key_desc;
    int32_t ret, orig_ret = 0;

    ret = orig_ret = sceSblPfsSetKeys(ekh, skh, eekpfs, eekc, pubkey_ver, key_ver, hdr, hdr_size, type, finalized, is_disc);
	
	if (ret) {
		if (finalized && is_disc != 0) 
		{
			ret = sceSblPfsSetKeys(ekh, skh, eekpfs, eekc, pubkey_ver, key_ver, hdr, hdr_size, type, finalized, 0); /* always use is_disc=0 here */
			if (ret) {
				ret = orig_ret;
				goto err;
			}
		} else {
			memset(&in_data, 0, sizeof(in_data));
			in_data.ptr = eekpfs;
			in_data.size = EEKPFS_SIZE;

			memset(&out_data, 0, sizeof(out_data));
			out_data.ptr = ekpfs;
			out_data.size = EKPFS_SIZE;

			memset(&key, 0, sizeof(key));
			key.p = (uint8_t*)g_ypkg_p;
			key.q = (uint8_t*)g_ypkg_q;
			key.dmp1 = (uint8_t*)g_ypkg_dmp1;
			key.dmq1 = (uint8_t*)g_ypkg_dmq1;
			key.iqmp = (uint8_t*)g_ypkg_iqmp;

			td = curthread;

			fpu_kern_enter(td, fpu_kern_ctx, 0);
			{
				ret = RsaesPkcs1v15Dec2048CRT(&out_data, &in_data, &key);
			}
			fpu_kern_leave(td, fpu_kern_ctx);

			if (ret) {
				ret = orig_ret;
				goto err;
			}

			A_sx_xlock_hard(sbl_pfs_sx,0);
			{
				memset(&enc_key_desc, 0, sizeof(enc_key_desc));
				{
					enc_key_desc.Pfs.obfuscatedKeyId = PFS_FAKE_OBF_KEY_ID;
					enc_key_desc.Pfs.keySize = sizeof(enc_key_desc.Pfs.escrowedKey);

					GenPfsEncKey(ekpfs, hdr->cryptSeed, enc_key_desc.Pfs.escrowedKey);

					fpu_kern_enter(td, fpu_kern_ctx, 0);
					{
						memset(iv, 0, sizeof(iv));
						ret = AesCbcCfb128Encrypt(enc_key_desc.Pfs.escrowedKey, enc_key_desc.Pfs.escrowedKey, sizeof(enc_key_desc.Pfs.escrowedKey), g_FakeKeySeed, sizeof(g_FakeKeySeed) * 8, iv);
					}
					fpu_kern_leave(td, fpu_kern_ctx);
				}
				if (ret) {
                    WriteLog(LL_Error, "AesCbcCfb128Encrypt returned (%d)", ret);
					A_sx_xunlock_hard(sbl_pfs_sx);
					ret = orig_ret;
					goto err;
				}

				memset(&sign_key_desc, 0, sizeof(sign_key_desc));
				{
					sign_key_desc.Pfs.obfuscatedKeyId = PFS_FAKE_OBF_KEY_ID;
					sign_key_desc.Pfs.keySize = sizeof(sign_key_desc.Pfs.escrowedKey);

					GenPfsSignKey(ekpfs, hdr->cryptSeed, sign_key_desc.Pfs.escrowedKey);

					fpu_kern_enter(td, fpu_kern_ctx, 0);
					{
						memset(iv, 0, sizeof(iv));
						ret = AesCbcCfb128Encrypt(sign_key_desc.Pfs.escrowedKey, sign_key_desc.Pfs.escrowedKey, sizeof(sign_key_desc.Pfs.escrowedKey), g_FakeKeySeed, sizeof(g_FakeKeySeed) * 8, iv);
					}
					fpu_kern_leave(td, fpu_kern_ctx);
				}
				if (ret) {
                    WriteLog(LL_Error, "AesCbcCfb128Encrypt returned (%d).", ret);
					A_sx_xunlock_hard(sbl_pfs_sx);
					ret = orig_ret;
					goto err;
				}

				ret = sceSblKeymgrSetKeyForPfs(&enc_key_desc, ekh);
				if (ret) {
					if (*ekh != 0xFFFFFFFF)
						sceSblKeymgrClearKey(*ekh);

					A_sx_xunlock_hard(sbl_pfs_sx);
					ret = orig_ret;
					goto err;
				}

				ret = sceSblKeymgrSetKeyForPfs(&sign_key_desc, skh);
				if (ret) {
					if (*skh != 0xFFFFFFFF)
						sceSblKeymgrClearKey(*skh);
					A_sx_xunlock_hard(sbl_pfs_sx);
					ret = orig_ret;
					goto err;
				}
			}
			A_sx_xunlock_hard(sbl_pfs_sx);

			ret = 0;
		}
	}

err:
	return ret;
}

int FakePkgManager::OnNpdrmDecryptIsolatedRif(KeymgrPayload* p_Payload)
{
    if (p_Payload == nullptr)
    {
        WriteLog(LL_Error, "p_Payload is nullptr.");
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;
    }

    auto sceSblKeymgrSmCallfunc = (int (*)(KeymgrPayload* payload))kdlsym(sceSblKeymgrSmCallfunc);

    // it's SM request, thus we have the GPU address here, so we need to convert it to the CPU address
    KeymgrRequest* s_Request = reinterpret_cast<KeymgrRequest*>(SceSblDriverGpuVaToCpuVa(p_Payload->data, nullptr));

    // // try to decrypt rif normally 
    int s_Ret = sceSblKeymgrSmCallfunc(p_Payload);
    if ((s_Ret != 0 || p_Payload->status != 0) && s_Request)
    {
        if (s_Request->DecryptRif.type == 0x200)
        {
            // fake?
            s_Ret = DecryptNpdrmDebugRif(s_Request->DecryptRif.type, s_Request->DecryptRif.data);
            p_Payload->status = s_Ret;
            s_Ret = 0;
        }
    }

    return s_Ret;
}

int FakePkgManager::OnNpdrmDecryptRifNew(KeymgrPayload* p_Payload)
{
    if (p_Payload == nullptr)
    {
        WriteLog(LL_Error, "p_Payload is nullptr.");
        return SCE_SBL_ERROR_NPDRM_ENOTSUP;
    }

    auto sceSblKeymgrSmCallfunc = (int (*)(KeymgrPayload* payload))kdlsym(sceSblKeymgrSmCallfunc);

    // it's SM request, thus we have the GPU address here, so we need to convert it to the CPU address 
    uint64_t s_BufferGpuVa = p_Payload->data;
    auto s_Request = reinterpret_cast<KeymgrRequest*>(SceSblDriverGpuVaToCpuVa(s_BufferGpuVa, nullptr));
    auto s_Response = reinterpret_cast<KeymgrResponse*>(s_Request);
    
    // try to decrypt rif normally
    int s_Ret = sceSblKeymgrSmCallfunc(p_Payload);
    int s_OriginalRet = s_Ret;

    // and if it fails then we check if it's fake rif and try to decrypt it by ourselves
    if ((s_Ret != 0 || p_Payload->status != 0) && s_Request)
    {
        if (s_Request->DecryptEntireRif.rif.format != 2)
        { 
            // not fake?
            //s_Ret = s_OriginalRet;
            goto err;
        }

        s_Ret = DecryptNpdrmDebugRif(s_Request->DecryptEntireRif.rif.format, s_Request->DecryptEntireRif.rif.digest);

        if (s_Ret)
        {
            s_Ret = s_OriginalRet;
            goto err;
        }

        /* XXX: sorry, i'm lazy to refactor this crappy code :D basically, we're copying decrypted data to proper place,
        consult with kernel code if offsets needs to be changed */
        //memcpy(s_Response->DecryptEntireRif.raw, s_Request->DecryptEntireRif.rif.digest, sizeof(s_Request->DecryptEntireRif.rif.digest) + sizeof(s_Request->DecryptEntireRif.rif.data));
        memcpy(s_Response->DecryptEntireRif.raw, s_Request->DecryptEntireRif.rif.digest, sizeof(s_Request->DecryptEntireRif.rif.digest));
        memcpy(s_Response->DecryptEntireRif.raw + sizeof(s_Request->DecryptEntireRif.rif.digest), s_Request->DecryptEntireRif.rif.data, sizeof(s_Request->DecryptEntireRif.rif.data));
        
        memset(s_Response->DecryptEntireRif.raw + 
        sizeof(s_Request->DecryptEntireRif.rif.digest) +
        sizeof(s_Request->DecryptEntireRif.rif.data), 
        0,
        sizeof(s_Response->DecryptEntireRif.raw) - 
        (sizeof(s_Request->DecryptEntireRif.rif.digest) + 
        sizeof(s_Request->DecryptEntireRif.rif.data)));

        p_Payload->status = s_Ret;
        //s_Ret = 0;
    }

err:
    return s_Ret;
}

SblKeyRbtreeEntry* FakePkgManager::sceSblKeymgrGetKey(unsigned int p_Handle)
{
    SblKeyRbtreeEntry* s_Entry = *(SblKeyRbtreeEntry**)kdlsym(sbl_keymgr_key_rbtree);

    // NOTE: Do we need to take a lock here of any kind?
    // TODO: Investigate
    while (s_Entry)
    {
        if (s_Entry->handle < p_Handle)
            s_Entry = s_Entry->right;
        else if (s_Entry->handle > p_Handle)
            s_Entry = s_Entry->left;
        else if (s_Entry->handle == p_Handle)
            return s_Entry;
    }

    return nullptr;
}

int FakePkgManager::OnSceSblKeymgrInvalidateKeySxXlock(struct sx* p_Sx, int p_Opts, const char* p_File, int p_Line) 
{
    if (p_Sx == nullptr)
    {
        WriteLog(LL_Error, "invalid lock provided.");
        return 0;
    }
    
    //WriteLog(LL_Debug, "OnSceSblKeymgrInvalidateKeySxXlock");
    auto sceSblKeymgrSetKeyStorage = (int (*)(uint64_t key_gpu_va, unsigned int key_size, uint32_t key_id, uint32_t key_handle))kdlsym(sceSblKeymgrSetKeyStorage);
    auto sblKeymgrKeySlots = (_SblKeySlotQueue *)kdlsym(sbl_keymgr_key_slots);
    auto sblKeymgrBufVa = (uint8_t*)kdlsym(sbl_keymgr_buf_va);
    auto sblKeymgrBufGva = (uint64_t*)kdlsym(sbl_keymgr_buf_gva);
    auto _sx_xlock = (int(*)(struct sx *sx, int opts, const char *file, int line))kdlsym(_sx_xlock);

    SblKeyRbtreeEntry *keyDesc;
    SblKeySlotDesc *keySlotDesc;

    unsigned keyHandle;
    int ret, ret2;

    ret = _sx_xlock(p_Sx, p_Opts, p_File, p_Line);

    if (TAILQ_EMPTY(sblKeymgrKeySlots))
        goto done;

    TAILQ_FOREACH(keySlotDesc, sblKeymgrKeySlots, list)
    {
        keyHandle = keySlotDesc->keyHandle;
        if (keyHandle == (unsigned int) -1) {
            /* unbounded */
            WriteLog(LL_Debug, "unbounded");
            continue;
        }
        keyDesc = sceSblKeymgrGetKey(keyHandle);
        if (!keyDesc) {
            /* shouldn't happen in normal situations */
            WriteLog(LL_Debug, "shouldn't happen in normal situations");
            continue;
        }
        if (!keyDesc->occupied) {
            WriteLog(LL_Debug, "!occupied");
            continue;
        }
        if (keyDesc->desc.Pfs.obfuscatedKeyId != PFS_FAKE_OBF_KEY_ID) {
            /* not our key, just skip, so it will be handled by original code */
            WriteLog(LL_Debug, "not our key, just skip, so it will be handled by original code");
            continue;
        }
        if (keyDesc->desc.Pfs.keySize != sizeof(keyDesc->desc.Pfs.escrowedKey)) {
            /* something weird with key params, just ignore and app will just crash... */
            WriteLog(LL_Debug, "something weird with key params, just ignore and app will just crash...");
            continue;
        }
        memcpy(sblKeymgrBufVa, keyDesc->desc.Pfs.escrowedKey, keyDesc->desc.Pfs.keySize);
        //WriteLog(LL_Debug, "sblKeymgrBufGva %p %p", sblKeymgrBufGva, *sblKeymgrBufGva);
        ret2 = sceSblKeymgrSetKeyStorage(*sblKeymgrBufGva, keyDesc->desc.Pfs.keySize, keyDesc->desc.Pfs.obfuscatedKeyId, keySlotDesc->keyId);
        if (ret2) {
            WriteLog(LL_Debug, "wtf?");
            /* wtf? */
            continue;
        }
    }

done:
    /* XXX: no need to call SX unlock because we'll jump to original code which expects SX is already locked */
    return ret;
}