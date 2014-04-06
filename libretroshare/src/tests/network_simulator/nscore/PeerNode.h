#pragma once

#include <retroshare/rstypes.h>
#include <turtle/p3turtle.h>

class MonitoredTurtleRouter ;
class MonitoredTurtleClient ;
class RsTurtle ;
class p3turtle ;
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

		const RsTurtle *turtle_service() const { return _turtle ; }

		// Turtle-related methods
		//
		void manageFileHash(const RsFileHash& hash) ;
		void provideFileHash(const RsFileHash& hash) ;

		const std::set<RsFileHash>& providedHashes() const { return _provided_hashes; }
		const std::set<RsFileHash>& managedHashes() const { return _managed_hashes; }

		void getTrafficInfo(NodeTrafficInfo& trinfo) ;	// 

	private:
		p3ServiceServer *_service_server ;
		p3turtle *_turtle ;
		MonitoredTurtleClient *_ftserver ;
		pqiPublisher *_publisher ;
		RsPeerId _id ;

		std::set<RsFileHash> _provided_hashes ;
		std::set<RsFileHash> _managed_hashes ;
};

