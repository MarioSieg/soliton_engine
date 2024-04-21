////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "LangServerSocket.h"

#include <NsCore/Error.h>
#include <NsCore/String.h>
#include <NsApp/LangServer.h>


#if defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif


#ifdef NS_LANG_SERVER_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServerSocket::Init()
{
  #if defined(NS_PLATFORM_WINDOWS)
    WSADATA wsadata;
    int error = WSAStartup(MAKEWORD(2,2), &wsadata);
    NS_ASSERT(error == 0);
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServerSocket::Shutdown()
{
  #ifdef NS_PLATFORM_WINDOWS
    WSACleanup();
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::Connected() const
{
  #if defined(NS_PLATFORM_WINDOWS)
    return mSocket != INVALID_SOCKET;
  #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
    return mSocket != -1;
  #else
    return false;
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServerSocket::Disconnect()
{
    if (Connected())
    {
      #if defined(NS_PLATFORM_WINDOWS)
        int error = closesocket(mSocket);
        NS_ASSERT(error == 0);
        mSocket = INVALID_SOCKET;

      #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        int error = close(mSocket);
        NS_ASSERT(error == 0);
        mSocket= -1;
      #endif
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::Send(const char* buffer, uint32_t len)
{
    if (Connected())
    {
      #if defined(NS_PLATFORM_WINDOWS)
        int sent = send(mSocket, buffer, len, 0);
        if (sent == SOCKET_ERROR || (uint32_t)sent != len)
        {
            Disconnect();
            return false;
        }

        return true;

      #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        ssize_t sent = send(mSocket, buffer, len, 0);
        if (sent == -1 || (uint32_t)sent != len)
        {
            Disconnect();
            return false;
        }

        return true;
      #endif
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::Recv(char* buffer, uint32_t len)
{
    if (Connected())
    {
      #if defined(NS_PLATFORM_WINDOWS)
        do
        {
            int received = recv(mSocket, buffer, len, 0);
            if (received == SOCKET_ERROR)
            {
                Disconnect();
                return false;
            }
            buffer += received;
            len -= (uint32_t)received;
        } while (len > 0);

        return true;

      #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        do
        {
            ssize_t received = recv(mSocket, buffer, len, 0);
            if (received == -1)
            {
                Disconnect();
                return false;
            }
            buffer += received;
            len -= (uint32_t)received;
        } while (len > 0);

        return true;
      #endif
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::PendingBytes()
{
    if (Connected())
    {
      #ifdef NS_PLATFORM_WINDOWS
        WSAPOLLFD poll;
        poll.fd = mSocket;
        poll.events = POLLRDNORM;
        poll.revents = 0;
        if (WSAPoll(&poll, 1, 0) == SOCKET_ERROR || (poll.revents & (POLLERR | POLLHUP)) != 0)
        {
            Disconnect();
            return false;
        }
        return (poll.revents & POLLRDNORM) != 0;

      #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        pollfd poll;
        poll.fd = mSocket;
        poll.events = POLLRDNORM;
        poll.revents = 0;
        if (::poll(&poll, 1, 0) == -1 || (poll.revents & (POLLERR | POLLHUP)) != 0)
        {
            Disconnect();
            return false;
        }

        return (poll.revents & POLLRDNORM) != 0;
      #endif
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServerSocket::Listen(const char* address, uint32_t port)
{
    NS_ASSERT(!Connected());

  #if defined(NS_PLATFORM_WINDOWS)
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
    NS_ASSERT(mSocket != INVALID_SOCKET);

    sockaddr_in sock = {};
    sock.sin_family = AF_INET;
    inet_pton(AF_INET, address, &sock.sin_addr);
    sock.sin_port = htons((u_short)port);

    if (bind(mSocket, (sockaddr*)&sock, sizeof(sock)) == SOCKET_ERROR)
    {
        Disconnect();
    }

    if (listen(mSocket, (int)ListenBacklog) == SOCKET_ERROR)
    {
        Disconnect();
    }

  #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
    mSocket = socket(AF_INET, SOCK_STREAM, 0);
    NS_ASSERT(mSocket != -1);

    sockaddr_in sock = {};
    sock.sin_family = AF_INET;
    inet_pton(AF_INET, address, &sock.sin_addr);
    sock.sin_port = htons((u_short)port);

    if (bind(mSocket, (sockaddr*)&sock, sizeof(sock)) == -1)
    {
        Disconnect();
    }

    if (listen(mSocket, (int)ListenBacklog) == -1)
    {
        Disconnect();
    }
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::Accept(LangServerSocket& socket)
{
    if (!PendingBytes())
    {
        return false;
    }

    if (socket.Connected())
    {
        return false;
    }

    return AcceptBlock(socket);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::AcceptBlock(LangServerSocket& socket)
{
  #if defined(NS_PLATFORM_WINDOWS)
    socket.mSocket = accept(mSocket, nullptr, nullptr);
  #elif defined(NS_PLATFORM_OSX)
    socket.mSocket = accept(mSocket, nullptr, nullptr);
    int option_value = 1;
    setsockopt(socket.mSocket, SOL_SOCKET, SO_NOSIGPIPE, &option_value, sizeof(option_value));
  #elif defined(NS_PLATFORM_LINUX)
    socket.mSocket = accept(mSocket, nullptr, nullptr);
  #endif

    return socket.Connected();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::LangServerSocket::Open()
{
    NS_ASSERT(!Connected());

  #if defined(NS_PLATFORM_WINDOWS)
    mSocket = socket(AF_INET, SOCK_DGRAM, 0);
    NS_ASSERT(mSocket != INVALID_SOCKET);

    int broadcastOn = 1;
    int error = setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastOn, sizeof(broadcastOn));
    NS_ASSERT(error == 0);

  #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
    mSocket = socket(AF_INET, SOCK_DGRAM, 0);
    NS_ASSERT(mSocket != -1);

    int broadcastOn = 1;
    int error = setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastOn, sizeof(broadcastOn));
    NS_ASSERT(error == 0);
  #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool NoesisApp::LangServerSocket::Broadcast(const char* buffer, uint32_t len, uint32_t port)
{
    if (Connected())
    {
      #if defined(NS_PLATFORM_WINDOWS)
        sockaddr_in sock = {};
        sock.sin_family = AF_INET;
        sock.sin_addr.s_addr = INADDR_BROADCAST;
        sock.sin_port = htons((u_short)port);

        int sent = sendto(mSocket, buffer, len, 0, (sockaddr*)&sock, sizeof(sock));
        if (sent == SOCKET_ERROR || (uint32_t)sent != len)
        {
            Disconnect();
            return false;
        }

        return true;

      #elif defined(NS_PLATFORM_OSX) || defined(NS_PLATFORM_LINUX)
        sockaddr_in sock = {};
        sock.sin_family = AF_INET;
        sock.sin_addr.s_addr = INADDR_BROADCAST;
        sock.sin_port = htons((u_short)port);

        ssize_t sent = sendto(mSocket, buffer, len, 0, (sockaddr*)&sock, sizeof(sock));
        if (sent == -1 || (uint32_t)sent != len)
        {
            Disconnect();
            return false;
        }

        return true;
      #endif
    }

    return false;
}

#endif
