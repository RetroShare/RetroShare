#ifndef RSGNP_H
#define RSGNP_H

/*
 * libretroshare/src/gxs: rsgnp.h
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

#include "gxs/rsgxs.h"
#include "services/p3service.h"


/*!
 * This is universal format for messages that is transported throughout
 * the general exchange service chain. From concrete service to
 */
class RsGxsSignedMessage : RsItem {

    uint32_t timestamp;
    void* signature;
    void* data;
    uint32_t msg_flags; /* encrypted */
    std::string msgId; /* hash of all message data */
    std::string grpId;

};

/*!
 * Item for dealing
 * with grp list
 */
class RsGxsMessageList : public RsGxsSignedMessage {

};

/*!
 * Item for dealing with group
 * description and msg list
 */
class RsGxsGroup : public RsGxsSignedMessage {


};


typedef std::map<std::string, std::set<std::string> > PeerGrp;

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
 *   - As this is where exchanges occur between peers, this is also where groups relationships
 *     should get resolved as far as
 *   - Per implemented GXS there are a set of rules which will determine whether data is transferred
 *     between any set of groups
 *
 *  1 allow transfers to any group
 *  2 transfers only between group
 *   - the also group matrix settings which is by default everyone can transfer to each other
 */
class RsNetworktExchangeService : public p3Service
{
public:

    RsNetworkExchangeService(uint16_t subtype);


    /*!
     * Use this to set how far back synchronisation of messages should take place
     * @param range how far back from current time to synchronise with other peers
     */
    virtual void setTimeRange(uint64_t range) = 0;

};

#endif // RSGNP_H
