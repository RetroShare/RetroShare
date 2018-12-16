/*******************************************************************************
 * unittests/libretroshare/services/gxs/nxstestitems.cc                        *
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

#include <iostream>

#include "rsgxstestitems.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#define TEST_DEBUG	1


uint32_t RsGxsTestSerialiser::size(RsItem *item)
{
	RsGxsTestGroupItem* grp_item = NULL;
	RsGxsTestMsgItem* msg_item = NULL;

	if((grp_item = dynamic_cast<RsGxsTestGroupItem*>(item)) != NULL)
	{
		return sizeGxsTestGroupItem(grp_item);
	}
	else if((msg_item = dynamic_cast<RsGxsTestMsgItem*>(item)) != NULL)
	{
		return sizeGxsTestMsgItem(msg_item);
	}
	return 0;
}

bool RsGxsTestSerialiser::serialise(RsItem *item, void *data, uint32_t *size)
{
	RsGxsTestGroupItem* grp_item = NULL;
	RsGxsTestMsgItem* msg_item = NULL;

	if((grp_item = dynamic_cast<RsGxsTestGroupItem*>(item)) != NULL)
	{
		return serialiseGxsTestGroupItem(grp_item, data, size);
	}
	else if((msg_item = dynamic_cast<RsGxsTestMsgItem*>(item)) != NULL)
	{
		return serialiseGxsTestMsgItem(msg_item, data, size);
	}
	return false;
}

RsItem* RsGxsTestSerialiser::deserialise(void* data, uint32_t* size)
{
		
#ifdef TEST_DEBUG
	std::cerr << "RsGxsTestSerialiser::deserialise()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
		
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_TEST != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}
		
	switch(getRsItemSubType(rstype))
	{
		
		case RS_PKT_SUBTYPE_TEST_GROUP_ITEM:
			return deserialiseGxsTestGroupItem(data, size);
			break;
		case RS_PKT_SUBTYPE_TEST_MSG_ITEM:
			return deserialiseGxsTestMsgItem(data, size);
			break;
		default:
#ifdef TEST_DEBUG
			std::cerr << "RsGxsTestSerialiser::deserialise(): unknown subtype";
			std::cerr << std::endl;
#endif
			break;
	}
	return NULL;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsTestGroupItem::clear()
{
	group.mTestString.clear();
}

std::ostream& RsGxsTestGroupItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTestGroupItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "TestString: " << group.mTestString << std::endl;
  
	printRsItemEnd(out ,"RsGxsTestGroupItem", indent);
	return out;
}


uint32_t RsGxsTestSerialiser::sizeGxsTestGroupItem(RsGxsTestGroupItem *item)
{

	const RsTestGroup& group = item->group;
	uint32_t s = 8; // header

	s += GetTlvStringSize(group.mTestString);
	return s;
}

bool RsGxsTestSerialiser::serialiseGxsTestGroupItem(RsGxsTestGroupItem *item, void *data, uint32_t *size)
{
	
#ifdef TEST_DEBUG
	std::cerr << "RsGxsTestSerialiser::serialiseGxsTestGroupItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsTestGroupItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestGroupItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsTestGroupItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DESCR, item->group.mTestString);
	
	if(offset != tlvsize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestGroupItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef TEST_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestGroupItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsTestGroupItem* RsGxsTestSerialiser::deserialiseGxsTestGroupItem(void *data, uint32_t *size)
{
	
#ifdef TEST_DEBUG
	std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestGroupItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_TEST != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_TEST_GROUP_ITEM != getRsItemSubType(rstype)))
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestGroupItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestGroupItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsTestGroupItem* item = new RsGxsTestGroupItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DESCR, item->group.mTestString);
	
	if (offset != rssize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestGroupItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestGroupItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}



/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/


void RsGxsTestMsgItem::clear()
{
	msg.mTestString.clear();
}

std::ostream& RsGxsTestMsgItem::print(std::ostream& out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTestMsgItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "TestString: " << msg.mTestString << std::endl;
  
	printRsItemEnd(out ,"RsGxsTestMsgItem", indent);
	return out;
}


uint32_t RsGxsTestSerialiser::sizeGxsTestMsgItem(RsGxsTestMsgItem *item)
{

	const RsTestMsg& msg = item->msg;
	uint32_t s = 8; // header

	s += GetTlvStringSize(msg.mTestString);

	return s;
}

bool RsGxsTestSerialiser::serialiseGxsTestMsgItem(RsGxsTestMsgItem *item, void *data, uint32_t *size)
{
	
#ifdef TEST_DEBUG
	std::cerr << "RsGxsTestSerialiser::serialiseGxsTestMsgItem()" << std::endl;
#endif
	
	uint32_t tlvsize = sizeGxsTestMsgItem(item);
	uint32_t offset = 0;
	
	if(*size < tlvsize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestMsgItem()" << std::endl;
#endif
		return false;
	}
	
	*size = tlvsize;
	
	bool ok = true;
	
	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);
	
	/* skip the header */
	offset += 8;
	
	/* GxsTestMsgItem */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_MSG, item->msg.mTestString);
	
	if(offset != tlvsize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestMsgItem() FAIL Size Error! " << std::endl;
#endif
		ok = false;
	}
	
#ifdef TEST_DEBUG
	if (!ok)
	{
		std::cerr << "RsGxsTestSerialiser::serialiseGxsTestMsgItem() NOK" << std::endl;
	}
#endif
	
	return ok;
	}
	
RsGxsTestMsgItem* RsGxsTestSerialiser::deserialiseGxsTestMsgItem(void *data, uint32_t *size)
{
	
#ifdef TEST_DEBUG
	std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestMsgItem()" << std::endl;
#endif
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);
	
	uint32_t offset = 0;
	
	
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_GXS_TYPE_TEST != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_TEST_MSG_ITEM != getRsItemSubType(rstype)))
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestMsgItem() FAIL wrong type" << std::endl;
#endif
		return NULL; /* wrong type */
	}
	
	if (*size < rssize)    /* check size */
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestMsgItem() FAIL wrong size" << std::endl;
#endif
		return NULL; /* not enough data */
	}
	
	/* set the packet length */
	*size = rssize;
	
	bool ok = true;
	
	RsGxsTestMsgItem* item = new RsGxsTestMsgItem();
	/* skip the header */
	offset += 8;
	
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_MSG, item->msg.mTestString);
	
	if (offset != rssize)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestMsgItem() FAIL size mismatch" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}
	
	if (!ok)
	{
#ifdef TEST_DEBUG
		std::cerr << "RsGxsTestSerialiser::deserialiseGxsTestMsgItem() NOK" << std::endl;
#endif
		delete item;
		return NULL;
	}
	
	return item;
}


/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

