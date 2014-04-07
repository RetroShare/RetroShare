#include "PeerNode.h"
#include "FakeComponents.h"
#include "MonitoredTurtleClient.h"
#include "MonitoredGRouterClient.h"

PeerNode::PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends)
	: _id(id)
{
	// add a service server.
	
	p3LinkMgr *link_mgr = new FakeLinkMgr(id, friends) ;
	p3PeerMgr *peer_mgr = new FakePeerMgr(id, friends) ;

	_publisher = new FakePublisher ;
    p3ServiceControl *ctrl = new FakeServiceControl(link_mgr) ;

	_service_server = new p3ServiceServer(_publisher,ctrl);

    RsServicePermissions perms;
    perms.mDefaultAllowed = true ;
    perms.mServiceId = RS_SERVICE_TYPE_TURTLE ;

    ctrl->updateServicePermissions(RS_SERVICE_TYPE_TURTLE,perms) ;

    perms.mDefaultAllowed = true ;
    perms.mServiceId = RS_SERVICE_TYPE_GROUTER ;

    ctrl->updateServicePermissions(RS_SERVICE_TYPE_GROUTER,perms) ;

    // Turtle business

	_service_server->addService(_turtle = new p3turtle(ctrl,link_mgr),true) ;
	_turtle_client = new MonitoredTurtleClient ;
	_turtle_client->connectToTurtleRouter(_turtle) ;

	// global router business.
	//

	_service_server->addService(_grouter = new p3GRouter(ctrl,link_mgr),true) ;
	_grouter_client = new MonitoredGRouterClient ;
	_grouter_client->connectToGlobalRouter(_grouter) ;
}

PeerNode::~PeerNode()
{
	delete _service_server ;
}

void PeerNode::tick()
{
	//std::cerr << "  ticking peer node " << _id << std::endl;
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
    _turtle_client->provideFileHash(hash) ;
}

void PeerNode::manageFileHash(const RsFileHash& hash)
{
	_managed_hashes.insert(hash) ;
	_turtle->monitorTunnels(hash,_turtle_client) ;
}
void PeerNode::sendToGRKey(const GRouterKeyId& key_id)
{
	_grouter_client->sendMessage(key_id) ;
}
void PeerNode::provideGRKey(const GRouterKeyId& key_id)
{
    _grouter_client->provideKey(key_id) ;
    _provided_keys.insert(key_id);
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

