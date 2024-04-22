////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef NS_PLATFORM_WINDOWS_DESKTOP
#pragma warning(disable:4996)
#endif


#include "LenientXamlParser.h"
#include "Workspace.h"

#include <NsCore/UTF8.h>
#include <NsApp/LangServer.h>


#ifdef NS_LANG_SERVER_ENABLED

static bool gParserInitialized = false;
static Noesis::Symbol gDefaultNS;
static Noesis::Symbol gXAMLNS;
static Noesis::Symbol gXMLNS;
static Noesis::Symbol gMarkupCompatNS;
static Noesis::Symbol gPresOptionsNS;
static Noesis::Symbol gInteractivityNS;
static Noesis::Symbol gInteractionsNS;
static Noesis::Symbol gBehaviorsNS;
static Noesis::Symbol gDesignerNS;

static Noesis::Symbol gIgnorableId;

static Noesis::Symbol gNoesisAppId;
static Noesis::Symbol gControlTemplateId;

static constexpr int32_t NoPartIndex = -1;


namespace
{
typedef Noesis::HashMap<Noesis::Symbol, Noesis::Vector<Noesis::Pair<int32_t, uint32_t>, 1>> NSPrefixDefinitionMap;

enum ParseState : uint32_t
{
    ParseState_FindNextTag,
    ParseState_StartTagBegin,
    ParseState_EndTagBegin,
    ParseState_EndTagEnd,
    ParseState_FindAttribute,
    ParseState_AttributeKey,
    ParseState_FindAttributeValue,
    ParseState_AttributeValue,
    ParseState_TagContent,
    ParseState_Comment
};

struct ParserInternalData
{
    NSPrefixDefinitionMap nsPrefixMap;
    int32_t nextPartIndex = 0;
    Noesis::FixedString<128> content;
    uint32_t characterIndex = 0;
    uint32_t lineIndex = 0;
    bool hasRootTag = false;
};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void SetFlag(uint32_t& flags, const uint32_t flag)
{
    flags |= flag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void ClearFlag(uint32_t& flags, const uint32_t flag)
{
    flags &= ~(flag);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool HasFlag(uint32_t flags, const uint32_t flag)
{
    return (flags & flag) == flag;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint8_t Mask8(char c)
{
    return static_cast<uint8_t>(0xff & c);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetParent(NoesisApp::XamlPart& part, const NoesisApp::XamlPart& parentPart)
{
    part.parentPartIndex = parentPart.partIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t NoesisApp::LenientXamlParser::UTF8SequenceLength(const char* text)
{
    NS_ASSERT(text != 0);
    uint8_t lead = Mask8(*text);

    if (lead < 0x80)
    {
        return 1;
    }
    else if ((lead >> 5) == 0x6)
    {
        return 2;
    }
    else if ((lead >> 4) == 0xe)
    {
        return 3;
    }
    else if ((lead >> 3) == 0x1e)
    {
        return 4;
    }

    NS_ASSERT_UNREACHABLE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t NoesisApp::LenientXamlParser::UTF8ByteLength(const char* utf8, uint32_t utf8Length)
{
    NS_ASSERT(utf8 != 0);

    uint32_t byteLength = 0;

    for (uint32_t i = 0; i < utf8Length; i++)
    {
        const uint32_t numBytes = UTF8SequenceLength(utf8);
        utf8 += numBytes;
        byteLength += numBytes;
    }

    return byteLength;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LenientXamlParser::UTF16Advance(const char*& utf8, uint32_t utf16Count)
{
    NS_ASSERT(utf8 != 0);

    uint32_t totalUtf16Count = 0;
    while (totalUtf16Count < utf16Count)
    {
        uint32_t c = Noesis::UTF8::Next(utf8);

        if (c <= 0xffff)
        {
            totalUtf16Count++;
        }
        else
        {
            totalUtf16Count += 2;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t NoesisApp::LenientXamlParser::UTF16Length(const char* utf8)
{
    NS_ASSERT(utf8 != 0);

    uint32_t utf16Length = 0;

    while (utf8 != nullptr)
    {
        const uint32_t c = Noesis::UTF8::Next(utf8);

        if (c <= 0xffff)
        {
            utf16Length++;
        }
        else
        {
            utf16Length += 2;
        }
    }

    return utf16Length;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t NoesisApp::LenientXamlParser::UTF16Length(const char* begin, const char* end)
{
    NS_ASSERT(begin != 0);

    uint32_t utf16Length = 0;

    const char* utf8 = begin;
    while (utf8 != end)
    {
        uint32_t c = Noesis::UTF8::Next(utf8);
        if (c <= 0xffff)
        {
            utf16Length++;
        }
        else
        {
            utf16Length += 2;
        }
    }

    return utf16Length;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsAlpha(const char* c)
{
    const uint32_t cp = Noesis::UTF8::Get(c);

    if (cp == ' ' || cp == '\t' || cp < 'A' || (cp > 'Z' && cp < 'a')
        || (cp > 'z' && cp < 0x007F))
    {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool TryAddPrefix(NoesisApp::LenientXamlParser::Parts& parts, NoesisApp::XamlPart& part,
    NoesisApp::LenientXamlParser::NSDefinitionMap& nsDefinitionMap, ParserInternalData& internalData)
{
    NoesisApp::XamlPart& keyPart = parts[part.parentPartIndex];
    if (keyPart.partKind != NoesisApp::XamlPartKind_AttributeKey ||
        !keyPart.HasFlag(NoesisApp::PartFlags_NSDefinition))
    {
        return false;
    }

    const Noesis::Symbol uriId(internalData.content.Str());

    Noesis::FixedString<256> namespaceValue{};
    if (Noesis::StrStartsWith(internalData.content.Str(), "clr-namespace:"))
    {
        int pos = internalData.content.Find(";");
        if (pos != -1)
        {
            namespaceValue.Append(internalData.content.Begin() + 14, pos - 14);
        }
        else
        {
            namespaceValue.Append(internalData.content.Begin() + 14);
        }

        if (namespaceValue.Empty())
        {
            SetFlag(part.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
            SetFlag(keyPart.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
            return false;
        }
        SetFlag(part.flags, NoesisApp::PartFlags_NSDefinition);
        SetFlag(parts[keyPart.parentPartIndex].flags, NoesisApp::PartFlags_HasNSDefinition);
        nsDefinitionMap[keyPart.parentPartIndex].PushBack({ keyPart.prefixId,
            {NoesisApp::XamlNSKind_Custom, uriId, Noesis::Symbol(namespaceValue.Str()), false } });
        return true;
    }

    NoesisApp::XamlNSKind kind = NoesisApp::XamlNSKind_Undefined;
    Noesis::Symbol clrNamespaceId;

    if (uriId == gDefaultNS)
    {
        if (!keyPart.HasFlag(NoesisApp::PartFlags_DefaultNSDefinition))
        {
            SetFlag(part.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
            SetFlag(keyPart.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
            return false;
        }
        ClearFlag(parts[keyPart.parentPartIndex].errorFlags, NoesisApp::ErrorFlags_MissingDefaultNS);
        SetFlag(part.flags, NoesisApp::PartFlags_DefaultNSDefinition);
        return true;
    }
    else if (uriId == gXAMLNS)
    {
        kind = NoesisApp::XamlNSKind_XAML;
    }
    else if (uriId == gXMLNS)
    {
        kind = NoesisApp::XamlNSKind_XML;
    }
    else if (uriId == gMarkupCompatNS)
    {
        kind = NoesisApp::XamlNSKind_MarkupCompat;
    }
    else if (uriId == gPresOptionsNS)
    {
        kind = NoesisApp::XamlNSKind_PresOptions;
    }
    else if (uriId == gInteractivityNS)
    {
        kind = NoesisApp::XamlNSKind_Interactivity;
        clrNamespaceId = gNoesisAppId;
    }
    else if (uriId == gInteractionsNS)
    {
        kind = NoesisApp::XamlNSKind_Interactions;
        clrNamespaceId = gNoesisAppId;
    }
    else if (uriId == gBehaviorsNS)
    {
        kind = NoesisApp::XamlNSKind_Behaviors;
        clrNamespaceId = gNoesisAppId;
    }
    else if (uriId == gDesignerNS)
    {
        kind = NoesisApp::XamlNSKind_Designer;
    }
    else
    {
        SetFlag(part.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
        SetFlag(keyPart.errorFlags, NoesisApp::ErrorFlags_InvalidNamespace);
        return false;
    }

    SetFlag(part.flags, NoesisApp::PartFlags_NSDefinition);
    SetFlag(parts[keyPart.parentPartIndex].flags, NoesisApp::PartFlags_HasNSDefinition);
    nsDefinitionMap[keyPart.parentPartIndex].PushBack({ keyPart.prefixId,
        { kind, uriId, clrNamespaceId, false } });

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetOpenTagForClose(NoesisApp::LenientXamlParser::Parts& parts,
    NoesisApp::XamlPart& part, int32_t& parentPartIndex)
{
    const bool checkValue = !(part.HasErrorFlag(NoesisApp::ErrorFlags_IdError) && part.content.Empty());
    for (int32_t i = static_cast<int32_t>(parentPartIndex); i >= 0; i--)
    {
        if (parts[i].partKind == NoesisApp::XamlPartKind_StartTagBegin &&
            (!checkValue || (parts[i].IsTypeMatch(part)))
            && HasFlag(parts[i].errorFlags, NoesisApp::ErrorFlags_NoEndTag))
        {
            //part.parentPartIndex = i;
            SetParent(part, parts[i]);
            if (checkValue)
            {
                ClearFlag(part.errorFlags, NoesisApp::ErrorFlags_NoStartTag);
                ClearFlag(parts[i].errorFlags, NoesisApp::ErrorFlags_NoEndTag);
                parentPartIndex = parts[i].parentPartIndex;
            }
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void SetIdErrorContent(const Noesis::BaseString& prefixName, 
    const Noesis::BaseString& typeName, const Noesis::BaseString& propertyName, 
    Noesis::BaseString& content)
{
    if (!prefixName.Empty())
    {
        content.Append(prefixName);
        content.PushBack(':');
    }
    if (!typeName.Empty())
    {
        const int pos = typeName.FindLast(".");
        if (pos != -1)
        {
            content.Append(typeName.Begin() + pos + 1, typeName.Size() - pos - 1);
        }
        else
        {
            content.Append(typeName);
        }
    }
    if (!propertyName.Empty())
    {
        content.PushBack('.');
        content.Append(propertyName);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool GetTextPositions(NoesisApp::DocumentContainer& document, NoesisApp::XamlPart& part, 
    uint32_t& startPosition, uint32_t& endPosition)
{
    startPosition = document.lineStartPositions[part.startLineIndex] + part.startCharacterIndex;
    endPosition = document.lineStartPositions[part.endLineIndex] + part.endCharacterIndex;

    const char* begin = document.text.Begin();
    for (; startPosition <= endPosition; startPosition++)
    {
        const char c = *(begin + startPosition);
        if (c != '<' && c != '>' && c != '/')
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessNSDefinition(NoesisApp::DocumentContainer& document, NoesisApp::XamlPart& part,
    NoesisApp::LenientXamlParser::Parts& parts,
    NoesisApp::LenientXamlParser::NSDefinitionMap& nsDefinitionMap, ParserInternalData& internalData)
{
    if (part.partKind == NoesisApp::XamlPartKind_AttributeKey)
    {
        uint32_t startPosition;
        uint32_t endPosition;
        if (!GetTextPositions(document, part, startPosition, endPosition))
        {
            return;
        }
        const char* begin = document.text.Begin() + startPosition;
        if (Noesis::StrStartsWith(begin, "xmlns"))
        {
            if (endPosition - startPosition == 4)
            {
                SetFlag(part.flags, NoesisApp::PartFlags_DefaultNSDefinition);
            }
            else if (*(begin + 5) != ':' || endPosition - startPosition == 5)
            {
                SetFlag(part.errorFlags, NoesisApp::ErrorFlags_InvalidPrefixId);
            }
            else
            {
                part.prefixId = Noesis::Symbol(Noesis::String(begin + 6, 
                    document.text.Begin() + 1 + endPosition).Str());
                SetFlag(part.flags, NoesisApp::PartFlags_NSDefinition);
                return;
            }
            return;
        }
    }
    
    if (!internalData.content.Empty())
    {
        internalData.content.RTrim();
        if (part.partKind != NoesisApp::XamlPartKind_AttributeValue
            || !TryAddPrefix(parts, part, nsDefinitionMap, internalData))
        {
            part.content = internalData.content.Str();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddPart(NoesisApp::DocumentContainer& document, NoesisApp::LenientXamlParser::Parts& parts,
     NoesisApp::LenientXamlParser::LinePartIndicies& lineParts, NoesisApp::XamlPart& part,
     NoesisApp::LenientXamlParser::NSDefinitionMap& nsDefinitionMap, ParserInternalData& internalData,
     NoesisApp::XamlPartKind newPartKind)
{
    if (part.partKind != NoesisApp::XamlPartKind_Undefined)
    {
        if (!internalData.hasRootTag && part.partKind != NoesisApp::XamlPartKind_Comment)
        {
            NS_ERROR("XAML document must start with valid tag");
            return;
        }

        ProcessNSDefinition(document, part, parts, nsDefinitionMap, internalData);
        internalData.content.Clear();

        for (uint32_t i = lineParts.Size(); i <= part.endLineIndex; i++)
        {
            lineParts.PushBack(part.partIndex);
        }

        parts.PushBack(part);
    }

    part = {};
    part.partKind = newPartKind;
    part.partIndex = internalData.nextPartIndex++;
    part.parentPartIndex = NoPartIndex;
    part.startLineIndex = internalData.lineIndex;
    part.startCharacterIndex = internalData.characterIndex;
    
    if (part.partKind == NoesisApp::XamlPartKind_AttributeKey)
    {
        SetFlag(part.errorFlags, NoesisApp::ErrorFlags_MissingValue 
            | NoesisApp::ErrorFlags_NoAttributeEquals | NoesisApp::ErrorFlags_NoAttributeValue);
    }
    else if (part.partKind == NoesisApp::XamlPartKind_AttributeValue)
    {
        SetFlag(part.errorFlags, NoesisApp::ErrorFlags_Unclosed);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void ParserInit()
{
    gDefaultNS = Noesis::Symbol::Static("http://schemas.microsoft.com/winfx/2006/xaml/presentation");
    gXAMLNS = Noesis::Symbol::Static("http://schemas.microsoft.com/winfx/2006/xaml");
    gXMLNS = Noesis::Symbol::Static("http://www.w3.org/XML/1998/namespace");
    gMarkupCompatNS = Noesis::Symbol::Static("http://schemas.openxmlformats.org/markup-compatibility/2006");
    gPresOptionsNS =
        Noesis::Symbol::Static("http://schemas.microsoft.com/winfx/2006/xaml/presentation/options");
    gInteractivityNS =
        Noesis::Symbol::Static("http://schemas.microsoft.com/expression/2010/interactivity");
    gInteractionsNS = Noesis::Symbol::Static("http://schemas.microsoft.com/expression/2010/interactions");
    gBehaviorsNS = Noesis::Symbol::Static("http://schemas.microsoft.com/xaml/behaviors");
    gDesignerNS = Noesis::Symbol::Static("http://schemas.microsoft.com/expression/blend/2008");

    gIgnorableId = Noesis::Symbol("Ignorable");

    gNoesisAppId = Noesis::Symbol("NoesisApp");
    gControlTemplateId = Noesis::Symbol("ControlTemplate");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static const NoesisApp::XamlNamespace* GetPrefixNamespace(Noesis::Symbol prefixId,
    NoesisApp::LenientXamlParser::NSDefinitionMap& nsDefinitionMap, NSPrefixDefinitionMap& nsPrefixMap)
{
    const auto prefixIt = nsPrefixMap.Find(prefixId);
    if (prefixIt == nsPrefixMap.End())
    {
        return nullptr;
    }
    const Noesis::Pair<unsigned, unsigned>& locPair = prefixIt->value.Back();
    const auto defIt = nsDefinitionMap.Find(locPair.first);
    if (defIt == nsDefinitionMap.End() || locPair.second >= defIt->value.Size())
    {
        return nullptr;
    }
    return &defIt->value[locPair.second].second;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void CullPrefixNamespaces(NSPrefixDefinitionMap& nsPrefixMap,
    int32_t parentPartIndex)
{
    for (auto& prefixEntry : nsPrefixMap)
    {
        for (int32_t i = static_cast<int32_t>(prefixEntry.value.Size()) - 1;
            i >= 0 && prefixEntry.value[i].first > parentPartIndex; i--)
        {
            prefixEntry.value.PopBack();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void PopulateIds(NoesisApp::DocumentContainer& document, NoesisApp::XamlPart& typedPart,
    NoesisApp::LenientXamlParser::Parts& parts, NoesisApp::LenientXamlParser::NSDefinitionMap& nsDefinitionMap,
    NSPrefixDefinitionMap& nsPrefixMap)
{
    Noesis::FixedString<16> prefixName;
    Noesis::FixedString<32> typeName;
    Noesis::FixedString<32> propertyName;

    uint32_t startPosition;
    uint32_t endPosition;
    if (!GetTextPositions(document, typedPart, startPosition, endPosition))
    {
        SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidTypeId);
        return;
    }

    const char* begin = document.text.Begin();
    for (uint32_t j = startPosition; j <= endPosition; j++)
    {
        const char* c = begin + j;

        if (*c == ' ' || *c == '>')
        {
            break;
        }
        if (*c == ':')
        {
            prefixName.Assign(propertyName);
            propertyName.Clear();
        }
        else if (*c == '.')
        {
            typeName.Assign(propertyName);
            propertyName.Clear();
        }
        else
        {
            propertyName.PushBack(*c);
        }
    }

    if (typedPart.partKind != NoesisApp::XamlPartKind_AttributeKey && typeName.Empty()
        && !propertyName.Empty())
    {
        typeName.Assign(propertyName);
        propertyName.Clear();
    }

    if (!propertyName.Empty())
    {
        if (typedPart.partKind == NoesisApp::XamlPartKind_StartTagBegin
            && typedPart.partIndex != typedPart.parentPartIndex)
        {
            SetFlag(typedPart.flags, NoesisApp::PartFlags_IsNodeProperty);
            SetFlag(parts[typedPart.parentPartIndex].flags, NoesisApp::PartFlags_HasNodeProperty);
        }
        else if (typedPart.partKind == NoesisApp::XamlPartKind_AttributeKey)
        {
            SetFlag(parts[typedPart.parentPartIndex].flags, NoesisApp::PartFlags_HasAttributeProperty);
        }
    }

    const NoesisApp::XamlNamespace* xamlNamespace = nullptr;
    if (!prefixName.Empty())
    {
        typedPart.prefixId = Noesis::Symbol(prefixName.Str(), Noesis::Symbol::NullIfNotFound());
        if (typedPart.prefixId.IsNull())
        {
            SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidPrefixId);
            SetIdErrorContent(prefixName, typeName, propertyName, typedPart.content);
            return;
        }
        xamlNamespace =
            GetPrefixNamespace(typedPart.prefixId, nsDefinitionMap, nsPrefixMap);
        if (xamlNamespace == nullptr)
        {
            SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidPrefixId);
            SetIdErrorContent(prefixName, typeName, propertyName, typedPart.content);
            return;
        }
        if (xamlNamespace->ignorable)
        {
            SetFlag(typedPart.flags, NoesisApp::PartFlags_Ignorable);
        }
    }

    if (typeName.Empty())
    {
        if (typedPart.partKind == NoesisApp::XamlPartKind_AttributeKey)
        {
            if (parts[typedPart.parentPartIndex].HasErrorFlag(NoesisApp::ErrorFlags_InvalidTypeId))
            {
                SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidTypeId);
            }
            else
            {
                typedPart.typeId = parts[typedPart.parentPartIndex].typeId;
                typeName = typedPart.typeId.Str();
            }
        }
        else
        {
            SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidTypeId);
            SetIdErrorContent(prefixName, typeName, propertyName, typedPart.content);
        }
    }
    else
    {
        bool ignorable = false;
        if (xamlNamespace != nullptr)
        {
            ignorable = xamlNamespace->ignorable;
            if (!xamlNamespace->clrNamespaceId.IsNull())
            {
                typeName.Insert(0, ".");
                typeName.Insert(0, xamlNamespace->clrNamespaceId.Str());
            }
        }
        if (ignorable)
        {
            typedPart.typeId = Noesis::Symbol(typeName.Str());
        }
        else
        {
            typedPart.typeId = Noesis::Symbol(typeName.Str(), Noesis::Symbol::NullIfNotFound());
        }
        if (typedPart.typeId.IsNull())
        {
            SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidTypeId);
            SetIdErrorContent(prefixName, typeName, propertyName, typedPart.content);
            return;
        }
        if (typedPart.partKind == NoesisApp::XamlPartKind_StartTagBegin
            && typedPart.typeId == gControlTemplateId)
        {
            SetFlag(typedPart.flags, NoesisApp::PartFlags::PartFlags_ControlTemplate);
        }
    }

    if (!propertyName.Empty())
    {
        if (xamlNamespace != nullptr && xamlNamespace->ignorable)
        {
            typedPart.propertyId = Noesis::Symbol(propertyName.Str());
        }
        else
        {
            typedPart.propertyId = Noesis::Symbol(propertyName.Str(), Noesis::Symbol::NullIfNotFound());
        }
        if (typedPart.propertyId.IsNull())
        {
            SetFlag(typedPart.errorFlags, NoesisApp::ErrorFlags_InvalidPropertyId);
            SetIdErrorContent(prefixName, typeName, propertyName, typedPart.content);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LenientXamlParser::ParseXaml(DocumentContainer& document, Parts& parts,
    LinePartIndicies& lineParts, NSDefinitionMap& nsDefinitionMap)
{
    if (NS_UNLIKELY(!gParserInitialized))
    {
        ParserInit();
    }

    ParserInternalData internalData;

    const char* c = document.text.Begin();

    XamlPart part;
    part.partKind = XamlPartKind_Undefined;

    ParseState parseState = ParseState_FindNextTag;

    int32_t currentStartTagPartIndex = NoPartIndex;
    
    uint32_t priorSequenceLength = 0;

    bool hasWindowLineEndings = false;
    bool hasPosixLineEndings = false;
    while (c != document.text.End())
    {
        bool lineBreak = false;
        while (*c == '\r' && *(c + 1) == '\n')
        {
            if (hasPosixLineEndings)
            {
                NS_ERROR("Document has mixed line endings");
                return;
            }
            hasWindowLineEndings = true;
            c += 2;
            internalData.lineIndex++;
            internalData.characterIndex = 0;
            lineBreak = true;
        }
        while (*c == '\n')
        {
            if (hasWindowLineEndings)
            {
                NS_ERROR("Document has mixed line endings");
                return;
            }
            hasPosixLineEndings = true;
            c++;
            internalData.lineIndex++;
            internalData.characterIndex = 0;
            lineBreak = true;
        }

        if (c == document.text.End())
        {
            break;
        }

        if (*c == '<')
        {
            AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                    XamlPartKind_Undefined);

            if (*(c + 1) == '!')
            {
                c++;
                internalData.characterIndex++;

                part.partKind = XamlPartKind_Comment;
                SetFlag(part.errorFlags, ErrorFlags_Unclosed);

                if (*(c + 1) == '-')
                {
                    c++;
                    internalData.characterIndex++;

                    if (*(c + 1) == '-')
                    {
                        c++;
                        internalData.characterIndex++;
                    }
                    else
                    {
                        SetFlag(part.errorFlags, ErrorFlags_InvalidSyntax);
                    }
                }
                else
                {
                    SetFlag(part.errorFlags, ErrorFlags_InvalidSyntax);
                }

                parseState = ParseState_Comment;
            }
            else if (*(c + 1) == '/')
            {
                if (!internalData.hasRootTag)
                {
                    NS_ERROR("XAML document root can not be an end tag");
                    return;
                }
                c++;
                internalData.characterIndex++;

                if (c + 1 != document.text.End() && !IsAlpha(c + 1))
                {
                    SetFlag(part.errorFlags, ErrorFlags_InvalidSyntax);
                }

                part.partKind = XamlPartKind_EndTag;
                SetFlag(part.errorFlags, ErrorFlags_MissingValue | ErrorFlags_Unclosed 
                    | ErrorFlags_NoStartTag);
                part.parentPartIndex = NoPartIndex;
                parseState = ParseState_EndTagBegin;
            }
            else
            {
                part.partKind = XamlPartKind_StartTagBegin;
                SetFlag(part.errorFlags, ErrorFlags_MissingValue | ErrorFlags_Unclosed 
                    | ErrorFlags_NoEndTag);
                if (!IsAlpha(c + 1))
                {
                    SetFlag(part.errorFlags, ErrorFlags_InvalidSyntax);
                }
                if (!internalData.hasRootTag)
                {
                    SetFlag(part.errorFlags, ErrorFlags_MissingDefaultNS);
                }
                internalData.hasRootTag = true;
                currentStartTagPartIndex = part.partIndex;

                parseState = ParseState_StartTagBegin;
            }

            part.endLineIndex = internalData.lineIndex;
            part.endCharacterIndex = internalData.characterIndex;

            c++;
            internalData.characterIndex++;
            continue;
        }
        if ((*c == '/' || *c == '>')
            && (parseState == ParseState_StartTagBegin || parseState == ParseState_FindAttribute 
                || parseState == ParseState_AttributeKey 
                || parseState == ParseState_FindAttributeValue))
        {
            AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                    XamlPartKind_StartTagEnd);    
            
            if (*c == '/')
            {
                SetFlag(part.flags, PartFlags_IsSelfEnding);

                c++;
                internalData.characterIndex++;
            }
            
            parseState = ParseState_FindNextTag;

            part.endLineIndex = internalData.lineIndex;
            part.endCharacterIndex = internalData.characterIndex;

            if (*c == '>')
            {
                c++;
                internalData.characterIndex++;
            }
            else if (HasFlag(part.flags, PartFlags_IsSelfEnding))
            {
                SetFlag(part.errorFlags, ErrorFlags_MissingBracket);
            }
            continue;
        }

        switch (parseState)
        {
            case ParseState_FindNextTag:
            {
                if (*c != ' ' && internalData.hasRootTag)
                {
                    AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                            XamlPartKind_TagContent);
                    
                    c -= priorSequenceLength;
                    internalData.characterIndex -= priorSequenceLength;

                    parseState = ParseState_TagContent;
                }
                break;
            }
            case ParseState_TagContent:
            {
                internalData.content.PushBack(*c);
                part.endLineIndex = internalData.lineIndex;
                part.endCharacterIndex = internalData.characterIndex;
                break;
            }
            case ParseState_StartTagBegin:
            {
                if (lineBreak)
                {
                    c--;
                    internalData.characterIndex--;
                    parseState = ParseState_FindAttribute;
                }
                else if (*c == ' ')
                {
                    parseState = ParseState_FindAttribute;
                }
                else
                {
                    ClearFlag(part.errorFlags, ErrorFlags_MissingValue);
                    part.endLineIndex = internalData.lineIndex;
                    part.endCharacterIndex = internalData.characterIndex;
                }
                break;
            }
            case ParseState_FindAttribute:
            {
                if (*c != ' ')
                {
                    if (*c == '\'' || *c == '"' || *c == '=')
                    {
                        c--;
                        internalData.characterIndex--;
                        parseState = ParseState_FindAttributeValue;
                        break;
                    }

                    AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                            XamlPartKind_AttributeKey);
                    
                    if (!IsAlpha(c))
                    {
                        SetFlag(part.errorFlags, ErrorFlags_InvalidSyntax);
                    }
                    
                    c -= priorSequenceLength;
                    internalData.characterIndex -= priorSequenceLength;

                    //part.parentPartIndex = currentStartTagPartIndex;
                    SetParent(part, parts[currentStartTagPartIndex]);
                    
                    parseState = ParseState_AttributeKey;
                }
                break;
            }
            case ParseState_AttributeKey:
            {
                if (*c == ' ')
                {
                    parseState = ParseState_FindAttributeValue;
                    break;
                }
                if (lineBreak || *c == '"' || *c == '\'' || *c == '=')
                {
                    c--;
                    internalData.characterIndex--;
                    parseState = ParseState_FindAttributeValue;
                    break;
                }

                part.endLineIndex = internalData.lineIndex;
                part.endCharacterIndex = internalData.characterIndex;
                
                ClearFlag(part.errorFlags, ErrorFlags_MissingValue);
                break;
            }
            case ParseState_FindAttributeValue:
            {
                if (*c == ' ')
                {
                    break;
                }
                if (*c == '=')
                {
                    int32_t attributeParentIndex = currentStartTagPartIndex;
                    bool hasAttributeKey = false;
                    bool isDuplicate = false;
                    if (part.partKind == XamlPartKind_AttributeKey)
                    {
                        hasAttributeKey = true;
                        attributeParentIndex = part.partIndex;
                        ClearFlag(part.errorFlags, ErrorFlags_NoAttributeEquals);
                    }
                    else if (part.partKind == XamlPartKind_AttributeEquals)
                    {
                        isDuplicate = true;
                        hasAttributeKey = !HasFlag(part.errorFlags, ErrorFlags_NoAttributeKey);
                        attributeParentIndex = part.parentPartIndex;
                    }
                    
                    AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                            XamlPartKind_AttributeEquals);

                    if (!hasAttributeKey)
                    {
                        SetFlag(part.errorFlags, ErrorFlags_NoAttributeKey);
                    }
                    if (isDuplicate)
                    {
                        SetFlag(part.errorFlags, ErrorFlags_Duplicate);
                    }
                    
                    //part.parentPartIndex = attributeParentIndex;
                    SetParent(part, parts[attributeParentIndex]);

                    part.endLineIndex = internalData.lineIndex;
                    part.endCharacterIndex = internalData.characterIndex;
                }
                else if (*c == '\'' || *c == '"')
                {
                    int32_t attributeParentIndex = currentStartTagPartIndex;
                    bool hasAttributeKey = false;
                    if (part.partKind == XamlPartKind_AttributeKey)
                    {
                        attributeParentIndex = part.partIndex;
                        hasAttributeKey = true;
                    }
                    else if (part.partKind == XamlPartKind_AttributeEquals)
                    {
                        attributeParentIndex = part.parentPartIndex;
                        hasAttributeKey = !HasFlag(part.errorFlags, ErrorFlags_NoAttributeKey);
                    }
                    else
                    {
                        hasAttributeKey = false;
                    }

                    AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
                            XamlPartKind_AttributeValue);

                    if (!hasAttributeKey)
                    {
                        SetFlag(part.errorFlags, ErrorFlags_NoAttributeKey);
                    }
                    else
                    {
                        ClearFlag(parts[attributeParentIndex].errorFlags,
                            ErrorFlags_NoAttributeValue);
                    }
                                        
                    //part.parentPartIndex = attributeParentIndex;
                    SetParent(part, parts[attributeParentIndex]);

                    part.endLineIndex = internalData.lineIndex;
                    part.endCharacterIndex = internalData.characterIndex;

                    if (*c == '\'')
                    {
                        SetFlag(part.flags, PartFlags_SingleQuotes);
                    }

                    parseState = ParseState_AttributeValue;
                }
                else
                {
                    c -= priorSequenceLength;
                    internalData.characterIndex -= priorSequenceLength;
                    parseState = ParseState_FindAttribute;
                }
                break;
            }
            case ParseState_AttributeValue:
            {
                if ((HasFlag(part.flags, PartFlags_SingleQuotes) && *c == '\'') 
                    || (!HasFlag(part.flags, PartFlags_SingleQuotes) && *c == '"'))
                {
                    ClearFlag(part.errorFlags, ErrorFlags_Unclosed);
                    parseState = ParseState_FindAttribute;
                }
                else
                {
                    internalData.content.PushBack(*c);
                }
                part.endLineIndex = internalData.lineIndex;
                part.endCharacterIndex = internalData.characterIndex;
                break;
            }
            case ParseState_EndTagBegin:
            {
                if (lineBreak)
                {
                    c--;
                    internalData.characterIndex--;
                    parseState = ParseState_EndTagEnd;
                }
                else if (*c == '>')
                {
                    c--;
                    internalData.characterIndex--;
                    parseState = ParseState_EndTagEnd;
                }
                else if (*c == ' ')
                {
                    parseState = ParseState_EndTagEnd;
                }
                else
                {
                    ClearFlag(part.errorFlags, ErrorFlags_MissingValue);
                    part.endLineIndex = internalData.lineIndex;
                    part.endCharacterIndex = internalData.characterIndex;
                }
                break;
            }
            case ParseState_EndTagEnd:
            {
                part.endLineIndex = internalData.lineIndex;
                part.endCharacterIndex = internalData.characterIndex;
                
                if (*c == '>')
                {
                    ClearFlag(part.errorFlags, ErrorFlags_Unclosed);
                    parseState = ParseState_FindNextTag;
                }
                else if (*c != ' ')
                {
                    SetFlag(part.errorFlags, ErrorFlags_HasInvalidContent);
                }
                break;
            }
            case ParseState_Comment:
            {
                if (c + 2 < document.text.End() && *c == '-' && *(c + 1) == '-' && *(c + 2) == '>')
                {
                    c += 2;
                    internalData.characterIndex += 2;
                    ClearFlag(part.errorFlags, ErrorFlags_Unclosed);
                    parseState = ParseState_FindNextTag;
                }

                part.endLineIndex = internalData.lineIndex;
                part.endCharacterIndex = internalData.characterIndex;
                break;
            }
        }        

        uint32_t sequenceLength = UTF8SequenceLength(c);
        c += sequenceLength;
        internalData.characterIndex += sequenceLength;
        priorSequenceLength = sequenceLength;
    }

    AddPart(document, parts, lineParts, part, nsDefinitionMap, internalData,
            XamlPartKind_Undefined);

    int32_t parentPartIndex = NoPartIndex;
    int32_t count = static_cast<int32_t>(parts.Size());
    for (int32_t i = 0; i < count; i++)
    {
        XamlPart& typedPart = parts[i];

        if (typedPart.parentPartIndex == NoPartIndex && parentPartIndex != NoPartIndex)
        {
            //typedPart.parentPartIndex = parentPartIndex;
            SetParent(typedPart, parts[parentPartIndex]);
        }

        if (typedPart.partKind == XamlPartKind_StartTagEnd)
        {
            if (!HasFlag(parts[typedPart.parentPartIndex].errorFlags, ErrorFlags_Unclosed))
            {
                SetFlag(part.errorFlags, ErrorFlags_Duplicate);
            }
            ClearFlag(parts[typedPart.parentPartIndex].errorFlags, ErrorFlags_Unclosed);
            if (typedPart.HasFlag(PartFlags_IsSelfEnding))
            {
                SetFlag(parts[typedPart.parentPartIndex].flags, PartFlags_IsSelfEnding);
                ClearFlag(parts[typedPart.parentPartIndex].errorFlags, ErrorFlags_NoEndTag);
                parentPartIndex = parts[typedPart.parentPartIndex].parentPartIndex;
                CullPrefixNamespaces(internalData.nsPrefixMap, parentPartIndex);
            }
        }

        if (typedPart.parentPartIndex != NoPartIndex
            && parts[typedPart.parentPartIndex].HasFlag(PartFlags_ControlTemplate))
        {
            SetFlag(typedPart.flags, PartFlags_ControlTemplate);
        }

        if ((typedPart.partKind != XamlPartKind_StartTagBegin
            && typedPart.partKind != XamlPartKind_EndTag
            && typedPart.partKind != XamlPartKind_AttributeKey)
            || typedPart.HasFlag(PartFlags_NSDefinition))
        {
            continue;
        }

        if (typedPart.partKind == XamlPartKind_StartTagBegin)
        {
            if (typedPart.parentPartIndex == NoPartIndex)
            {
                typedPart.parentPartIndex = typedPart.partIndex;
            }
            else
            {
                SetFlag(parts[typedPart.parentPartIndex].flags, PartFlags_HasChild);
            }
            parentPartIndex = typedPart.partIndex;

            if (typedPart.HasFlag(PartFlags_HasNSDefinition))
            {
                auto defIt = nsDefinitionMap.Find(typedPart.partIndex);
                if (defIt != nsDefinitionMap.End())
                {
                    for (uint32_t j = 0; j < defIt->value.Size(); j++)
                    {
                        auto prefixIt = internalData.nsPrefixMap.Find(defIt->value[j].first);
                        if (prefixIt != internalData.nsPrefixMap.End())
                        {
                            prefixIt->value.EmplaceBack(typedPart.partIndex, j);
                        }
                        else
                        {
                            Noesis::Vector<Noesis::Pair<int32_t, uint32_t>, 1> vec;
                            vec.EmplaceBack(typedPart.partIndex, j);
                            internalData.nsPrefixMap[defIt->value[j].first] = vec;
                        }
                    }
                }
            }
        }
        
        PopulateIds(document, typedPart, parts, nsDefinitionMap, internalData.nsPrefixMap);
        
        if (typedPart.partKind == XamlPartKind_EndTag)
        {
            GetOpenTagForClose(parts, typedPart, parentPartIndex);
            CullPrefixNamespaces(internalData.nsPrefixMap, parentPartIndex);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LenientXamlParser::PopulatePrefixes(int32_t partIndex, Parts& parts,
    const NSDefinitionMap& nsDefinitionMap, PrefixMap& prefixMap)
{
    while (true)
    {
        XamlPart& part = parts[partIndex];
        if (part.partKind == XamlPartKind_StartTagBegin && part.HasFlag(PartFlags_HasNSDefinition))
        {
            auto entry = nsDefinitionMap.Find(partIndex);
            if (entry != nsDefinitionMap.End())
            {
                for (const Noesis::Pair<Noesis::Symbol, XamlNamespace>& definition : entry->value)
                {
                    if (prefixMap.Find(definition.first) == prefixMap.End())
                    {
                        prefixMap[definition.first] = definition.second;
                    }
                }
            }
        }

        if (partIndex == NoPartIndex || partIndex == part.parentPartIndex)
        {
            break;
        }
        partIndex = part.parentPartIndex;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
NoesisApp::FindXamlPartResult NoesisApp::LenientXamlParser::FindPartAtPosition(Parts& parts,
    LinePartIndicies& linePartIndices, const uint32_t lineIndex, uint32_t characterIndex,
    bool findClosest, XamlPart& foundPart)
{
    if (parts.Empty())
    {
        return FindXamlPartResult::None;
    }
    if (lineIndex >= linePartIndices.Size())
    {
        if (findClosest)
        {
            foundPart = parts[parts.Size() - 1];
            return FindXamlPartResult::ClosestMatch;
        }
        return FindXamlPartResult::None;
    }
    int32_t i = linePartIndices[lineIndex] == 0 ? 0 : linePartIndices[lineIndex] - 1;
    const int32_t length = static_cast<int32_t>(parts.Size());
    for (; i < length; i++)
    {
        const XamlPart& part = parts[i];
        if (part.endLineIndex < lineIndex 
            || (part.endLineIndex == lineIndex && part.endCharacterIndex < characterIndex))
        {
            continue;
        }
        const uint32_t checkStartCharacterIndex = findClosest ? part.startCharacterIndex + 1
            : part.startCharacterIndex;
        if (part.startLineIndex < lineIndex
            || (part.startLineIndex == lineIndex && characterIndex >= checkStartCharacterIndex))
        {
            foundPart = part;
            return FindXamlPartResult::Match;
        }
        if (findClosest && i > 0)
        {
            foundPart = parts[i - 1];
            return FindXamlPartResult::ClosestMatch;
        }
        return FindXamlPartResult::None;
    }
    if (findClosest)
    {
        foundPart = parts[parts.Size() - 1];
        return FindXamlPartResult::ClosestMatch;
    }
    return FindXamlPartResult::None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LenientXamlParser::GetString(const XamlPart& part, const DocumentContainer& document, 
    Noesis::BaseString& output)
{
    Noesis::String partKindName;
    switch (part.partKind)
    {
        case XamlPartKind_Undefined:
        {
            partKindName = "Undefined";
            break;
        }
        case XamlPartKind_StartTagBegin:
        {
            partKindName = "StartTagBegin";
            break;
        }
        case XamlPartKind_StartTagEnd:
        {
            partKindName = "StartTagEnd";
            break;
        }
        case XamlPartKind_EndTag:
        {
            partKindName = "EndTag";
            break;
        }
        case XamlPartKind_AttributeKey:
        {
            partKindName = "AttributeKey";
            break;
        }
        case XamlPartKind_AttributeEquals:
        {
            partKindName = "AttributeEquals";
            break;
        }
        case XamlPartKind_AttributeValue:
        {
            partKindName = "AttributeValue";
            break;
        }
        case XamlPartKind_TagContent:
        {
            partKindName = "TagContent";
            break;
        }
        case XamlPartKind_Comment:
        {
            partKindName = "Comment";
            break;
        }
    }

    Noesis::String flagsText;

    if (HasFlag(part.flags, PartFlags_HasChild))
    {
        flagsText.Append(" I_HasChild");
    }
    if (HasFlag(part.flags, PartFlags_HasProperty))
    {
        flagsText.Append(" I_HasProperty");
    }
    if (HasFlag(part.flags, PartFlags_HasAttributeProperty))
    {
        flagsText.Append(" I_HasAttributeProperty");
    }
    if (HasFlag(part.flags, PartFlags_HasNodeProperty))
    {
        flagsText.Append(" I_HasNodeProperty");
    }
    if (HasFlag(part.flags, PartFlags_IsNodeProperty))
    {
        flagsText.Append(" I_IsNodeProperty");
    }
    if (HasFlag(part.flags, PartFlags_SingleQuotes))
    {
        flagsText.Append(" I_SingleQuotes");
    }
    if (HasFlag(part.flags, PartFlags_IsSelfEnding))
    {
        flagsText.Append(" I_IsSelfEnding");
    }
    if (HasFlag(part.flags, PartFlags_NSDefinition))
    {
        flagsText.Append(" I_NSDefinition");
    }
    if (HasFlag(part.flags, PartFlags_DefaultNSDefinition))
    {
        flagsText.Append(" I_DefaultNSDefinition");
    }
    if (HasFlag(part.flags, PartFlags_HasNSDefinition))
    {
        flagsText.Append(" I_HasNSDefinition");
    }
    if (HasFlag(part.flags, PartFlags_Ignorable))
    {
        flagsText.Append(" I_Ignorable");
    }

    if (HasFlag(part.errorFlags, ErrorFlags_Unclosed))
    {
        flagsText.Append(" W_Unclosed");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_NoEndTag))
    {
        flagsText.Append(" W_NoEndTag");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_NoStartTag))
    {
        flagsText.Append(" E_NoStartTag");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_NoAttributeEquals))
    {
        flagsText.Append(" W_NoAttributeEquals");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_NoAttributeValue))
    {
        flagsText.Append(" W_NoAttributeValue");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_MissingBracket))
    {
        flagsText.Append(" W_MissingBracket");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_HasInvalidContent))
    {
        flagsText.Append(" W_HasInvalidContent");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_Duplicate))
    {
        flagsText.Append(" E_Duplicate");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_NoAttributeKey))
    {
        flagsText.Append(" E_NoAttributeKey");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_MissingValue))
    {
        flagsText.Append(" E_MissingValue");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_IdError))
    {
        flagsText.Append(" E_IdError");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_InvalidPrefixId))
    {
        flagsText.Append(" E_InvalidPrefix");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_InvalidTypeId))
    {
        flagsText.Append(" E_InvalidTypeId");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_InvalidPropertyId))
    {
        flagsText.Append(" E_InvalidPropertyId");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_InvalidNamespace))
    {
        flagsText.Append(" E_InvalidNamespace");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_MissingDefaultNS))
    {
        flagsText.Append(" E_MissingDefaultNS");
    }
    if (HasFlag(part.errorFlags, ErrorFlags_InvalidSyntax))
    {
        flagsText.Append(" E_InvalidSyntax");
    }

    uint32_t startCharacterIndex = document.lineStartPositions[part.startLineIndex] + part.startCharacterIndex;
    uint32_t endCharacterIndex = document.lineStartPositions[part.endLineIndex] + part.endCharacterIndex;

    Noesis::String typeString;
    part.GetTypeString(typeString);

    output.AppendFormat("##########################\r\n  PartKind: %s\r\n  Index: %d, ParentIndex: %d\r\n  Value: '%s'\r\n  Content: '%s'\r\n  Flags: %s\r\n  Raw Text (%u-%u,%u-%u): '%s'\r\n\r\n",
        partKindName.Str(), part.partIndex, part.parentPartIndex, typeString.Str(),
        part.content.Str(), flagsText.Str(), part.startLineIndex, part.endLineIndex,
        part.startCharacterIndex, part.endCharacterIndex, Noesis::String(document.text.Str(),
        startCharacterIndex, endCharacterIndex + 1 - startCharacterIndex).Str());
}

#endif
