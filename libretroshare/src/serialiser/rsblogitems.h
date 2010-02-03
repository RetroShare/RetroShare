#ifndef RS_BLOG_ITEMS_H
#define RS_BLOG_ITEMS_H

/*
 * libretroshare/src/serialiser: rsblogitems.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Cyril, Chris Parker.
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

const uint8_t RS_PKT_SUBTYPE_BLOG_MSG      = 0x01;

/**************************************************************************/

class RsBlogMsg: public RsDistribMsg
{
        public:
        RsBlogMsg()
	:RsDistribMsg(RS_SERVICE_TYPE_QBLOG, RS_PKT_SUBTYPE_BLOG_MSG) { return; }
virtual ~RsBlogMsg() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	/*
	 * RsDistribMsg has:
	 * grpId, timestamp.
	 * Not Used: parentId, threadId 
	 */

        std::wstring subject;
        std::wstring message;
        /// message id to which the reply applies to
        std::string mIdReply;

        RsTlvFileSet attachment;

};

class RsBlogSerialiser: public RsSerialType
{
	public:
	RsBlogSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHANNEL)
	{ return; }
virtual     ~RsBlogSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* For RS_PKT_SUBTYPE_CHANNEL_MSG */
virtual	uint32_t    sizeMsg(RsBlogMsg *);
virtual	bool        serialiseMsg(RsBlogMsg *item, void *data, uint32_t *size);
virtual	RsBlogMsg *deserialiseMsg(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_BLOG_ITEMS_H */


