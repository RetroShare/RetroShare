/*******************************************************************************
 * libretroshare/src/gxs: rsnxs.h                                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare.project@gmail.com>         *
 * Copyright 2011-2011 by Christopher Evi-Parker                               *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/

#ifndef RSGNP_H
#define RSGNP_H

#include <set>
#include <string>
#include "util/rstime.h"
#include <stdlib.h>
#include <list>
#include <map>

#include "services/p3service.h"
#include "retroshare/rsreputations.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsturtle.h"
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

    virtual uint16_t serviceType() const =0;
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

    virtual bool msgAutoSync() const =0;
    virtual bool grpAutoSync() const =0;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                          DISTANT SEARCH FUNCTIONS                                           ///
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /*!
     * \brief turtleGroupRequest
     * 			Requests a particular group meta data. The request protects the group ID.
     * \param group_id
     * \return
     * 			returns the turtle request ID that might be associated to some results.
     */
    virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id)=0;

    /*!
     * \brief turtleSearchRequest
     * 			Uses distant search to match the substring to the group meta data.
     * \param match_string
     * \return
     * 			returns the turtle request ID that might be associated to some results.
     */
    virtual TurtleRequestId turtleSearchRequest(const std::string& match_string)=0;

    /*!
     * \brief receiveTurtleSearchResults
     * 			Called by turtle (through RsGxsNetTunnel) when new results are received
     * \param req			Turtle search request ID associated with this result
     * \param group_infos	Group summary information for the groups returned by the search
     */
    virtual void receiveTurtleSearchResults(TurtleRequestId req,const std::list<RsGxsGroupSummary>& group_infos)=0;

    /*!
     * \brief receiveTurtleSearchResults
     * 			Called by turtle (through RsGxsNetTunnel) when new data is received
     * \param req			        Turtle search request ID associated with this result
     * \param encrypted_group_data  Group data
     */
	virtual void receiveTurtleSearchResults(TurtleRequestId req,const unsigned char *encrypted_group_data,uint32_t encrypted_group_data_len)=0;

    /*!
     * \brief retrieveTurtleSearchResults
     * 			To be used to retrieve the search results that have been notified (or not)
     * \param req			request that match the results to retrieve
     * \param group_infos	results to retrieve.
     * \return
     * 			false when the request is unknown.
     */
	virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSearchResults> &group_infos)=0;
    /*!
     * \brief getDistantSearchResults
     * \param id
     * \param group_infos
     * \return
     */
    virtual bool clearDistantSearchResults(const TurtleRequestId& id)=0;
    virtual bool retrieveDistantGroupSummary(const RsGxsGroupId&,RsGxsGroupSearchResults&)=0;

    virtual bool search(const std::string& substring,std::list<RsGxsGroupSummary>& group_infos) =0;
	virtual bool search(const Sha1CheckSum& hashed_group_id,unsigned char *& encrypted_group_data,uint32_t& encrypted_group_data_len)=0;

    /*!
     * \brief getDistantSearchStatus
     * 			Request status of a possibly ongoing/finished search. If UNKNOWN is returned, it means that no
     * 			such group is under request
     * \return
     */
    virtual DistantSearchGroupStatus getDistantSearchStatus(const RsGxsGroupId&) =0;

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

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                           DATA ACCESS FUNCTIONS                                             ///
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    virtual bool getGroupServerUpdateTS(const RsGxsGroupId& gid,rstime_t& grp_server_update_TS,rstime_t& msg_server_update_TS) =0;

    /*!
     * \brief stampMsgServerUpdateTS
     * 		Updates the msgServerUpdateMap structure to time(NULL), so as to trigger sending msg lists to friends.
     * 		This is needed when e.g. posting a new message to a group.
     * \param gid the group to stamp in msgServerUpdateMap
     * \return
     */
    virtual bool stampMsgServerUpdateTS(const RsGxsGroupId& gid) =0;

    /*!
     * \brief isDistantPeer
     * \param pid		peer that is a virtual peer provided by GxsNetTunnel
     * \return
     * 					true if the peer exists (adn therefore is online)
     */
    virtual bool isDistantPeer(const RsPeerId& pid)=0;

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
	static RsReputationLevel minReputationForRequestingMessages(
	        uint32_t /* group_sign_flags */, uint32_t /* identity_flags */ )
	{
		// We always request messages, except if the author identity is locally banned.
		return RsReputationLevel::REMOTELY_NEGATIVE;
	}
	static RsReputationLevel minReputationForForwardingMessages(
	        uint32_t group_sign_flags, uint32_t identity_flags )
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
			return RsReputationLevel::NEUTRAL;
		else if(identity_flags & RS_IDENTITY_FLAGS_PGP_LINKED)
		{
			if(group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN)
				return RsReputationLevel::REMOTELY_POSITIVE;
			else
				return RsReputationLevel::NEUTRAL;
		}
		else
		{
			if( (group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN) || (group_sign_flags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG))
				return RsReputationLevel::REMOTELY_POSITIVE;
			else
				return RsReputationLevel::NEUTRAL;
		}
	}
};

#endif // RSGNP_H
