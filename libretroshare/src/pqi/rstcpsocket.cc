/******************************* BEGIN WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#else
#include <ws2tcpip.h>
// Missing defines in MinGW
#ifndef MSG_WAITALL
#define MSG_WAITALL 8
#endif
#endif
/********************************* END WINDOWS/UNIX SPECIFIC PART ******************/
#include <string.h>
#include <iostream>

#include "rstcpsocket.h"

RsTcpSocket::RsTcpSocket(const std::string& tcp_address,uint16_t tcp_port)
    :RsFdBinInterface(0),mState(DISCONNECTED),mConnectAddress(tcp_address),mConnectPort(tcp_port),mSocket(0)
{
}

RsTcpSocket::RsTcpSocket()
    :RsFdBinInterface(0),mState(DISCONNECTED),mConnectAddress("0.0.0.0"),mConnectPort(0),mSocket(0)
{
}

bool RsTcpSocket::connect(const std::string& tcp_address,uint16_t tcp_port)
{
    if(mState == CONNECTED)
        close();

    mConnectPort = tcp_port;
    mConnectAddress = tcp_address;

    return connect();
}
bool RsTcpSocket::connect()
{
    int CreateSocket = 0;
    char dataReceived[1024];
    struct sockaddr_in ipOfServer;

    memset(dataReceived, '0' ,sizeof(dataReceived));

    if((CreateSocket = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("Socket not created \n");
        return false;
    }
    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(mConnectPort);
    ipOfServer.sin_addr.s_addr = inet_addr(mConnectAddress.c_str());

    if(::connect(CreateSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection failed due to port and ip problems, or server is not available. Socket=%d,ConnectPort=%d,ConnectAddress=%s Errno=%d\n",mSocket,mConnectPort,mConnectAddress.c_str(),errno);
        return false;
    }
    mState = CONNECTED;

#ifndef WINDOWS_SYS
    int flags = fcntl(CreateSocket,F_GETFL);
    fcntl(CreateSocket, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long int on = 1;
    ret = ioctlsocket(fd, FIONBIO, &on);
#endif

    setSocket(CreateSocket);

    return true;
}

int RsTcpSocket::close()
{
    RsFdBinInterface::close();
    mState = DISCONNECTED;

    return !::close(mSocket);
}

RsThreadedTcpSocket::RsThreadedTcpSocket(const std::string& tcp_address,uint16_t tcp_port)
    : RsTcpSocket(tcp_address,tcp_port)
{
}
RsThreadedTcpSocket::RsThreadedTcpSocket() : RsTcpSocket()
{
}
void RsThreadedTcpSocket::run()
{
    while(connectionState() == CONNECTED)
    {
        tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    RsWarn() << "Connection to " << connectAddress() << ":" << connectPort() << " is now closed.";
}

RsThreadedTcpSocket::~RsThreadedTcpSocket()
{
    fullstop();   // fully wait for stopping.

    close();
}
