#ifndef RSGXS_H
#define RSGXS_H

/*
 * libretroshare/src/gxs   : rsgxs.h
 *
 * Copyright 2012 Christopher Evi-Parker
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
 *
 */

#include "gxs/rsgxsdata.h"

#include <inttypes.h>
#include <string>
#include <list>
#include <set>
#include <map>

/* data types used throughout Gxs from netservice to genexchange */

typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgReq;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgIdResult;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgMetaData*> > GxsMsgMetaResult;

class RsGxsService
{

public:

    RsGxsService(){}
    virtual ~RsGxsService(){}

    virtual void tick() = 0;

};

#endif // RSGXS_H
