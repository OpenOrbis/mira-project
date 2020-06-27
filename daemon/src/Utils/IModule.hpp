namespace Mira
{
    namespace Utils
    {
        class IModule
        {
        public:
            IModule() { }
            virtual ~IModule() { }

            virtual bool OnLoad() { return true; }
            virtual bool OnUnload() { return true; }
            virtual bool OnSuspend() { return true; }
            virtual bool OnResume() { return true; }

            virtual const char* GetName() { return ""; };
            virtual const char* GetDescription() { return ""; };
        };
    }
}