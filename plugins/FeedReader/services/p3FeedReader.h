/*******************************************************************************
 * plugins/FeedReader/services/p3FeedReader.h                                  *
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

#ifndef P3_FEEDREADER
#define P3_FEEDREADER

#include "retroshare/rsplugin.h"
#include "plugins/rspqiservice.h"
#include "interface/rsFeedReader.h"

#include "retroshare/rsgxsifacetypes.h"

class RsFeedReaderFeed;
class RsFeedReaderMsg;
class p3FeedReaderThread;

class RsGxsForums;
struct RsGxsForumGroup;

class p3FeedReader : public RsPQIService, public RsFeedReader
{
public:
	p3FeedReader(RsPluginHandler *pgHandler, RsGxsForums *forums);

	/****************** FeedReader Interface *************/
	virtual void stop();
	virtual void setNotify(RsFeedReaderNotify *notify);

	virtual uint32_t getStandardStorageTime();
	virtual void     setStandardStorageTime(uint32_t storageTime);
	virtual uint32_t getStandardUpdateInterval();
	virtual void     setStandardUpdateInterval(uint32_t updateInterval);
	virtual bool     getStandardProxy(std::string &proxyAddress, uint16_t &proxyPort);
	virtual void     setStandardProxy(bool useProxy, const std::string &proxyAddress, uint16_t proxyPort);
	virtual bool     getSaveInBackground();
	virtual void     setSaveInBackground(bool saveInBackground);

	virtual RsFeedAddResult addFolder(const std::string parentId, const std::string &name, std::string &feedId);
	virtual RsFeedAddResult setFolder(const std::string &feedId, const std::string &name);
	virtual RsFeedAddResult addFeed(const FeedInfo &feedInfo, std::string &feedId);
	virtual RsFeedAddResult setFeed(const std::string &feedId, const FeedInfo &feedInfo);
	virtual bool            removeFeed(const std::string &feedId);
	virtual bool            addPreviewFeed(const FeedInfo &feedInfo, std::string &feedId);
	virtual void            getFeedList(const std::string &parentId, std::list<FeedInfo> &feedInfos);
	virtual bool            getFeedInfo(const std::string &feedId, FeedInfo &feedInfo);
	virtual bool            getMsgInfo(const std::string &feedId, const std::string &msgId, FeedMsgInfo &msgInfo);
	virtual bool            removeMsg(const std::string &feedId, const std::string &msgId);
	virtual bool            removeMsgs(const std::string &feedId, const std::list<std::string> &msgIds);
	virtual bool            getMessageCount(const std::string &feedId, uint32_t *msgCount, uint32_t *newCount, uint32_t *unreadCount);
	virtual bool            getFeedMsgList(const std::string &feedId, std::list<FeedMsgInfo> &msgInfos);
	virtual bool            getFeedMsgIdList(const std::string &feedId, std::list<std::string> &msgIds);
	virtual bool            processFeed(const std::string &feedId);
	virtual bool            setMessageRead(const std::string &feedId, const std::string &msgId, bool read);
	virtual bool            retransformMsg(const std::string &feedId, const std::string &msgId);
	virtual bool            clearMessageCache(const std::string &feedId);

	virtual RsFeedReaderErrorState processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString);
	virtual RsFeedReaderErrorState processXslt(const std::string &xslt, std::string &description, std::string &errorString);

	/****************** p3Service STUFF ******************/
	virtual int tick();
	virtual RsServiceInfo getServiceInfo() ;

	/****************** internal STUFF *******************/
	bool getFeedToDownload(RsFeedReaderFeed &feed, const std::string &neededFeedId);
	void onDownloadSuccess(const std::string &feedId, const std::string &content, std::string &icon);
	void onDownloadError(const std::string &feedId, RsFeedReaderErrorState result, const std::string &errorString);
	void onProcessSuccess_filterMsg(const std::string &feedId, std::list<RsFeedReaderMsg*> &msgs);
	void onProcessSuccess_addMsgs(const std::string &feedId, std::list<RsFeedReaderMsg*> &msgs, bool single);
	void onProcessError(const std::string &feedId, RsFeedReaderErrorState result, const std::string &errorString);

	bool getFeedToProcess(RsFeedReaderFeed &feed, const std::string &neededFeedId);

	void setFeedInfo(const std::string &feedId, const std::string &name, const std::string &description);

	bool getForumGroup(const RsGxsGroupId &groupId, RsGxsForumGroup &forumGroup);
	bool updateForumGroup(const RsGxsForumGroup &forumGroup, const std::string &groupName, const std::string &groupDescription);
	bool waitForToken(uint32_t token);

protected:
	/****************** p3Config STUFF *******************/
	virtual RsSerialiser *setupSerialiser();
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
	virtual bool loadList(std::list<RsItem *>& load);
	virtual void saveDone();

private:
	void cleanFeeds();
	void deleteAllMsgs_locked(RsFeedReaderFeed *fi);
	void stopPreviewThreads_locked();

private:
	time_t   mLastClean;
	RsGxsForums *mForums;
	RsFeedReaderNotify *mNotify;
	volatile bool mStopped;

	RsMutex mFeedReaderMtx;
	std::list<RsItem*> cleanSaveData;
	bool mSaveInBackground;
	std::list<p3FeedReaderThread*> mThreads;
	uint32_t mNextFeedId;
	uint32_t mNextMsgId;
	int32_t mNextPreviewFeedId;
	int32_t mNextPreviewMsgId;
	uint32_t mStandardUpdateInterval;
	uint32_t mStandardStorageTime;
	bool mStandardUseProxy;
	std::string mStandardProxyAddress;
	uint16_t mStandardProxyPort;
	std::map<std::string, RsFeedReaderFeed*> mFeeds;

	RsMutex mDownloadMutex;
	std::list<std::string> mDownloadFeeds;

	RsMutex mProcessMutex;
	std::list<std::string> mProcessFeeds;

	RsMutex mPreviewMutex;
	p3FeedReaderThread *mPreviewDownloadThread;
	p3FeedReaderThread *mPreviewProcessThread;
};

#endif 
