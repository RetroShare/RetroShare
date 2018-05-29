/*******************************************************************************
 * libretroshare/src/rsitems: rsgxsitems.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
 * Copyright 2012-2012 by Christopher Evi-Parker                               *
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
#ifndef RSGXSITEMS_H
#define RSGXSITEMS_H

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
