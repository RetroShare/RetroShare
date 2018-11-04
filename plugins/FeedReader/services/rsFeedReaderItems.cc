/*******************************************************************************
 * plugins/FeedReader/services/rsFeedReaderItems.cc                            *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "rsFeedReaderItems.h"

/*************************************************************************/

RsFeedReaderFeed::RsFeedReaderFeed()
	: RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PLUGIN_FEEDREADER, RS_PKT_SUBTYPE_FEEDREADER_FEED),
	  xpathsToUse(TLV_TYPE_STRINGSET), xpathsToRemove(TLV_TYPE_STRINGSET)
{
	clear();
}

void RsFeedReaderFeed::clear()
{
	feedId.clear();
	parentId.clear();
	name.clear();
	url.clear();
	user.clear();
	password.clear();
	proxyAddress.clear();
	proxyPort = 0;
	updateInterval = 0;
	lastUpdate = 0;
	storageTime = 0;
	flag = 0;
	forumId.clear();
	description.clear();
	icon.clear();
	errorState = RS_FEED_ERRORSTATE_OK;
	errorString.clear();
	transformationType = RS_FEED_TRANSFORMATION_TYPE_NONE;
	xpathsToUse.ids.clear();
	xpathsToRemove.ids.clear();
	xslt.clear();

	preview = false;
	workstate = WAITING;
	content.clear();
}

std::ostream &RsFeedReaderFeed::print(std::ostream &out, uint16_t /*indent*/)
{
	return out;
}

uint32_t RsFeedReaderSerialiser::sizeFeed(RsFeedReaderFeed *item)
{
	uint32_t s = 8; /* header */
	s += 2; /* version */
	s += GetTlvStringSize(item->feedId);
	s += GetTlvStringSize(item->parentId);
	s += GetTlvStringSize(item->url);
	s += GetTlvStringSize(item->name);
	s += GetTlvStringSize(item->description);
	s += GetTlvStringSize(item->icon);
	s += GetTlvStringSize(item->user);
	s += GetTlvStringSize(item->password);
	s += GetTlvStringSize(item->proxyAddress);
	s += sizeof(uint16_t); /* proxyPort */
	s += sizeof(uint32_t); /* updateInterval */
    s += sizeof(uint32_t); /* lastUpdate */
	s += sizeof(uint32_t); /* storageTime */
	s += sizeof(uint32_t); /* flag */
	s += GetTlvStringSize(item->forumId);
	s += sizeof(uint32_t); /* errorstate */
	s += GetTlvStringSize(item->errorString);
	s +=  sizeof(uint32_t); /* transformationType */
	s += item->xpathsToUse.TlvSize();
	s += item->xpathsToRemove.TlvSize();
	s += GetTlvStringSize(item->xslt);

	return s;
}

/* serialise the data to the buffer */
bool RsFeedReaderSerialiser::serialiseFeed(RsFeedReaderFeed *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeFeed(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add values */
	ok &= setRawUInt16(data, tlvsize, &offset, 1); /* version */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->feedId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->parentId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LINK, item->url);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->description);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->icon);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->user);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->password);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->proxyAddress);
	ok &= setRawUInt16(data, tlvsize, &offset, item->proxyPort);
	ok &= setRawUInt32(data, tlvsize, &offset, item->updateInterval);
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastUpdate);
	ok &= setRawUInt32(data, tlvsize, &offset, item->storageTime);
	ok &= setRawUInt32(data, tlvsize, &offset, item->flag);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->forumId);
	ok &= setRawUInt32(data, tlvsize, &offset, item->errorState);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->errorString);
	ok &= setRawUInt32(data, tlvsize, &offset, item->transformationType);
	ok &= item->xpathsToUse.SetTlv(data, tlvsize, &offset);
	ok &= item->xpathsToRemove.SetTlv(data, tlvsize, &offset);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->xslt);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFeedReaderSerialiser::serialiseFeed() Size Error! " << std::endl;
	}

	return ok;
}

RsFeedReaderFeed *RsFeedReaderSerialiser::deserialiseFeed(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_PLUGIN_FEEDREADER != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_FEEDREADER_FEED != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFeedReaderFeed *item = new RsFeedReaderFeed();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get values */
	uint16_t version = 0;
	ok &= getRawUInt16(data, rssize, &offset, &version);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GENID, item->feedId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->parentId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LINK, item->url);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->name);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->description);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->icon);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->user);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->password);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->proxyAddress);
	ok &= getRawUInt16(data, rssize, &offset, &(item->proxyPort));
	ok &= getRawUInt32(data, rssize, &offset, &(item->updateInterval));
	ok &= getRawUInt32(data, rssize, &offset, (uint32_t*) &(item->lastUpdate));
	ok &= getRawUInt32(data, rssize, &offset, &(item->storageTime));
	ok &= getRawUInt32(data, rssize, &offset, &(item->flag));
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->forumId);
	uint32_t errorState = 0;
	ok &= getRawUInt32(data, rssize, &offset, &errorState);
	item->errorState = (RsFeedReaderErrorState) errorState;
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->errorString);
	if (version >= 1) {
		uint32_t value = RS_FEED_TRANSFORMATION_TYPE_NONE;
		ok &= getRawUInt32(data, rssize, &offset, &value);
		if (ok)
		{
			item->transformationType = (RsFeedTransformationType) value;
		}
	}
	ok &= item->xpathsToUse.GetTlv(data, rssize, &offset);
	ok &= item->xpathsToRemove.GetTlv(data, rssize, &offset);
	if (version >= 1) {
		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->xslt);
	}

	if (version == 0)
	{
		if (!item->xpathsToUse.ids.empty() || !item->xpathsToRemove.ids.empty())
		{
			/* set transformation type */
			item->transformationType = RS_FEED_TRANSFORMATION_TYPE_XPATH;
		}
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

/*************************************************************************/

RsFeedReaderMsg::RsFeedReaderMsg() : RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PLUGIN_FEEDREADER, RS_PKT_SUBTYPE_FEEDREADER_MSG)
{
	clear();
}

void RsFeedReaderMsg::clear()
{
	msgId.clear();
	feedId.clear();
	title.clear();
	link.clear();
	author.clear();
	description.clear();
	descriptionTransformed.clear();
	pubDate = 0;
	flag = 0;
}

std::ostream &RsFeedReaderMsg::print(std::ostream &out, uint16_t /*indent*/)
{
	return out;
}

uint32_t RsFeedReaderSerialiser::sizeMsg(RsFeedReaderMsg *item)
{
	uint32_t s = 8; /* header */
	s += 2; /* version */
	s += GetTlvStringSize(item->msgId);
	s += GetTlvStringSize(item->feedId);
	s += GetTlvStringSize(item->title);
	s += GetTlvStringSize(item->link);
	s += GetTlvStringSize(item->author);
	s += GetTlvStringSize(item->description);
	s += GetTlvStringSize(item->descriptionTransformed);
	s += sizeof(uint32_t); /* pubDate */
	s += sizeof(uint32_t); /* flag */

	return s;
}

/* serialise the data to the buffer */
bool RsFeedReaderSerialiser::serialiseMsg(RsFeedReaderMsg *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeMsg(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add values */
	ok &= setRawUInt16(data, tlvsize, &offset, 1); /* version */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_GENID, item->msgId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->feedId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, item->title);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LINK, item->link);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, item->author);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->description);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_COMMENT, item->descriptionTransformed);
	ok &= setRawUInt32(data, tlvsize, &offset, item->pubDate);
	ok &= setRawUInt32(data, tlvsize, &offset, item->flag);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFeedReaderSerialiser::serialiseMsg() Size Error! " << std::endl;
	}

	return ok;
}

RsFeedReaderMsg *RsFeedReaderSerialiser::deserialiseMsg(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_PLUGIN_FEEDREADER != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_FEEDREADER_MSG != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsFeedReaderMsg *item = new RsFeedReaderMsg();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get values */
	uint16_t version = 0;
	ok &= getRawUInt16(data, rssize, &offset, &version);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_GENID, item->msgId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->feedId);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_NAME, item->title);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LINK, item->link);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VALUE, item->author);
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->description);
	if (version >= 1) {
		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_COMMENT, item->descriptionTransformed);
	}
	ok &= getRawUInt32(data, rssize, &offset, (uint32_t*) &(item->pubDate));
	ok &= getRawUInt32(data, rssize, &offset, &(item->flag));

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

/*************************************************************************/

uint32_t RsFeedReaderSerialiser::size(RsItem *item)
{
	RsFeedReaderFeed *fi;
	RsFeedReaderMsg *ei;

	if (NULL != (fi = dynamic_cast<RsFeedReaderFeed*>(item)))
	{
		return sizeFeed((RsFeedReaderFeed*) item);
	}
	if (NULL != (ei = dynamic_cast<RsFeedReaderMsg*>(item)))
	{
		return sizeMsg((RsFeedReaderMsg*) item);
	}

	return 0;
}

bool RsFeedReaderSerialiser::serialise(RsItem *item, void *data, uint32_t *pktsize)
{
	RsFeedReaderFeed *fi;
	RsFeedReaderMsg *ei;

	if (NULL != (fi = dynamic_cast<RsFeedReaderFeed*>(item)))
	{
		return serialiseFeed((RsFeedReaderFeed*) item, data, pktsize);
	}
	if (NULL != (ei = dynamic_cast<RsFeedReaderMsg*>(item)))
	{
		return serialiseMsg((RsFeedReaderMsg*) item, data, pktsize);
	}

	return false;
}

RsItem *RsFeedReaderSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_PLUGIN_FEEDREADER != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch (getRsItemSubType(rstype))
	{
	case RS_PKT_SUBTYPE_FEEDREADER_FEED:
		return deserialiseFeed(data, pktsize);
	case RS_PKT_SUBTYPE_FEEDREADER_MSG:
		return deserialiseMsg(data, pktsize);
	}

	return NULL;
}
