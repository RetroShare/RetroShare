
/*
 * libretroshare/src/serialiser: rsbaseitems.cc
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

#include <stdexcept>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rsmsgitems.h"
#include "serialiser/rstlvbase.h"

/***
#define RSSERIAL_DEBUG 1
#define CHAT_DEBUG 1
***/

#include <iostream>

/*************************************************************************/

std::ostream& RsChatMsgItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatMsgItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "QblogMs " << chatFlags << std::endl;

	printIndent(out, int_Indent);
	out << "sendTime:  " << sendTime  << " (" << time(NULL)-sendTime << " secs ago)" << std::endl;

	printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
	out << "msg:  " << cnv_message  << std::endl;

	printRsItemEnd(out, "RsChatMsgItem", indent);
	return out;
}
std::ostream& RsChatLobbyListItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyListItem", indent);

	for(uint32_t i=0;i<lobby_ids.size();++i)
	{
		printIndent(out, indent+2);
		out << "lobby 0x" << std::hex << lobby_ids[i] << std::dec << " (name=\"" << lobby_names[i] << "\", count=" << lobby_counts[i] << std::endl ;
	}

	printRsItemEnd(out, "RsChatLobbyListItem", indent);
	return out;
}
std::ostream& RsChatLobbyListRequestItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyListRequestItem", indent);
	printRsItemEnd(out, "RsChatLobbyListRequestItem", indent);
	return out;
}
std::ostream& RsChatLobbyBouncingObject::print(std::ostream &out, uint16_t indent)
{
	printIndent(out, indent); out << "Lobby ID: " << std::hex << lobby_id << std::endl;
	printIndent(out, indent); out << "Msg ID: " << std::hex << msg_id << std::dec << std::endl;
	printIndent(out, indent); out << "Nick: " << nick << std::dec << std::endl;

	return out;
}
std::ostream& RsChatLobbyUnsubscribeItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyUnsubscribeItem", indent);
	printIndent(out, indent);
	out << "Lobby id: " << std::hex << lobby_id << std::endl;
	printRsItemEnd(out, "RsChatLobbyUnsubscribeItem", indent);
	return out;
}
std::ostream& RsChatLobbyEventItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyEventItem", indent);
	RsChatLobbyBouncingObject::print(out,indent) ;
	printIndent(out, indent); out << "Event type  : " << event_type  << std::endl;
	printIndent(out, indent); out << "String param: " << string1  << std::endl;
	printIndent(out, indent); out << "Send time: " << sendTime  << " (" << time(NULL)-sendTime << " secs ago)" << std::endl;
	printRsItemEnd(out, "RsChatLobbyEventItem", indent);
	return out;
} 
std::ostream& RsChatLobbyConnectChallengeItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyConnectChallengeItem", indent);
	printIndent(out, indent);
	out << "Challenge Code: " << std::hex << challenge_code << std::endl;
	printRsItemEnd(out, "RsChatLobbyConnectChallengeItem", indent);
	return out;
}
std::ostream& RsChatLobbyMsgItem::print(std::ostream &out, uint16_t indent)
{
	RsChatMsgItem::print(out,indent) ;
	RsChatLobbyBouncingObject::print(out,indent) ;

	printRsItemEnd(out, "RsChatLobbyMsgItem", indent);
	return out;
}

std::ostream& RsChatLobbyInviteItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatLobbyInviteItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "peerId:  " << PeerId()  << std::endl;

	printIndent(out, int_Indent);
	out << "lobby id: " << std::hex << lobby_id << std::dec << std::endl;

	printIndent(out, int_Indent);
	out << "lobby name: " << lobby_name << std::endl;

	printRsItemEnd(out, "RsChatLobbyInviteItem", indent);
	return out;
}
std::ostream& RsPrivateChatMsgConfigItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsPrivateChatMsgConfigItem", indent);
	uint16_t int_Indent = indent + 2;

	out << "peerId:  " << configPeerId  << std::endl;

	printIndent(out, int_Indent);
	out << "QblogMs " << chatFlags << std::endl;

	printIndent(out, int_Indent);
	out << "QblogMs " << configFlags << std::endl;

	printIndent(out, int_Indent);
	out << "sendTime:  " << sendTime  << " (" << time(NULL)-sendTime << " secs ago)" << std::endl;

	printIndent(out, int_Indent);

	std::string cnv_message(message.begin(), message.end());
	out << "msg:  " << cnv_message  << std::endl;

	printRsItemEnd(out, "RsPrivateChatMsgConfigItem", indent);
	return out;
}

std::ostream& RsChatStatusItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatStatusItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "Status string: " << status_string << std::endl;
	out << "Flags : " << (void*)flags << std::endl;

	printRsItemEnd(out, "RsChatStatusItem", indent);
	return out;
}

std::ostream& RsChatAvatarItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsChatAvatarItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "Image size: " << image_size << std::endl;
	printRsItemEnd(out, "RsChatStatusItem", indent);

	return out;
}
RsItem *RsChatSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

#ifdef CHAT_DEBUG
	std::cerr << "deserializing packet..."<< std::endl ;
#endif
	// look what we have...
	if (*pktsize < rssize)    /* check size */
	{
#ifdef CHAT_DEBUG
		std::cerr << "chat deserialisation: not enough size: pktsize=" << *pktsize << ", rssize=" << rssize << std::endl ;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	/* ready to load */

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_CHAT != getRsItemService(rstype))) 
	{
#ifdef CHAT_DEBUG
		std::cerr << "chat deserialisation: wrong type !" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DEFAULT:						return new RsChatMsgItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG:	return new RsPrivateChatMsgConfigItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_STATUS:					return new RsChatStatusItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_AVATAR:					return new RsChatAvatarItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_MSG:				return new RsChatLobbyMsgItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE:			return new RsChatLobbyInviteItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE:		return new RsChatLobbyConnectChallengeItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE:	return new RsChatLobbyUnsubscribeItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT:			return new RsChatLobbyEventItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST:	return new RsChatLobbyListRequestItem(data,*pktsize) ;
		case RS_PKT_SUBTYPE_CHAT_LOBBY_LIST:        	return new RsChatLobbyListItem(data,*pktsize) ;
		default:
			std::cerr << "Unknown packet type in chat!" << std::endl ;
			return NULL ;
	}
}

uint32_t RsChatMsgItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4; /* chatFlags */
	s += 4; /* sendTime  */
	s += GetTlvWideStringSize(message);

	return s;
}

uint32_t RsChatLobbyUnsubscribeItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 8;			// challenge code
	return s ;
}


uint32_t RsChatLobbyConnectChallengeItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 8;			// challenge code
	return s ;
}

uint32_t RsChatLobbyBouncingObject::serial_size()
{
	uint32_t s = 0 ;	// no header!
	s += 8 ; // lobby_id
	s += 8 ; // msg_id
	s += GetTlvStringSize(nick) ; // nick

	return s ;
}

uint32_t RsChatLobbyEventItem::serial_size()
{
	uint32_t s = 8 ;	// header
	s += RsChatLobbyBouncingObject::serial_size() ;
	s += 1 ; // event_type
	s += GetTlvStringSize(string1) ;	// string1
	s += 4 ; // send time

	return s ;
}

uint32_t RsChatLobbyListRequestItem::serial_size()
{
	uint32_t s = 8 ;	// header
	return s ;
}
uint32_t RsChatLobbyListItem::serial_size()
{
	uint32_t s = 8 ;	// header
	s += 4 ; 			// number of elements in the vectors
	s += lobby_ids.size() * 8 ;	// lobby_ids

	for(uint32_t i=0;i<lobby_names.size();++i)
		s += GetTlvStringSize(lobby_names[i]) ;	// lobby_names

	s += lobby_counts.size() * 4 ;	// lobby_counts
	return s ;
}

uint32_t RsChatLobbyMsgItem::serial_size()
{
	uint32_t s = RsChatMsgItem::serial_size() ;		// parent 
	s += RsChatLobbyBouncingObject::serial_size() ;
	s += 1;											// subpacket id
	s += 8;											// parent_msg_id

	return s;
}
uint32_t RsChatLobbyInviteItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 8;											// lobby_id
	s += GetTlvStringSize(lobby_name) ;		// lobby_name
	s += 4;											// lobby_privacy_level

	return s;
}
uint32_t RsPrivateChatMsgConfigItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4; /* version */
	s += GetTlvStringSize(configPeerId);
	s += 4; /* chatFlags */
	s += 4; /* configFlags */
	s += 4; /* sendTime  */
	s += GetTlvWideStringSize(message);
	s += 4; /* recvTime  */

	return s;
}

uint32_t RsChatStatusItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4 ; // flags
	s += GetTlvStringSize(status_string); 			 /* status */

	return s;
}

uint32_t RsChatAvatarItem::serial_size()
{
	uint32_t s = 8; /* header */
	s += 4 ;						// size
	s += image_size ;			// data

	return s;
}

RsChatAvatarItem::~RsChatAvatarItem()
{
	if(image_data != NULL)
	{
		delete[] image_data ;
		image_data = NULL ;
	}
}

/* serialise the data to the buffer */
bool RsChatMsgItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = RsChatMsgItem::serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, chatFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, sendTime);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, message);
#ifdef CHAT_DEBUG
	std::cerr << "Serialized the following message:" << std::endl;
	std::cerr << "========== BEGIN MESSAGE =========" << std::endl;
	for(uint32_t i=0;i<message.length();++i)
		std::cerr << (char)message[i] ;
	std::cerr << std::endl;
	std::cerr << "=========== END MESSAGE ==========" << std::endl;
#endif

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	return ok ;
}

bool RsChatLobbyBouncingObject::serialise(void *data,uint32_t tlvsize,uint32_t& offset)
{
	bool ok = true ;

	ok &= setRawUInt64(data, tlvsize, &offset, lobby_id);
	ok &= setRawUInt64(data, tlvsize, &offset, msg_id);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, nick);

	return ok ;
}

/* serialise the data to the buffer */
bool RsChatLobbyMsgItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	bool ok = true;
	ok &= RsChatMsgItem::serialise(data,pktsize) ;						// first, serialize parent

	uint32_t offset = pktsize;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!
	pktsize = tlvsize;

	ok &= RsChatLobbyBouncingObject::serialise(data,tlvsize,offset) ;
	ok &= setRawUInt8(data, tlvsize, &offset, subpacket_id);
	ok &= setRawUInt64(data, tlvsize, &offset, parent_msg_id);

	/* add mandatory parts first */
	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	return ok ;
}

bool RsChatLobbyListRequestItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize ;
	return ok ;
}

bool RsChatLobbyListItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!

	if (pktsize < tlvsize)
		return false; /* not enough space */

	if(lobby_ids.size() != lobby_counts.size() || lobby_ids.size() != lobby_names.size())
	{
		std::cerr << "Consistency error in RsChatLobbyListItem!! Sizes don't match!" << std::endl;
		return false ;
	}
	pktsize = tlvsize ;

	uint32_t offset = 8 ;
	ok &= setRawUInt32(data, tlvsize, &offset, lobby_ids.size());

	for(uint32_t i=0;i<lobby_ids.size();++i)
	{
		ok &= setRawUInt64(data, tlvsize, &offset, lobby_ids[i]);
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, lobby_names[i]);
		ok &= setRawUInt32(data, tlvsize, &offset, lobby_counts[i]);
	}
	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
	return ok ;
}
bool RsChatLobbyEventItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!

	if (pktsize < tlvsize)
		return false; /* not enough space */

	uint32_t offset = 8 ;

	ok &= RsChatLobbyBouncingObject::serialise(data,tlvsize,offset) ;		// first, serialize parent
	ok &= setRawUInt8(data, tlvsize, &offset, event_type);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, string1);
	ok &= setRawUInt32(data, tlvsize, &offset, sendTime);

	pktsize = tlvsize ;

	/* add mandatory parts first */
	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	return ok ;
}

bool RsChatLobbyUnsubscribeItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!
	uint32_t offset = 8 ;

	ok &= setRawUInt64(data, tlvsize, &offset, lobby_id);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	pktsize = tlvsize ;
	return ok ;
}
bool RsChatLobbyConnectChallengeItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!
	uint32_t offset = 8 ;

	ok &= setRawUInt64(data, tlvsize, &offset, challenge_code);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	pktsize = tlvsize ;
	return ok ;
}

bool RsChatLobbyInviteItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	bool ok = true ;
	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);		// correct header!
	uint32_t offset = 8 ;

	ok &= setRawUInt64(data, tlvsize, &offset, lobby_id);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, lobby_name);
	ok &= setRawUInt32(data, tlvsize, &offset, lobby_privacy_level);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif
	pktsize = tlvsize ;
	return ok ;
}

bool RsPrivateChatMsgConfigItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, 0);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, configPeerId);
	ok &= setRawUInt32(data, tlvsize, &offset, chatFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, configFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, sendTime);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, message);
	ok &= setRawUInt32(data, tlvsize, &offset, recvTime);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

bool RsChatStatusItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser serialising chat status item." << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= SetTlvString(data, tlvsize, &offset,TLV_TYPE_STR_MSG, status_string);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}

bool RsChatAvatarItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef CHAT_DEBUG
	std::cerr << "RsChatSerialiser serialising chat avatar item." << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsChatSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset,image_size);

	memcpy((void*)( (unsigned char *)data + offset),image_data,image_size) ;
	offset += image_size ;

	if (offset != tlvsize)
	{
		ok = false;
#ifdef CHAT_DEBUG
		std::cerr << "RsChatSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}
#ifdef CHAT_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}
RsChatMsgItem::RsChatMsgItem(void *data,uint32_t /*size*/,uint8_t subtype)
	: RsChatItem(subtype)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
		std::cerr << "Received packet result: " ;
	for(int i=0;i<20;++i)
		std::cerr << (int)((uint8_t*)data)[i] << " " ;
	std::cerr << std::endl ;
#endif

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &chatFlags);
	ok &= getRawUInt32(data, rssize, &offset, &sendTime);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, message);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat msg item." << std::endl ;
#endif
	if (getRsItemSubType(getRsItemId(data)) == RS_PKT_SUBTYPE_DEFAULT && offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsChatLobbyMsgItem::RsChatLobbyMsgItem(void *data,uint32_t /*size*/)
	: RsChatMsgItem(data,0,RS_PKT_SUBTYPE_CHAT_LOBBY_MSG)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	uint32_t offset = RsChatMsgItem::serial_size() ;

	ok &= RsChatLobbyBouncingObject::deserialise(data,rssize,offset) ;
	ok &= getRawUInt8(data, rssize, &offset, &subpacket_id);
	ok &= getRawUInt64(data, rssize, &offset, &parent_msg_id);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat lobby msg item." << std::endl ;
#endif
	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}
RsChatLobbyListRequestItem::RsChatLobbyListRequestItem(void *data,uint32_t)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;
	uint32_t offset = 8; // skip the header 

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsChatLobbyListItem::RsChatLobbyListItem(void *data,uint32_t)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;
	uint32_t offset = 8; // skip the header 

	uint32_t n=0 ;
	ok &= getRawUInt32(data, rssize, &offset, &n);

	lobby_ids.resize(n) ;
	lobby_names.resize(n) ;
	lobby_counts.resize(n) ;

	for(uint32_t i=0;i<n;++i)
	{
		ok &= getRawUInt64(data, rssize, &offset, &lobby_ids[i]);
		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, lobby_names[i]);
		ok &= getRawUInt32(data, rssize, &offset, &lobby_counts[i]);
	}

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}
	
bool RsChatLobbyBouncingObject::deserialise(void *data,uint32_t rssize,uint32_t& offset)
{
	bool ok = true ;
	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &lobby_id);
	ok &= getRawUInt64(data, rssize, &offset, &msg_id);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, nick);

	return ok ;
}

RsChatLobbyEventItem::RsChatLobbyEventItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	uint32_t offset = 8 ;

	ok &= RsChatLobbyBouncingObject::deserialise(data,rssize,offset) ;

	ok &= getRawUInt8(data, rssize, &offset, &event_type);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, string1);
	ok &= getRawUInt32(data, rssize, &offset, &sendTime);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat lobby status item." << std::endl ;
#endif
	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}
RsChatLobbyUnsubscribeItem::RsChatLobbyUnsubscribeItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "RsChatLobbyUnsubscribeItem: rsitem size is " << rssize << std::endl;
#endif
	uint32_t offset = 8 ;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &lobby_id);

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}
RsChatLobbyConnectChallengeItem::RsChatLobbyConnectChallengeItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "RsChatLobbyConnectChallengeItem: rsitem size is " << rssize << std::endl;
#endif
	uint32_t offset = 8 ;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &challenge_code);

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}
RsChatLobbyInviteItem::RsChatLobbyInviteItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE)
{
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "RsChatLobbyInviteItem: rsitem size is " << rssize << std::endl;
#endif
	uint32_t offset = 8 ;

	/* get mandatory parts first */
	ok &= getRawUInt64(data, rssize, &offset, &lobby_id);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, lobby_name);
	ok &= getRawUInt32(data, rssize, &offset, &lobby_privacy_level);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat msg item." << std::endl ;
#endif
	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsPrivateChatMsgConfigItem::RsPrivateChatMsgConfigItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	/* get mandatory parts first */
	uint32_t version = 0;
	ok &= getRawUInt32(data, rssize, &offset, &version);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, configPeerId);
	ok &= getRawUInt32(data, rssize, &offset, &chatFlags);
	ok &= getRawUInt32(data, rssize, &offset, &configFlags);
	ok &= getRawUInt32(data, rssize, &offset, &sendTime);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, message);
	ok &= getRawUInt32(data, rssize, &offset, &recvTime);

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat msg config item." << std::endl ;
#endif
	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

/* set data from RsChatMsgItem to RsPrivateChatMsgConfigItem */
void RsPrivateChatMsgConfigItem::set(RsChatMsgItem *ci, const std::string &/*peerId*/, uint32_t confFlags)
{
	PeerId(ci->PeerId());
	configPeerId = ci->PeerId();
	chatFlags = ci->chatFlags;
	configFlags = confFlags;
	sendTime = ci->sendTime;
	message = ci->message;
	recvTime = ci->recvTime;
}

/* get data from RsPrivateChatMsgConfigItem to RsChatMsgItem */
void RsPrivateChatMsgConfigItem::get(RsChatMsgItem *ci)
{
	ci->PeerId(configPeerId);
	ci->chatFlags = chatFlags;
	//configFlags not used
	ci->sendTime = sendTime;
	ci->message = message;
	ci->recvTime = recvTime;
}

RsChatStatusItem::RsChatStatusItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_STATUS)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat status item." << std::endl ;
#endif
	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &flags);
	ok &= GetTlvString(data, rssize, &offset,TLV_TYPE_STR_MSG, status_string);

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}

RsChatAvatarItem::RsChatAvatarItem(void *data,uint32_t /*size*/)
	: RsChatItem(RS_PKT_SUBTYPE_CHAT_AVATAR)
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef CHAT_DEBUG
	std::cerr << "Building new chat status item." << std::endl ;
#endif
	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset,&image_size);

	image_data = new unsigned char[image_size] ;
	memcpy(image_data,(void*)((unsigned char*)data+offset),image_size) ;
	offset += image_size ;

	if (offset != rssize)
		std::cerr << "Size error while deserializing." << std::endl ;
	if (!ok)
		std::cerr << "Unknown error while deserializing." << std::endl ;
}


/*************************************************************************/


RsMsgItem::~RsMsgItem()
{
	return;
}

void 	RsMsgItem::clear()
{
	msgId    = 0;
	msgFlags = 0;
	sendTime = 0;
	recvTime = 0;
	subject.clear();
	message.clear();

	msgto.TlvClear();
	msgcc.TlvClear();
	msgbcc.TlvClear();

	attachment.TlvClear();
}

std::ostream &RsMsgItem::print(std::ostream &out, uint16_t indent)
{
        printRsItemBase(out, "RsMsgItem", indent);
	uint16_t int_Indent = indent + 2;
        printIndent(out, int_Indent);
        out << "msgId (not serialised): " << msgId << std::endl;
        printIndent(out, int_Indent);
        out << "msgFlags: " << msgFlags << std::endl;

        printIndent(out, int_Indent);
        out << "sendTime:  " << sendTime  << std::endl;
        printIndent(out, int_Indent);
        out << "recvTime:  " << recvTime  << std::endl;

        printIndent(out, int_Indent);
        out << "Message To: " << std::endl;
	msgto.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message CC: " << std::endl;
	msgcc.print(out, int_Indent);

        printIndent(out, int_Indent);
        out << "Message BCC: " << std::endl;
	msgbcc.print(out, int_Indent);

        printIndent(out, int_Indent);
	std::string cnv_subject(subject.begin(), subject.end());
        out << "subject:  " << cnv_subject  << std::endl;

        printIndent(out, int_Indent);
	std::string cnv_message(message.begin(), message.end());
        out << "msg:  " << cnv_message  << std::endl;

        printIndent(out, int_Indent);
        out << "Attachment: " << std::endl;
	attachment.print(out, int_Indent);

        printRsItemEnd(out, "RsMsgItem", indent);
        return out;
}

void RsMsgTagType::clear()
{
	text.clear();
	tagId = 0;
	rgb_color = 0;
}


void RsMsgTags::clear()
{
	msgId = 0;
	tagIds.clear();
}

std::ostream& RsMsgTagType::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgTagType", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "rgb_color : " << rgb_color  << std::endl;


	printIndent(out, int_Indent);
	out << "text: " << text << std::endl;

	printIndent(out, int_Indent);
	out << "tagId: " << tagId << std::endl;

	printRsItemEnd(out, "RsMsgTagTypeItem", indent);

	return out;
}

std::ostream& RsMsgTags::print(std::ostream &out, uint16_t indent)
{

	printRsItemBase(out, "RsMsgTagsItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	std::list<uint32_t>::iterator it;

	for(it=tagIds.begin(); it != tagIds.end(); it++)
	{
		printIndent(out, int_Indent);
		out << "tagId : " << *it << std::endl;
	}

	printRsItemEnd(out, "RsMsgTags", indent);

	return out;
}

RsMsgTagType::~RsMsgTagType()
{
	return;
}

RsMsgTags::~RsMsgTags()
{
	return;
}

uint32_t    RsMsgSerialiser::sizeMsgItem(RsMsgItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* msgFlags */
	s += 4; /* sendTime  */
	s += 4; /* recvTime  */

	s += GetTlvWideStringSize(item->subject);
	s += GetTlvWideStringSize(item->message);

	s += item->msgto.TlvSize();
	s += item->msgcc.TlvSize();
	s += item->msgbcc.TlvSize();
	s += item->attachment.TlvSize();

	if (m_bConfiguration) {
		// serialise msgId too
		s += 4;
	}
	
	return s;
}

/* serialise the data to the buffer */
bool     RsMsgSerialiser::serialiseMsgItem(RsMsgItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeMsgItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->msgFlags);
	ok &= setRawUInt32(data, tlvsize, &offset, item->sendTime);
	ok &= setRawUInt32(data, tlvsize, &offset, item->recvTime);

	ok &= SetTlvWideString(data,tlvsize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	ok &= SetTlvWideString(data, tlvsize, &offset, TLV_TYPE_WSTR_MSG, item->message);

	ok &= item->msgto.SetTlv(data, tlvsize, &offset);
	ok &= item->msgcc.SetTlv(data, tlvsize, &offset);
	ok &= item->msgbcc.SetTlv(data, tlvsize, &offset);

	ok &= item->attachment.SetTlv(data, tlvsize, &offset);

	if (m_bConfiguration) {
		// serialise msgId too
		ok &= setRawUInt32(data, tlvsize, &offset, item->msgId);
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsMsgItem *RsMsgSerialiser::deserialiseMsgItem(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DEFAULT != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgItem *item = new RsMsgItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgFlags));
	ok &= getRawUInt32(data, rssize, &offset, &(item->sendTime));
	ok &= getRawUInt32(data, rssize, &offset, &(item->recvTime));

	ok &= GetTlvWideString(data,rssize,&offset,TLV_TYPE_WSTR_SUBJECT,item->subject);
	ok &= GetTlvWideString(data, rssize, &offset, TLV_TYPE_WSTR_MSG, item->message);
	ok &= item->msgto.GetTlv(data, rssize, &offset);
	ok &= item->msgcc.GetTlv(data, rssize, &offset);
	ok &= item->msgbcc.GetTlv(data, rssize, &offset);
	ok &= item->attachment.GetTlv(data, rssize, &offset);

	if (m_bConfiguration) {
		// deserialise msgId too
		// ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
		getRawUInt32(data, rssize, &offset, &(item->msgId)); //use this line for backward compatibility 
	}

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}


uint32_t RsMsgSerialiser::sizeTagItem(RsMsgTagType* item)
{
	uint32_t s = 8; /* header */

	s += GetTlvStringSize(item->text);
	s +=  4; /* color */
	s += 4; /* tag id */

	return s;
}


bool RsMsgSerialiser::serialiseTagItem(RsMsgTagType *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeTagItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data,tlvsize,&offset, TLV_TYPE_STR_NAME, item->text);
	ok &= setRawUInt32(data, tlvsize, &offset, item->rgb_color);
	ok &= setRawUInt32(data, tlvsize, &offset, item->tagId);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsMsgTagType* RsMsgSerialiser::deserialiseTagItem(void *data,uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_TAG_TYPE != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgTagType *item = new RsMsgTagType();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= GetTlvString(data,rssize,&offset,TLV_TYPE_STR_NAME,item->text);
	ok &= getRawUInt32(data, rssize, &offset, &(item->rgb_color));
	ok &= getRawUInt32(data, rssize, &offset, &(item->tagId));

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

uint32_t RsMsgSerialiser::sizeMsgTagItem(RsMsgTags* item)
{
	uint32_t s = 8; /* header */

	s += 4; /* msgId */
	s += item->tagIds.size() * 4; /* tagIds */

	return s;
}

bool RsMsgSerialiser::serialiseMsgTagItem(RsMsgTags *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeMsgTagItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data,tlvsize,&offset, item->msgId);

	std::list<uint32_t>::iterator mit = item->tagIds.begin();
	for(;mit != item->tagIds.end(); mit++)
	{
		ok &= setRawUInt32(data, tlvsize, &offset, *mit);
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgTagItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsMsgTags* RsMsgSerialiser::deserialiseMsgTagItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_TAGS != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgTags *item = new RsMsgTags();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &item->msgId);

	uint32_t tagId;
	while (offset != rssize)
	{
		tagId = 0;

		ok &= getRawUInt32(data, rssize, &offset, &tagId);

		item->tagIds.push_back(tagId);
	}

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}


/************************************** Message SrcId **********************/



RsMsgSrcId::~RsMsgSrcId()
{
	return;
}

std::ostream& RsMsgSrcId::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgSrcIdItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	printIndent(out, int_Indent);
	out << "SrcId: " << srcId << std::endl;


	printRsItemEnd(out, "RsMsgItem", indent);

	return out;
}

void RsMsgSrcId::clear()
{
	msgId = 0;
	srcId.clear();

	return;
}

uint32_t RsMsgSerialiser::sizeMsgSrcIdItem(RsMsgSrcId* item)
{
	uint32_t s = 8; /* header */

	s += 4;
	s += GetTlvStringSize(item->srcId);

	return s;
}


bool RsMsgSerialiser::serialiseMsgSrcIdItem(RsMsgSrcId *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeMsgSrcIdItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data, tlvsize, &offset, item->msgId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PEERID, (item->srcId));

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgSrcIdItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsMsgSrcId* RsMsgSerialiser::deserialiseMsgSrcIdItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_SRC_TAG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgSrcId *item = new RsMsgSrcId();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PEERID, item->srcId);

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/************************* end of definition of msgSrcId serialisation functions ************************/

/************************************** Message ParentId **********************/

RsMsgParentId::~RsMsgParentId()
{
	return;
}

std::ostream& RsMsgParentId::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsMsgParentIdItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "msgId : " << msgId << std::endl;

	printIndent(out, int_Indent);
	out << "msgParentId: " << msgParentId << std::endl;


	printRsItemEnd(out, "RsMsgParentId", indent);

	return out;
}

void RsMsgParentId::clear()
{
	msgId = 0;
	msgParentId = 0;

	return;
}

uint32_t RsMsgSerialiser::sizeMsgParentIdItem(RsMsgParentId* /*item*/)
{
	uint32_t s = 8; /* header */

	s += 4; // srcId
	s += 4; // msgParentId

	return s;
}

bool RsMsgSerialiser::serialiseMsgParentIdItem(RsMsgParentId *item, void *data, uint32_t* pktsize)
{
	uint32_t tlvsize = sizeMsgParentIdItem(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Header: " << ok << std::endl;
	std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	ok &= setRawUInt32(data, tlvsize, &offset, item->msgId);
	ok &= setRawUInt32(data, tlvsize, &offset, item->msgParentId);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsMsgSerialiser::serialiseMsgParentIdItem() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsMsgParentId* RsMsgSerialiser::deserialiseMsgParentIdItem(void* data, uint32_t* pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_MSG_PARENT_TAG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsMsgParentId *item = new RsMsgParentId();
	item->clear();

	/* skip the header */
	offset += 8;


	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgId));
	ok &= getRawUInt32(data, rssize, &offset, &(item->msgParentId));

	if (offset != rssize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/************************* end of definition of msgParentId serialisation functions ************************/

uint32_t    RsMsgSerialiser::size(RsItem *i)
{
	RsMsgItem *mi;
	RsMsgTagType *mtt;
	RsMsgTags *mts;
	RsMsgSrcId *msi;
	RsMsgParentId *msp;

	/* in order of frequency */
	if (NULL != (mi = dynamic_cast<RsMsgItem *>(i)))
	{
		return sizeMsgItem(mi);
	}
	else if (NULL != (msi = dynamic_cast<RsMsgSrcId *>(i)))
	{
		return sizeMsgSrcIdItem(msi);
	}
	else if (NULL != (msp = dynamic_cast<RsMsgParentId *>(i)))
	{
		return sizeMsgParentIdItem(msp);
	}
	else if (NULL != (mtt = dynamic_cast<RsMsgTagType *>(i)))
	{
		return sizeTagItem(mtt);
	}
	else if (NULL != (mts = dynamic_cast<RsMsgTags *>(i)))
	{
		return sizeMsgTagItem(mts);
	}

	return 0;
}

bool     RsMsgSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::serialise()" << std::endl;
#endif

	RsMsgItem *mi;
	RsMsgSrcId* msi;
	RsMsgParentId* msp;
	RsMsgTagType *mtt;
	RsMsgTags *mts;


	if (NULL != (mi = dynamic_cast<RsMsgItem *>(i)))
	{
		return serialiseMsgItem(mi, data, pktsize);
	}
	else if (NULL != (msi = dynamic_cast<RsMsgSrcId *>(i)))
	{
		return serialiseMsgSrcIdItem(msi, data, pktsize);
	}
	else if (NULL != (msp = dynamic_cast<RsMsgParentId *>(i)))
	{
		return serialiseMsgParentIdItem(msp, data, pktsize);
	}
	else if (NULL != (mtt = dynamic_cast<RsMsgTagType *>(i)))
	{
		return serialiseTagItem(mtt, data, pktsize);
	}
	else if (NULL != (mts = dynamic_cast<RsMsgTags *>(i)))
	{
		return serialiseMsgTagItem(mts, data, pktsize);
	}

	return false;
}

RsItem* RsMsgSerialiser::deserialise(void *data, uint32_t *pktsize)
{
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsMsgSerialiser::deserialise()" << std::endl;
#endif

	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_MSG != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DEFAULT:
			return deserialiseMsgItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_SRC_TAG:
			return deserialiseMsgSrcIdItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_PARENT_TAG:
			return deserialiseMsgParentIdItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_TAG_TYPE:
			return deserialiseTagItem(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_MSG_TAGS:
			return deserialiseMsgTagItem(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}

	return NULL;
}


/*************************************************************************/

