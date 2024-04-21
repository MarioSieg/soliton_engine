////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LOCALTEXTUREPROVIDER_H__
#define __APP_LOCALTEXTUREPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsApp/FileTextureProvider.h>


class ProviderFileWatcher;

namespace Noesis { template<class T> class Ptr; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A texture provider that searches textures in local directories
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_PROVIDERS_API LocalTextureProvider: public FileTextureProvider
{
public:
    LocalTextureProvider(const char* rootPath = "");
    ~LocalTextureProvider();

private:
    /// From FileTextureProvider
    //@{
    Noesis::Ptr<Noesis::Stream> OpenStream(const Noesis::Uri& uri) const override;
    //@}

private:
    Noesis::Ptr<ProviderFileWatcher> mWatcher;
    char mRootPath[512];
};

NS_WARNING_POP

}

#endif
