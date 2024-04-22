////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_FIND_H__
#define __APP_FIND_H__


#include <NsCore/Noesis.h>
#include <NsApp/ProvidersApi.h>


namespace NoesisApp
{

struct FindData
{
    char filename[512];
    char extension[16];
    void* handle;
};

/// Searches a directory for files with the given extension (like ".ttf")
/// Returns false if no files found
NS_APP_PROVIDERS_API bool FindFirst(const char* directory, const char* extension, FindData& data);

/// Continues a file search from a previous call to the FindFirst function
/// Returns false if no more matching files found
NS_APP_PROVIDERS_API bool FindNext(FindData& data);

/// Closes a file search handle opened by the FindFirst function
NS_APP_PROVIDERS_API void FindClose(FindData& data);

}

#endif
