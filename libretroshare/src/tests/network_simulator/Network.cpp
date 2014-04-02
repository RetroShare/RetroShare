#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <list>
#include <string.h>
#include <retroshare/rsids.h>

#include <retroshare/rspeers.h>
#include <turtle/p3turtle.h>
#include <serialiser/rsserial.h>
#include <pqi/p3linkmgr.h>
#include <pqi/p3peermgr.h>
#include <ft/ftserver.h>
#include <ft/ftcontroller.h>
#include <services/p3service.h>
#include "Network.h"
#include "MonitoredTurtle.h"

bool Network::initRandom(uint32_t nb_nodes,float connexion_probability)
{
	_nodes.clear() ;
	_neighbors.clear() ;

	std::vector<RsPeerId> ids ;

	_neighbors.resize(nb_nodes) ;
	ids.resize(nb_nodes) ;

	for(uint32_t i=0;i<nb_nodes;++i)
	{
		ids[i] = RsPeerId::random() ;
		_node_ids[ids[i]] = i ;
	}
		
	// Each node has an exponential law of connectivity to friends.
	//
	for(uint32_t i=0;i<nb_nodes;++i)
	{
		int nb_friends = std::min((int)nb_nodes-1,(int)ceil(-log(1-drand48())/(1.00001-connexion_probability))) ;

		for(uint32_t j=0;j<nb_friends;++j)
		{
			int f = i ;
			while(f==i)
				f = lrand48()%nb_nodes ;

			_neighbors[i].insert(f) ;
			_neighbors[f].insert(i) ;
		}
	}

	for(uint32_t i=0;i<nb_nodes;++i)
	{
		std::cerr << "Added new node with id " << ids[i] << std::endl;

		std::list<RsPeerId> friends ;
		for(std::set<uint32_t>::const_iterator it(_neighbors[i].begin());it!=_neighbors[i].end();++it)
			friends.push_back( ids[*it] ) ;

		_nodes.push_back( new PeerNode( ids[i], friends ));
	}
	return true ;
}

void Network::tick()
{
	std::cerr<< "network loop: tick()" << std::endl;

	// Tick all nodes.

	for(uint32_t i=0;i<n_nodes();++i)
		node(i).tick() ;

	// Get items for each components and send them to their destination.
	//
	for(uint32_t i=0;i<n_nodes();++i)
	{
		RsRawItem *item ;

		while( (item = node(i).outgoing()) != NULL)
		{
			std::cerr << "Tick: send item from " << item->PeerId() << " to " << Network::node(i).id() << std::endl;

			PeerNode& node = node_by_id(item->PeerId()) ;
			item->PeerId(Network::node(i).id()) ;

			node.incoming(item) ;
		}
	}
}

PeerNode& Network::node_by_id(const RsPeerId& id)
{
	std::map<RsPeerId,uint32_t>::const_iterator it = _node_ids.find(id) ;

	if(it == _node_ids.end())
		throw std::runtime_error("Error. Bad id passed to node_by_id ("+id.toStdString()+")") ;

	return node(it->second) ;
}

class FakeLinkMgr: public p3LinkMgrIMPL
{
	public:
		FakeLinkMgr(const RsPeerId& own_id,const std::list<RsPeerId>& friends)
			: p3LinkMgrIMPL(NULL,NULL),_own_id(own_id),_friends(friends)
		{
		}

		virtual const RsPeerId& getOwnId() { return _own_id ; }
		virtual void getOnlineList(std::list<RsPeerId>& lst) { lst = _friends ; }
		virtual uint32_t getLinkType(const RsPeerId&) { return RS_NET_CONN_TCP_ALL | RS_NET_CONN_SPEED_NORMAL; }

		virtual bool getPeerName(const RsPeerId &ssl_id, std::string &name) { name = ssl_id.toStdString() ; return true ;}

	private:
		RsPeerId _own_id ;
		std::list<RsPeerId> _friends ;
};

class FakePublisher: public pqiPublisher
{
	public:
		virtual bool sendItem(RsRawItem *item) 
		{
			_item_queue.push_back(item) ;
		}

		RsRawItem *outgoing() 
		{
            if(_item_queue.empty())
                return NULL ;

            RsRawItem *item = _item_queue.front() ;
			_item_queue.pop_front() ;
			return item ;
		}

	private:
		std::list<RsRawItem*> _item_queue ;
};

class FakePeerMgr: public p3PeerMgrIMPL
{
	public:
		FakePeerMgr(const RsPeerId& own,const std::list<RsPeerId>& ids)
			: p3PeerMgrIMPL(own,RsPgpId(),"no name","location name")
		{
			for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
				_ids.insert(*it) ;
		}

		virtual bool idFriend(const RsPeerId& ssl_id) { return _ids.find(ssl_id) != _ids.end() ; }

		std::set<RsPeerId> _ids ;
};

const RsTurtle *PeerNode::turtle_service() const 
{
	return _turtle ;
}

PeerNode::PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends)
	: _id(id)
{
	// add a service server.
	
	p3LinkMgr *link_mgr = new FakeLinkMgr(id, friends) ;
	p3PeerMgr *peer_mgr = new FakePeerMgr(id, friends) ;

	_publisher = new FakePublisher ;
	p3ServiceControl *ctrl = new p3ServiceControl(link_mgr) ;

	_service_server = new p3ServiceServer(_publisher,ctrl);

	ftServer *ft_server = new ftServer(peer_mgr,ctrl) ;

	_service_server->addService(_turtle = new MonitoredTurtleRouter(ctrl,link_mgr,ft_server),true) ;

	// add a turtle router.
	//
}

PeerNode::~PeerNode()
{
	delete _service_server ;
}

void PeerNode::tick()
{
	std::cerr << "  ticking peer node " << _id << std::endl;
	_service_server->tick() ;
}

void PeerNode::incoming(RsRawItem *item)
{
	_service_server->recvItem(item) ;
}
RsRawItem *PeerNode::outgoing()
{
	return dynamic_cast<FakePublisher*>(_publisher)->outgoing() ;
}

void PeerNode::provideFileHash(const RsFileHash& hash)
{
	_provided_hashes.insert(hash) ;
	_turtle->provideFileHash(hash) ;
}

void PeerNode::manageFileHash(const RsFileHash& hash)
{
	_managed_hashes.insert(hash) ;
	_turtle->monitorTunnels(hash,_ftserver) ;
}

void PeerNode::getTrafficInfo(NodeTrafficInfo& info)
{
	std::vector<std::vector<std::string> > hashes_info ;
	std::vector<std::vector<std::string> > tunnels_info ;
	std::vector<TurtleRequestDisplayInfo > search_reqs_info ;
	std::vector<TurtleRequestDisplayInfo > tunnel_reqs_info ;

	_turtle->getInfo(hashes_info,tunnels_info,search_reqs_info,tunnel_reqs_info) ;

	for(uint32_t i=0;i<tunnels_info.size();++i)
	{
		info.local_src[tunnels_info[i][1]] = tunnels_info[i][0] ;
		info.local_src[tunnels_info[i][2]] = tunnels_info[i][0] ;
	}
}

