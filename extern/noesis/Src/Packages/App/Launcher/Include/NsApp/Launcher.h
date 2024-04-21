////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LAUNCHER_H__
#define __APP_LAUNCHER_H__


#include <NsCore/Noesis.h>
#include <NsApp/LauncherApi.h>


namespace NoesisApp
{

class CommandLine;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for application launchers
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_LAUNCHER_API Launcher
{
public:
    Launcher();
    Launcher(const Launcher&) = delete;
    Launcher& operator=(const Launcher&) = delete;
    ~Launcher();

    /// Gets the current launcher object
    static Launcher* Current();

    /// Registers app framework components
    static void RegisterAppComponents();

    /// Sets command line arguments
    void SetArguments(int argc, char** argv);

    /// Retrieves command line arguments
    const CommandLine& GetArguments() const;

protected:
    /// Launcher initialization
    void Init() const;

private:
    /// Registers application components
    virtual void RegisterComponents() const;
};

}

#endif
