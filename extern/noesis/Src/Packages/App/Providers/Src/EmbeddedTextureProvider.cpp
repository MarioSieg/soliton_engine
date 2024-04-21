////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/EmbeddedTextureProvider.h>
#include <NsCore/StringUtils.h>
#include <NsRender/Texture.h>
#include <NsGui/MemoryStream.h>
#include <NsGui/Uri.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
EmbeddedTextureProvider::EmbeddedTextureProvider(ArrayRef<EmbeddedTexture> textures,
    TextureProvider* fallback): mFallback(fallback)
{
    mTextures.Assign(textures.Begin(), textures.End());

    if (fallback != 0)
    {
        fallback->TextureChanged() += [this](const Uri& uri)
        {
            RaiseTextureChanged(uri);
        };
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TextureInfo EmbeddedTextureProvider::GetTextureInfo(const Uri& uri)
{
    TextureInfo info = FileTextureProvider::GetTextureInfo(uri);
    if (info.width == 0 && info.height == 0 && mFallback != 0)
    {
        info = mFallback->GetTextureInfo(uri);
    }

    return info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> EmbeddedTextureProvider::LoadTexture(const Uri& uri, RenderDevice* device)
{
    Ptr<Texture> texture = FileTextureProvider::LoadTexture(uri, device);
    if (texture == 0 && mFallback != 0)
    {
        texture = mFallback->LoadTexture(uri, device);
    }

    return texture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> EmbeddedTextureProvider::OpenStream(const Uri& uri_) const
{
    FixedString<512> uri;
    uri_.GetPath(uri);

    for (const auto& texture: mTextures)
    {
        if (StrEquals(uri.Str(), texture.name))
        {
            return *new MemoryStream(texture.data.Data(), texture.data.Size());
        }
    }

    return nullptr;
}
