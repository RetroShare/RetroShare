#ifndef RSGXS_H
#define RSGXS_H

/*
 * libretroshare/src/gxs   : rsgxs.h
 *
 * GXS  interface for RetroShare.
 * Convenience header
 *
 * Copyright 2011 Christopher Evi-Parker
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
 * This is *THE* auth manager. It provides the web-of-trust via
 * gpgme, and authenticates the certificates that are managed
 * by the sublayer AuthSSL.
 *
 */

#include "serialiser/rsnxsitems.h"

#include <inttypes.h>
#include <string>
#include <list>
#include <set>
#include <map>


typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgReq;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > GxsMsgIdResult;
typedef std::map<RsGxsGroupId, std::vector<RsGxsMsgMetaData*> > GxsMsgMetaResult;
typedef std::map<RsGxsGroupId, std::vector<RsNxsMsg*> > NxsMsgDataResult;
typedef std::map<RsGxsGroupId, std::vector<RsNxsMsg*> > GxsMsgResult; // <grpId, msgs>

class RsGxsService
{

public:

    RsGxsService(){}
    virtual ~RsGxsService(){}

    virtual void tick() = 0;

};

#endif // RSGXS_H
