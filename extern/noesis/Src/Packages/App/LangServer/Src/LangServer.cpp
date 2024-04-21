////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LangServer.h>
#include <NsCore/HighResTimer.h>
#include <NsCore/Log.h>
#include <NsCore/String.h>
#include <NsRender/Texture.h>
#include <NsGui/MemoryStream.h>
#include <NsGui/IntegrationAPI.h>

#include "DocumentHelper.h"
#include "LangServerSocket.h"
#include "MessageBuffer.h"
#include "MessageReader.h"
#include "Workspace.h"


#ifdef NS_LANG_SERVER_ENABLED


#ifdef NS_COMPILER_MSVC
#define NS_STDCALL __stdcall
#define NS_EXPORT
#endif

#ifdef NS_COMPILER_GCC
#define NS_STDCALL
#define NS_EXPORT __attribute__ ((visibility("default")))
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Common private definitions
////////////////////////////////////////////////////////////////////////////////////////////////////
static constexpr int TcpPortRangeBegin = 16629;
static constexpr int TcpPortRangeEnd = 16649;
static constexpr int UdpPortRangeBegin = 16629;
static constexpr int UdpPortRangeEnd = 16649;
static constexpr char ServerAddress[] = "127.0.0.1";

static NoesisApp::LangServerSocket gListenSocket;
static NoesisApp::LangServerSocket gDataSocket;
static NoesisApp::LangServerSocket gBroadcastSocket;
static NoesisApp::Workspace gWorkspace;
static char gReceiveBuffer;
static NoesisApp::MessageBuffer gMessageBuffer;
static Noesis::FixedString<2048> gSendBuffer;
static Noesis::FixedString<254> gAnnouncementMessage;
static Noesis::FixedString<64> gServerName = "Unknown";
static int gServerPriority = 1000;
static bool gInitialized = false;
static double gLastBroadcastTime;
static uint64_t gStartTicks;
static bool gIsConnected;
static NoesisApp::LangServer::RenderCallback gRenderCallback = nullptr;
static void* gRenderCallbackUser = 0;
static NoesisApp::LangServer::DocumentCallback gDocumentClosedCallback = nullptr;
static void* gDocumentClosedCallbackUser = 0;

static Noesis::XamlProvider* gXamlProvider;
static Noesis::TextureProvider* gTextureProvider;
static Noesis::FontProvider* gFontProvider;

////////////////////////////////////////////////////////////////////////////////////////////////////
static void TrySendMessages(Noesis::BaseString& sendBuffer, NoesisApp::LangServerSocket& dataSocket)
{
    if (!sendBuffer.Empty())
    {
        //NS_LOG_INFO("### Send message\n%s\n###\n\n", sendBuffer.Str());
        dataSocket.Send(sendBuffer.Str(), sendBuffer.Size());
        sendBuffer.Clear();
    }
}

#ifdef NS_MANAGED
////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_Init()
{
    #ifdef NS_PLATFORM_WINDOWS
    #pragma comment(linker, "/export:Noesis_LangServer_Init=" __FUNCDNAME__)
    #endif
    NoesisApp::LangServer::Init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_RunTick()
{
    #ifdef NS_PLATFORM_WINDOWS
    #pragma comment(linker, "/export:Noesis_LangServer_RunTick=" __FUNCDNAME__)
    #endif
    NoesisApp::LangServer::RunTick();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_Shutdown()
{
    #ifdef NS_PLATFORM_WINDOWS
    #pragma comment(linker, "/export:Noesis_LangServer_Shutdown=" __FUNCDNAME__)
    #endif
    NoesisApp::LangServer::Shutdown();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetName(const char* serverName)
{
#ifdef NS_PLATFORM_WINDOWS
#pragma comment(linker, "/export:Noesis_LangServer_SetName=" __FUNCDNAME__)
#endif

    NoesisApp::LangServer::SetName(serverName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetPriority(int serverPriority)
{
    #ifdef NS_PLATFORM_WINDOWS
    #pragma comment(linker, "/export:Noesis_LangServer_SetPriority=" __FUNCDNAME__)
    #endif

    NoesisApp::LangServer::SetPriority(serverPriority);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetXamlProvider(
    Noesis::XamlProvider* xamlProvider)
{
#ifdef NS_PLATFORM_WINDOWS
#pragma comment(linker, "/export:Noesis_LangServer_SetXamlProvider=" __FUNCDNAME__)
#endif

    NoesisApp::LangServer::SetXamlProvider(xamlProvider);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetTextureProvider(
    Noesis::TextureProvider* textureProvider)
{
#ifdef NS_PLATFORM_WINDOWS
#pragma comment(linker, "/export:Noesis_LangServer_SetTextureProvider=" __FUNCDNAME__)
#endif

    NoesisApp::LangServer::SetTextureProvider(textureProvider);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetFontProvider(
    Noesis::FontProvider* fontProvider)
{
#ifdef NS_PLATFORM_WINDOWS
#pragma comment(linker, "/export:Noesis_LangServer_SetFontProvider=" __FUNCDNAME__)
#endif

    NoesisApp::LangServer::SetFontProvider(fontProvider);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (NS_STDCALL* RenderCallbackFn)(Noesis::UIElement* content, int renderWidth,
    int renderHeight, double renderTime, const char* savePath);
RenderCallbackFn gManagedRenderCallback = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" NS_EXPORT void NS_STDCALL Noesis_LangServer_SetRenderCallback(
    RenderCallbackFn renderCallback)
{
#ifdef NS_PLATFORM_WINDOWS
#pragma comment(linker, "/export:Noesis_LangServer_SetRenderCallback=" __FUNCDNAME__)
#endif

    if (renderCallback == nullptr)
    {
        gRenderCallbackUser = nullptr;
        gRenderCallback = nullptr;
        return;
    }

    gManagedRenderCallback = renderCallback;

    gRenderCallback = [](void*, Noesis::UIElement* content, int renderWidth,
        int renderHeight, double renderTime, const char* savePath)
    {
        gManagedRenderCallback(content, renderWidth, renderHeight, renderTime, savePath);
    };
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetName(const char* name)
{
    if (gInitialized)
    {
        NS_ERROR("LangServer name cannot be set after initialization");
        return;
    }

    if (Noesis::StrIsNullOrEmpty(name))
    {
        NS_ERROR("LangServer name cannot be empty");
        return;
    }

    gServerName = name;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetPriority(int priority)
{
    if (gInitialized)
    {
        NS_ERROR("LangServer priority cannot be set after initialization");
        return;
    }

    gServerPriority = priority;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetXamlProvider(Noesis::XamlProvider* provider)
{
    if (gXamlProvider != provider)
    {
        if (provider != 0)
        {
            provider->AddReference();
        }

        if (gXamlProvider != 0)
        {
            gXamlProvider->Release();
        }

        gXamlProvider = provider;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetTextureProvider(Noesis::TextureProvider* provider)
{
    if (gTextureProvider != provider)
    {
        if (provider != 0)
        {
            provider->AddReference();
        }

        if (gTextureProvider != 0)
        {
            gTextureProvider->Release();
        }

        gTextureProvider = provider;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetFontProvider(Noesis::FontProvider* provider)
{
    if (gFontProvider != provider)
    {
        if (provider != 0)
        {
            provider->AddReference();
        }

        if (gFontProvider != 0)
        {
            gFontProvider->Release();
        }

        gFontProvider = provider;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetRenderCallback(void* user, RenderCallback renderCallback)
{
    gRenderCallbackUser = user;
    gRenderCallback = renderCallback;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::SetDocumentClosedCallback(void* user, DocumentCallback callback)
{
    gDocumentClosedCallbackUser = user;
    gDocumentClosedCallback = callback;
}

namespace
{

////////////////////////////////////////////////////////////////////////////////////////////////////
class LangServerXamlProvider: public Noesis::XamlProvider
{
public:
    Noesis::Ptr<Noesis::Stream> LoadXaml(const Noesis::Uri& uri) override
    {
        NS_ASSERT(uri.IsValid());

        if (gWorkspace.IsInitialized())
        {
            if (gWorkspace.HasDocument(uri))
            {
                const NoesisApp::DocumentContainer& document = gWorkspace.GetDocument(uri);
                return *new Noesis::MemoryStream(document.text.Str(), document.text.Size());
            }
        }

        return gXamlProvider ? gXamlProvider->LoadXaml(uri) : nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class LangServerTextureProvider: public Noesis::TextureProvider
{
public:
    Noesis::TextureInfo GetTextureInfo(const Noesis::Uri& uri) override
    {
        return gTextureProvider ? gTextureProvider->GetTextureInfo(uri) : Noesis::TextureInfo{};
    }

    Noesis::Ptr<Noesis::Texture> LoadTexture(const Noesis::Uri& uri, Noesis::RenderDevice* device) override
    {
        return gTextureProvider ? gTextureProvider->LoadTexture(uri, device) : nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
class LangServerFontProvider: public Noesis::FontProvider
{
public:
    Noesis::FontSource MatchFont(const Noesis::Uri& uri, const char* name, Noesis::FontWeight& weight,
        Noesis::FontStretch& stretch, Noesis::FontStyle& style) override
    {
        return gFontProvider ? gFontProvider->MatchFont(uri, name, weight, stretch, style) : Noesis::FontSource{};
    }

    bool FamilyExists(const Noesis::Uri& baseUri, const char* familyName) override
    {
        return gFontProvider ? gFontProvider->FamilyExists(baseUri, familyName) : false;
    }
};

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::Init()
{
    if (!gInitialized)
    {
        if (!gXamlProvider)
        {
            NS_ERROR("(LangServer) XAML provider not set");
        }

        if (!gTextureProvider)
        {
            NS_ERROR("(LangServer) Texture provider not set");
        }

        if (!gFontProvider)
        {
            NS_ERROR("(LangServer) Font provider not set");
        }

        Noesis::GUI::SetSchemeXamlProvider("lang", Noesis::MakePtr<LangServerXamlProvider>());
        Noesis::GUI::SetSchemeTextureProvider("lang", Noesis::MakePtr<LangServerTextureProvider>());
        Noesis::GUI::SetSchemeFontProvider("lang", Noesis::MakePtr<LangServerFontProvider>());

        LangServerSocket::Init();

        int serverPort = TcpPortRangeBegin;

        do
        {
            gListenSocket.Listen(ServerAddress, serverPort);
        }
        while (!gListenSocket.Connected() && serverPort++ < TcpPortRangeEnd);

        if (!gListenSocket.Connected())
        {
            NS_LOG_INFO("LangServer failed to bind to socket");
            return;
        }

        NS_LOG_INFO("LangServer listening on socket: %s:%d", ServerAddress, serverPort);

        gAnnouncementMessage.Format("{\"serverPort\":%u,\"serverName\":\"%s\",\"serverPriority\":%d,\"canRenderPreview\":%u}",
            serverPort, gServerName.Str(), gServerPriority, gRenderCallback == nullptr ? 0 : 1);

        gBroadcastSocket.Open();
        gLastBroadcastTime = 0;

        gInitialized = true;
        gIsConnected = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void TryConnect(double time)
{
    if (time > gLastBroadcastTime + 0.25)
    {
        gLastBroadcastTime = time;

        for (int p = UdpPortRangeBegin; p < UdpPortRangeEnd; ++p)
        {
            gBroadcastSocket.Broadcast(gAnnouncementMessage.Str(), gAnnouncementMessage.Size(), p);
        }
    }

    if (gIsConnected)
    {
        gIsConnected = false;
        NS_LOG_INFO("LangServer client disconnected");
    }

    if (gListenSocket.PendingBytes())
    {
        gIsConnected = true;
        gLastBroadcastTime = 0;

        gListenSocket.Accept(gDataSocket);
        NS_LOG_INFO("LangServer client connected");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void ReceiveData(double time)
{
    gDataSocket.Recv(&gReceiveBuffer, 1);
    gMessageBuffer.HandleChar(gReceiveBuffer);

    if (gMessageBuffer.IsMessageCompleted())
    {
        const NoesisApp::JsonObject body = gMessageBuffer.GetBody();

        gMessageBuffer.Clear();

        NoesisApp::MessageReader::HandleMessage(gRenderCallbackUser, gRenderCallback,
            gDocumentClosedCallbackUser, gDocumentClosedCallback, body, gWorkspace, time,
            gSendBuffer);

        TrySendMessages(gSendBuffer, gDataSocket);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::RunMessageLoop()
{
    if (gInitialized)
    {
        while (true)
        {
            double time = Noesis::HighResTimer::Seconds(Noesis::HighResTimer::Ticks() - gStartTicks);

            if (!gDataSocket.Connected())
            {
                TryConnect(time);
            }
            else
            {
                ReceiveData(time);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::RunTick()
{
    if (gInitialized)
    {
        double time = Noesis::HighResTimer::Seconds(Noesis::HighResTimer::Ticks() - gStartTicks);

        if (!gDataSocket.Connected())
        {
            TryConnect(time);
        }
        else
        {
            while (gDataSocket.PendingBytes())
            {
                ReceiveData(time);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServer::Shutdown()
{
    if (gInitialized)
    {
        gInitialized = false;
		
        if (gDocumentClosedCallback != nullptr)
        {
			Workspace::DocumentMap& documents = gWorkspace.GetDocuments();
            for (auto& entry : documents)
            {
                gDocumentClosedCallback(gDocumentClosedCallbackUser, entry.value.uri);
            }
        }

        gRenderCallback = nullptr;

#ifdef NS_MANAGED
        gManagedRenderCallback = nullptr;
#endif

        gSendBuffer.ClearShrink();
        gAnnouncementMessage.ClearShrink();
        gServerName.ClearShrink();

        gWorkspace.DeInitialize();
        gMessageBuffer.ClearShrink();

        gListenSocket.Disconnect();
        gDataSocket.Disconnect();
        gBroadcastSocket.Disconnect();

        LangServerSocket::Shutdown();

        if (gXamlProvider != 0)
        {
            gXamlProvider->Release();
            gXamlProvider = 0;
        }

        if (gTextureProvider != 0)
        {
            gTextureProvider->Release();
            gTextureProvider = 0;
        }

        if (gFontProvider != 0)
        {
            gFontProvider->Release();
            gFontProvider = 0;
        }

        Noesis::GUI::SetSchemeXamlProvider("lang", 0);
        Noesis::GUI::SetSchemeTextureProvider("lang", 0);
        Noesis::GUI::SetSchemeFontProvider("lang", 0);
    }
}

#endif
