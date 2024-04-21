////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_CAPABILITYCOMPLETION_H__
#define __APP_CAPABILITYCOMPLETION_H__


#include <NsCore/StringFwd.h>


namespace NoesisApp
{

struct DocumentContainer;
struct TextPosition;

namespace CapabilityCompletion
{

void CompletionRequest(int bodyId, DocumentContainer& document, const TextPosition& position,
    Noesis::BaseString& responseBuffer);
void AutoInsertCloseRequest(int bodyId, DocumentContainer& document, const TextPosition& position,
    Noesis::BaseString& responseBuffer);
void AutoInsertQuotesRequest(int bodyId, DocumentContainer& document, const TextPosition& position,
    Noesis::BaseString& responseBuffer);

}
}

#endif
