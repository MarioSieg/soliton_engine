////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/Find.h>
#include <NsCore/Memory.h>
#include <NsCore/UTF8.h>
#include <NsCore/StringUtils.h>


#if defined(NS_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>

#elif defined(NS_PLATFORM_SCE)
    #include <kernel.h>
    #include <sceerror.h>

    struct Dir
    {
        int fd;
        int blk_size;
        char *buf;
        int bytes;
        int pos;
    };

#else
    #include <sys/stat.h>
    #include <dirent.h>
    #include <errno.h>
#endif


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::FindFirst(const char* directory, const char* extension, FindData& findData)
{
  #if defined(NS_PLATFORM_WINDOWS)
    char fullPath[sizeof(findData.filename)];
    StrCopy(fullPath, sizeof(fullPath), directory);
    StrAppend(fullPath, sizeof(fullPath), "/*");
    StrAppend(fullPath, sizeof(fullPath), extension);

    uint16_t u16str[sizeof(fullPath)];
    uint32_t numChars = UTF8::UTF8To16(fullPath, u16str, sizeof(fullPath));
    NS_ASSERT(numChars <= sizeof(fullPath));

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileExW((LPCWSTR)u16str, FindExInfoBasic, &fd, FindExSearchNameMatch, 0, 0);
    if (h != INVALID_HANDLE_VALUE)
    {
        numChars = UTF8::UTF16To8((uint16_t*)fd.cFileName, findData.filename, sizeof(fullPath));
        NS_ASSERT(numChars <= sizeof(fullPath));
        StrCopy(findData.extension, sizeof(findData.extension), extension);
        findData.handle = h;
        return true;
    }

    return false;

  #elif defined(NS_PLATFORM_SCE)
    SceKernelMode mode = SCE_KERNEL_S_IRU | SCE_KERNEL_S_IFDIR;
    int fd = sceKernelOpen(directory, SCE_KERNEL_O_RDONLY | SCE_KERNEL_O_DIRECTORY, mode);
    if (fd >= 0)
    {
        SceKernelStat sb = {0};
        int status = sceKernelFstat(fd, &sb);
        NS_ASSERT(status == SCE_OK);

        Dir* dir = (Dir*)Alloc(sizeof(Dir));
        dir->fd = fd;
        dir->blk_size = sb.st_blksize;
        dir->buf = (char*)Alloc(sb.st_blksize);
        dir->bytes = 0;
        dir->pos = 0;

        StrCopy(findData.extension, sizeof(findData.extension), extension);
        findData.handle = dir;

        if (FindNext(findData))
        {
            return true;
        }
        else
        {
            FindClose(findData);
            return false;
        }
    }

    return false;

  #else
    DIR* dir = opendir(directory);

    if (dir != 0)
    {
        StrCopy(findData.extension, sizeof(findData.extension), extension);
        findData.handle = dir;

        if (FindNext(findData))
        {
            return true;
        }
        else
        {
            FindClose(findData);
            return false;
        }
    }

    return false;
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::FindNext(FindData& findData)
{
  #if defined(NS_PLATFORM_WINDOWS)
    WIN32_FIND_DATAW fd;
    int res = FindNextFileW(findData.handle, &fd);
    
    if (res)
    {
        const int MaxFilename = sizeof(findData.filename);
        uint32_t n = UTF8::UTF16To8((uint16_t*)fd.cFileName, findData.filename, MaxFilename);
        NS_ASSERT(n <= MaxFilename);
        return true;
    }

    return false;

  #elif defined(NS_PLATFORM_SCE)
    Dir* dir  = (Dir*)findData.handle;

    while (true)
    {
        if (dir->bytes == 0)
        {
            dir->pos = 0;
            dir->bytes = sceKernelGetdents(dir->fd, dir->buf, dir->blk_size);
            if (dir->bytes == 0)
            {
                return false;
            }
        }

        while (dir->pos < dir->bytes)
        {
            SceKernelDirent *dirent = (SceKernelDirent *)&(dir->buf[dir->pos]);
            dir->pos += dirent->d_reclen;

            if (dirent->d_fileno != 0)
            {
                if (StrCaseEndsWith(dirent->d_name, findData.extension))
                {
                    StrCopy(findData.filename, sizeof(findData.filename), dirent->d_name);
                    return true;
                }
            }
        }

        dir->bytes = 0;
    }

    return false;

  #else
    DIR* dir = (DIR*)findData.handle;

    while (true)
    {
        dirent* entry = readdir(dir);

        if (entry != 0)
        {
            if (StrCaseEndsWith(entry->d_name, findData.extension))
            {
                StrCopy(findData.filename, sizeof(findData.filename), entry->d_name);
                return true;
            }
        }
        else
        {
            return false;
        }
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::FindClose(FindData& findData)
{
  #if defined(NS_PLATFORM_WINDOWS)
    int r = ::FindClose(findData.handle);
    NS_ASSERT(r != 0);

  #elif defined(NS_PLATFORM_SCE)
    Dir* dir  = (Dir*)findData.handle;
    int error = sceKernelClose(dir->fd);
    NS_ASSERT(error == SCE_OK);
    Dealloc(dir->buf);
    Dealloc(dir);

  #else
    DIR* dir = (DIR*)findData.handle;
    int r = closedir(dir);
    NS_ASSERT(r == 0);
  #endif
}
