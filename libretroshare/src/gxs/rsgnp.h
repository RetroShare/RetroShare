#ifndef RSGNP_H
#define RSGNP_H

/*
 * libretroshare/src/gxs: rsgnp.h
 *
 * General Exchange Protocol interface for RetroShare.
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

#include "gxs/rsgxs.h"

typedef std::map<std::string, std::set<std::string> > PeerGrp;

/*!
 * Retroshare General Network Exchange Service: \n
 * This provides a module to service peers requests for GXS message \n
 * and also request GXS messages from other peers. \n
 * Users can make a general request for message under a grp id for a peer, all grp ids held \n
 * by a set of peers. And can also apply to timerange to them
 * An interface is provided for the observer pattern is provided to alert clients this class
 * when requests have been served
 */
class RsNetworktExchangeService
{
public:

    RsNetworkExchangeService();

    /*!
     * Queries peers for a set of grp ids
     * @param pg a map of peer to a set of grp ids to query message from
     * @param from start range of time
     * @param to end range of time
     * @return a request code to be kept to redeem query later
     */
    virtual int requestAvailableMsgs(PeerGrp& pg, time_t from, time_t to) = 0;

    /*!
     * Queries peer for msgs of peer grp id pairs
     * @param pg peer, grp id peers
     * @param from start range of time
     * @param to end range of time
     * @return a request code to be kept to redeem query later
     */
    virtual int requestMsgs(PeerGrp& pg, time_t from, time_t to) = 0;

    /*!
     * Queries peers in list for groups avaialble
     *
     * @param peers the peers to query
     * @param from start range of time
     * @param to end range of time
     * @return a request code to be kept to redeem query later
     */
    int requestAvailableGrps(const std::set<std::string>& peers, time_t from, time_t to) = 0;


    /*!
     * When messages are received this function should be call containing the messages
     * @param msgs the messages received from peers
     */
    void messageFromPeers(std::list<RsGxsSignedMessage*>& msgs) = 0;


public:



    /*!
     * attempt to retrieve requested messages
     * @param requestCode
     * @param msgs
     * @return false if not received, true is requestCode has been redeemed
     * @see RsGeneralNetExchangeService::requestAvailableGrps()
     */
    bool getRequested(uint32_t requestCode, std::list<RsGxsSignedMessage*>& msgs) = 0;

    /*!
     * Allows observer pattern for checking if a request has been satisfied
     *
     * @param requestCode the code returned msg request functions
     * @return false if not ready, true otherwise
     * @see RsGeneralNetExchangeService::requestAvailableGrps()
     * @see RsGeneralNetExchangeService::requestAvailableMsgs()
     */
    bool requestCodeReady(uint32_t reuqestCode);

};

#endif // RSGNP_H
