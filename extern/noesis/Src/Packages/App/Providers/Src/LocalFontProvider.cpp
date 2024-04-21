////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LocalFontProvider.h>
#include <NsApp/Find.h>
#include <NsGui/Stream.h>
#include <NsGui/Uri.h>



using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
LocalFontProvider::LocalFontProvider(const char* rootPath)
{
    StrCopy(mRootPath, sizeof(mRootPath), rootPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocalFontProvider::ScanFolder(const Uri& folder)
{
    char uri[512] = "";

    if (!StrIsNullOrEmpty(mRootPath))
    {
        StrCopy(uri, sizeof(uri), mRootPath);
        StrAppend(uri, sizeof(uri), "/");
    }

    FixedString<512> path;
    folder.GetPath(path);

    StrAppend(uri, sizeof(uri), path.Str());

    ScanFolder(uri, folder, ".ttf");
    ScanFolder(uri, folder, ".otf");
    ScanFolder(uri, folder, ".ttc");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> LocalFontProvider::OpenFont(const Uri& folder, const char* filename) const
{
    char uri[512] = "";

    if (!StrIsNullOrEmpty(mRootPath))
    {
        StrCopy(uri, sizeof(uri), mRootPath);
        StrAppend(uri, sizeof(uri), "/");
    }

    FixedString<512> path;
    folder.GetPath(path);

    StrAppend(uri, sizeof(uri), path.Str());
    StrAppend(uri, sizeof(uri), "/");
    StrAppend(uri, sizeof(uri), filename);

    return OpenFileStream(uri);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocalFontProvider::ScanFolder(const char* path, const Uri& folder, const char* ext)
{
    FindData findData;

    if (FindFirst(path, ext, findData))
    {
        do
        {
            RegisterFont(folder, findData.filename);
        }
        while (FindNext(findData));

        FindClose(findData);
    }
}
