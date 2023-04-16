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

#ifndef RETROSHARE_FEEDREADER_GUI_INTERFACE_H
#define RETROSHARE_FEEDREADER_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>
#include <vector>

class RsFeedReader;
class RsGxsForums;
class RsPosted;
class RsGxsForumGroup;
class RsPostedGroup;
extern RsFeedReader *rsFeedReader;

enum RsFeedReaderErrorState {
	RS_FEED_ERRORSTATE_OK                             = 0,

	/* download */
	RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR        = 1,
	RS_FEED_ERRORSTATE_DOWNLOAD_ERROR                 = 2,
	RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE  = 3,
	RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND             = 4,
	RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE  = 5,

	/* process */
	RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR         = 50,
	RS_FEED_ERRORSTATE_PROCESS_UNKNOWN_FORMAT         = 51,
//	RS_FEED_ERRORSTATE_PROCESS_FORUM_CREATE           = 100,
	RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_FOUND        = 101,
	RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_ADMIN         = 102,
	RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_AUTHOR        = 103,
//	RS_FEED_ERRORSTATE_PROCESS_POSTED_CREATE          = 104,
	RS_FEED_ERRORSTATE_PROCESS_POSTED_NOT_FOUND       = 105,
	RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_ADMIN        = 106,
	RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_AUTHOR       = 107,

	RS_FEED_ERRORSTATE_PROCESS_HTML_ERROR             = 150,
	RS_FEED_ERRORSTATE_PROCESS_XPATH_INTERNAL_ERROR   = 151,
	RS_FEED_ERRORSTATE_PROCESS_XPATH_WRONG_EXPRESSION = 152,
	RS_FEED_ERRORSTATE_PROCESS_XPATH_NO_RESULT        = 153,
	RS_FEED_ERRORSTATE_PROCESS_XSLT_FORMAT_ERROR      = 154,
	RS_FEED_ERRORSTATE_PROCESS_XSLT_TRANSFORM_ERROR   = 155,
	RS_FEED_ERRORSTATE_PROCESS_XSLT_NO_RESULT         = 156
};

enum RsFeedResult
{
	RS_FEED_RESULT_SUCCESS,
	RS_FEED_RESULT_FEED_NOT_FOUND,
	RS_FEED_RESULT_PARENT_NOT_FOUND,
	RS_FEED_RESULT_PARENT_IS_NO_FOLDER,
	RS_FEED_RESULT_FEED_IS_FOLDER,
	RS_FEED_RESULT_FEED_IS_NO_FOLDER
};

enum RsFeedTransformationType
{
	RS_FEED_TRANSFORMATION_TYPE_NONE  = 0,
	RS_FEED_TRANSFORMATION_TYPE_XPATH = 1,
	RS_FEED_TRANSFORMATION_TYPE_XSLT  = 2
};

class FeedInfo
{
public:
	enum WorkState
	{
		WAITING,
		WAITING_TO_DOWNLOAD,
		DOWNLOADING,
		WAITING_TO_PROCESS,
		PROCESSING
	};

public:
	FeedInfo()
	{
		proxyPort = 0;
		updateInterval = 0;
		lastUpdate = 0;
		storageTime = 0;
		errorState = RS_FEED_ERRORSTATE_OK;
		flag.folder = false;
		flag.infoFromFeed = false;
		flag.standardStorageTime = false;
		flag.standardUpdateInterval = false;
		flag.standardProxy = false;
		flag.authentication = false;
		flag.deactivated = false;
		flag.forum = false;
		flag.updateForumInfo = false;
		flag.posted = false;
		flag.updatePostedInfo = false;
		flag.embedImages = false;
		flag.saveCompletePage = false;
		flag.preview = false;
		transformationType = RS_FEED_TRANSFORMATION_TYPE_NONE;
	}

	uint32_t                 feedId;
	uint32_t                 parentId;
	std::string              url;
	std::string              name;
	std::string              description;
	std::string              icon;
	std::string              user;
	std::string              password;
	std::string              proxyAddress;
	uint16_t                 proxyPort;
	uint32_t                 updateInterval;
	time_t                   lastUpdate;
	uint32_t                 storageTime;
	std::string              forumId;
	std::string              postedId;
	WorkState                workstate;
	RsFeedReaderErrorState   errorState;
	std::string              errorString;

	RsFeedTransformationType transformationType;
	std::list<std::string>   xpathsToUse;
	std::list<std::string>   xpathsToRemove;
	std::string              xslt;

	struct {
		bool folder : 1;
		bool infoFromFeed : 1;
		bool standardStorageTime : 1;
		bool standardUpdateInterval : 1;
		bool standardProxy : 1;
		bool authentication : 1;
		bool deactivated : 1;
		bool forum : 1;
		bool updateForumInfo : 1;
		bool posted : 1;
		bool updatePostedInfo : 1;
		bool postedFirstImage : 1;
		bool postedOnlyImage : 1;
		bool embedImages : 1;
		bool saveCompletePage : 1;
		bool preview : 1;
	} flag;
};

class FeedMsgInfo
{
public:
	FeedMsgInfo()
	{
		pubDate = 0;
		flag.isnew = false;
		flag.read = false;
		flag.deleted = false;
	}

	std::string msgId;
	uint32_t    feedId;
	std::string title;
	std::string link;
	std::string author;
	std::string description;
	std::string descriptionTransformed;
	time_t      pubDate;

	struct {
		bool isnew : 1;
		bool read : 1;
		bool deleted : 1;
	} flag;
};

class RsFeedReaderNotify
{
public:
	RsFeedReaderNotify() {}

	virtual void notifyFeedChanged(uint32_t /*feedId*/, int /*type*/) {}
	virtual void notifyMsgChanged(uint32_t /*feedId*/, const std::string &/*msgId*/, int /*type*/) {}
};

class RsFeedReader
{
public:
	RsFeedReader() {}
	virtual ~RsFeedReader() {}

	virtual void stop() = 0;
	virtual void setNotify(RsFeedReaderNotify *notify) = 0;

	virtual uint32_t getStandardStorageTime() = 0;
	virtual void     setStandardStorageTime(uint32_t storageTime) = 0;
	virtual uint32_t getStandardUpdateInterval() = 0;
	virtual void     setStandardUpdateInterval(uint32_t updateInterval) = 0;
	virtual bool     getStandardProxy(std::string &proxyAddress, uint16_t &proxyPort) = 0;
	virtual void     setStandardProxy(bool useProxy, const std::string &proxyAddress, uint16_t proxyPort) = 0;
	virtual bool     getSaveInBackground() = 0;
	virtual void     setSaveInBackground(bool saveInBackground) = 0;

	virtual RsFeedResult addFolder(uint32_t parentId, const std::string &name, uint32_t &feedId) = 0;
	virtual RsFeedResult setFolder(uint32_t feedId, const std::string &name) = 0;
	virtual RsFeedResult addFeed(const FeedInfo &feedInfo, uint32_t &feedId) = 0;
	virtual RsFeedResult setFeed(uint32_t feedId, const FeedInfo &feedInfo) = 0;
	virtual RsFeedResult setParent(uint32_t feedId, uint32_t parentId) = 0;
	virtual bool         removeFeed(uint32_t feedId) = 0;
	virtual bool         addPreviewFeed(const FeedInfo &feedInfo, uint32_t &feedId) = 0;
	virtual void         getFeedList(uint32_t parentId, std::list<FeedInfo> &feedInfos) = 0;
	virtual bool         getFeedInfo(uint32_t feedId, FeedInfo &feedInfo) = 0;
	virtual bool         getMsgInfo(uint32_t feedId, const std::string &msgId, FeedMsgInfo &msgInfo) = 0;
	virtual bool         removeMsg(uint32_t feedId, const std::string &msgId) = 0;
	virtual bool         removeMsgs(uint32_t feedId, const std::list<std::string> &msgIds) = 0;
	virtual bool         getMessageCount(uint32_t feedId, uint32_t *msgCount, uint32_t *newCount, uint32_t *unreadCount) = 0;
	virtual bool         getFeedMsgList(uint32_t feedId, std::list<FeedMsgInfo> &msgInfos) = 0;
	virtual bool         getFeedMsgIdList(uint32_t feedId, std::list<std::string> &msgIds) = 0;
	virtual bool         processFeed(uint32_t feedId) = 0;
	virtual bool         setMessageRead(uint32_t feedId, const std::string &msgId, bool read) = 0;
	virtual bool         retransformMsg(uint32_t feedId, const std::string &msgId) = 0;
	virtual bool         clearMessageCache(uint32_t feedId) = 0;

	virtual RsGxsForums* forums() = 0;
	virtual RsPosted*    posted() = 0;
	virtual bool         getForumGroups(std::vector<RsGxsForumGroup> &groups, bool onlyOwn) = 0;
	virtual bool         getPostedGroups(std::vector<RsPostedGroup> &groups, bool onlyOwn) = 0;

	virtual RsFeedReaderErrorState processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString) = 0;
	virtual RsFeedReaderErrorState processXslt(const std::string &xslt, std::string &description, std::string &errorString) = 0;
};

#endif
