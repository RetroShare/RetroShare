/*
 * libretroshare/src/serialiser: rsgxstestserviceitems.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#pragma once

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rsgxsitems.h"
#include "gxstestservice.h"

const uint8_t RS_PKT_SUBTYPE_TEST_GROUP_ITEM = 0x02;
const uint8_t RS_PKT_SUBTYPE_TEST_MSG_ITEM = 0x03;

class RsGxsTestGroupItem : public RsGxsGrpItem
{

public:

	RsGxsTestGroupItem():  RsGxsGrpItem(RS_SERVICE_GXS_TYPE_TEST,
			RS_PKT_SUBTYPE_TEST_GROUP_ITEM) { return;}
        virtual ~RsGxsTestGroupItem() { return;}

        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	RsTestGroup group;
};

class RsGxsTestMsgItem : public RsGxsMsgItem
{
public:

	RsGxsTestMsgItem(): RsGxsMsgItem(RS_SERVICE_GXS_TYPE_TEST,
			RS_PKT_SUBTYPE_TEST_MSG_ITEM) {return; }
        virtual ~RsGxsTestMsgItem() { return;}
        void clear();
	std::ostream &print(std::ostream &out, uint16_t indent = 0);
	RsTestMsg msg;
};

class RsGxsTestSerialiser : public RsSerialType
{
public:

	RsGxsTestSerialiser()
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_GXS_TYPE_TEST)
	{ return; }
	virtual     ~RsGxsTestSerialiser() { return; }

	uint32_t    size(RsItem *item);
	bool        serialise  (RsItem *item, void *data, uint32_t *size);
	RsItem *    deserialise(void *data, uint32_t *size);

	private:

	uint32_t    sizeGxsTestGroupItem(RsGxsTestGroupItem *item);
	bool        serialiseGxsTestGroupItem  (RsGxsTestGroupItem *item, void *data, uint32_t *size);
	RsGxsTestGroupItem *    deserialiseGxsTestGroupItem(void *data, uint32_t *size);

	uint32_t    sizeGxsTestMsgItem(RsGxsTestMsgItem *item);
	bool        serialiseGxsTestMsgItem  (RsGxsTestMsgItem *item, void *data, uint32_t *size);
	RsGxsTestMsgItem *    deserialiseGxsTestMsgItem(void *data, uint32_t *size);

};

