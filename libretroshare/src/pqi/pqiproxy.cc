#include "util/rsdebug.h"
#include "util/rsnet.h"

#include "pqi/pqiproxy.h"

//#define PROXY_DEBUG 1

int pqiproxyconnection::proxy_negotiate_connection(int sockfd)
{
    int ret = 0;
    switch(mProxyState)
    {
    case PROXY_STATE_INIT:
        ret = Proxy_Send_Method(sockfd); // checks basic conn, sends Method when able.
        break;

    case PROXY_STATE_WAITING_METHOD_RESPONSE:
        ret = Proxy_Send_Address(sockfd); // waits for Method Response, send Address when able.
        break;

    case PROXY_STATE_WAITING_SOCKS_RESPONSE:
        ret = Proxy_Connection_Complete(sockfd); // wait for ACK.
        break;

    case PROXY_STATE_CONNECTION_COMPLETE:

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Basic_Connection_Complete() COMPLETED";
        std::cerr << std::endl;
#endif

        return 1;

    case PROXY_STATE_FAILED:

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Basic_Connection_Complete() FAILED";
        std::cerr << std::endl;
#endif

        return -1;
    }


#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Basic_Connection_Complete() IN PROGRESS";
    std::cerr << std::endl;
#endif

    // In Progress.
    return 0;
}


int	pqiproxyconnection::Proxy_Send_Method(int sockfd)
{

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Send_Method() Basic complete, sending Method";
    std::cerr << std::endl;
#endif

    /* send hello to proxy server */
    char method_hello_data[3] = { 0x05, 0x01, 0x00 };  // [ Ver | nMethods (1) | No Auth Method ]

    int sent = send(sockfd, method_hello_data, 3, 0);
    if (sent != 3)
    {

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Send_Method() Send Failure";
        std::cerr << std::endl;
#endif
        return -1;
    }

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Send_Method() Send Method Okay";
    std::cerr << std::endl;
#endif

    mProxyState = PROXY_STATE_WAITING_METHOD_RESPONSE;

    return 1;
}

int	pqiproxyconnection::Proxy_Method_Response(int sockfd)
{

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Method_Response()";
    std::cerr << std::endl;
#endif

    /* get response from proxy server */

    char method_response[2];

    /*
      first it was:

      int recvd = recv(sockfd, method_response, 2, MSG_WAITALL);

      this does not work on windows, because the socket is in nonblocking mode
      the winsock reference says about the recv function and MSG_WAITALL:

      "Note that if the underlying transport does not support MSG_WAITALL,
       or if the socket is in a non-blocking mode, then this call will fail with WSAEOPNOTSUPP."

       now it is a two step process:

       int recvd = recv(sockfd, method_response, 2, MSG_PEEK); // test how many bytes are in the input queue
       if (enaugh bytes available){
           recvd = recv(sockfd, method_response, 2, 0);
       }

       this does not work on windows:
       if ((recvd == -1) && (errno == EAGAIN)) return TRY_AGAIN_LATER;

       instead have to do:
       if ((recvd == -1) && (WSAGetLastError() == WSAEWOULDBLOCK)) return TRY_AGAIN_LATER;
     */

    // test how many bytes can be read from the queue
    int recvd = recv(sockfd, method_response, 2, MSG_PEEK);
    if (recvd != 2)
    {
#ifdef WINDOWS_SYS
        if ((recvd == -1) && (WSAGetLastError() == WSAEWOULDBLOCK))
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Method_Response() waiting for more data (windows)";
            std::cerr << std::endl;
#endif
            return 0;
        }
#endif
        if ((recvd == -1) && (errno == EAGAIN))
        {

#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Method_Response() EAGAIN";
            std::cerr << std::endl;
#endif

            return 0;
        }
        else if (recvd == -1)
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Method_Response() recv error peek";
            std::cerr << std::endl;
#endif
            return -1;
        }
        else
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Method_Response() waiting for more data";
            std::cerr << std::endl;
#endif
            return 0;
        }
    }

    // read the bytes
    recvd = recv(sockfd, method_response, 2, 0);
    if (recvd != 2)
    {
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Method_Response() recv error";
        std::cerr << std::endl;
#endif
        return -1;
    }

    // does it make sense?
    if (method_response[0] != 0x05)
    {

        // Error.
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Method_Response() Error response[0] != 0x05. Is: ";
        std::cerr << (uint32_t) method_response[0];
        std::cerr << std::endl;
#endif
        return -1;
    }

    if (method_response[1] != 0x00)
    {

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Method_Response() Error response[0] != 0x00. Is: ";
        std::cerr << (uint32_t) method_response[1];
        std::cerr << std::endl;
#endif
        // Error.
        return -1;
    }


#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Method_Response() Response Okay";
    std::cerr << std::endl;
#endif

    return 1;
}

#define MAX_SOCKS_REQUEST_LEN  262  // 4 + 1 + 255 + 2.

int	pqiproxyconnection::Proxy_Send_Address(int sockfd)
{

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Send_Address() Checking Method Response";
    std::cerr << std::endl;
#endif

    // Check Method Response.
    int ret = Proxy_Method_Response(sockfd);
    if (ret != 1)
    {
        return ret; // Method Response not complete.
    }

    char socks_request[MAX_SOCKS_REQUEST_LEN] =
    { 0x05, // SOCKS VERSION.
      0x01, // CONNECT (Tor doesn't support BIND or UDP).
      0x00, // RESERVED.
      0x03, // ADDRESS TYPE (Domain Name)
      0x00, // Length of Domain name... the rest is variable so can't hard code it!
    };

    /* get the length of the domain name, pack so we can't overflow uint8_t */
    uint8_t len = mDomainAddress.length();
    socks_request[4] = len;
    for(int i = 0; i < len; i++)
    {
        socks_request[5 + i] = mDomainAddress[i];
    }

    /* now add the port, being careful with packing */
    uint16_t net_port = htons(mRemotePort);
    socks_request[5 + len] = ((uint8_t *) &net_port)[0];
    socks_request[5 + len + 1] = ((uint8_t *) &net_port)[1];

    int pkt_len = 5 + len + 2;

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Send_Address() Sending String: ";
    for(int i = 0; i < pkt_len; i++)
        std::cerr << (uint32_t) socks_request[i];
    std::cerr << std::endl;
#endif
    int sent = send(sockfd, socks_request, pkt_len, 0);
    if (sent != pkt_len)
    {

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Send_Address() Send Error";
        std::cerr << std::endl;
#endif

        return -1;
    }


#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Send_Address() Sent Okay";
    std::cerr << std::endl;
#endif

    mProxyState = PROXY_STATE_WAITING_SOCKS_RESPONSE;
    return 1;
}

int	pqiproxyconnection::Proxy_Connection_Complete(int sockfd)
{
    /* get response from proxy server */
    /* response is similar format to request - with variable length data */

    char socks_response[MAX_SOCKS_REQUEST_LEN];

    // test how many bytes can be read
    int recvd = recv(sockfd, socks_response, 5, MSG_PEEK);
    if (recvd != 5)
    {
#ifdef WINDOWS_SYS
        if ((recvd == -1) && (WSAGetLastError() == WSAEWOULDBLOCK))
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() waiting for more data (windows)";
            std::cerr << std::endl;
#endif
            return 0;
        }
#endif
        if ((recvd == -1) && (errno == EAGAIN))
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() EAGAIN";
            std::cerr << std::endl;
#endif
            return 0;
        }
        else if (recvd == -1)
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() recv error peek";
            std::cerr << std::endl;
#endif
            return -1;
        }
        else
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() waiting for more data";
            std::cerr << std::endl;
#endif
            return 0;
        }
    }

    // read the bytes
    recvd = recv(sockfd, socks_response, 5, 0);
    if (recvd != 5)
    {
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() recv error";
        std::cerr << std::endl;
#endif
        return -1;
    }

    // error checking.
    if (socks_response[0] != 0x05)
    {

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() ERROR socks_response[0] != 0x05. is: ";
        std::cerr << (uint32_t) socks_response[0];
        std::cerr << std::endl;
#endif

        // error.
        return -1;
    }

    if (socks_response[1] != 0x00)
    {

#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() ERROR socks_response[1] != 0x00. is: ";
        std::cerr << (uint32_t) socks_response[1];
        std::cerr << std::endl;
#endif

        // connection failed.
        return -1;
    }

    int address_bytes = 0;
    switch(socks_response[3]) // Address Type.
    {
    case 0x01:
        // IPv4 4 address bytes.
        address_bytes = 4;
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() IPv4 Address Type";
        std::cerr << std::endl;
#endif
        break;
    case 0x04:
        // IPv6 16 address bytes.
        address_bytes = 16;
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() IPv6 Address Type";
        std::cerr << std::endl;
#endif
        break;
    case 0x03:
        // Variable address bytes - specified in next byte.
        address_bytes = 1 + socks_response[4];
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() Domain Address Type. len: " << address_bytes;
        std::cerr << std::endl;
#endif
        break;
    default:
        // unknown error.
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() ERROR Unknown Address Type";
        std::cerr << std::endl;
#endif
        return -1;
        break;
    }


    // test how many bytes can be read
    recvd = recv(sockfd, &(socks_response[5]), address_bytes + 1, MSG_PEEK); // address_bytes - 1 + 2...
    if (recvd != address_bytes + 1)
    {
#ifdef WINDOWS_SYS
        if((recvd == -1) && (WSAGetLastError() == WSAEWOULDBLOCK))
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() waiting for more data(2) (windows)";
            std::cerr << std::endl;
#endif
            return 0;
        }
#endif
        if ((recvd == -1) && (errno == EAGAIN))
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() ERROR EAGAIN at end.";
            std::cerr << std::endl;
#endif
            // Waiting - shouldn't happen.
            return 0;
        }
        else if (recvd == -1)
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() ERROR recving(2)";
            std::cerr << std::endl;
#endif
            return -1;
        }
        else
        {
#ifdef PROXY_DEBUG
            std::cerr << "pqisslproxy::Proxy_Connection_Complete() waiting for more data(2)";
            std::cerr << std::endl;
#endif
            return 0;
        }
    }

    // read the bytes
    recvd = recv(sockfd, &(socks_response[5]), address_bytes + 1, 0); // address_bytes - 1 + 2...
    if (recvd != address_bytes + 1)
    {
#ifdef PROXY_DEBUG
        std::cerr << "pqisslproxy::Proxy_Connection_Complete() recv error (2)";
        std::cerr << std::endl;
#endif
        return -1;
    }

#ifdef PROXY_DEBUG
    std::cerr << "pqisslproxy::Proxy_Connection_Complete() Received String: ";
    for(int i = 0; i < 4 + address_bytes + 2; i++)
        std::cerr << (uint32_t) socks_response[i];
    std::cerr << std::endl;
#endif

    // should print address.
    // if we get here - connection is good!.
    mProxyState = PROXY_STATE_CONNECTION_COMPLETE;
    return 1;

}

