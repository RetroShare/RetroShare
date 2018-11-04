/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/nxsdummyservice.h                      *
 *                                                                             *
 * Copyright (C) 2014, Crispy <retroshare.team@gmailcom>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#ifndef NXSDUMMYSERVICES_H_
#define NXSDUMMYSERVICES_H_

#include <list>
#include <map>
#include <retroshare/rstypes.h>
#include <gxs/rsgixs.h>
#include <gxs/rsgxsnetutils.h>
#include <algorithm>
#include <pgp/pgpauxutils.h>


namespace rs_nxs_test
{


	/*!
	 * This is a dummy circles implementation
	 */
	class RsNxsSimpleDummyCircles : public RsGcxs
	{
	public:

		/*!
		 *
		 * @param membership
		 * @param cached
		 */
		RsNxsSimpleDummyCircles();

		/* GXS Interface - for working out who can receive */
		bool isLoaded(const RsGxsCircleId &circleId);
		bool loadCircle(const RsGxsCircleId &circleId);

		int canSend(const RsGxsCircleId &circleId, const RsPgpId &id,bool& should_encrypt);
		int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id);
		virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist);
		virtual bool recipients(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, std::list<RsGxsId>& idlist) ;
		virtual bool isRecipient(const RsGxsCircleId &circleId, const RsGxsGroupId& destination_group, const RsGxsId& id) ;
		virtual bool getLocalCircleServerUpdateTS(const RsGxsCircleId& /*gid*/,time_t& /*grp_server_update_TS*/,time_t& /*msg_server_update_TS*/) { return true ; }
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

		int canSend(const RsGxsCircleId &circleId, const RsPgpId &id,bool& should_encrypt);
		int canReceive(const RsGxsCircleId &circleId, const RsPgpId &id);
		virtual bool recipients(const RsGxsCircleId &circleId, std::list<RsPgpId> &friendlist);
		virtual bool recipients(const RsGxsCircleId &/*circleId*/, const RsGxsGroupId& /*destination_group*/, std::list<RsGxsId>& /*idlist*/) { return true ;}
		virtual bool isRecipient(const RsGxsCircleId &circleId, const RsGxsGroupId& /*destination_group*/, const RsGxsId& /*id*/) { return allowed(circleId) ; }
		virtual bool getLocalCircleServerUpdateTS(const RsGxsCircleId& /*gid*/,time_t& /*grp_server_update_TS*/,time_t& /*msg_server_update_TS*/) { return true ; }
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

		virtual RsReputations::ReputationLevel overallReputationLevel(const RsGxsId&,uint32_t */*identity_flags*/=NULL) { return RsReputations::REPUTATION_NEUTRAL ; }

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
		void getOnlineList(const uint32_t /*serviceId*/, std::set<RsPeerId>& ssl_peers)
		{
			RsPeerId::std_list::iterator lit = mPeers.begin();

			for(; lit != mPeers.end(); lit++)
				ssl_peers.insert(*lit);
		}

	private:

		RsPeerId mOwnId;
		std::list<RsPeerId> mPeers;

	};

	class RsDummyPgpUtils : public PgpAuxUtils
	{
		public:

		virtual ~RsDummyPgpUtils(){}
		const RsPgpId &getPGPOwnId() ;
		RsPgpId getPGPId(const RsPeerId& sslid) ;
		bool getGPGAllList(std::list<RsPgpId> &ids) ;
		bool getKeyFingerprint(const RsPgpId& id,PGPFingerprintType& fp) const;

		bool parseSignature(unsigned char *sign, unsigned int signlen, RsPgpId& issuer) const;
		bool VerifySignBin(const void *data, uint32_t len, unsigned char *sign, unsigned int signlen, const PGPFingerprintType& withfingerprint);
		bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, int& signature_result , std::string reason = "");


		private:

			RsPgpId mOwnId;
	};

}

#endif /* NXSDUMMYSERVICES_H_ */
