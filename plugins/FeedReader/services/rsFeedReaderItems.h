/*******************************************************************************
 * plugins/FeedReader/services/rsFeedReaderItems.h                             *
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

#ifndef RS_FEEDREADER_ITEMS_H
#define RS_FEEDREADER_ITEMS_H

#include "rsitems/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvstring.h"

#include "p3FeedReader.h"

const uint32_t CONFIG_TYPE_FEEDREADER = 0xf001; // is this correct?

const uint8_t RS_PKT_SUBTYPE_FEEDREADER_FEED  = 0x02;
const uint8_t RS_PKT_SUBTYPE_FEEDREADER_MSG   = 0x03;

/**************************************************************************/

#define RS_FEED_FLAG_FOLDER                        0x0001
#define RS_FEED_FLAG_INFO_FROM_FEED                0x0002
#define RS_FEED_FLAG_STANDARD_STORAGE_TIME         0x0004
#define RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL      0x0008
#define RS_FEED_FLAG_STANDARD_PROXY                0x0010
#define RS_FEED_FLAG_AUTHENTICATION                0x0020
#define RS_FEED_FLAG_DEACTIVATED                   0x0040
#define RS_FEED_FLAG_FORUM                         0x0080
#define RS_FEED_FLAG_UPDATE_FORUM_INFO             0x0100
#define RS_FEED_FLAG_EMBED_IMAGES                  0x0200
#define RS_FEED_FLAG_SAVE_COMPLETE_PAGE            0x0400
#define RS_FEED_FLAG_POSTED                        0x0800
#define RS_FEED_FLAG_UPDATE_POSTED_INFO            0x1000
#define RS_FEED_FLAG_POSTED_FIRST_IMAGE            0x2000
#define RS_FEED_FLAG_POSTED_ONLY_IMAGE             0x4000

class RsFeedReaderFeed : public RsItem
{
public:
	enum WorkState {
		WAITING,
		WAITING_TO_DOWNLOAD,
		DOWNLOADING,
		WAITING_TO_PROCESS,
		PROCESSING
	};

public:
	RsFeedReaderFeed();
	virtual ~RsFeedReaderFeed() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob, RsGenericSerializer::SerializeContext&) {}

	uint32_t                 feedId;
	uint32_t                 parentId;
	std::string              name;
	std::string              url;
	std::string              user;
	std::string              password;
	std::string              proxyAddress;
	uint16_t                 proxyPort;
	uint32_t                 updateInterval;
	time_t                   lastUpdate;
	uint32_t                 flag; // RS_FEED_FLAG_...
	std::string              forumId;
	std::string              postedId;
	uint32_t                 storageTime;
	std::string              description;
	std::string              icon;
	RsFeedReaderErrorState   errorState;
	std::string              errorString;

	RsFeedTransformationType transformationType;
	RsTlvStringSet           xpathsToUse;
	RsTlvStringSet           xpathsToRemove;
	std::string              xslt;

	/* Not Serialised */
	bool        preview;
	WorkState   workstate;
	std::string content;

	std::map<std::string, RsFeedReaderMsg*> msgs;
};

#define RS_FEEDMSG_FLAG_DELETED                   1
#define RS_FEEDMSG_FLAG_NEW                       2
#define RS_FEEDMSG_FLAG_READ                      4

class RsFeedReaderMsg : public RsItem
{
public:
	RsFeedReaderMsg();
	virtual ~RsFeedReaderMsg() {}

	virtual void clear();
	virtual void serial_process(RsGenericSerializer::SerializeJob, RsGenericSerializer::SerializeContext&) {}
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string msgId;
	uint32_t feedId;
	std::string title;
	std::string link;
	std::string author;
	std::string description;
	std::string descriptionTransformed;
	time_t      pubDate;
	uint32_t    flag; // RS_FEEDMSG_FLAG_...

	// Only in memory when receiving messages
	std::vector<unsigned char> postedFirstImage;
	std::string postedDescriptionWithoutFirstImage;
};

class RsFeedReaderSerialiser: public RsSerialType
{
public:
	RsFeedReaderSerialiser()	: RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_PLUGIN_FEEDREADER) {}
	virtual ~RsFeedReaderSerialiser() {}
	
	virtual	uint32_t size(RsItem *item);
	virtual	bool     serialise(RsItem *item, void *data, uint32_t *size);
	virtual	RsItem  *deserialise(void *data, uint32_t *size);

private:
	/* For RS_PKT_SUBTYPE_FEEDREADER_FEED */
	virtual uint32_t         sizeFeed(RsFeedReaderFeed *item);
	virtual bool             serialiseFeed(RsFeedReaderFeed *item, void *data, uint32_t *size);
	virtual RsFeedReaderFeed *deserialiseFeed(void *data, uint32_t *size);

	/* For RS_PKT_SUBTYPE_FEEDREADER_MSG */
	virtual uint32_t         sizeMsg(RsFeedReaderMsg *item);
	virtual bool             serialiseMsg(RsFeedReaderMsg *item, void *data, uint32_t *size);
	virtual RsFeedReaderMsg  *deserialiseMsg(void *data, uint32_t *size);
};

/**************************************************************************/

#endif
