////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsRender/RenderContext.h>
#include <NsRender/RenderDevice.h>
#include <NsCore/Factory.h>
#include <NsCore/Error.h>
#include <NsCore/Ptr.h>
#include <NsCore/Sort.h>
#include <NsCore/Symbol.h>
#include <NsCore/String.h>
#include <NsCore/DynamicCast.h>
#include <NsCore/ReflectionImplementEmpty.h>
#include <NsApp/FastLZ.h>

#include <stdio.h>


using namespace Noesis;
using namespace NoesisApp;


static RenderContext* gCurrentContext;


////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderContext> RenderContext::Create()
{
    Vector<Ptr<RenderContext>, 32> contexts;

    for (Symbol id : Factory::EnumComponents(Symbol("RenderContext")))
    {
        contexts.PushBack(StaticPtrCast<RenderContext>(Factory::CreateComponent(id)));
    }

    InsertionSort(contexts.Begin(), contexts.End(), [](RenderContext* c0, RenderContext* c1)
    {
        return c1->Score() < c0->Score();
    });

    for (Ptr<RenderContext>& context : contexts)
    {
        if (context->Validate())
        {
            gCurrentContext = context;
            return context;
        }
    }

    NS_FATAL("No valid render context found");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<RenderContext> RenderContext::Create(const char* name)
{
    char id[256];
    snprintf(id, 256, "%sRenderContext", name);

    Ptr<RenderContext> context = StaticPtrCast<RenderContext>(Factory::CreateComponent(Symbol(id)));

    if (context == nullptr || !context->Validate())
    {
        NS_FATAL("No valid render context found");
    }

    gCurrentContext = context;
    return context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
RenderContext* RenderContext::Current()
{
    return gCurrentContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static ArrayRef<uint8_t> FindLang(const char* lang, ArrayRef<uint8_t> blob)
{
    struct Entry
    {
        uint32_t id;
        uint32_t size;
    };

    uint32_t id = StrHash(lang);
    uint32_t pos = 0;

    while (pos < blob.Size())
    {
        Entry* e = (Entry*)(blob.Data() + pos);
        if (e->id == id)
        {
            // Uncompress
            const uint8_t* start = blob.Data() + pos + sizeof(Entry);
            uint32_t size = FastLZ::DecompressBufferSize(start);
            uint8_t* buffer = (uint8_t*)Alloc(size);
            FastLZ::Decompress(start, e->size, buffer);

            return ArrayRef<uint8_t>(buffer, size);
        }

        pos += sizeof(Entry) + e->size;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static ArrayRef<uint8_t> FindByteCode(const char* name, ArrayRef<uint8_t> blob)
{
    struct Entry
    {
        uint32_t id;
        uint32_t size;
        uint32_t aligned_size;
        uint32_t _;
    };

    uint32_t id = StrHash(name);
    uint32_t pos = 0;

    while (pos < blob.Size())
    {
        Entry* e = (Entry*)(blob.Data() + pos);

        if (e->id == id)
        {
            NS_ASSERT(Alignment<uint64_t>::IsAligned(pos + sizeof(Entry)));
            return ArrayRef<uint8_t>(blob.Data() + pos + sizeof(Entry), e->size);
        }

        pos += sizeof(Entry) + e->aligned_size;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void* CreateShader(RenderContext* context, const char* label, Shader::Enum shader,
    ArrayRef<uint8_t> lang, const char* permutation)
{
    ArrayRef<uint8_t> code = FindByteCode(permutation, lang);

    if (!code.Empty())
    {
        return context->CreatePixelShader(label, (uint8_t)shader, code);
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::EnsureShaders(BrushShaders& shaders, const char* label, ArrayRef<uint8_t> blob)
{
    if (NS_UNLIKELY(!shaders.created))
    {
        shaders.created = true;

        RenderContext* context = RenderContext::Current();
        NS_ASSERT(context != 0);
        ArrayRef<uint8_t> lang = FindLang(context->ShaderLang(), blob);

        if (!lang.Empty())
        {
            shaders.shaders[0] = CreateShader(context, (String(label) + "_Path").Str(),
                Shader::Path_Pattern, lang, "EFFECT_PATH");

            shaders.shaders[1] = CreateShader(context, (String(label) + "_Path_AA").Str(),
                Shader::Path_AA_Pattern, lang, "EFFECT_PATH_AA");

            shaders.shaders[2] = CreateShader(context, (String(label) + "_SDF").Str(),
                Shader::SDF_Pattern, lang, "EFFECT_SDF");

            shaders.shaders[3] = CreateShader(context, (String(label) + "_SDF_LCD").Str(),
                Shader::SDF_LCD_Pattern, lang, "EFFECT_SDF_LCD");

            shaders.shaders[4] = CreateShader(context, (String(label) + "_Opacity").Str(),
                Shader::Opacity_Pattern, lang, "EFFECT_OPACITY");

            Dealloc((void*)lang.Data());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::EnsureShaders(EffectShaders& shaders, const char* label, ArrayRef<uint8_t> blob)
{
    if (NS_UNLIKELY(!shaders.created))
    {
        shaders.created = true;

        RenderContext* context = RenderContext::Current();
        NS_ASSERT(context != 0);
        ArrayRef<uint8_t> lang = FindLang(context->ShaderLang(), blob);

        if (!lang.Empty())
        {
            shaders.shaders[0] = CreateShader(context, label, Shader::Custom_Effect, lang,
                "PAINT_SOLID");

            Dealloc((void*)lang.Data());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const char* RenderContext::ShaderLang() const
{
    return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t RenderContext::Score() const
{
    return 100;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool RenderContext::Validate() const
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::Init(void*, uint32_t&, bool, bool)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::Shutdown()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::SetWindow(void*)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::SaveState()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::BeginRender()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::EndRender()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::Resize()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float RenderContext::GetGPUTimeMs() const
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::SetClearColor(float, float, float, float)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::SetDefaultRenderTarget(uint32_t, uint32_t, bool)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<NoesisApp::Image> RenderContext::CaptureRenderTarget(RenderTarget*) const
{
    NS_ASSERT_UNREACHABLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderContext::Swap()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void* RenderContext::CreatePixelShader(const char*, uint8_t, ArrayRef<uint8_t>)
{
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION_(RenderContext)

NS_END_COLD_REGION
