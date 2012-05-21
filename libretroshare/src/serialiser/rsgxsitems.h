#ifndef RSGXSITEMS_H
#define RSGXSITEMS_H

/*
 * libretroshare/src/serialiser: rsgxsitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012   Christopher Evi-Parker, Robert Fernie.
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


#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"



/*!
 * Base class used by client API
 * for messages.
 * Main purpose is for msg flows
 * between GDS and GXS API clients
 */
class RsGxsMsg : public RsItem
{
public:

    RsGxsMsg() : RsItem(0) {}
    virtual ~RsGxsMsg(){ return; }


    /***** Don not serialise or set these members *****/

    std::string grpId; /// group id
    std::string msgId; /// message id
    uint32_t timeStamp; /// UTC time when created
    std::string identity; /// identity associated with (no id means no signed)

    /*!
     * The purpose of the flag is to note
     * If this message is an edit (note that edits only applies to
     * signed msgs)
     */
    uint32_t msgFlag;

    /***** Don not serialise or set these members *****/

};


/*!
 * Base class used by client API
 * for groups.
 * Main purpose is for msg flows
 * between GDS and GXS API clients
 */
class RsGxsGroup : public RsItem
{


public:



    /*** type of msgs ***/

    RsGxsGroup(uint16_t servtype, uint8_t subtype)
        : RsItem(servtype) { return; }

    virtual ~RsGxsGroup() { return; }


    std::string grpId; /// group id
    std::string identity; /// identity associated with group
    uint32_t timeStamp; /// UTC time
    bool subscribed;
    bool read;

    /*!
     * Three thing flag represents:
     * Is it signed by identity?
     * Group type (private, public, restricted)
     * msgs allowed (signed and anon)
     */
    uint32_t grpFlag;

    /*****                          *****/


};


class RsGxsSearch : public RsItem {

    RsGxsSearch(uint32_t servtype) : RsItem(servtype) { return ; }
    virtual ~RsGxsSearch() { return;}

};

class RsGxsSrchResMsgCtx : public RsItem {

    RsGxsSrchResMsgCtx(uint32_t servtype) : RsItem(servtype) { return; }
    virtual ~RsGxsSrchResMsgCtx() {return; }
    std::string msgId;
    RsTlvKeySignature sign;

};

class RsGxsSrchResGrpCtx : public RsItem {

    RsGxsSrchResGrpCtx(uint32_t servtype) : RsItem(servtype) { return; }
    virtual ~RsGxsSrchResGrpCtx() {return; }
    std::string msgId;
    RsTlvKeySignature sign;

};

class RsGxsSerialiser : public RsSerialType
{
public:

    RsGxsSerialiser(uint32_t servtype) : RsSerialType(servtype) { return; }
    virtual ~RsGxsSerialiser() { return; }
};


#endif // RSGXSITEMS_H
