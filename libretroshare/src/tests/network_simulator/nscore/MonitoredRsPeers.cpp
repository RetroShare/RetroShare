#include <iostream>
#include "MonitoredRsPeers.h"

MonitoredRsPeers::MonitoredRsPeers(const Network& net)
	: p3Peers(NULL,NULL,NULL),_network(net)
{
}

bool MonitoredRsPeers::getPeerDetails(const std::string& str,RsPeerDetails& details)
{
	std::cerr << __PRETTY_FUNCTION__ << " called" << std::endl;
}
