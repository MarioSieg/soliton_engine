////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


//#undef NS_LOG_TRACE
//#define NS_LOG_TRACE(...) NS_LOG_(NS_LOG_LEVEL_TRACE, __VA_ARGS__)


#include <NsApp/FileSystemWatcher.h>
#include <NsCore/String.h>
#include <NsCore/Vector.h>
#include <NsCore/UTF8.h>
#include <NsCore/Log.h>

#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifdef NS_PLATFORM_OSX
#include <sys/stat.h>
#include <CoreServices/CoreServices.h>
void RunLoopWakeUp();
#endif


using namespace Noesis;
using namespace NoesisApp;


namespace
{

struct FileSystemWatcherImpl
{
   Delegate<void (void*, const char*)> changedCallback;
   Delegate<void (void*, const char*)> createdCallback;
   Delegate<void (void*, const char*)> deletedCallback;
   Delegate<void (void*, const char*, const char*)> renamedCallback;

   void OnChanged(const char* name)
   {
       NS_LOG_TRACE("[CHANGED] %s", name);
       changedCallback(this, name);
   }

   void OnCreated(const char* name)
   {
       NS_LOG_TRACE("[CREATED] %s", name);
       createdCallback(this, name);
   }

   void OnDeleted(const char* name)
   {
       NS_LOG_TRACE("[DELETED] %s", name);
       deletedCallback(this, name);
   }

   void OnRenamed(const char* oldName, const char* newName)
   {
       NS_LOG_TRACE("[RENAMED] %s => %s", oldName, newName);
       renamedCallback(this, oldName, newName);
   }

    String path;
    bool watchSubtree;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    HANDLE handle;
    DWORD buffer[1024];
    OVERLAPPED overlapped;
    HANDLE timer;
    Vector<String> pendingChanges;

    static bool FileIsLocked(const char* path)
    {
        WCHAR pathW[MAX_PATH];
        UTF8::UTF8To16(path, (uint16_t*)pathW, MAX_PATH);

        HANDLE h = CreateFileW(pathW, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (h == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                // File still being written to
                return true;
            }

            // Unexpected error, remove file from queue
            return false;
        }

        CloseHandle(h);
        return false;
    }

    static void CALLBACK TimerProc_(LPVOID arg, DWORD, DWORD)
    {
        ((FileSystemWatcherImpl*)arg)->TimerProc();
    }

    void TimerProc()
    {
        NS_ASSERT(!pendingChanges.Empty());

        while (!pendingChanges.Empty())
        {
            Noesis::FixedString<MAX_PATH> filename = path.Str();
            if (!filename.Empty()) { filename += "/"; }
            filename += pendingChanges.Back().Str();

            if (!FileIsLocked(filename.Str()))
            {
                OnChanged(pendingChanges.Back().Str());
                pendingChanges.PopBack();
            }
            else
            {
                break;
            }
        }

        if (pendingChanges.Empty())
        {
            BOOL err = CancelWaitableTimer(timer);
            NS_ASSERT(err > 0);
        }
    }

    static void CALLBACK ChangeNotification_(DWORD error, DWORD numBytes, LPOVERLAPPED overlapped)
    {
        ((FileSystemWatcherImpl*)overlapped->hEvent)->ChangeNotification(error, numBytes);
    }

    void ChangeNotification(DWORD error, DWORD numBytes)
    {
        if (error != ERROR_SUCCESS)
        {
            return;
        }

        if (numBytes != 0)
        {
            uint8_t* byteBuffer = (uint8_t*)buffer;
            FILE_NOTIFY_INFORMATION* fni;

            char oldName[MAX_PATH] = "";

            do
            {
                fni = (FILE_NOTIFY_INFORMATION*)byteBuffer;

                NS_ASSERT(fni->FileNameLength < MAX_PATH);
                uint16_t fileNameUTF16[MAX_PATH];
                memcpy(fileNameUTF16, fni->FileName, fni->FileNameLength);
                fileNameUTF16[fni->FileNameLength / sizeof(uint16_t)] = 0;

                char name[MAX_PATH];
                UTF8::UTF16To8(fileNameUTF16, name, MAX_PATH);

                switch (fni->Action)
                {
                    case FILE_ACTION_ADDED:
                    {
                        OnCreated(name);
                        break;
                    }
                    case FILE_ACTION_REMOVED:
                    {
                        OnDeleted(name);
                        break;
                    }
                    case FILE_ACTION_MODIFIED:
                    {
                        if (pendingChanges.Find(name) == pendingChanges.End())
                        {
                            pendingChanges.PushBack(name);

                            if (pendingChanges.Size() == 1)
                            {
                                // ACTION_MODIFIED may be invoked several times while the file
                                // is still locked. A timer is created to periodically check when
                                // it is available. This also avoids double notifications
                                LARGE_INTEGER dueTime;
                                dueTime.QuadPart = -1000000;
                                BOOL err = SetWaitableTimer(timer, &dueTime, 100,
                                    FileSystemWatcherImpl::TimerProc_, this, false);
                                NS_ASSERT(err != 0);
                            }
                        }

                        break;
                    }
                    case FILE_ACTION_RENAMED_OLD_NAME:
                    {
                        // We wish to collapse the poorly done rename notifications from the
                        // ReadDirectoryChangesW API into a nice rename event. So to do that,
                        // it's assumed that a FILE_ACTION_RENAMED_OLD_NAME will be followed
                        // immediately by a FILE_ACTION_RENAMED_NEW_NAME in the buffer
                        NS_ASSERT(StrIsEmpty(oldName));
                        StrCopy(oldName, sizeof(oldName), name);
                        break;
                    }
                    case FILE_ACTION_RENAMED_NEW_NAME:
                    {
                        NS_ASSERT(!StrIsEmpty(oldName));
                        OnRenamed(oldName, name);
                        StrCopy(oldName, sizeof(oldName), "");
                        break;
                    }
                }

                byteBuffer += fni->NextEntryOffset;
            }
            while (fni->NextEntryOffset != 0);
        }

        BOOL err = ReadDirectoryChangesW(handle, buffer, sizeof(buffer), watchSubtree,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &overlapped,
            FileSystemWatcherImpl::ChangeNotification_);
        NS_ASSERT(err > 0);
    }
  #endif

  #ifdef NS_PLATFORM_OSX
    FSEventStreamRef stream;

    static void EventCallback_(ConstFSEventStreamRef streamRef, void* ctx, size_t count,
        void* paths, const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[])
    {
        ((FileSystemWatcherImpl*)ctx)->ProcessEvents(count, (char**)paths, flags, ids);
        RunLoopWakeUp();
    }

    void ProcessEvents(size_t count, char** paths, const FSEventStreamEventFlags flags[],
        const FSEventStreamEventId ids[])
    {
        FSEventStreamEventFlags changedFlags = kFSEventStreamEventFlagItemInodeMetaMod |
            kFSEventStreamEventFlagItemFinderInfoMod | kFSEventStreamEventFlagItemModified |
            kFSEventStreamEventFlagItemChangeOwner | kFSEventStreamEventFlagItemXattrMod;

        int lastRenamedEvent = -1;

        for (int i = 0; i < count; i++)
        {
            if (path == paths[i])
            {
                // Match Windows and don't notify about changes to the root folder
                continue;
            }

            if (i == lastRenamedEvent)
            {
                // Skip if this event is the second in a rename pair
                continue;
            }

            int index = Noesis::StrFindLast(paths[i], "/");

            if ((index > 0) && (watchSubtree || strncmp(path.Str(), paths[i], index - 1) == 0))
            {
                if (flags[i] & changedFlags)
                {
                    OnChanged(paths[i] + index + 1);
                }

                if (flags[i] & kFSEventStreamEventFlagItemCreated)
                {
                    OnCreated(paths[i] + index + 1);
                }

                if (flags[i] & kFSEventStreamEventFlagItemRemoved)
                {
                    OnDeleted(paths[i] + index + 1);
                }

                if (flags[i] & kFSEventStreamEventFlagItemRenamed)
                {
                    if (i < count - 1 && ids[i] + 1 == ids[i + 1] &&
                        flags[i + 1] & kFSEventStreamEventFlagItemRenamed)
                    {
                        OnRenamed(paths[i] + index + 1, paths[i + 1] + index + 1);
                        lastRenamedEvent = i;
                    }
                    else
                    {
                        // Renames without a pair mean the file or directory was moved out from
                        // the watched directory (counts as deleted) or moved in from an external
                        // directory (counts as created)

                        struct stat buf;
                        if (stat(paths[i], &buf) == 0)
                        {
                            if ((flags[i] & kFSEventStreamEventFlagItemCreated) == 0)
                            {
                                OnCreated(paths[i] + index + 1);
                            }
                        }
                        else
                        {
                            if ((flags[i] & kFSEventStreamEventFlagItemRemoved) == 0)
                            {
                                OnDeleted(paths[i] + index + 1);
                            }
                        }
                    }
                }
            }
        }
    }
  #endif
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* FileSystemWatcher::Create(const char* path, bool watchSubtree)
{
    NS_LOG_TRACE("[WATCHING] %s (RECURSIVE=%s)", path, watchSubtree ? "YES" : "NO");

    FileSystemWatcherImpl* impl = new (Placement(), Alloc(sizeof(FileSystemWatcherImpl))) FileSystemWatcherImpl();
    impl->path = path;
    impl->watchSubtree = watchSubtree;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    WCHAR pathW[MAX_PATH];
    UTF8::UTF8To16(path, (uint16_t*)pathW, MAX_PATH);

    impl->handle = CreateFileW(pathW, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);
    NS_ASSERT(impl->handle != INVALID_HANDLE_VALUE);

    impl->overlapped.hEvent = impl;

    BOOL err = ReadDirectoryChangesW(impl->handle, impl->buffer, sizeof(impl->buffer), watchSubtree,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &impl->overlapped,
        FileSystemWatcherImpl::ChangeNotification_);
    NS_ASSERT(err > 0);

    impl->timer = CreateWaitableTimer(NULL, false, NULL);
    NS_ASSERT(impl->timer != 0);
  #endif

  #ifdef NS_PLATFORM_OSX
    CFMutableArrayRef paths = CFArrayCreateMutable(kCFAllocatorDefault, 1, NULL);
    CFStringRef pathRef = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayAppendValue(paths, pathRef);

    FSEventStreamContext ctx = { 0, impl, NULL, NULL, NULL };
    impl->stream = FSEventStreamCreate(NULL, FileSystemWatcherImpl::EventCallback_, &ctx, paths,
        kFSEventStreamEventIdSinceNow, 0.1, kFSEventStreamCreateFlagFileEvents);
    FSEventStreamScheduleWithRunLoop(impl->stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(impl->stream);

    CFRelease(pathRef);
    CFRelease(paths);
  #endif

    return impl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* FileSystemWatcher::Path(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;
    return impl->path.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Delegate<void (void*, const char*)>& FileSystemWatcher::Changed(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;
    return impl->changedCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Delegate<void (void*, const char*)>& FileSystemWatcher::Created(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;
    return impl->createdCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Delegate<void (void*, const char*)>& FileSystemWatcher::Deleted(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;
    return impl->deletedCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Delegate<void (void*, const char*, const char*)>& FileSystemWatcher::Renamed(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;
    return impl->renamedCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FileSystemWatcher::Destroy(void* watcher)
{
    FileSystemWatcherImpl* impl = (FileSystemWatcherImpl*)watcher;

  #ifdef NS_PLATFORM_WINDOWS_DESKTOP
    BOOL ret0 = CloseHandle(impl->handle);
    NS_ASSERT(ret0 != 0);

    BOOL ret1 = CloseHandle(impl->timer);
    NS_ASSERT(ret1 != 0);
  #endif

  #ifdef NS_PLATFORM_OSX
    FSEventStreamStop(impl->stream);
    FSEventStreamInvalidate(impl->stream);
    FSEventStreamRelease(impl->stream);
  #endif

    impl->~FileSystemWatcherImpl();
    Dealloc(impl);
}
