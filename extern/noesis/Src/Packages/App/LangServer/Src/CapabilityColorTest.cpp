////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/UnitTest.h>
#include <NsGui/IntegrationAPI.h>
#include <NsApp/LangServer.h>

#include "CapabilityColor.h"
#include "Document.h"
#include "DocumentHelper.h"


using namespace Noesis;
using namespace NoesisApp;


#ifdef NS_LANG_SERVER_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////
static void PrepareDocument(DocumentContainer& document, const char* xaml)
{
    document.text = xaml;
    DocumentHelper::GenerateLineNumbers(document);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool DocumentColorTest(const char* xaml, const char* checkResponseContains)
{
    DocumentContainer document{};
    PrepareDocument(document, xaml);
    String response;
    CapabilityColor::DocumentColorRequest(123, document, response);
    const int pos = response.Find("{");
    const char* testResponse = response.Begin() + pos;
    return StrFindFirst(testResponse, checkResponseContains) != -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UNITTEST(CapabilityColor)
{
    // DocumentColor

    // Attribute property values
    {
        const char* xaml = "<Button Background=\"Blue\" Foreground=\"#FF0DE70D\" BorderBrush=\"{x:Null}\"><Border><Border.Background><SolidColorBrush Color=\"#FF2600FF\"/></Border.Background></Border></Button>";
        const char* expectedResponse = R"([{"range":{"start":{"line":0,"character":20},"end":{"line":0,"character":24}},"color":{"red":0.000000,"green":0.000000,"blue":1.000000,"alpha":1.000000}},{"range":{"start":{"line":0,"character":38},"end":{"line":0,"character":47}},"color":{"red":0.050980,"green":0.905882,"blue":0.050980,"alpha":1.000000}},{"range":{"start":{"line":0,"character":62},"end":{"line":0,"character":70}},"color":{"red":0.000000,"green":0.000000,"blue":0.000000,"alpha":1.000000}},{"range":{"start":{"line":0,"character":123},"end":{"line":0,"character":132}},"color":{"red":0.149020,"green":0.000000,"blue":1.000000,"alpha":1.000000}}])";
        const bool success = DocumentColorTest(xaml, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Node property values
    {
        const char* xaml = "<Button><Button.Background>Blue</Button.Background><Button.Foreground>#FF0DE70D</Button.Foreground><Button.BorderBrush><SolidColorBrush><SolidColorBrush.Color>Red</SolidColorBrush.Color></SolidColorBrush></Button.BorderBrush></Button>";
        const char* expectedResponse = R"([{"range":{"start":{"line":0,"character":27},"end":{"line":0,"character":31}},"color":{"red":0.000000,"green":0.000000,"blue":1.000000,"alpha":1.000000}},{"range":{"start":{"line":0,"character":70},"end":{"line":0,"character":79}},"color":{"red":0.050980,"green":0.905882,"blue":0.050980,"alpha":1.000000}},{"range":{"start":{"line":0,"character":159},"end":{"line":0,"character":162}},"color":{"red":1.000000,"green":0.000000,"blue":0.000000,"alpha":1.000000}}])";
        const bool success = DocumentColorTest(xaml, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }
}

#endif
