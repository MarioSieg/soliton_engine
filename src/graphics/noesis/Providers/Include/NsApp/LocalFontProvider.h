
////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LOCALFONTPROVIDER_H__
#define __APP_LOCALFONTPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsGui/CachedFontProvider.h>
#include <NsApp/ProvidersApi.h>


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A font provider that searches fonts in local directories
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_PROVIDERS_API LocalFontProvider: public Noesis::CachedFontProvider
{
public:
    LocalFontProvider(const char* rootPath = "");

private:
    /// From CachedFontProvider
    //@{
    void ScanFolder(const Noesis::Uri& folder) override;
    Noesis::Ptr<Noesis::Stream> OpenFont(const Noesis::Uri& folder,
        const char* filename) const override;
    //@}

    void ScanFolder(const char* path, const Noesis::Uri& folder, const char* ext);

private:
    char mRootPath[512];
};

}

#endif
