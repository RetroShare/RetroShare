/*******************************************************************************
 * unittests/libretroshare/services/gxs/nxstestitems.h                         *
 *                                                                             *
 * Copyright 2012      by Robert Fernie    <retroshare.project@gmail.com>      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#pragma once

#include <map>

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "rsitems/rsgxsitems.h"
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

