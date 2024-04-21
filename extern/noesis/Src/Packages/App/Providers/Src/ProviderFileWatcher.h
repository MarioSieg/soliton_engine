////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_PROVIDERFILEWATCHER_H__
#define __APP_PROVIDERFILEWATCHER_H__


#include <NsCore/BaseRefCounted.h>
#include <NsCore/String.h>
#include <NsCore/HashMap.h>
#include <NsCore/Vector.h>
#include <NsGui/Uri.h>
#include <NsApp/FileSystemWatcher.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
class ProviderFileWatcher: public Noesis::BaseRefCounted
{
public:
    using FSW = NoesisApp::FileSystemWatcher;

    void Watch(const char* path, const Noesis::Uri& uri)
    {
        if (mWatchedFiles.Insert(path, uri).second)
        {
            Noesis::FixedString<512> directory;

            int pos = Noesis::StrFindLast(path, "/");
            if (pos != -1)
            {
                directory.Assign(path, pos);
            }

            if (mWatchedDirectories.Insert(directory.Str()).second)
            {
                void* watcher = FSW::Create(directory.Str(), false);
                FSW::Changed(watcher) += MakeDelegate(this, &ProviderFileWatcher::ChangeNotification);
                FSW::Created(watcher) += MakeDelegate(this, &ProviderFileWatcher::ChangeNotification);
                mFileSystemWatchers.PushBack(watcher);
            }
        }
    }

    ~ProviderFileWatcher()
    {
        for (void* watcher: mFileSystemWatchers)
        {
            FSW::Destroy(watcher);
        }
    }

    Noesis::Delegate<void (const Noesis::Uri&)>& Changed()
    {
        return mChangedCallback;
    }

private:
    void ChangeNotification(void* source, const char* filename)
    {
        Noesis::FixedString<512> path = FSW::Path(source);
        if (!path.Empty()) { path += "/"; }
        path += filename;

        auto it = mWatchedFiles.Find(path);
        if (it != mWatchedFiles.End())
        {
            mChangedCallback(it->value);
        }
    }

    Noesis::Vector<void*> mFileSystemWatchers;
    Noesis::HashSet<Noesis::String> mWatchedDirectories;
    Noesis::HashMap<Noesis::String, Noesis::Uri> mWatchedFiles;

    Noesis::Delegate<void (const Noesis::Uri&)> mChangedCallback;
};

#endif
