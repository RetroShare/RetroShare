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
 * Base class for messages
 *
 *
 */
class RsGxsMsg : public RsItem
{

    std::string grpId;
    std::string msgId;
    std::list<uint32_t> versions;
    uint32_t flags;
    uint32_t type;


};


class RsGxsSignedMsg : public RsItem
{

    RsGxsSignedMsg(uint16_t servtype, uint8_t subtype)
        : RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }

    virtual ~RsGxsSignedMsg() { return; }

    virtual void clear();
    virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

    uint32_t timestamp;
    RsTlvBinaryData data;
    RsTlvBinaryData signature;
};

/*!
 *
 *
 *
 *
 */
class RsGxsGroup : public RsItem
{


    RsGxsGroup(uint16_t servtype, uint8_t subtype)
        : RsItem() { return; }

};

class RsGxsKey : public RsItem {





};


class RsGxsSerialiser : public RsSerialType
{

public:




};


#endif // RSGXSITEMS_H
