#include <rsserver/p3peers.h>

class Network ;

class MonitoredRsPeers: public p3Peers
{
	public:
		MonitoredRsPeers(const Network& net) ;

		virtual bool getPeerDetails(const std::string& peer_id,RsPeerDetails& details) ;

	private:
		const Network& _network ;
};
