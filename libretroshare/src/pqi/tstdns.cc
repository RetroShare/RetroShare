
#include "pqi/pqinetwork.h"

int main()
{
	struct sockaddr_in addr;
	addr.sin_port = htons(1101);

	LookupDNSAddr("www.google.com", addr);
	LookupDNSAddr("10.0.0.19", addr);
	LookupDNSAddr("localhost", addr);
	LookupDNSAddr("nameless", addr);

	return 1;
}

