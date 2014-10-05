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

    /*!
     * Use this to set how far back synchronisation of messages should take place
     * @param age the max age a sync item can to be allowed in a synchronisation
     */
    virtual void setSyncAge(uint32_t age) = 0;

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
     * Request for this group is sent through to peers on your network
     * and how many hops from them you've indicated
     */
    virtual int sharePublishKey(const RsGxsGroupId& grpId,const std::list<RsPeerId>& peers)=0 ;

};

#endif // RSGNP_H
