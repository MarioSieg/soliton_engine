////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LocalTextureProvider.h>
#include <NsCore/Ptr.h>
#include <NsCore/String.h>
#include <NsCore/StringUtils.h>
#include <NsGui/Stream.h>
#include <NsGui/Uri.h>

#include "ProviderFileWatcher.h"


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
LocalTextureProvider::LocalTextureProvider(const char* rootPath)
{
    StrCopy(mRootPath, sizeof(mRootPath), rootPath);

  #ifdef NS_PROFILE
    mWatcher = MakePtr<ProviderFileWatcher>();
    mWatcher->Changed() += [this](const Uri& uri)
    {
        RaiseTextureChanged(uri);
    };
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LocalTextureProvider::~LocalTextureProvider()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> LocalTextureProvider::OpenStream(const Uri& uri) const
{
    FixedString<512> path;
    uri.GetPath(path);

    char filename[512];

    if (StrIsEmpty(mRootPath))
    {
        StrCopy(filename, sizeof(filename), path.Str());
    }
    else
    {
        StrCopy(filename, sizeof(filename), mRootPath);
        StrAppend(filename, sizeof(filename), "/");
        StrAppend(filename, sizeof(filename), path.Str());
    }

    Ptr<Stream> stream = OpenFileStream(filename);

  #ifdef NS_PROFILE
    if (stream)
    {
        mWatcher->Watch(filename, uri);
    }
  #endif

    return stream;
}
