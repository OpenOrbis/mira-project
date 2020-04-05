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
                
            };

        public:
            // Format
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

        protected:
            self_t m_Self;

        public:
            SelfDecrypt(const char* p_FilePath);
            ~SelfDecrypt();

            bool VerifyHeader();
            bool LoadSegments();
            void Close();
        };
    }
}