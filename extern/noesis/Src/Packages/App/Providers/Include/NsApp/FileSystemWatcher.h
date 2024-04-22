////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_FILESYSTEMWATCHER_H__
#define __APP_FILESYSTEMWATCHER_H__


#include <NsApp/ProvidersApi.h>
#include <NsCore/Delegate.h>


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Listens to the file system change notifications and raises events when a directory, or file
/// in a directory, changes.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_APP_PROVIDERS_API FileSystemWatcher
{
    template<class T> using Delegate = Noesis::Delegate<T>;

    /// Creates a new instance given the specified directory to monitor
    static void* Create(const char* path, bool watchSubtree);

    /// Gets the path of the directory being watched
    static const char* Path(void* watcher);

    /// Occurs when a file or directory in the specified path is changed
    static Delegate<void (void* sender, const char* name)>& Changed(void* watcher);

    /// Occurs when a file or directory in the specified path is created
    static Delegate<void (void* sender, const char* name)>& Created(void* watcher);

    /// Occurs when a file or directory in the specified path is deleted
    static Delegate<void (void* sender, const char* name)>& Deleted(void* watcher);

    /// Occurs when a file or directory in the specified path is renamed
    static Delegate<void (void* sender, const char* oldName, const char* newName)>& Renamed(void* watcher);

    /// Destroys a previously created instance
    static void Destroy(void* watcher);
};

}

#endif
