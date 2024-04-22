////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_LENIENTXAMLPARSER_H__
#define __APP_LENIENTXAMLPARSER_H__


#include <NsCore/HashMap.h>

#include "Document.h"


namespace NoesisApp
{

struct DocumentContainer;

////////////////////////////////////////////////////////////////////////////////////////////////////
enum XamlPartKind : uint32_t
{
    XamlPartKind_Undefined,
    XamlPartKind_StartTagBegin,
    XamlPartKind_StartTagEnd,
    XamlPartKind_EndTag,
    XamlPartKind_AttributeKey,
    XamlPartKind_AttributeEquals,
    XamlPartKind_AttributeValue,
    XamlPartKind_TagContent,
    XamlPartKind_Comment
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum PartFlags : uint32_t
{
    PartFlags_IsSelfEnding = 1,
    PartFlags_SingleQuotes = 2,
    PartFlags_HasChild = 4,
    PartFlags_HasProperty = 8,
    PartFlags_HasAttributeProperty = 16 | PartFlags_HasProperty,
    PartFlags_HasNodeProperty = 32 | PartFlags_HasProperty,
    PartFlags_NSDefinition = 64,
    PartFlags_DefaultNSDefinition = 128 | PartFlags_NSDefinition,
    PartFlags_Ignorable = 256,
    PartFlags_HasNSDefinition = 512,
    PartFlags_IsNodeProperty = 1024,
    PartFlags_HasInheritedFlag = 134217728,
    PartFlags_ControlTemplate = 268435456 | PartFlags_HasInheritedFlag
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum ErrorFlags : uint32_t
{
    ErrorFlags_Unclosed = 1,
    ErrorFlags_NoEndTag = 2,
    ErrorFlags_NoAttributeEquals = 4,
    ErrorFlags_NoAttributeValue = 8,
    ErrorFlags_MissingBracket = 16,
    ErrorFlags_HasInvalidContent = 32,
    ErrorFlags_Error = 65536,
    ErrorFlags_Duplicate = 131072,
    ErrorFlags_NoAttributeKey = 262144,
    ErrorFlags_NoStartTag = 524288,
    ErrorFlags_MissingValue = 1048576,
    ErrorFlags_IdError = 2097152,
    ErrorFlags_InvalidPrefixId = 4194304 | ErrorFlags_IdError,
    ErrorFlags_InvalidTypeId = 8388608 | ErrorFlags_IdError,
    ErrorFlags_InvalidPropertyId = 16777216 | ErrorFlags_IdError,
    ErrorFlags_InvalidNamespace = 33554432 | ErrorFlags_IdError,
    ErrorFlags_MissingDefaultNS = 67108864,
    ErrorFlags_InvalidSyntax = 134217728,
    ErrorFlags_InvalidValue = 268435456
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum XamlNSKind : uint32_t
{
    XamlNSKind_Undefined,
    XamlNSKind_Default,
    XamlNSKind_XAML,
    XamlNSKind_XML,
    XamlNSKind_MarkupCompat,
    XamlNSKind_PresOptions,
    XamlNSKind_Interactivity,
    XamlNSKind_Interactions,
    XamlNSKind_Behaviors,
    XamlNSKind_Designer,
    XamlNSKind_Custom

};

////////////////////////////////////////////////////////////////////////////////////////////////////
struct XamlNamespace
{
    XamlNSKind kind{};
    Noesis::Symbol uriId;
    Noesis::Symbol clrNamespaceId;
    bool ignorable{};
    
    XamlNamespace()
    {
        
    }

    XamlNamespace(XamlNSKind kind_, Noesis::Symbol uriId_, Noesis::Symbol clrNamespaceId_, bool ignorable_)
    {
        kind = kind_;
        uriId = uriId_;
        clrNamespaceId = clrNamespaceId_;
        ignorable = ignorable_;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
enum class FindXamlPartResult : uint32_t
{
    None,
    Match,
    ClosestMatch
};

////////////////////////////////////////////////////////////////////////////////////////////////////
struct XamlPart
{
    XamlPartKind partKind{};
    int32_t partIndex{};
    int32_t parentPartIndex{};
    uint32_t startLineIndex{};
    uint32_t startCharacterIndex{};
    uint32_t endLineIndex{};
    uint32_t endCharacterIndex{};
    uint32_t flags{};
    uint32_t errorFlags{};
    Noesis::Symbol prefixId;
    Noesis::Symbol typeId;
    Noesis::Symbol propertyId;
    Noesis::String content;

    bool HasFlag(PartFlags flag) const
    {
        return (flags & flag) == flag;
    }

    bool IsRoot() const
    {
        return partIndex == parentPartIndex;
    }

    bool HasErrorFlag(ErrorFlags errorFlag) const
    {
        return (errorFlags & errorFlag) == errorFlag;
    }

    bool IsTypeMatch(const XamlPart& other) const
    {
        if (HasErrorFlag(ErrorFlags_IdError) || other.HasErrorFlag(ErrorFlags_IdError))
        {
            if (HasErrorFlag(ErrorFlags_IdError) == other.HasErrorFlag(ErrorFlags_IdError))
            {
                return content.Size() > 0 && Noesis::StrEquals(content.Str(), other.content.Str());
            }
            return false;
        }
        return prefixId == other.prefixId && typeId == other.typeId
            && propertyId == other.propertyId;
    }

    void GetTypeString(Noesis::BaseString& output) const
    {
        if (HasErrorFlag(ErrorFlags_IdError))
        {
            return;
        }
        if (!prefixId.IsNull())
        {
            output.Append(prefixId.Str());
        }
        if (!typeId.IsNull())
        {
            if (!prefixId.IsNull())
            {
                output.PushBack(':');
            }
            const int pos = Noesis::StrFindLast(typeId.Str(), ".");
            if (pos != -1)
            {
                output.Append(typeId.Str() + pos + 1);
            }
            else
            {
                output.Append(typeId.Str());
            }
        }
        if (!propertyId.IsNull())
        {
            output.PushBack('.');
            output.Append(propertyId.Str());
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace LenientXamlParser
{

typedef Noesis::HashMap<Noesis::Symbol, XamlNamespace> PrefixMap;
typedef Noesis::HashMap<int32_t, Noesis::Vector<Noesis::Pair<Noesis::Symbol, XamlNamespace>, 1>> NSDefinitionMap;
typedef Noesis::Vector<XamlPart, 1> Parts;
typedef Noesis::Vector<int32_t, 1> LinePartIndicies;

void ParseXaml(DocumentContainer& document, Parts& parts, LinePartIndicies& lineParts,
    NSDefinitionMap& nsDefinitionMap);
void PopulatePrefixes(int32_t partIndex, Parts& parts, const NSDefinitionMap& nsDefinitionMap,
    PrefixMap& prefixMap);
FindXamlPartResult FindPartAtPosition(Parts& parts, LinePartIndicies& linePartIndices,
    const uint32_t lineIndex, uint32_t characterIndex, bool findClosest, XamlPart& foundPart);
uint32_t UTF8SequenceLength(const char* text);
uint32_t UTF8ByteLength(const char* utf8, uint32_t utf8Length);
void UTF16Advance(const char*& utf8, uint32_t utf16Count);
uint32_t UTF16Length(const char* utf8);
uint32_t UTF16Length(const char* begin, const char* end);
void GetString(const XamlPart& part, const DocumentContainer& document, Noesis::BaseString& output);

}
}

#endif
