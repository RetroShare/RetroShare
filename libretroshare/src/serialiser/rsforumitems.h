#ifndef RS_FORUM_ITEMS_H
#define RS_FORUM_ITEMS_H

/*
 * libretroshare/src/serialiser: rsforumitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
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
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"

#include "serialiser/rsdistribitems.h"

const uint8_t RS_PKT_SUBTYPE_FORUM_GRP      = 0x01;
const uint8_t RS_PKT_SUBTYPE_FORUM_MSG      = 0x02;

/**************************************************************************/

class RsForumMsg: public RsDistribMsg
{
        public:
        RsForumMsg()
	:RsDistribMsg(RS_SERVICE_TYPE_FORUM, RS_PKT_SUBTYPE_FORUM_MSG) { return; }
virtual ~RsForumMsg() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	/*
	 * RsDistribMsg has:
	 * grpId, parentId, threadId & timestamp.
	 */

        std::string  srcId;

        std::wstring title;
        std::wstring msg;

};

class RsForumSerialiser: public RsSerialType
{
	public:
	RsForumSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_FORUM)
	{ return; }
virtual     ~RsForumSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* For RS_PKT_SUBTYPE_FORUM_GRP */
//virtual	uint32_t    sizeGrp(RsForumGrp *);
//virtual	bool        serialiseGrp  (RsForumGrp *item, void *data, uint32_t *size);
//virtual	RsForumGrp *deserialiseGrp(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_FORUM_MSG */
virtual	uint32_t    sizeMsg(RsForumMsg *);
virtual	bool        serialiseMsg(RsForumMsg *item, void *data, uint32_t *size);
virtual	RsForumMsg *deserialiseMsg(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_FORUM_ITEMS_H */


