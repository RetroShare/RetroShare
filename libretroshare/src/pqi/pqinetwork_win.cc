int errno;

#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

void showSocketError(std::string &out)
{
	int err = WSAGetLastError();
	rs_sprintf_append(out, "\tSocket Error(%d) : %s\n", err, socket_errorType(err).c_str());
}

std::string socket_errorType(int err)
{
	if (err == WSAEBADF)
	{
		return std::string("WSABADF");
	}


	else if (err == WSAEINTR)
	{
		return std::string("WSAEINTR");
	}
	else if (err == WSAEACCES)
	{
		return std::string("WSAEACCES");
	}
	else if (err == WSAEFAULT)
	{
		return std::string("WSAEFAULT");
	}
	else if (err == WSAEINVAL)
	{
		return std::string("WSAEINVAL");
	}
	else if (err == WSAEMFILE)
	{
		return std::string("WSAEMFILE");
	}
	else if (err == WSAEWOULDBLOCK)
	{
		return std::string("WSAEWOULDBLOCK");
	}
	else if (err == WSAEINPROGRESS)
	{
		return std::string("WSAEINPROGRESS");
	}
	else if (err == WSAEALREADY)
	{
		return std::string("WSAEALREADY");
	}
	else if (err == WSAENOTSOCK)
	{
		return std::string("WSAENOTSOCK");
	}
	else if (err == WSAEDESTADDRREQ)
	{
		return std::string("WSAEDESTADDRREQ");
	}
	else if (err == WSAEMSGSIZE)
	{
		return std::string("WSAEMSGSIZE");
	}
	else if (err == WSAEPROTOTYPE)
	{
		return std::string("WSAEPROTOTYPE");
	}
	else if (err == WSAENOPROTOOPT)
	{
		return std::string("WSAENOPROTOOPT");
	}
	else if (err == WSAENOTSOCK)
	{
		return std::string("WSAENOTSOCK");
	}
	else if (err == WSAEISCONN)
	{
		return std::string("WSAISCONN");
	}
	else if (err == WSAECONNREFUSED)
	{
		return std::string("WSACONNREFUSED");
	}
	else if (err == WSAECONNRESET)
	{
		return std::string("WSACONNRESET");
	}
	else if (err == WSAETIMEDOUT)
	{
		return std::string("WSATIMEDOUT");
	}
	else if (err == WSAENETUNREACH)
	{
		return std::string("WSANETUNREACH");
	}
	else if (err == WSAEADDRINUSE)
	{
		return std::string("WSAADDRINUSE");
	}
	else if (err == WSAEAFNOSUPPORT)
	{
		return std::string("WSAEAFNOSUPPORT (normally UDP related!)");
	}

	return std::string("----WINDOWS OPERATING SYSTEM FAILURE----");
}

// implement the improved unix inet address fn.
// using old one.
int inet_aton(const char *name, struct in_addr *addr)
{
	return (((*addr).s_addr = inet_addr(name)) != INADDR_NONE);
}

int	WinToUnixError(int error)
{
#ifdef NET_DEBUG
	std::cerr << "WinToUnixError(" << error << ")" << std::endl;
#endif
	switch(error)
	{
	    case WSAEINPROGRESS:
		    return EINPROGRESS;
		    break;
	    case WSAEWOULDBLOCK:
		    return EINPROGRESS;
		    break;
	    case WSAENETUNREACH:
		    return ENETUNREACH;
		    break;
	    case WSAETIMEDOUT:
		    return ETIMEDOUT;
		    break;
	    case WSAEHOSTDOWN:
		    return EHOSTDOWN;
		    break;
	    case WSAECONNREFUSED:
		    return ECONNREFUSED;
		    break;
	    case WSAECONNRESET:
		    return ECONNRESET;
		    break;
	    default:
#ifdef NET_DEBUG
		    std::cerr << "WinToUnixError(" << error << ") Code Unknown!";
			std::cerr << std::endl;
#endif
		    break;
	}
	return ECONNREFUSED; /* sensible default? */
}

bool _getLocalAddresses(std::list<sockaddr_storage> &addrs) {
	// Seems strange to me but M$ documentation suggests to allocate this way...
	DWORD bf_size = 16000;
	IP_ADAPTER_ADDRESSES* adapter_addresses = (IP_ADAPTER_ADDRESSES*) rs_malloc(bf_size);

	if(adapter_addresses == NULL)
		return false ;

	DWORD error = GetAdaptersAddresses(AF_UNSPEC,
	                                   GAA_FLAG_SKIP_MULTICAST |
	                                   GAA_FLAG_SKIP_DNS_SERVER |
	                                   GAA_FLAG_SKIP_FRIENDLY_NAME,
	                                   NULL,
	                                   adapter_addresses,
	                                   &bf_size);
	if (error != ERROR_SUCCESS)
	{
	   std::cerr << "FATAL ERROR: getLocalAddresses failed!" << std::endl;
	   return false ;
	}

	IP_ADAPTER_ADDRESSES* adapter(NULL);
	for(adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next)
	{
		IP_ADAPTER_UNICAST_ADDRESS* address;
		for ( address = adapter->FirstUnicastAddress; address; address = address->Next)
		{
			sockaddr_storage tmp;
			sockaddr_storage_clear(tmp);
			if (sockaddr_storage_copyip(tmp, * reinterpret_cast<sockaddr_storage*>(address->Address.lpSockaddr)))
				addrs.push_back(tmp);
		}
	}

	free(adapter_addresses);
	return true;
}

int _unix_close(int fd) {
	return ret = closesocket(fd);
}

int _unix_connect(int ret) {
	if (ret != 0)
	{
		errno = WinToUnixError(WSAGetLastError());
		ret = -1;
	}
	return ret;
}

int _unix_fcntl_nonblock(int fd) {
	int ret;
	unsigned long int on = 1;
	ret = ioctlsocket(fd, FIONBIO, &on);

#ifdef NET_DEBUG
	std::cerr << "unix_fcntl_nonblock()" << std::endl;
#endif

	if (ret != 0)
	{
		/* store unix-style error */
		ret = -1;
		errno = WinToUnixError(WSAGetLastError());
	}

	return ret;
}

int _unix_getsockopt_error(int sockfd, int *err) {
	int ret;
	int optlen = 4;

	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) err, &optlen);

	/* translate */
#ifdef NET_DEBUG
	std::cerr << "unix_getsockopt_error() returned: " << (int) err << std::endl;
#endif

	if (*err != 0)
		*err = WinToUnixError(*err);

	return ret;
}

void _unix_socket(int osock) {
	if ((unsigned) osock == INVALID_SOCKET)
	{
		// Invalidate socket Unix style.
		osock = -1;
		errno = WinToUnixError(WSAGetLastError());
	}
}
