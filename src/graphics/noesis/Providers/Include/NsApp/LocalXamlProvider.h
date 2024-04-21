////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LOCALXAMLPROVIDER_H__
#define __APP_LOCALXAMLPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsApp/ProvidersApi.h>
#include <NsGui/XamlProvider.h>


class ProviderFileWatcher;

namespace Noesis { template<class T> class Ptr; }

namespace NoesisApp
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A XAML provider that searches in local directories
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_PROVIDERS_API LocalXamlProvider: public Noesis::XamlProvider
{
public:
    LocalXamlProvider(const char* rootPath = "");
    ~LocalXamlProvider();

private:
    /// From XamlProvider
    //@{
    Noesis::Ptr<Noesis::Stream> LoadXaml(const Noesis::Uri& uri) override;
    //@}

private:
    Noesis::Ptr<ProviderFileWatcher> mWatcher;
    char mRootPath[512];
};

NS_WARNING_POP

}

#endif
