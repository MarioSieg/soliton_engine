////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/EmbeddedFontProvider.h>
#include <NsCore/StringUtils.h>
#include <NsGui/MemoryStream.h>
#include <NsGui/Uri.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
EmbeddedFontProvider::EmbeddedFontProvider(ArrayRef<EmbeddedFont> fonts, FontProvider* fallback):
    mFallback(fallback)
{
    mFonts.Assign(fonts.Begin(), fonts.End());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FontSource EmbeddedFontProvider::MatchFont(const Uri& baseUri, const char* familyName,
    FontWeight& weight, FontStretch& stretch, FontStyle& style)
{
    FontSource match = CachedFontProvider::MatchFont(baseUri, familyName, weight, stretch, style);
    if (match.file == 0 && mFallback != 0)
    {
        match = mFallback->MatchFont(baseUri, familyName, weight, stretch, style);
    }

    return match;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool EmbeddedFontProvider::FamilyExists(const Uri& baseUri, const char* familyName)
{
    bool exists = CachedFontProvider::FamilyExists(baseUri, familyName);
    if (!exists && mFallback != 0)
    {
        exists = mFallback->FamilyExists(baseUri, familyName);
    }

    return exists;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void EmbeddedFontProvider::ScanFolder(const Uri& folder_)
{
    char id[8];

    FixedString<512> folder;
    folder_.GetPath(folder);

    for (uint32_t i = 0; i < mFonts.Size(); i++)
    {
        if (StrEquals(folder.Str(), mFonts[i].folder))
        {
            snprintf(id, sizeof(id), "%d", i);
            RegisterFont(folder_, id);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> EmbeddedFontProvider::OpenFont(const Uri& folder_, const char* filename) const
{
    char id[8];

    FixedString<512> folder;
    folder_.GetPath(folder);

    for (uint32_t i = 0; i < mFonts.Size(); i++)
    {
        if (StrEquals(folder.Str(), mFonts[i].folder))
        {
            snprintf(id, sizeof(id), "%d", i);
            if (StrEquals(filename, id))
            {
                return *new MemoryStream(mFonts[i].data.Data(), mFonts[i].data.Size());
            }
        }
    }

    return nullptr;
}
