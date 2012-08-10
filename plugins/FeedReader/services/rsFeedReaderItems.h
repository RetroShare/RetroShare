/****************************************************************
 *  RetroShare GUI is distributed under the following license:
 *
 *  Copyright (C) 2012 by Thunder
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef RS_FEEDREADER_ITEMS_H
#define RS_FEEDREADER_ITEMS_H

#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

#include "p3FeedReader.h"

const uint8_t RS_PKT_SUBTYPE_FEEDREADER_FEED  = 0x02;
const uint8_t RS_PKT_SUBTYPE_FEEDREADER_MSG   = 0x03;

/**************************************************************************/

#define RS_FEED_ERRORSTATE_OK                            0
#define RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR       1
#define RS_FEED_ERRORSTATE_DOWNLOAD_ERROR                2
#define RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE 3
#define RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND            4
#define RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE 5

#define RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR        50

#define RS_FEED_ERRORSTATE_FORUM_CREATE                  100
#define RS_FEED_ERRORSTATE_FORUM_NOT_FOUND               101
#define RS_FEED_ERRORSTATE_FORUM_NO_ADMIN                102
#define RS_FEED_ERRORSTATE_FORUM_NO_ANONYMOUS_FORUM      103

#define RS_FEED_FLAG_FOLDER                        0x001
#define RS_FEED_FLAG_INFO_FROM_FEED                0x002
#define RS_FEED_FLAG_STANDARD_STORAGE_TIME         0x004
#define RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL      0x008
#define RS_FEED_FLAG_STANDARD_PROXY                0x010
#define RS_FEED_FLAG_AUTHENTICATION                0x020
#define RS_FEED_FLAG_DEACTIVATED                   0x040
#define RS_FEED_FLAG_FORUM                         0x080
#define RS_FEED_FLAG_UPDATE_FORUM_INFO             0x100
#define RS_FEED_FLAG_EMBED_IMAGES                  0x200
#define RS_FEED_FLAG_SAVE_COMPLETE_PAGE            0x400

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
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string feedId;
	std::string parentId;
	std::string name;
	std::string url;
	std::string user;
	std::string password;
	std::string proxyAddress;
	uint16_t    proxyPort;
	uint32_t    updateInterval;
	time_t      lastUpdate;
	uint32_t    flag; // RS_FEED_FLAG_...
	std::string forumId;
	uint32_t    storageTime;
	std::string description;
	std::string icon;
	uint32_t    errorState;
	std::string errorString;

	/* Not Serialised */
	WorkState   workstate;
	std::string content;

	std::map<std::string, RsFeedReaderMsg*> mMsgs;
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
	virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

	std::string msgId;
	std::string feedId;
	std::string title;
	std::string link;
	std::string author;
	std::string description;
	time_t      pubDate;
	uint32_t    flag; // RS_FEEDMSG_FLAG_...
};

class RsFeedReaderSerialiser: public RsSerialType
{
public:
	RsFeedReaderSerialiser()	: RsSerialType(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_FEEDREADER_CONFIG) {}
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
