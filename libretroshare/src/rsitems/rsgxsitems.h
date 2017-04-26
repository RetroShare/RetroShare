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

#include "rsitems/rsitem.h"
#include "rsitems/rsserviceids.h"

#include "retroshare/rsgxsifacetypes.h"

std::ostream &operator<<(std::ostream &out, const RsGroupMetaData &meta);
std::ostream &operator<<(std::ostream &out, const RsMsgMetaData &meta);

class RsGxsGrpItem : public RsItem
{

public:

    RsGxsGrpItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsGrpItem(){}


    RsGroupMetaData meta;
};

class RsGxsMsgItem : public RsItem
{

public:
    RsGxsMsgItem(uint16_t service, uint8_t subtype)
    : RsItem(RS_PKT_VERSION_SERVICE, service, subtype) { return; }
    virtual ~RsGxsMsgItem(){}

    RsMsgMetaData meta;
};





#endif // RSGXSITEMS_H
