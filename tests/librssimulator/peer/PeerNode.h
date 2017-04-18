#pragma once

#include <list>

#include "retroshare/rsids.h"

class RsRawItem ;
class p3ServiceServer ;
class FakePublisher;
class p3ServiceControl;
class FakeLinkMgr;
class FakePeerMgr;
class FakeNetMgr;
class RsNxsNetMgr;
class FakeNxsNetMgr;
class pqiMonitor;
class pqiServiceMonitor;
class pqiService;

#warning: Cppcheck(noCopyConstructor): class 'PeerNode' does not have a copy constructor which is recommended since the class contains a pointer to allocated memory.
// cppcheck-suppress noCopyConstructor
class PeerNode
{
	public:
		PeerNode(const RsPeerId& id,const std::list<RsPeerId>& friends, bool online) ;
		~PeerNode() ;

		RsRawItem *outgoing() ;
		void incoming(RsRawItem *) ;
		bool haveOutgoingPackets();

		const RsPeerId& id() const { return _id ;}

		void tick() ;

		p3ServiceControl *getServiceControl() const { return _service_control; }
//		p3LinkMgr *getLinkMgr();
//		p3PeerMgr *getPeerMgr();
//		p3NetMgr *getNetMgr();
		RsNxsNetMgr *getNxsNetMgr();

		void AddService(pqiService *service);
		void AddPqiMonitor(pqiMonitor *service);
		void AddPqiServiceMonitor(pqiServiceMonitor *service);

		// 
		void notifyOfFriends();
		void bringOnline(std::list<RsPeerId> &friends);
		void takeOffline(std::list<RsPeerId> &friends);

	private:
		RsPeerId _id;

		p3ServiceServer *_service_server ;
		FakePublisher *mPublisher ;
		p3ServiceControl *_service_control; 
		FakeLinkMgr *mLinkMgr;
		FakePeerMgr *mPeerMgr;
//UNUSED FakeNetMgr *mNetMgr;
		FakeNxsNetMgr *mNxsNetMgr;

		/* for monitors */
		std::list<pqiMonitor *> mPqiMonitors;
		std::list<pqiServiceMonitor *> mPqiServiceMonitors;
};





