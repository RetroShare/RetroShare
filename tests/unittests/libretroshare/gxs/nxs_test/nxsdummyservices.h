/*
 * nxsdummyservices.h
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#ifndef NXSDUMMYSERVICES_H_
#define NXSDUMMYSERVICES_H_


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
		RsNxsSimpleDummyCircles(std::list<Membership>& membership, bool cached)

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

		RsNxsNetDummyMgr(RsPeerId ownId, std::set<RsPeerId> peers) : mOwnId(ownId), mPeers(peers) {

		}

		const RsPeerId& getOwnId()  { return mOwnId; }
		void getOnlineList(uint32_t serviceId, std::set<RsPeerId>& ssl_peers) { ssl_peers = mPeers; }

	private:

		RsPeerId mOwnId;
		std::set<RsPeerId> mPeers;

	};
}

#endif /* NXSDUMMYSERVICES_H_ */
