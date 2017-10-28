#include <netdb.h>

#include <ifaddrs.h>
#include <net/if.h>

void showSocketError(std::string &out)
{
	int err = errno;
	rs_sprintf_append(out, "\tSocket Error(%d) : %s\n", err, socket_errorType(err).c_str());
}

std::string socket_errorType(int err)
{
	if (err == EBADF)
	{
		return std::string("EBADF");
	}
	else if (err == EINVAL)
	{
		return std::string("EINVAL");
	}
	else if (err == EFAULT)
	{
		return std::string("EFAULT");
	}
	else if (err == ENOTSOCK)
	{
		return std::string("ENOTSOCK");
	}
	else if (err == EISCONN)
	{
		return std::string("EISCONN");
	}
	else if (err == ECONNREFUSED)
	{
		return std::string("ECONNREFUSED");
	}
	else if (err == ETIMEDOUT)
	{
		return std::string("ETIMEDOUT");
	}
	else if (err == ENETUNREACH)
	{
		return std::string("ENETUNREACH");
	}
	else if (err == EADDRINUSE)
	{
		return std::string("EADDRINUSE");
	}
	else if (err == EINPROGRESS)
	{
		return std::string("EINPROGRESS");
	}
	else if (err == EALREADY)
	{
		return std::string("EALREADY");
	}
	else if (err == EAGAIN)
	{
		return std::string("EAGAIN");
	}
	else if (err == EISCONN)
	{
		return std::string("EISCONN");
	}
	else if (err == ENOTCONN)
	{
		return std::string("ENOTCONN");
	}
	// These ones have been turning up in SSL CONNECTION FAILURES.
	else if (err == EPIPE)
	{
		return std::string("EPIPE");
	}
	else if (err == ECONNRESET)
	{
		return std::string("ECONNRESET");
	}
	else if (err == EHOSTUNREACH)
	{
		return std::string("EHOSTUNREACH");
	}
	else if (err == EADDRNOTAVAIL)
	{
		return std::string("EADDRNOTAVAIL");
	}
	//

	return std::string("UNKNOWN ERROR CODE - ASK RS-DEVS TO ADD IT!");
}

bool getLocalAddresses_unix(std::list<sockaddr_storage> &addrs) {
	struct ifaddrs *ifsaddrs, *ifa;

	if(getifaddrs(&ifsaddrs) != 0)
	{
	   std::cerr << "FATAL ERROR: getLocalAddresses failed!" << std::endl;
	   return false ;
	}

	for ( ifa = ifsaddrs; ifa; ifa = ifa->ifa_next ) {
		if ( ifa->ifa_addr && (ifa->ifa_flags & IFF_UP) )
		{
			sockaddr_storage tmp;
			sockaddr_storage_clear(tmp);
			if (sockaddr_storage_copyip(tmp, * reinterpret_cast<sockaddr_storage*>(ifa->ifa_addr)))
				addrs.push_back(tmp);
		}
	}

	freeifaddrs(ifsaddrs);
	return true;
}

int unix_close_unix(int fd) {
	return close(fd);
}

int unix_connect_unix(int ret) {
	// emtpy
	return ret;
}

int unix_fcntl_nonblock_unix(int fd) {
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int unix_getsockopt_error_unix(int sockfd, int *err) {
	socklen_t optlen = 4;
	return getsockopt(sockfd, SOL_SOCKET, SO_ERROR, err, &optlen);
}

void unix_socket_unix(int fd)  {
	// empty
	(void) fd;
}

const struct pqinetworkOps netOps = {
	.getLocalAddresses = getLocalAddresses_unix,
	.unix_close = unix_close_unix,
	.unix_connect = unix_connect_unix,
	.unix_fcntl_nonblock = unix_fcntl_nonblock_unix,
	.unix_getsockopt_error = unix_getsockopt_error_unix,
	.unix_socket = unix_socket_unix
};
