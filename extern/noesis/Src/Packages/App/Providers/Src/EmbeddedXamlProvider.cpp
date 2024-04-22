////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/EmbeddedXamlProvider.h>
#include <NsCore/StringUtils.h>
#include <NsGui/MemoryStream.h>
#include <NsGui/Uri.h>

using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
EmbeddedXamlProvider::EmbeddedXamlProvider(ArrayRef<EmbeddedXaml> xamls, XamlProvider* fallback):
    mFallback(fallback)
{
    mXamls.Assign(xamls.Begin(), xamls.End());

    if (fallback != 0)
    {
        fallback->XamlChanged() += [this](const Uri& uri)
        {
            RaiseXamlChanged(uri);
        };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> EmbeddedXamlProvider::LoadXaml(const Uri& uri_)
{
    FixedString<512> uri;
    uri_.GetPath(uri);

    for (const auto& xaml: mXamls)
    {
        if (StrEquals(uri.Str(), xaml.name))
        {
            return *new MemoryStream(xaml.data.Data(), xaml.data.Size());
        }
    }

    return mFallback ? mFallback->LoadXaml(uri_) : nullptr;
}
