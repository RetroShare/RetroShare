#ifndef RS_RANK_ITEMS_H
#define RS_RANK_ITEMS_H

/*
 * libretroshare/src/serialiser: rsrankitems.h
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

const uint8_t RS_PKT_SUBTYPE_RANK_OLD_LINK  = 0x02; /* defunct - don't use! */
const uint8_t RS_PKT_SUBTYPE_RANK_OLD_LINK2 = 0x03;
const uint8_t RS_PKT_SUBTYPE_RANK_LINK3     = 0x04;

const uint8_t RS_PKT_SUBTYPE_RANK_PHOTO     = 0x05;

/**************************************************************************/


class RsRankMsg: public RsItem
{
        public:
        RsRankMsg(uint8_t subtype)
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_RANK, 
		subtype) { return; }
virtual ~RsRankMsg() { return; }
virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string rid; /* Random Id */
	std::string pid; /* Peer Id (cannot use RsItem::PeerId - as FoF transport!) */
        uint32_t    timestamp;
        std::wstring title;
        std::wstring comment;
        int32_t     score;
};


/* Flags */
const uint32_t RS_LINK_TYPE_WEB = 0x0001;
const uint32_t RS_LINK_TYPE_OFF = 0x0002;

class RsRankLinkMsg: public RsRankMsg
{
        public:
        RsRankLinkMsg()
	:RsRankMsg(RS_PKT_SUBTYPE_RANK_LINK3) { return; }
virtual ~RsRankLinkMsg() { return; }
virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	/**** SAME as RsRankMsg ****
	std::string rid; 
        uint32_t    timestamp;
        std::wstring title;
        std::wstring comment;
	int32_t	    score;
	***************************/

	/* Link specific Fields */
	uint32_t    linktype; /* to be used later! */
        std::wstring link;
};

class RsRankSerialiser: public RsSerialType
{
	public:
	RsRankSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_RANK)
	{ return; }
virtual     ~RsRankSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* For RS_PKT_SUBTYPE_RANK_LINK */
virtual	uint32_t    sizeLink(RsRankLinkMsg *);
virtual	bool        serialiseLink  (RsRankLinkMsg *item, void *data, uint32_t *size);
virtual	RsRankLinkMsg *deserialiseLink(void *data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_RANK_ITEMS_H */


