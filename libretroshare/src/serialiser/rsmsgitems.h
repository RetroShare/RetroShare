#ifndef RS_MSG_ITEMS_H
#define RS_MSG_ITEMS_H

/*
 * libretroshare/src/serialiser: rsmsgitems.h
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

#include "retroshare/rstypes.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"

#if 0
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvfileitem.h"
#endif

/**************************************************************************/

// for defining tags themselves and msg tags
const uint8_t RS_PKT_SUBTYPE_MSG_TAG_TYPE 	= 0x03;
const uint8_t RS_PKT_SUBTYPE_MSG_TAGS 			= 0x04;
const uint8_t RS_PKT_SUBTYPE_MSG_SRC_TAG 		= 0x05;
const uint8_t RS_PKT_SUBTYPE_MSG_PARENT_TAG 	= 0x06;
const uint8_t RS_PKT_SUBTYPE_MSG_INVITE    	= 0x07;


/**************************************************************************/

const uint32_t RS_MSG_FLAGS_OUTGOING              = 0x00000001;
const uint32_t RS_MSG_FLAGS_PENDING               = 0x00000002;
const uint32_t RS_MSG_FLAGS_DRAFT                 = 0x00000004;
const uint32_t RS_MSG_FLAGS_NEW                   = 0x00000010;
const uint32_t RS_MSG_FLAGS_TRASH                 = 0x00000020;
const uint32_t RS_MSG_FLAGS_UNREAD_BY_USER        = 0x00000040;
const uint32_t RS_MSG_FLAGS_REPLIED               = 0x00000080;
const uint32_t RS_MSG_FLAGS_FORWARDED             = 0x00000100;
const uint32_t RS_MSG_FLAGS_STAR                  = 0x00000200;
const uint32_t RS_MSG_FLAGS_PARTIAL               = 0x00000400;
const uint32_t RS_MSG_FLAGS_USER_REQUEST          = 0x00000800;
const uint32_t RS_MSG_FLAGS_FRIEND_RECOMMENDATION = 0x00001000;
const uint32_t RS_MSG_FLAGS_SYSTEM                = RS_MSG_FLAGS_USER_REQUEST | RS_MSG_FLAGS_FRIEND_RECOMMENDATION;
const uint32_t RS_MSG_FLAGS_RETURN_RECEPT         = 0x00002000;
const uint32_t RS_MSG_FLAGS_ENCRYPTED             = 0x00004000;
const uint32_t RS_MSG_FLAGS_DISTANT               = 0x00008000;
const uint32_t RS_MSG_FLAGS_SIGNATURE_CHECKS      = 0x00010000;
const uint32_t RS_MSG_FLAGS_SIGNED                = 0x00020000;
const uint32_t RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES  = 0x00040000;
const uint32_t RS_MSG_FLAGS_DECRYPTED             = 0x00080000;
const uint32_t RS_MSG_FLAGS_ROUTED                = 0x00100000;

class RsMessageItem: public RsItem
{
	public:
		RsMessageItem(uint8_t msg_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_MSG,msg_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_MSG_ITEM) ;
		}

		virtual ~RsMessageItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size,bool config) = 0 ;	
		virtual uint32_t serial_size(bool config) = 0 ; 						
};


class RsMsgItem: public RsMessageItem
{
	public:
		RsMsgItem() :RsMessageItem(RS_PKT_SUBTYPE_DEFAULT) {}

		virtual ~RsMsgItem() {}
		virtual void clear();

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// ----------- Specific fields ------------- //

		uint32_t msgFlags;
		uint32_t msgId;

		uint32_t sendTime;
		uint32_t recvTime;

		std::string subject;
		std::string message;

		RsTlvPeerIdSet rspeerid_msgto;
		RsTlvPeerIdSet rspeerid_msgcc;
		RsTlvPeerIdSet rspeerid_msgbcc;

		RsTlvGxsIdSet rsgxsid_msgto;
		RsTlvGxsIdSet rsgxsid_msgcc;
		RsTlvGxsIdSet rsgxsid_msgbcc;

		RsTlvFileSet attachment;
};

class RsMsgTagType : public RsMessageItem
{
	public:
		RsMsgTagType() :RsMessageItem(RS_PKT_SUBTYPE_MSG_TAG_TYPE) {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgTagType() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		std::string text;
		uint32_t rgb_color;
		uint32_t tagId;
};

class RsMsgTags : public RsMessageItem
{
public:
	RsMsgTags()
		:RsMessageItem(RS_PKT_SUBTYPE_MSG_TAGS) {}

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual ~RsMsgTags() {}
	virtual void clear();

		// ----------- Specific fields ------------- //
		//
	uint32_t msgId;
	std::list<uint32_t> tagIds;
};

class RsMsgSrcId : public RsMessageItem
{
	public:
		RsMsgSrcId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_SRC_TAG) {}

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgSrcId() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//

		uint32_t msgId;
		RsPeerId srcId;
};
class RsPublicMsgInviteConfigItem : public RsMessageItem
{
	public:
		RsPublicMsgInviteConfigItem() : RsMessageItem(RS_PKT_SUBTYPE_MSG_INVITE) {}

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual ~RsPublicMsgInviteConfigItem() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		std::string hash ;
		time_t time_stamp ;
};


class RsMsgParentId : public RsMessageItem
{
	public:
		RsMsgParentId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_PARENT_TAG) {}

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgParentId() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		uint32_t msgId;
		uint32_t msgParentId;
};

class RsMsgSerialiser: public RsSerialType
{
	public:
		RsMsgSerialiser(bool bConfiguration = false)
			:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG), m_bConfiguration (bConfiguration) {}

		RsMsgSerialiser(uint16_t type)
			:RsSerialType(RS_PKT_VERSION_SERVICE, type), m_bConfiguration (false) {}

		virtual     ~RsMsgSerialiser() {}

		virtual	uint32_t    size(RsItem *item) 
		{ 
			return dynamic_cast<RsMessageItem*>(item)->serial_size(m_bConfiguration) ; 
		}
		virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size)
		{
			return dynamic_cast<RsMessageItem*>(item)->serialise(data,*size,m_bConfiguration) ; 
		}
		virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

		virtual	RsMsgItem                   *deserialiseMsgItem(void *data, uint32_t *size);
		virtual	RsMsgTagType                *deserialiseTagItem(void *data, uint32_t *size);
		virtual	RsMsgTags                   *deserialiseMsgTagItem(void *data, uint32_t *size);
		virtual	RsMsgSrcId                  *deserialiseMsgSrcIdItem(void *data, uint32_t *size);
		virtual	RsMsgParentId               *deserialiseMsgParentIdItem(void *data, uint32_t *size);
		virtual	RsPublicMsgInviteConfigItem *deserialisePublicMsgInviteConfigItem(void *data, uint32_t *size);

		bool m_bConfiguration; // is set to true for saving configuration (enables serialising msgId)
};

/**************************************************************************/

#endif /* RS_MSG_ITEMS_H */


