////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/FileTextureProvider.h>
#include <NsCore/Log.h>
#include <NsCore/StringUtils.h>
#include <NsRender/RenderDevice.h>
#include <NsRender/Texture.h>
#include <NsGui/Stream.h>
#include <NsGui/Uri.h>

#include "lunam_stream.hpp"

#include <vector>

NS_MSVC_WARNING_DISABLE(4505)

NS_WARNING_PUSH

#ifdef _PREFAST_
#include <codeanalysis/warnings.h>
NS_MSVC_WARNING_DISABLE(ALL_CODE_ANALYSIS_WARNINGS)
#endif

NS_MSVC_WARNING_DISABLE(4244 4242 4100)
NS_GCC_CLANG_WARNING_DISABLE("-Wconversion")
NS_GCC_CLANG_WARNING_DISABLE("-Wunused-function")
NS_CLANG_WARNING_DISABLE("-Wunknown-warning-option")
NS_CLANG_WARNING_DISABLE("-Wcomma")

#define STBI_MALLOC(sz) Noesis::Alloc(sz)
#define STBI_REALLOC(p,sz) Noesis::Realloc(p, sz)
#define STBI_FREE(p) Noesis::Dealloc(p)
#define STBI_ASSERT(x) NS_ASSERT(x)
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#ifndef NS_PROFILE
  #define STBI_NO_FAILURE_STRINGS
#endif
#include "stb_image.h"

#define STBIR_MALLOC(sz,user) Noesis::Alloc(sz)
#define STBIR_FREE(p,user) Noesis::Dealloc(p)
#define STBIR_ASSERT(x) NS_ASSERT(x)
#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

NS_WARNING_POP

using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
static int Read(void *user, char *data, int size)
{
    Stream* stream = (Stream*)user;
    return stream->Read(data, size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void Skip(void *user, int n)
{
    Stream* stream = (Stream*)user;
    stream->SetPosition((int)stream->GetPosition() + n);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static int Eof(void *user)
{
    Stream* stream = (Stream*)user;
    return stream->GetPosition() >= stream->GetLength();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FileTextureProvider::FileTextureProvider()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TextureInfo FileTextureProvider::GetTextureInfo(const Uri& uri)
{
    TextureInfo info;

    Ptr<Stream> file = OpenStream(uri);
    if (file != 0)
    {
        int x, y, n;
        stbi_io_callbacks callbacks = { Read, Skip, Eof };
        if (stbi_info_from_callbacks(&callbacks, file, &x, &y, &n))
        {
            info.width = x;
            info.height = y;
        }
        else
        {
            NS_LOG_WARNING("%s: %s", uri.Str(), stbi_failure_reason());
        }

        file->Close();
    }

    return info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> FileTextureProvider::LoadTexture(const Uri& uri, RenderDevice* device)
{
    Ptr<Stream> file = OpenStream(uri);
    if (file == 0) { return nullptr; }

    int x, y, n;
    stbi_uc* img;

    std::vector<uint8_t> buffer {};
    auto* lunam_stream = dynamic_cast<noesis_lunam_file_stream*>(file.GetPtr());
    if (!lunam_stream || !lunam_stream->get_stream()) return nullptr;
    lunam_stream->get_stream()->read_all_bytes(buffer);
    const stbi_uc* base = buffer.data();

    img = stbi_load_from_memory(base, buffer.size(), &x, &y, &n, 4);

    if (img == 0)
    {
        NS_LOG_WARNING("%s: %s", uri.Str(), stbi_failure_reason());
        return nullptr;
    }

    //     N=#comp     components
    //       1           grey
    //       2           grey, alpha
    //       3           red, green, blue
    //       4           red, green, blue, alpha
    bool hasAlpha = n == 2 || n == 4;

    // Premultiply alpha
    if (hasAlpha)
    {
        if (device->GetCaps().linearRendering)
        {
            for (int i = 0; i < x * y; i++)
            {
                float a = img[4 * i + 3] / 255.0f;
                float r = stbir__srgb_uchar_to_linear_float[img[4 * i + 0]] * a;
                float g = stbir__srgb_uchar_to_linear_float[img[4 * i + 1]] * a;
                float b = stbir__srgb_uchar_to_linear_float[img[4 * i + 2]] * a;

                img[4 * i + 0] = stbir__linear_to_srgb_uchar(r);
                img[4 * i + 1] = stbir__linear_to_srgb_uchar(g);
                img[4 * i + 2] = stbir__linear_to_srgb_uchar(b);
            }
        }
        else
        {
            for (int i = 0; i < x * y; i++)
            {
                stbi_uc a = img[4 * i + 3];
                img[4 * i + 0] = (stbi_uc)(((uint32_t)img[4 * i] * a) / 255);
                img[4 * i + 1] = (stbi_uc)(((uint32_t)img[4 * i + 1] * a) / 255);
                img[4 * i + 2] = (stbi_uc)(((uint32_t)img[4 * i + 2] * a) / 255);
            }
        }
    }

    // Create Mipmaps
    const void* data[32] = { img };
    uint32_t numLevels = Max(Log2(x), Log2(y)) + 1;

    for (uint32_t i = 1; i < numLevels; i++)
    {
        uint32_t inW = Max(x >> (i - 1), 1);
        uint32_t inH = Max(y >> (i - 1), 1);
        uint32_t outW = Max(x >> i, 1);
        uint32_t outH = Max(y >> i, 1);

        data[i] = Alloc(outW * outH * 4);

        unsigned char* in = (unsigned char*)data[i - 1];
        unsigned char* out = (unsigned char*)data[i];

        int r = stbir_resize_uint8_generic(in, inW, inH, 4 * inW, out, outW, outH, 4 * outW, 4, 3,
            STBIR_FLAG_ALPHA_PREMULTIPLIED, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, 
            STBIR_COLORSPACE_SRGB, nullptr);

        NS_ASSERT(r == 1);
    }

    int index = StrFindLast(uri.Str(), "/");
    const char* label = index == -1 ? uri.Str() : uri.Str() + index + 1;

    // Not using compressed textures is far from ideal but it would complicate the pipeline as
    // having to support and detect availability for all formats in all supported GPUs is not a
    // simple task. Here we are just following the easy path, the texture is created uncompressed
    // using the RenderDevice. This is fine for the App framework and for our examples.
    // For production, we do *not* recommend doing this. Just preprocess your textures, compress
    // them offline and wrap them inside a Texture object using D3D11Factory::WrapTexture or similar

    TextureFormat::Enum format = hasAlpha ? TextureFormat::RGBA8 : TextureFormat::RGBX8;
    Ptr<Texture> texture = device->CreateTexture(label, x, y, numLevels, format, data);

    stbi_image_free(img);
    file->Close();

    for (uint32_t i = 1; i < numLevels; i++)
    {
        Dealloc((void*)data[i]);
    }

    return texture;
}
