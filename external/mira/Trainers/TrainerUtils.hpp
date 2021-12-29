#pragma once

namespace Mira
{
    namespace Trainers
    {
        class TrainerUtils
        {
        private:
            static bool CompareByteArray(const unsigned char* p_Data, const char* p_Signature)
            {
                for (; *p_Signature; ++p_Signature, ++p_Data)
                {
                    if (*p_Signature == '\x00')
                        continue;
                    if (*p_Data != *p_Signature)
                        return false;
                }
                return true;
            }

            static unsigned int my_strlen(const char *s)
            {
                unsigned int count = 0;

                while(*s!='\0')
                {
                    count++;
                    s++;
                }
                return count;
            }
        public:
            static const unsigned char* FindSignature(const unsigned char* p_StartAddress, unsigned long long p_Size, const char* p_Signature)
            {
                char s_First = p_Signature[0];
                const unsigned char* p_MaxAddress = (p_StartAddress + p_Size) - my_strlen(p_Signature);

                for (; p_StartAddress < p_MaxAddress; ++p_StartAddress)
                {
                    if (*p_StartAddress != s_First)
                        continue;
                    if (CompareByteArray(p_StartAddress, p_Signature))
                        return p_StartAddress;
                }

                return nullptr;
            }
        }
    }
}