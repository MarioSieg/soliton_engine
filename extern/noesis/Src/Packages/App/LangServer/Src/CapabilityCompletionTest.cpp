////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/UnitTest.h>
#include <NsGui/IntegrationAPI.h>
#include <NsApp/LangServer.h>

#include "CapabilityCompletion.h"
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
static void PreparePosition(DocumentContainer& document, TextPosition& position, uint32_t line, 
    uint32_t character)
{
    position.line = line;
    position.character = character;
    DocumentHelper::PopulateTextPosition(document, position);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool DiagnosticsRequestTest(const char* xaml, uint32_t line, uint32_t character, 
    const char* checkResponseContains)
{
    DocumentContainer document{};
    PrepareDocument(document, xaml);
    TextPosition position{};
    PreparePosition(document, position, line, character);
    String response;
    CapabilityCompletion::CompletionRequest(123, document, position, response);
    const int pos = response.Find("{");
    const char* testResponse = response.Begin() + pos;
    return StrFindFirst(testResponse, checkResponseContains) != -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool AutoInsertCloseRequestTest(const char* xaml, uint32_t line, uint32_t character,
    const char* expectedResponse)
{
    DocumentContainer document{};
    PrepareDocument(document, xaml);
    TextPosition position{};
    PreparePosition(document, position, line, character);
    String response;
    CapabilityCompletion::AutoInsertCloseRequest(123, document, position, response);
    int pos = response.Find("{");
    const char* testResponse = response.Begin() + pos;
    return StrEquals(testResponse, expectedResponse);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool AutoInsertQuotesRequestTest(const char* xaml, uint32_t line, uint32_t character,
    const char* expectedResponse)
{
    DocumentContainer document{};
    PrepareDocument(document, xaml);
    TextPosition position{};
    PreparePosition(document, position, line, character);
    String response;
    CapabilityCompletion::AutoInsertQuotesRequest(123, document, position, response);
    int pos = response.Find("{");
    const char* testResponse = response.Begin() + pos;
    return StrEquals(testResponse, expectedResponse);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UNITTEST(CapabilityCompletion)
{
    // AutoInsertQuotes

    // Auto insert quotes, unclosed tag, no other attributes
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text=\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"\"\""}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 18, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert quotes, closed tag, other attribute
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text= FontWeight=\"Bold\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"\"\""}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 18, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert quotes, closed tag, between attributes
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Margin=\"20\" Text= FontWeight=\"Bold\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"\"\""}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 30, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert quotes, with an equals already in place
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text== FontWeight=\"Bold\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: Equals already exist."}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 18, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert quotes, quotes already exist
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text=\"\" FontWeight=\"Bold\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: Quotes already exist."}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 18, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert quotes, no key
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock =\"\" FontWeight=\"Bold\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: No attribute key."}})";
        const bool success = AutoInsertQuotesRequestTest(xaml, 2, 14, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // AutoInsertClose - Self Close

    // Auto insert self close tag, known type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text=\"\" /\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":">"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 22, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert self close tag, unknown type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TrfgBgvhj Text=\"\" /\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":">"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 22, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert self close tag, unknown type with invalid starting character
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <3rfgBgvhj Text=\"\" /\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":">"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 22, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert self close, where tag is already self-closed
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <TextBlock Text=\"\" />\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: Tag is already self-closed."}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 23, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // AutoInsertClose - End Tag, ">" trigger

    // Auto insert end tag, ">" trigger, known type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Canvas>\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"</Canvas>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 10, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, ">" trigger, already self-closed tag
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Canvas/>\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: Tag is already self-closed."}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 11, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, ">" trigger, unknown type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Cegcbh>\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"</Cegcbh>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 10, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, ">" trigger, unknown type with invalid starting character
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <3egcbh>\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"</3egcbh>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 10, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, ">" trigger, unknown type, prefixed
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <x:Cegcbh>\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"</x:Cegcbh>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 12, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger

    // Auto insert end tag, "</" trigger, known type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Canvas>\r\n</</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"Canvas>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, unknown type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <x:Cegcbh>\r\n</</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"x:Cegcbh>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, unknown type with invalid starting character
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <x:3egcbh>\r\n</</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"x:3egcbh>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, known type, same outer parent
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Grid><Grid>\r\n</</Grid></Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"Grid>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, unknown type, same outer parent
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Griz><Griz>\r\n</</Griz></Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"Griz>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, known type, different outer parent
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Canvas><Grid>\r\n</</Grid></Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: No open tag found to close."}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 2, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, known type, with closed child element of same type
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Grid><Grid>\r\n</Grid></</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","result":{"snippet":"Grid>"}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 3, 9, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // Auto insert end tag, "</" trigger, known type, already self-closed tag
    {
        const char* xaml = "<Grid xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n  <Panel/></\r\n</Grid>";
        const char* expectedResponse = R"({"id":123,"jsonrpc":"2.0","error":{"code":-32800,"message":"RequestCancelled: No open tag found to close."}})";
        const bool success = AutoInsertCloseRequestTest(xaml, 2, 12, expectedResponse);
        NS_UNITTEST_CHECK(success);
    }

    // CompletionRequest

    // Completion request, root node
    {
        const char* xaml = "<";
        const bool success = DiagnosticsRequestTest(xaml, 0, 1,
            R"({"label":"Page","kind":7})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, root node with invalid content before
    {
        const char* xaml = ">abc <";
        const bool success = DiagnosticsRequestTest(xaml, 0, 6,
            R"({"label":"Page","kind":7})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, root node, after multiple newlines
    {
        const char* xaml = "\r\n\r\n\r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 4, 1,
            R"({"label":"Page","kind":7})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, no key, unclosed tag
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 3, 0, R"({"label":"Orientation","kind":10,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, no key, unclosed tag, multiple initial newlines
    {
        const char* xaml = "\r\n\r\n\r\n\r\n<Page\r\n\r\n  <StackPanel\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 7, 0, R"({"label":"Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, no key, unclosed tag, after multiple newlines
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel\r\n\r\n \r\n\r\n  \r\n\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 8, 0, R"({"label":"Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, after multiple newlines
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel>\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 8, 1, R"({"label":"StackPanel.Orientation","kind":10})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, multiple initial newlines, multiple after tag newlines
    {
        const char* xaml = "\r\n\r\n\r\n\r\n<Page\r\n\r\n  <StackPanel>\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 12, 1, R"({"label":"StackPanel.Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, node property, no key, after multiple newlines
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel>\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 8, 1, R"({"label":"StackPanel.Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, after multiple newlines
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel>\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 8, 1, R"({"label":"Button",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, after multiple newlines, after self-closed node
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n<ListBox /> <";
        const bool success = DiagnosticsRequestTest(xaml, 2, 13, R"({"label":"Button",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, after multiple newlines, after self-closed node
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n<ListBox></ListBox><";
        const bool success = DiagnosticsRequestTest(xaml, 2, 20, R"({"label":"Button",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child node, ItemCollection ContentProperty
    {
        const char* xaml = "<Page\r\n\r\n  <ListBox>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 3, 1, R"({"label":"ListBoxItem",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, node property, no key, after multiple newlines, check for duplicate property
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel Orientation=\"Vertical\">\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 8, 1, R"({"label":"StackPanel.Orientation",)");
        NS_UNITTEST_CHECK(!success);
    }

    // Completion request, node property, no key, unclosed tag, after multiple newlines, check for duplicate property
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel Orientation=\"Vertical\"\r\n\r\n \r\n\r\n  \r\n\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 8, 1, R"({"label":"StackPanel.Orientation",)");
        NS_UNITTEST_CHECK(!success);
    }

    // Completion request, attribute property, no key, unclosed tag
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 3, 0, R"({"label":"Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, no key, unclosed tag, check for duplicate property
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel Orientation=\"Vertical\"\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 3, 0, R"({"label":"Orientation",)");
        NS_UNITTEST_CHECK(!success);
    }

    // Completion request, attribute property, partial key, unclosed tag
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel Vert\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 2, 18, R"("insertText":"Focusable=\"$1\"$0","insertTextFormat":2,"command":)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, partial key, unclosed tag, after multiple newlines
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel\r\n\r\n \r\n\r\n  \r\n\r\nVert=";
        const bool success = DiagnosticsRequestTest(xaml, 8, 4, R"({"label":"Orientation",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, attribute property, partial key, equals after, unclosed tag
    {
        const char* xaml = "<Page\r\n\r\n  <StackPanel Vert=\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 2, 18, R"({"label":"Focusable",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside node property, with a previous sibling, and missing noesis xmlns declaration
    {
        const char* xaml = "<Page xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\r\n  xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\r\n\r\n\r\n  <StackPanel>\r\n    <StackPanel.Effect>\r\n      <noesis:PinchEffect></noesis:PinchEffect>\r\n      <\r\n    </StackPanel.Effect>\r\n\r\n  </StackPanel>\r\n\r\n\r\n</Page>";
        const bool success = DiagnosticsRequestTest(xaml, 7, 7, R"({"label":"DropShadowEffect",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside node property, custom namespace
    {
        const char* xaml = "<Page xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\" xmlns:testPrefix=\"clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions\"><StackPanel><StackPanel.Effect>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 1, 1, R"({"label":"testPrefix:PinchEffect",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside Resources property, verifying ResourceDictionary entry
    {
        const char* xaml = "<Page><Page.Resources>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 1, 1, R"({"label":"ResourceDictionary",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside Resources property, verifying non-ResourceDictionary entry
    {
        const char* xaml = "<Page><Page.Resources>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 1, 1, R"({"label":"DataTemplate",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside ResourcesDictionary, verifying ResourceDictionary property node entry
    {
        const char* xaml = "<ResourceDictionary>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 1, 1, R"({"label":"ResourceDictionary.MergedDictionaries",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, inside ResourcesDictionary, verifying non-ResourceDictionary entry
    {
        const char* xaml = "<ResourceDictionary>\r\n<";
        const bool success = DiagnosticsRequestTest(xaml, 1, 1, R"({"label":"DataTemplate",)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, Brush attribute property value
    {
        const char* xaml = "<Border Background=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 20, R"({"label":"Green","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, Color attribute property value
    {
        const char* xaml = "<Border><Border.Background><SolidColorBrush Color=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 51, R"({"label":"Green","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, Null result for BaseComponent derived attribute property value
    {
        const char* xaml = "<Border Background=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 20, R"({"label":"{x:Null}","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, bool attribute property value
    {
        const char* xaml = "<CheckBox IsChecked=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 21, R"({"label":"True","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, Null result for Nullable attribute property value
    {
        const char* xaml = "<CheckBox IsChecked=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 21, R"({"label":"{x:Null}","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, enum attribute property value
    {
        const char* xaml = "<Border Visibility=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 20, R"({"label":"Visible","kind":12})");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, non-enum attribute property value, snippet result
    {
        const char* xaml = "<Border Height=\"\"";
        const bool success = DiagnosticsRequestTest(xaml, 0, 16, R"({"label":"dynamicres","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion Request Snippets

    // Completion request, snippet, empty document
    {
        const char* xaml = "";
        const bool success = DiagnosticsRequestTest(xaml, 0, 0,
            R"({"label":"button","kind":15,"insertText":"<Button)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, snippet, whitespace only document
    {
        const char* xaml = "    ";
        const bool success = DiagnosticsRequestTest(xaml, 0, 2,
            R"({"label":"button","kind":15,"insertText":"<Button)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, snippet, text with no root
    {
        const char* xaml = " ab ";
        const bool success = DiagnosticsRequestTest(xaml, 0, 2,
            R"({"label":"button","kind":15,"insertText":"<Button)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child snippet
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 2, 0, R"({"label":"button","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child snippet, no invalid snippet result
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n";
        const bool success = !DiagnosticsRequestTest(xaml, 2, 0, R"({"label":"linearbrush","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child snippet, after self-closed node
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n<ListBox /> ";
        const bool success = DiagnosticsRequestTest(xaml, 2, 12, R"({"label":"button","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child snippet, after self-closed node
    {
        const char* xaml = "<Page>\r\n<StackPanel>\r\n<ListBox></ListBox> ";
        const bool success = DiagnosticsRequestTest(xaml, 2, 20, R"({"label":"button","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }

    // Completion request, child snippet, in property
    {
        const char* xaml = "<Page>\r\n<Border><Border.Background>\r\n";
        const bool success = DiagnosticsRequestTest(xaml, 2, 0, R"({"label":"radialbrush","kind":15,)");
        NS_UNITTEST_CHECK(success);
    }
}

#endif
