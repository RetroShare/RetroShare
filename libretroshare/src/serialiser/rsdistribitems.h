#ifndef RS_DISTRIB_ITEMS_H
#define RS_DISTRIB_ITEMS_H

/*
 * libretroshare/src/serialiser: rsdistribitems.h
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
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvkeys.h"

const uint8_t RS_PKT_SUBTYPE_DISTRIB_GRP      		= 0x01;
const uint8_t RS_PKT_SUBTYPE_DISTRIB_GRP_KEY   		= 0x02;
const uint8_t RS_PKT_SUBTYPE_DISTRIB_SIGNED_MSG  	= 0x03;
const uint8_t RS_PKT_SUBTYPE_DISTRIB_CONFIG_DATA	= 0x04;

/**************************************************************************/

/*!
 * This should be subclassed by p3distrib subclass's to store their data
 * save data
 */
class RsDistribChildConfig: public RsItem
{
public:
	RsDistribChildConfig(uint16_t servtype, uint8_t subtype)
	: RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype),save_type(0) {return;}

	virtual ~RsDistribChildConfig() { return; }

	virtual void clear();
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	/// use this to id the type of data you want to save
	uint32_t save_type;
};


/*!
 * This should be used to save the service data such as settings
 */
class RsDistribConfigData: public RsItem
{
public:
	RsDistribConfigData()
	: RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISTRIB, RS_PKT_SUBTYPE_DISTRIB_CONFIG_DATA), service_data(TLV_TYPE_BIN_SERIALISE)
	{return;}

	virtual ~RsDistribConfigData() { return; }

	virtual void clear();

	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	// this is where a derived distrib service saves its data
	RsTlvBinaryData service_data;

};


/*!
 * This is used by p3Distrib for storing messages
 * of derived services, attributes are given for writing
 * personal signatures (confirms user) and publish signatures (to
 * confirm consistency source)
 */
class RsDistribMsg: public RsItem
{
        public:
        RsDistribMsg(uint16_t servtype, uint8_t subtype)
	:RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }

virtual ~RsDistribMsg() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string grpId; /* Grp Id */

	/// Parent Msg Id, msgs above a thread
	std::string parentId;

	/// Thread Msg Id: useful identifyingfor responses
	///to a forum msg and replies in general
	std::string threadId;
    uint32_t    timestamp;

	/* Not Serialised */

	std::string msgId;   /* Msg Id */
	time_t childTS; /* timestamp of most recent child */

	/// used to confirm the message is from a group author, or someone with valid publish key
	RsTlvKeySignature publishSignature;

	/// used to confirm message is from a particular peer
	RsTlvKeySignature personalSignature;
};


/*!
 * This is used as a storage container for messages from a service
 * via the binary data container (packet)
 */
class RsDistribSignedMsg: public RsItem
{
	public:
	RsDistribSignedMsg()
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISTRIB, RS_PKT_SUBTYPE_DISTRIB_SIGNED_MSG), 
	packet(TLV_TYPE_BIN_SERIALISE)
	{ return; }

virtual ~RsDistribSignedMsg() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		std::string 	  grpId;

		/// should be taken publishSignature
		std::string 	  msgId;
		uint32_t    	  flags;
        uint32_t    	  timestamp;

        /// in order to tranfer messages from service level to distrib level
		RsTlvBinaryData   packet;
		RsTlvKeySignature publishSignature;
		RsTlvKeySignature personalSignature;
};


class RsDistribGrp: public RsItem
{
        public:
//        RsDistribGrp(uint16_t servtype, uint8_t subtype)
//	:RsItem(RS_PKT_VERSION_SERVICE, servtype, subtype) { return; }

        RsDistribGrp()
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISTRIB, RS_PKT_SUBTYPE_DISTRIB_GRP)
	{ return; }

virtual ~RsDistribGrp() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string grpId; /* Grp Id */
	uint32_t     timestamp; 
	uint32_t     grpFlags;
	std::wstring grpName; 
	std::wstring grpDesc; 
	std::wstring grpCategory;

	RsTlvImage   grpPixmap;
	
	uint32_t     grpControlFlags;
	RsTlvPeerIdSet grpControlList;

	RsTlvSecurityKey adminKey;
	RsTlvSecurityKeySet publishKeys;

	RsTlvKeySignature adminSignature;
};


class RsDistribGrpKey: public RsItem
{
	public:

		RsDistribGrpKey(uint16_t service_type)
	:RsItem(RS_PKT_VERSION_SERVICE, service_type, RS_PKT_SUBTYPE_DISTRIB_GRP_KEY)
		{ return; }

        RsDistribGrpKey()
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISTRIB, RS_PKT_SUBTYPE_DISTRIB_GRP_KEY)
        { return; }

virtual ~RsDistribGrpKey() { return; }

virtual void clear();
virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string grpId; /* Grp Id */
	RsTlvSecurityKey key;
};

		
class RsDistribSerialiser: public RsSerialType
{
	public:

	// optional to allow use in p3service/sockets
	RsDistribSerialiser(uint16_t service_type = RS_SERVICE_TYPE_DISTRIB)
	: RsSerialType(RS_PKT_VERSION_SERVICE, service_type), SERVICE_TYPE(service_type)
	{ return; }

virtual     ~RsDistribSerialiser()
	{ return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	/* For RS_PKT_SUBTYPE_DISTRIB_GRP */
virtual	uint32_t    sizeGrp(RsDistribGrp *);
virtual	bool        serialiseGrp  (RsDistribGrp *item, void *data, uint32_t *size);
virtual	RsDistribGrp *deserialiseGrp(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_DISTRIB_GRP_KEY */
virtual	uint32_t    sizeGrpKey(RsDistribGrpKey *);
virtual	bool        serialiseGrpKey  (RsDistribGrpKey *item, void *data, uint32_t *size);
virtual	RsDistribGrpKey *deserialiseGrpKey(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_DISTRIB_SIGNED_MSG */
virtual	uint32_t    sizeSignedMsg(RsDistribSignedMsg *);
virtual	bool        serialiseSignedMsg  (RsDistribSignedMsg *item, void *data, uint32_t *size);
virtual	RsDistribSignedMsg *deserialiseSignedMsg(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_DISTRIB_SAVE_DATA */
virtual uint32_t sizeConfigData(RsDistribConfigData *);
virtual bool serialiseConfigData(RsDistribConfigData *item, void *data, uint32_t *size);
virtual RsDistribConfigData *deserialiseConfigData(void* data, uint32_t *size);


const uint16_t SERVICE_TYPE;

};

/**************************************************************************/

#endif /* RS_FORUM_ITEMS_H */


