#ifndef RSGXSITEMS_H
#define RSGXSITEMS_H

/*
 * libretroshare/src/serialiser: rsgxsitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2012 Christopher Evi-Parker, Robert Fernie.
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
 * for their messages.
 * Main purpose is to transfer
 * between GDS and client side GXS
 */
class RsGxsMsg : public RsItem
{
public:

    RsGxsMsg() : RsItem() {}


    std::string grpId; /// grp id
    std::string msgId; /// message id
    uint32_t timestamp; /// when created
    std::string identity;
    uint32_t flag;
};

/*!
 *
 *
 *
 */
class RsGxsSignedMsg : public RsItem
{

    RsGxsSignedMsg(uint16_t servtype, uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }

    virtual ~RsGxsSignedMsg() { return; }

    virtual void clear();
    virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

    std::string grpId; /// group id
    std::string msgId; /// message id
    uint32_t timestamp;
    std::string identity;
    uint32_t flag;
    RsTlvBinaryData* msg;
    RsTlvBinaryData signature; // needs to be a set of signatures with bitfield flag ids
    // set should cover idenitity and publication key
};

/*!
 *
 *
 *
 *
 */
class RsGxsSignedGroup
{


    std::string grpId;
    std::string identity;
    RsTlvKeySignature sign;
    uint32_t timestamp; // UTC time
    RsTlvBinaryData* grp;
    uint32_t flag;
};

/*!
 * This is the base group used by API client to exchange
 * data
 *
 */
class RsGxsGroup : public RsItem
{


public:

    // publication type
    static const uint32_t FLAG_GRP_TYPE_KEY_SHARED;
    static const uint32_t FLAG_GRP_TYPE_PRIVATE_ENC;
    static const uint32_t FLAG_GRP_TYPE_PUBLIC;
    static const uint32_t FLAG_GRP_TYPE_MASK;

    // type of msgs
    static const uint32_t FLAG_MSGS_TYPE_SIGNED;
    static const uint32_t FLAG_MSGS_TYPE_ANON;
    static const uint32_t FLAG_MSGS_TYPE_MASK;

    RsGxsGroup(uint16_t servtype, uint8_t subtype)
        : RsItem(servtype) { return; }

    virtual ~RsGxsGroup() { return; }

    std::string grpId;
    std::string identity; /// identity associated with group
    uint32_t timeStamp; // UTC time
    uint32_t flag;


};

class RsGxsKey : public RsItem {





};


class RsGxsSerialiser : public RsSerialType
{

public:




};


#endif // RSGXSITEMS_H
