#ifndef RS_CHANNEL_ITEMS_H
#define RS_CHANNEL_ITEMS_H

/*
 * libretroshare/src/serialiser: rschannelitems.h
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

const uint8_t RS_PKT_SUBTYPE_CHANNEL_MSG         = 0x01;
const uint8_t RS_PKT_SUBTYPE_CHANNEL_READ_STATUS = 0x02;

/**************************************************************************/

class RsChannelMsg: public RsDistribMsg
{
        public:
        RsChannelMsg()
	:RsDistribMsg(RS_SERVICE_TYPE_CHANNEL, RS_PKT_SUBTYPE_CHANNEL_MSG) { return; }
virtual ~RsChannelMsg() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	/*
	 * RsDistribMsg has:
	 * grpId, timestamp.
	 * Not Used: parentId, threadId 
	 */

        std::wstring subject;
        std::wstring message;

        RsTlvFileSet attachment;
        RsTlvImage thumbnail;

};

/*!
 * This is used to keep track of whether a message has been read
 * by client
 */
class RsChannelReadStatus : public RsDistribChildConfig
{
public:
	RsChannelReadStatus()
	: RsDistribChildConfig(RS_SERVICE_TYPE_CHANNEL, RS_PKT_SUBTYPE_CHANNEL_READ_STATUS)
	{ return; }

	virtual ~RsChannelReadStatus() {return; }

	virtual void clear();
	virtual std::ostream& print(std::ostream &out, uint16_t indent);

	std::string channelId;

	/// a map which contains the read for messages within a forum
	std::map<std::string, uint32_t> msgReadStatus;

};

class RsChannelSerialiser: public RsSerialType
{
	public:
	RsChannelSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHANNEL)
	{ return; }
virtual     ~RsChannelSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* For RS_PKT_SUBTYPE_CHANNEL_MSG */
virtual	uint32_t    sizeMsg(RsChannelMsg *);
virtual	bool        serialiseMsg(RsChannelMsg *item, void *data, uint32_t *size);
virtual	RsChannelMsg *deserialiseMsg(void *data, uint32_t *size);

virtual uint32_t sizeReadStatus(RsChannelReadStatus* );
virtual bool serialiseReadStatus(RsChannelReadStatus* item, void* data, uint32_t *size);
virtual RsChannelReadStatus *deserialiseReadStatus(void* data, uint32_t *size);

};

/**************************************************************************/

#endif /* RS_CHANNEL_ITEMS_H */


