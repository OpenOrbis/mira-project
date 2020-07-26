// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

namespace Mira
{
    namespace Plugins
    {
        class TTYRedirector : public Utils::IModule
        {
        private:
            void* m_DeciTTYWrite_orig;

        public:
            TTYRedirector();
            virtual ~TTYRedirector();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static int OnDeciTTYWrite(struct cdev* dev, struct uio* uio, int ioflag);
        };
    }
}