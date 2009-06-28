/*
 * libretroshare/src/serialiser: rsdiscitems.h
 *
 * Serialiser for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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



#ifndef RS_DISC_ITEMS_H
#define RS_DISC_ITEMS_H

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rsserviceids.h"

const uint8_t RS_PKT_SUBTYPE_DISC_OWN    = 0x01;
const uint8_t RS_PKT_SUBTYPE_DISC_REPLY  = 0x02;
const uint8_t RS_PKT_SUBTYPE_DISC_ISSUER = 0x03;
const uint8_t RS_PKT_SUBTYPE_DISC_VERSION = 0x04;

class RsDiscItem: public RsItem
{
	protected:
		RsDiscItem(uint8_t subtype) :RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISC, subtype) {}
};


class RsDiscOwnItem: public RsDiscItem
{
	public:

	RsDiscOwnItem() :RsDiscItem(RS_PKT_SUBTYPE_DISC_OWN ) {}


	virtual ~RsDiscOwnItem();

	virtual  void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	struct sockaddr_in laddr;
	struct sockaddr_in saddr;

	// time frame of recent connections.
	uint16_t contact_tf;
	// flags...
	uint32_t discFlags;
};

class RsDiscReply: public RsDiscItem
{
	public:

	RsDiscReply()
	:RsDiscItem(RS_PKT_SUBTYPE_DISC_REPLY),
	certDER(TLV_TYPE_CERT_XPGP_DER)
	{ return; }

virtual ~RsDiscReply();

virtual  void clear();
virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	struct sockaddr_in laddr;
	struct sockaddr_in saddr;

	// time frame of recent connections.
	uint16_t contact_tf;
	// flags...
	uint32_t discFlags;

	std::string aboutId;
	RsTlvBinaryData certDER;
};

class RsDiscIssuer: public RsDiscItem
{
	public:

	RsDiscIssuer()
	:RsDiscItem(RS_PKT_SUBTYPE_DISC_ISSUER)
	{ return; }

virtual ~RsDiscIssuer();

virtual  void clear();
virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	std::string issuerCert;
};

class RsDiscVersion: public RsDiscItem
{
public:
    RsDiscVersion() :RsDiscItem(RS_PKT_SUBTYPE_DISC_VERSION)
    { return; }

    virtual ~RsDiscVersion();

    virtual void clear();
    virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

    std::string version;
};

class RsDiscSerialiser: public RsSerialType
{
        public:
        RsDiscSerialiser()
        :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISC)
        { return; }

virtual     ~RsDiscSerialiser() { return; }

virtual uint32_t    size(RsItem *);
virtual bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual RsItem *    deserialise(void *data, uint32_t *size);

	private:

virtual uint32_t    sizeItem(RsDiscOwnItem *);
virtual bool        serialiseItem  (RsDiscOwnItem *item, void *data, uint32_t *size);
virtual RsDiscOwnItem *deserialiseOwnItem(void *data, uint32_t *size);

virtual uint32_t    sizeReply(RsDiscReply *);
virtual bool        serialiseReply   (RsDiscReply *item, void *data, uint32_t *size);
virtual RsDiscReply *deserialiseReply(void *data, uint32_t *size);

virtual uint32_t    sizeIssuer(RsDiscIssuer *);
virtual bool        serialiseIssuer   (RsDiscIssuer *item, void *data, uint32_t *size);
virtual RsDiscIssuer *deserialiseIssuer(void *data, uint32_t *size);

virtual uint32_t        sizeVersion(RsDiscVersion *);
virtual bool            serialiseVersion(RsDiscVersion *item, void *data, uint32_t *size);
virtual RsDiscVersion   *deserialiseVersion(void *data, uint32_t *size);

};


#endif // RS_DISC_ITEMS_H

