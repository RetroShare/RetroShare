/*
 * nxsdummyservices.h
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#ifndef NXSDUMMYSERVICES_H_
#define NXSDUMMYSERVICES_H_

#include <list>
#include <map>
#include <retroshare/rstypes.h>
#include <gxs/rsgixs.h>
#include <gxs/rsgxsnetutils.h>
#include <algorithm>


namespace rs_nxs_test
{


	/*!
	 * This dummy circles implementation
	 * allow instantiation with simple membership
	 * list for a given circle
	 */
	class RsNxsSimpleDummyCircles : public RsGcxs
	{
	public:


		typedef std::map<RsGxsCircleId, std::list<RsPgpId> > Membership;

		/*!
		 *
		 * @param membership
		 * @param cached
		 */
		RsNxsSimpleDummyCircles(std::list<Membership>& membership, bool cached);

		/* GXS Interface - for working out who can receive */
		bool isLoaded(const RsGxsCircleId &circleId);
		bool loadCircle(const RsGxsCircleId &circleId);

		int canSend(const RsGxsCircleId &circleId, const RsPgpId &id);
		int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id);
		bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist);

	private:

		std::list<Membership> mMembership;
	};

	/*!
	 * This dummy circles implementation
	 * allow instantiation with simple membership
	 * list for a given circle
	 */
	class RsNxsDelayedDummyCircles : public RsGcxs
	{
	public:

		/*!
		 *
		 * @param membership
		 * @param countBeforePresent how many times a pgpid is checked before it becomes present
		 */
		RsNxsDelayedDummyCircles(int countBeforePresent);
		virtual ~RsNxsDelayedDummyCircles();

		/* GXS Interface - for working out who can receive */
		bool isLoaded(const RsGxsCircleId &circleId);
		bool loadCircle(const RsGxsCircleId &circleId);

		int canSend(const RsGxsCircleId &circleId, const RsPgpId &id);
		int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id);
		bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist);

	private:

		bool allowed(const RsGxsCircleId& circleId);

	private:

		typedef int CallCount;
		std::map<RsGxsCircleId, CallCount> mMembershipCallCount;
		int mCountBeforePresent;
	};


	/*!
	 * This dummy reputation allows you to set the
	 * reputations of peers
	 */
	class RsNxsSimpleDummyReputation : public RsGixsReputation
	{
	public:

		typedef std::map<RsGxsId, GixsReputation> RepMap;


		/*!
		 * Instantiate the dummy rep service with
		 * a reputation map
		 * @param repMap should contain the reputations of a set of ids
		 * @param cached this means initial call for an ids \n
		 * 	 	  rep will return false, until a request has been made to load it
		 */
		RsNxsSimpleDummyReputation(RepMap& repMap, bool cached );

		bool haveReputation(const RsGxsId &id);
		bool loadReputation(const RsGxsId &id, const std::list<RsPeerId>& peers);
		bool getReputation(const RsGxsId &id, GixsReputation &rep);

	private:

		RepMap mRepMap;
	};


	/*!
	 * Very simple net manager
	 */
	class RsNxsNetDummyMgr : public RsNxsNetMgr
	{

	public:

		RsNxsNetDummyMgr(const RsPeerId& ownId, const std::list<RsPeerId>& peers) : mOwnId(ownId), mPeers(peers) {

		}
		const RsPeerId& getOwnId()  { return mOwnId; }
		void getOnlineList(const uint32_t serviceId, std::set<RsPeerId>& ssl_peers)
		{
			RsPeerId::std_list::iterator lit = mPeers.begin();

			for(; lit != mPeers.end(); lit++)
				ssl_peers.insert(*lit);
		}

	private:

		RsPeerId mOwnId;
		std::list<RsPeerId> mPeers;

	};


}

#endif /* NXSDUMMYSERVICES_H_ */
