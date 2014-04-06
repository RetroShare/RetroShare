#pragma once

#include <retroshare/rstypes.h>
#include <turtle/p3turtle.h>
#include <grouter/p3grouter.h>

class MonitoredTurtleClient ;
class MonitoredGRouterClient ;
class RsTurtle ;
class p3turtle ;
class p3GRouter ;
class pqiPublisher ;
class RsRawItem ;
class p3ServiceServer ;

class PeerNode
{
	public:
		struct NodeTrafficInfo
		{
			std::map<std::string,std::string> local_src ;	// peer id., tunnel id
			std::map<std::string,std::string> local_dst ;
		};

		PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends) ;
		~PeerNode() ;

		RsRawItem *outgoing() ;
		void incoming(RsRawItem *) ;

		const RsPeerId& id() const { return _id ;}

		void tick() ;

		// Turtle-related methods
		//
		const RsTurtle *turtle_service() const { return _turtle ; }
        const RsGRouter *global_router_service() const { return _grouter ; }

		void manageFileHash(const RsFileHash& hash) ;
		void provideFileHash(const RsFileHash& hash) ;

		const std::set<RsFileHash>& providedHashes() const { return _provided_hashes; }
		const std::set<RsFileHash>& managedHashes() const { return _managed_hashes; }

		void getTrafficInfo(NodeTrafficInfo& trinfo) ;	// 

		// GRouter-related methods
		//
		void provideGRKey(const GRouterKeyId& key_id) ;
		void sendToGRKey(const GRouterKeyId& key_id) ;

		const std::set<GRouterKeyId>& providedGRKeys() const { return _provided_keys; }

	private:
		p3ServiceServer *_service_server ;
		pqiPublisher *_publisher ;
		RsPeerId _id ;

		// turtle stuff
		//
		p3turtle *_turtle ;
		MonitoredTurtleClient *_turtle_client ;

		// grouter stuff
		//
		p3GRouter *_grouter ;
		MonitoredGRouterClient *_grouter_client ;

		std::set<RsFileHash> _provided_hashes ;
		std::set<RsFileHash> _managed_hashes ;
		std::set<GRouterKeyId> _provided_keys ;
};

