// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#include "GlobalServices.h"
#include "Log.h"
#include "Console.h"
#include "../Utility/Streams/FileUtils.h"
#include "../Utility/Streams/PathUtils.h"
#include "../Utility/SystemUtils.h"
#include "../Utility/StringFormat.h"
#include <assert.h>
#include <random>

namespace ConsoleRig
{
    static void SetWorkingDirectory()
    {
            //
            //      For convenience, set the working directory to be ../Working 
            //              (relative to the application path)
            //
        nchar_t appPath     [MaxPath];
        nchar_t appDir      [MaxPath];
        nchar_t workingDir  [MaxPath];

        XlGetProcessPath    (appPath, dimof(appPath));
        XlSimplifyPath      (appPath, dimof(appPath), appPath, a2n("\\/"));
        XlDirname           (appDir, dimof(appDir), appPath);
        const auto* fn = a2n("..\\Working");
        XlConcatPath        (workingDir, dimof(workingDir), appDir, fn, &fn[XlStringLen(fn)]);
        XlSimplifyPath      (workingDir, dimof(workingDir), workingDir, a2n("\\/"));
        XlChDir             (workingDir);
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    static auto Fn_GetConsole = ConstHash64<'getc', 'onso', 'le'>::Value;
    static auto Fn_ConsoleMainModule = ConstHash64<'cons', 'olem', 'ain'>::Value;
    static auto Fn_GetAppName = ConstHash64<'appn', 'ame'>::Value;
    static auto Fn_LogCfg = ConstHash64<'logc', 'fg'>::Value;

    static void MainRig_Startup(const StartupConfig& cfg, VariantFunctions& serv)
    {
        std::string appNameString = cfg._applicationName;
        std::string logCfgString = cfg._logConfigFile;
        serv.Add<std::string()>(
            Fn_GetAppName, [appNameString](){ return appNameString; });
        serv.Add<std::string()>(
            Fn_LogCfg, [logCfgString](){ return logCfgString; });

        srand(std::random_device().operator()());

            //
            //      We need to initialize logging output.
            //      The "int" directory stands for "intermediate." We cache processed 
            //      models and textures in this directory
            //      But it's also a convenient place for log files (since it's excluded from
            //      git and it contains only temporary data).
            //      Note that we overwrite the log file every time, destroying previous data.
            //
        CreateDirectoryRecursive("int");

        if (cfg._setWorkingDir)
            SetWorkingDirectory();
    }

    StartupConfig::StartupConfig()
    {
        _applicationName = "XLEApp";
        _logConfigFile = "log.cfg";
        _setWorkingDir = true;
    }

    StartupConfig::StartupConfig(const char applicationName[]) : StartupConfig()
    {
        _applicationName = applicationName;
    }

    static void MainRig_Attach()
    {
        auto& serv = GlobalServices::GetInstance()._services;

        Logging_Startup(
            serv.Call<std::string>(Fn_LogCfg).c_str(), 
            StringMeld<MaxPath>() << "int/log_" << serv.Call<std::string>(Fn_GetAppName) << ".txt");

        if (!serv.Has<ModuleId()>(Fn_ConsoleMainModule)) {
            auto console = std::make_shared<Console>();
            auto currentModule = GetCurrentModuleId();
            serv.Add(
                Fn_GetConsole, 
                [console]() { return console.get(); });
            serv.Add(
                Fn_ConsoleMainModule, 
                [currentModule]() { return currentModule; });
        } else {
            Console::SetInstance(serv.Call<Console*>(Fn_GetConsole));
        }
    }

    static void MainRig_Detach()
    {
            // this will throw an exception if no module has successfully initialised
            // logging
        auto& serv = GlobalServices::GetInstance()._services;
        if (serv.Call<ModuleId>(Fn_ConsoleMainModule) == GetCurrentModuleId()) {
            serv.Remove(Fn_GetConsole);
            serv.Remove(Fn_ConsoleMainModule);
        } else {
            Console::SetInstance(nullptr);
        }

        Logging_Shutdown();
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    GlobalServices::AttachReference::AttachReference(AttachReference&& moveFrom)
    {
        _isAttached = moveFrom._isAttached;
        _attachedServices = moveFrom._attachedServices;
        moveFrom._isAttached = false;
        moveFrom._attachedServices = nullptr;
    }

    auto GlobalServices::AttachReference::operator=(AttachReference&& moveFrom)
        -> GlobalServices::AttachReference&
    {
        _isAttached = moveFrom._isAttached;
        _attachedServices = moveFrom._attachedServices;
        moveFrom._isAttached = false;
        moveFrom._attachedServices = nullptr;
        return *this;
    }

    GlobalServices::AttachReference::AttachReference()
    {
        _isAttached = false;
        _attachedServices = nullptr;
    }

    GlobalServices::AttachReference::AttachReference(GlobalServices& services)
        : _attachedServices(&services)
    {
        GlobalServices::SetInstance(&services);
        ++services._attachReferenceCount;
        _isAttached = true;

        MainRig_Attach();
    }
    
    void GlobalServices::AttachReference::Detach()
    {
        if (_isAttached) {
            MainRig_Detach();

            assert(_attachedServices == &GlobalServices::GetInstance());
            assert(_attachedServices->_attachReferenceCount > 0);
            --_attachedServices->_attachReferenceCount;
            _isAttached = false;
            _attachedServices = nullptr;
            GlobalServices::SetInstance(nullptr);
        }
    }

    GlobalServices::AttachReference::~AttachReference()
    {
        Detach();
    }
    
    static bool CurrentModuleIsAttached = false;

    auto GlobalServices::Attach() -> AttachReference 
    {
        assert(!CurrentModuleIsAttached);
        CurrentModuleIsAttached = true;
        return AttachReference(*this);
    }

    GlobalServices* GlobalServices::s_instance = nullptr;

    GlobalServices::GlobalServices(const StartupConfig& cfg) 
    {
        MainRig_Startup(cfg, _services);

        _attachReferenceCount = 0;
            // we automatically attach for the main module
        _mainAttachReference = Attach();
    }

    GlobalServices::~GlobalServices() 
    {
        _mainAttachReference.Detach();
        assert(_attachReferenceCount == 0);
    }

    void GlobalServices::SetInstance(GlobalServices* instance)
    {
        assert(instance == nullptr || s_instance == nullptr);
        s_instance = instance;
    }

}