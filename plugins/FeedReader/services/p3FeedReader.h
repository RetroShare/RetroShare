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
class RsPosted;
struct RsPostedGroup;
class RsGxsIfaceHelper;

class p3FeedReader : public RsPQIService, public RsFeedReader
{
public:
	p3FeedReader(RsPluginHandler *pgHandler, RsGxsForums *forums, RsPosted *posted);

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

	virtual RsFeedAddResult addFolder(uint32_t parentId, const std::string &name, uint32_t &feedId);
	virtual RsFeedAddResult setFolder(uint32_t feedId, const std::string &name);
	virtual RsFeedAddResult addFeed(const FeedInfo &feedInfo, uint32_t &feedId);
	virtual RsFeedAddResult setFeed(uint32_t feedId, const FeedInfo &feedInfo);
	virtual bool            removeFeed(uint32_t feedId);
	virtual bool            addPreviewFeed(const FeedInfo &feedInfo, uint32_t &feedId);
	virtual void            getFeedList(uint32_t parentId, std::list<FeedInfo> &feedInfos);
	virtual bool            getFeedInfo(uint32_t feedId, FeedInfo &feedInfo);
	virtual bool            getMsgInfo(uint32_t feedId, const std::string &msgId, FeedMsgInfo &msgInfo);
	virtual bool            removeMsg(uint32_t feedId, const std::string &msgId);
	virtual bool            removeMsgs(uint32_t feedId, const std::list<std::string> &msgIds);
	virtual bool            getMessageCount(uint32_t feedId, uint32_t *msgCount, uint32_t *newCount, uint32_t *unreadCount);
	virtual bool            getFeedMsgList(uint32_t feedId, std::list<FeedMsgInfo> &msgInfos);
	virtual bool            getFeedMsgIdList(uint32_t feedId, std::list<std::string> &msgIds);
	virtual bool            processFeed(uint32_t feedId);
	virtual bool            setMessageRead(uint32_t feedId, const std::string &msgId, bool read);
	virtual bool            retransformMsg(uint32_t feedId, const std::string &msgId);
	virtual bool            clearMessageCache(uint32_t feedId);

	virtual RsFeedReaderErrorState processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString);
	virtual RsFeedReaderErrorState processXslt(const std::string &xslt, std::string &description, std::string &errorString);

	/****************** p3Service STUFF ******************/
	virtual int tick();
	virtual RsServiceInfo getServiceInfo() ;

	/****************** internal STUFF *******************/
	bool getFeedToDownload(RsFeedReaderFeed &feed, uint32_t neededFeedId);
	void onDownloadSuccess(uint32_t feedId, const std::string &content, std::string &icon);
	void onDownloadError(uint32_t feedId, RsFeedReaderErrorState result, const std::string &errorString);
	void onProcessSuccess_filterMsg(uint32_t feedId, std::list<RsFeedReaderMsg*> &msgs);
	void onProcessSuccess_addMsgs(uint32_t feedId, std::list<RsFeedReaderMsg*> &msgs, bool single);
	void onProcessError(uint32_t feedId, RsFeedReaderErrorState result, const std::string &errorString);

	bool getFeedToProcess(RsFeedReaderFeed &feed, uint32_t neededFeedId);

	void setFeedInfo(uint32_t feedId, const std::string &name, const std::string &description);

	bool getForumGroup(const RsGxsGroupId &groupId, RsGxsForumGroup &forumGroup);
	bool updateForumGroup(const RsGxsForumGroup &forumGroup, const std::string &groupName, const std::string &groupDescription);
	bool getPostedGroup(const RsGxsGroupId &groupId, RsPostedGroup &postedGroup);
	bool updatePostedGroup(const RsPostedGroup &postedGroup, const std::string &groupName, const std::string &groupDescription);
	bool waitForToken(RsGxsIfaceHelper *interface, uint32_t token);

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
	RsPosted *mPosted;
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
	std::map<uint32_t, RsFeedReaderFeed*> mFeeds;

	RsMutex mDownloadMutex;
	std::list<uint32_t> mDownloadFeeds;

	RsMutex mProcessMutex;
	std::list<uint32_t> mProcessFeeds;

	RsMutex mPreviewMutex;
	p3FeedReaderThread *mPreviewDownloadThread;
	p3FeedReaderThread *mPreviewProcessThread;
};

#endif 
