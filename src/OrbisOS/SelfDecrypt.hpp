/*
    Port of self-decryption

    Credits: https://github.com/AlexAltea/orbital/
    AlexAltea, fail0verflow, flatz
*/
#pragma once
#include <Utils/Types.hpp>

namespace Mira
{
    namespace OrbisOS
    {
        class SelfDecrypt
        {
        protected:
            enum
            {
                SELF_MAGIC = 0x1D3D154F,
                SELF_VERSION = 0x0,
                SELF_MODE = 0x1,
                SELF_ENDIANNESS = 0x1,

                SELF_AUTH_INFO_SIZE = 0x88,
                SELF_KEY_SIZE = 0x10,
                SELF_DIGEST_SIZE = 0x20,
                SELF_SEGMENT_BLOCK_ALIGNMENT = 0x10,

                SELF_MAX_CONTEXTS = 0x4,
                
                AUTHMGR_CMD_VERIFY_HEADER = 0x01,
                AUTHMGR_CMD_LOAD_SELF_SEGMENT = 0x02,
                AUTHMGR_CMD_LOAD_SELF_BLOCK  = 0x06,
            };

        public:
            // Format
            typedef struct blob_t 
            {
                struct blob_t *next;
                char *path;
                size_t size;
                uint8_t *data;
            } blob_t;

            typedef struct self_entry_t 
            {
                uint32_t props;
                uint32_t reserved;
                uint64_t offset;
                uint64_t filesz;
                uint64_t memsz;
            } self_entry_t;

            typedef struct self_header_t 
            {
                uint32_t magic;
                uint8_t version;
                uint8_t mode;
                uint8_t endian;
                uint8_t attr;
                uint32_t key_type;
                uint16_t header_size;
                uint16_t meta_size;
                uint64_t file_size;
                uint16_t num_entries;
                uint16_t flags;
                uint32_t reserved;
                self_entry_t entries[0];
            } self_header_t;

            // Context
            struct self_auth_info_t 
            {
                uint8_t buf[0x88];
            };

            typedef struct self_context_t 
            {
                uint32_t format;
                uint32_t elf_auth_type;
                uint32_t total_header_size;
                uint32_t unk_0C;
                void *segment;
                uint32_t unk_18;
                uint32_t ctx_id;
                uint64_t svc_id;
                uint64_t unk_28;
                uint32_t buf_id;
                uint32_t unk_34;
                struct self_header_t *header;
                uint8_t mtx_struct[0x20];
            } self_context_t;

            typedef struct self_t {
                int fd;
                char *file_path;
                size_t file_size;
                size_t entries_size;
                size_t data_offset;
                size_t data_size;
                /* contents */
                struct self_header_t header;
                struct self_entry_t *entries;
                uint8_t *data;
                /* kernel */
                struct self_auth_info_t auth_info;
                struct self_context_t *ctx;
                int auth_ctx_id;
                int ctx_id;
                int svc_id;
                int verified;
                /* blobs */
                struct blob_t *blobs;
            } self_t;


            typedef struct self_block_extent_t {
                uint32_t offset;
                uint32_t size;
            } self_block_extent_t;

            typedef struct self_block_info_t {
                uint32_t size;
                uint16_t index;
                struct self_block_extent_t extent;
                uint8_t digest[SELF_DIGEST_SIZE];
            } self_block_info_t;

            typedef struct sbl_authmgr_verify_header_t 
            {
                uint32_t function;
                uint32_t status;
                uint64_t header_addr;
                uint32_t header_size;
                uint32_t zero_0C;
                uint32_t zero_10;
                uint32_t context_id;
                uint64_t auth_info_addr;
                uint32_t unk_20;
                uint32_t key_id;
                uint8_t key[0x10];
            } sbl_authmgr_verify_header_t;

            typedef struct sbl_authmgr_load_self_segment_t 
            {
                uint32_t function;
                uint32_t status;
                uint64_t chunk_table_addr;
                uint32_t segment_index;
                uint32_t is_block_table;
                uint64_t zero_10;
                uint64_t zero_18;
                uint32_t zero_20;
                uint32_t zero_24;
                uint32_t context_id;
            } sbl_authmgr_load_self_segment_t;

            typedef struct sbl_authmgr_load_self_block_t 
            {
                uint32_t function;
                uint32_t status;
                uint64_t pages_addr;
                uint32_t segment_index;
                uint32_t context_id;
                uint8_t digest[0x20];
                uint8_t extent[0x8];
                uint32_t block_index;
                uint32_t data_offset;
                uint32_t data_size;
                uint64_t data_start_addr;
                uint64_t data_end_addr;
                uint32_t zero;
            } sbl_authmgr_load_self_block_t;

        protected:
            self_t m_Self;

        public:
            SelfDecrypt(const char* p_FilePath);
            ~SelfDecrypt();

            bool VerifyHeader();
            bool LoadSegments();
            void Close();
            bool ReleaseContext();

            bool DecryptSegment(uint8_t* p_InputData, size_t p_InputDataLength, uint64_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_OutputData, uint64_t* p_InOutOutputSize);
            bool DecryptBlock(uint8_t* p_BlobData, uint64_t p_BlobSize);
        };
    }
}