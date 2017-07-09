#ifndef RSGNP_H
#define RSGNP_H

/*
 * libretroshare/src/gxs: rsnxs.h
 *
 * Network Exchange Service interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie, Christopher Evi-Prker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <set>
#include <string>
#include <time.h>
#include <stdlib.h>
#include <list>
#include <map>

#include "services/p3service.h"
#include "retroshare/rsreputations.h"
#include "retroshare/rsidentity.h"
#include "rsgds.h"

/*!
 * Retroshare General Network Exchange Service: \n
 * Interface:
 *   - This provides a module to service peer's requests for GXS messages \n
 *      and also request GXS messages from other peers. \n
 *   - The general mode of operation is to sychronise all messages/grps between
 *     peers
 *
 * The interface is sparse as this service is mostly making the requests to other GXS components
 *
 * Groups:
 *   - As this is where exchanges occur between peers, this is also where group's relationships
 *     should get resolved as far as
 *   - Per implemented GXS there are a set of rules which will determine whether data is transferred
 *     between any set of groups
 *
 *  1 allow transfers to any group
 *  2 transfers only between group
 *   - the also group matrix settings which is by default everyone can transfer to each other
 */
class RsNetworkExchangeService
{
public:

	RsNetworkExchangeService(){ return;}
    virtual ~RsNetworkExchangeService() {}

    /*!
     * Use this to set how far back synchronisation of messages should take place
     * @param age in seconds the max age a sync/store item can to be allowed in a synchronisation
     */
    virtual void setSyncAge(const RsGxsGroupId& id,uint32_t age_in_secs) =0;
    virtual void setKeepAge(const RsGxsGroupId& id,uint32_t age_in_secs) =0;

    virtual uint32_t getSyncAge(const RsGxsGroupId& id) =0;
    virtual uint32_t getKeepAge(const RsGxsGroupId& id) =0;

	virtual void setDefaultKeepAge(uint32_t t) =0;
	virtual void setDefaultSyncAge(uint32_t t) =0;

    virtual uint32_t getDefaultSyncAge() =0;
    virtual uint32_t getDefaultKeepAge() =0;

    /*!
     * Initiates a search through the network
     * This returns messages which contains the search terms set in RsGxsSearch
     * @param search contains search terms of requested from service
     * @param hops how far into friend tree for search
     * @return search token that can be redeemed later, implementation should indicate how this should be used
     */
    //virtual int searchMsgs(RsGxsSearch* search, uint8_t hops = 1, bool retrieve = 0) = 0;

    /*!
     * Initiates a search of groups through the network which goes
     * a given number of hops deep into your friend network
     * @param search contains search term requested from service
     * @param hops number of hops deep into peer network
     * @return search token that can be redeemed later
     */
    //virtual int searchGrps(RsGxsSearch* search, uint8_t hops = 1, bool retrieve = 0) = 0;


    /*!
     * pauses synchronisation of subscribed groups and request for group id
     * from peers
     * @param enabled set to false to disable pause, and true otherwise
     */
    virtual void pauseSynchronisation(bool enabled) = 0;


    /*!
     * Request for this message is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param msgId the messages to retrieve
     * @return request token to be redeemed
     */
    virtual int requestMsg(const RsGxsGrpMsgIdPair& msgId) = 0;

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     * @param enabled set to false to disable pause, and true otherwise
     * @return request token to be redeemed
     */
    virtual int requestGrp(const std::list<RsGxsGroupId>& grpId, const RsPeerId& peerId) = 0;

    /*!
     * returns some stats about this group related to the network visibility.
     * For now, only one statistics:
     *     max_known_messages:   max number of messages reported by a friend. This is used to display unsubscribed group content.
     */
    virtual bool getGroupNetworkStats(const RsGxsGroupId& grpId,RsGroupNetworkStats& stats)=0;

    virtual void subscribeStatusChanged(const RsGxsGroupId& id,bool subscribed) =0;

    /*!
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     */
    virtual int sharePublishKey(const RsGxsGroupId& grpId,const std::set<RsPeerId>& peers)=0 ;

    /*!
     * \brief rejectMessage
     * 		Tells the network exchange service to not download this message again, at least for some time (maybe 24h or more)
     * 		in order to avoid cluttering the network pipe with copied of this rejected message.
     * \param msgId
     */
    virtual void rejectMessage(const RsGxsMessageId& msgId) =0;
    
    /*!
     * \brief getGroupServerUpdateTS
     * 		Returns the server update time stamp for that group. This is used for synchronisation of TS between
     * 		various network exchange services, suhc as channels/circles or forums/circles
     * \param gid	group for that request
     * \param tm	time stamp computed
     * \return 		false if the group is not found, true otherwise
     */
    virtual bool getGroupServerUpdateTS(const RsGxsGroupId& gid,time_t& grp_server_update_TS,time_t& msg_server_update_TS) =0;

    /*!
     * \brief stampMsgServerUpdateTS
     * 		Updates the msgServerUpdateMap structure to time(NULL), so as to trigger sending msg lists to friends.
     * 		This is needed when e.g. posting a new message to a group.
     * \param gid the group to stamp in msgServerUpdateMap
     * \return
     */
    virtual bool stampMsgServerUpdateTS(const RsGxsGroupId& gid) =0;

    /*!
     * \brief removeGroups
     * 			Removes time stamp information from the list of groups. This allows to re-sync them if suppliers are present.
     * \param groups		list of groups to remove from the update maps
     * \return 				true if nothing bad happens.
     */
    virtual bool removeGroups(const std::list<RsGxsGroupId>& groups)=0;

    /*!
     * \brief minReputationForForwardingMessages
     * 				Encodes the policy for sending/requesting messages depending on anti-spam settings.
     *
     * \param group_sign_flags	Sign flags from the group meta data
     * \param identity_flags	Flags of the identity
     * \return
     */
    static RsReputations::ReputationLevel minReputationForRequestingMessages(uint32_t /* group_sign_flags */, uint32_t /* identity_flags */)
	{
		// We always request messages, except if the author identity is locally banned.

		return RsReputations::REPUTATION_REMOTELY_NEGATIVE;
	}
    static RsReputations::ReputationLevel minReputationForForwardingMessages(uint32_t group_sign_flags, uint32_t identity_flags)
	{
		// If anti-spam is enabled, do not send messages from authors with bad reputation. The policy is to only forward messages if the reputation of the author is at least
		// equal to the minimal reputation in the table below (R=remotely, L=locally, P=positive, N=negative, O=neutral) :
		//
		//
		//                            +----------------------------------------------------+
		//                            |                Identity flags                      |
		//                            +----------------------------------------------------+
		//                            | Anonymous          Signed          Signed+Known    |
		//  +-------------+-----------+----------------------------------------------------+
		//  |             |NONE       |     O                 O                  O         |
		//  | Forum flags |GPG_AUTHED |    RP                 O                  O         |
		//  |             |GPG_KNOWN  |    RP                RP                  O         |
		//  +-------------+-----------+----------------------------------------------------+
		//

		if(identity_flags & RS_IDENTITY_FLAGS_PGP_KNOWN)
			return RsReputations::REPUTATION_NEUTRAL;
		else if(identity_flags & RS_IDENTITY_FLAGS_PGP_LINKED)
		{
			if(group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN)
				return RsReputations::REPUTATION_REMOTELY_POSITIVE;
			else
				return RsReputations::REPUTATION_NEUTRAL;
		}
		else
		{
			if( (group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN) || (group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG))
				return RsReputations::REPUTATION_REMOTELY_POSITIVE;
			else
				return RsReputations::REPUTATION_NEUTRAL;
		}
	}
};

#endif // RSGNP_H
