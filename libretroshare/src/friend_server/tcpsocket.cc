#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>

#include "tcpsocket.h"

TcpSocket::TcpSocket(const std::string& tcp_address,uint16_t tcp_port)
    :FsBioInterface(0),mState(DISCONNECTED),mConnectAddress(tcp_address),mConnectPort(tcp_port),mSocket(0)
{
}
int TcpSocket::connect()
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

    if(::connect(mSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection failed due to port and ip problems, or server is not available\n");
        return false;
    }
    mState = CONNECTED;
    setSocket(mSocket);

    return true;
}

int TcpSocket::close()
{
    FsBioInterface::close();

    return !::close(mSocket);
}

ThreadedTcpSocket::ThreadedTcpSocket(const std::string& tcp_address,uint16_t tcp_port)
    : TcpSocket(tcp_address,tcp_port)
{
}

void ThreadedTcpSocket::run()
{
    if(!connect())
    {
        RsErr() << "Cannot connect socket to " << connectAddress() << ":" << connectPort() ;
        return ;
    }

    while(connectionState() == CONNECTED)
    {
        tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    RsWarn() << "Connection to " << connectAddress() << ":" << connectPort() << " is now closed.";
}

ThreadedTcpSocket::~ThreadedTcpSocket()
{
    fullstop();   // fully wait for stopping.

    close();
}
