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

#include "rsFeedReaderItems.h"
#include "p3FeedReader.h"
#include "p3FeedReaderThread.h"
#include "serialiser/rsconfigitems.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsforums.h"
#include "util/rsstring.h"

RsFeedReader *rsFeedReader = NULL;

#define FEEDREADER_CLEAN_INTERVAL 1 * 60 * 60 // check every hour

/*********
 * #define FEEDREADER_DEBUG
 *********/

p3FeedReader::p3FeedReader(RsPluginHandler* pgHandler)
	: RsPQIService(RS_PKT_TYPE_FEEDREADER_CONFIG, CONFIG_TYPE_FEEDREADER, 5, pgHandler),
	  mFeedReaderMtx("p3FeedReader"), mDownloadMutex("p3FeedReaderDownload"), mProcessMutex("p3FeedReaderProcess")
{
	mNextFeedId = 1;
	mNextMsgId = 1;
	mStandardUpdateInterval = 60 * 60; // 60 minutes
	mStandardStorageTime = 30 * 60 * 60 * 24; // 30 days
	mStandardUseProxy = false;
	mStandardProxyPort = 0;
	mLastClean = 0;
	mNotify = NULL;

	/* start download thread */
	p3FeedReaderThread *frt = new p3FeedReaderThread(this, p3FeedReaderThread::DOWNLOAD);
	mThreads.push_back(frt);
	frt->start();

	/* start process thread */
	frt = new p3FeedReaderThread(this, p3FeedReaderThread::PROCESS);
	mThreads.push_back(frt);
	frt->start();
}

/***************************************************************************/
/****************************** RsFeedReader *******************************/
/***************************************************************************/

static void feedToInfo(const RsFeedReaderFeed *feed, FeedInfo &info)
{
	info.feedId = feed->feedId;
	info.parentId = feed->parentId;
	info.url = feed->url;
	info.name = feed->name;
	info.description = feed->description;
	info.icon = feed->icon;
	info.user = feed->user;
	info.password = feed->password;
	info.proxyAddress = feed->proxyAddress;
	info.proxyPort = feed->proxyPort;
	info.updateInterval = feed->updateInterval;
	info.lastUpdate = feed->lastUpdate;
	info.forumId = feed->forumId;
	info.storageTime = feed->storageTime;
	info.error = (feed->errorState != RS_FEED_ERRORSTATE_OK); // currently only as bool
	info.errorString = feed->errorString;

	info.flag.folder = (feed->flag & RS_FEED_FLAG_FOLDER);
	info.flag.infoFromFeed = (feed->flag & RS_FEED_FLAG_INFO_FROM_FEED);
	info.flag.standardStorageTime = (feed->flag & RS_FEED_FLAG_STANDARD_STORAGE_TIME);
	info.flag.standardUpdateInterval = (feed->flag & RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL);
	info.flag.standardProxy = (feed->flag & RS_FEED_FLAG_STANDARD_PROXY);
	info.flag.authentication = (feed->flag & RS_FEED_FLAG_AUTHENTICATION);
	info.flag.deactivated = (feed->flag & RS_FEED_FLAG_DEACTIVATED);
	info.flag.forum = (feed->flag & RS_FEED_FLAG_FORUM);
	info.flag.updateForumInfo = (feed->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO);

	switch (feed->workstate) {
	case RsFeedReaderFeed::WAITING:
		info.workstate = FeedInfo::WAITING;
		break;
	case RsFeedReaderFeed::WAITING_TO_DOWNLOAD:
		info.workstate = FeedInfo::WAITING_TO_DOWNLOAD;
		break;
	case RsFeedReaderFeed::DOWNLOADING:
		info.workstate = FeedInfo::DOWNLOADING;
		break;
	case RsFeedReaderFeed::WAITING_TO_PROCESS:
		info.workstate = FeedInfo::WAITING_TO_PROCESS;
		break;
	case RsFeedReaderFeed::PROCESSING:
		info.workstate = FeedInfo::PROCESSING;
		break;
	}
}

static void infoToFeed(const FeedInfo &info, RsFeedReaderFeed *feed, bool add)
{
//	feed->feedId = info.feedId;
	feed->parentId = info.parentId;
	feed->url = info.url;
	feed->name = info.name;
	feed->description = info.description;
//	feed->icon = info.icon;
	feed->user = info.user;
	feed->password = info.password;
	feed->proxyAddress = info.proxyAddress;
	feed->proxyPort = info.proxyPort;
	feed->updateInterval = info.updateInterval;
//	feed->lastUpdate = info.lastUpdate;
	feed->storageTime = info.storageTime;
	if (add) {
		/* set forum id only when adding a feed */
		feed->forumId = info.forumId;
	}

	uint32_t oldFlag = feed->flag;
	feed->flag = 0;
	if (info.flag.infoFromFeed) {
		feed->flag |= RS_FEED_FLAG_INFO_FROM_FEED;
	}
	if (info.flag.standardStorageTime) {
		feed->flag |= RS_FEED_FLAG_STANDARD_STORAGE_TIME;
	}
	if (info.flag.standardUpdateInterval) {
		feed->flag |= RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL;
	}
	if (info.flag.standardProxy) {
		feed->flag |= RS_FEED_FLAG_STANDARD_PROXY;
	}
	if (info.flag.authentication) {
		feed->flag |= RS_FEED_FLAG_AUTHENTICATION;
	}
	if (info.flag.deactivated) {
		feed->flag |= RS_FEED_FLAG_DEACTIVATED;
	}
	if (add) {
		/* only set when adding a new feed */
		if (info.flag.folder) {
			feed->flag |= RS_FEED_FLAG_FOLDER;
		}
		if (info.flag.forum) {
			feed->flag |= RS_FEED_FLAG_FORUM;
		}
	} else {
		/* use old bits */
		feed->flag |= (oldFlag & (RS_FEED_FLAG_FOLDER | RS_FEED_FLAG_FORUM));
	}
	if (info.flag.updateForumInfo) {
		feed->flag |= RS_FEED_FLAG_UPDATE_FORUM_INFO;
	}
}

static void feedMsgToInfo(const RsFeedReaderMsg *msg, FeedMsgInfo &info)
{
	info.msgId = msg->msgId;
	info.feedId = msg->feedId;
	info.title = msg->title;
	info.link = msg->link;
	info.author = msg->author;
	info.description = msg->description;
	info.pubDate = msg->pubDate;

	info.flag.isnew = (msg->flag & RS_FEEDMSG_FLAG_NEW);
	info.flag.read = (msg->flag & RS_FEEDMSG_FLAG_READ);
}

void p3FeedReader::setNotify(RsFeedReaderNotify *notify)
{
	mNotify = notify;
}

uint32_t p3FeedReader::getStandardStorageTime()
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	return mStandardStorageTime;
}

void p3FeedReader::setStandardStorageTime(uint32_t storageTime)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	if (mStandardStorageTime != storageTime) {
		mStandardStorageTime = storageTime;
		IndicateConfigChanged();
	}
}

uint32_t p3FeedReader::getStandardUpdateInterval()
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	return mStandardUpdateInterval;
}

void p3FeedReader::setStandardUpdateInterval(uint32_t updateInterval)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	if (mStandardUpdateInterval != updateInterval) {
		mStandardUpdateInterval = updateInterval;
		IndicateConfigChanged();
	}
}

bool p3FeedReader::getStandardProxy(std::string &proxyAddress, uint16_t &proxyPort)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	proxyAddress = mStandardProxyAddress;
	proxyPort = mStandardProxyPort;
	return mStandardUseProxy;
}

void p3FeedReader::setStandardProxy(bool useProxy, const std::string &proxyAddress, uint16_t proxyPort)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/
	if (useProxy != mStandardUseProxy || proxyAddress != mStandardProxyAddress || proxyPort != mStandardProxyPort) {
		mStandardProxyAddress = proxyAddress;
		mStandardProxyPort = proxyPort;
		mStandardUseProxy = useProxy;
		IndicateConfigChanged();
	}
}

void p3FeedReader::stop()
{
	/* stop threads */
	std::list<p3FeedReaderThread*>::iterator it;
	for (it = mThreads.begin(); it != mThreads.end(); ++it) {
		(*it)->join();
	}
	mThreads.clear();
}

RsFeedAddResult p3FeedReader::addFolder(const std::string parentId, const std::string &name, std::string &feedId)
{
	feedId.clear();

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		if (!parentId.empty()) {
			/* check parent id */
			std::map<std::string, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFolder - parent id " << parentId << " not found" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFolder - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = new RsFeedReaderFeed;
		rs_sprintf(fi->feedId, "%lu", mNextFeedId++);
		fi->parentId = parentId;
		fi->name = name;
		fi->flag = RS_FEED_FLAG_FOLDER;
		mFeeds[fi->feedId] = fi;

		feedId = fi->feedId;
	}

	IndicateConfigChanged();

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_ADD);
	}

	return RS_FEED_ADD_RESULT_SUCCESS;
}

RsFeedAddResult p3FeedReader::setFolder(const std::string &feedId, const std::string &name)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::setFolder - feed id " << feedId << ", name " << name << std::endl;
#endif

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFolder - feed id " << feedId << " not found" << std::endl;
#endif
			return RS_FEED_ADD_RESULT_FEED_NOT_FOUND;
		}

		if ((feedIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFolder - feed " << feedIt->second->name << " is no folder" << std::endl;
#endif
			return RS_FEED_ADD_RESULT_FEED_IS_NO_FOLDER;
		}

		RsFeedReaderFeed *fi = feedIt->second;
		if (fi->name == name) {
			return RS_FEED_ADD_RESULT_SUCCESS;
		}
		fi->name = name;
	}

	IndicateConfigChanged();

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return RS_FEED_ADD_RESULT_SUCCESS;
}

RsFeedAddResult p3FeedReader::addFeed(const FeedInfo &feedInfo, std::string &feedId)
{
	feedId.clear();

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::addFeed - add feed " << feedInfo.name << ", url " << feedInfo.url << std::endl;
#endif

		if (!feedInfo.parentId.empty()) {
			/* check parent id */
			std::map<std::string, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(feedInfo.parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFeed - parent id " << feedInfo.parentId << " not found" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFeed - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = new RsFeedReaderFeed;
		infoToFeed(feedInfo, fi, true);
		rs_sprintf(fi->feedId, "%lu", mNextFeedId++);

		mFeeds[fi->feedId] = fi;

		feedId = fi->feedId;
	}

	IndicateConfigChanged();

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_ADD);
	}

	return RS_FEED_ADD_RESULT_SUCCESS;
}

RsFeedAddResult p3FeedReader::setFeed(const std::string &feedId, const FeedInfo &feedInfo)
{
	std::string forumId;
	ForumInfo forumInfo;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::setFeed - set feed " << feedInfo.name << ", url " << feedInfo.url << std::endl;
#endif

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - feed id " << feedId << " not found" << std::endl;
#endif
			return RS_FEED_ADD_RESULT_FEED_NOT_FOUND;
		}

		if (feedIt->second->flag & RS_FEED_FLAG_FOLDER) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - feed " << feedIt->second->name << " is a folder" << std::endl;
#endif
			return RS_FEED_ADD_RESULT_FEED_IS_FOLDER;
		}

		if (!feedInfo.parentId.empty()) {
			/* check parent id */
			std::map<std::string, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(feedInfo.parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setFeed - parent id " << feedInfo.parentId << " not found" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setFeed - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_ADD_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = feedIt->second;
		std::string oldName = fi->name;
		std::string oldDescription = fi->description;

		infoToFeed(feedInfo, fi, false);

		if ((fi->flag & RS_FEED_FLAG_FORUM) && (fi->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO) && !fi->forumId.empty() &&
			(fi->name != oldName || fi->description != oldDescription)) {
			/* name or description changed, update forum */
			forumId = fi->forumId;
			librs::util::ConvertUtf8ToUtf16(fi->name, forumInfo.forumName);
			librs::util::ConvertUtf8ToUtf16(fi->description, forumInfo.forumDesc);
		}
	}

	IndicateConfigChanged();

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	if (!forumId.empty()) {
		/* name or description changed, update forum */
		if (!rsForums->setForumInfo(forumId, forumInfo)) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - can't change forum " << forumId << std::endl;
#endif
		}
	}

	return RS_FEED_ADD_RESULT_SUCCESS;
}

void p3FeedReader::deleteAllMsgs_locked(RsFeedReaderFeed *fi)
{
	if (!fi) {
		return;
	}

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->mMsgs.begin(); msgIt != fi->mMsgs.end(); ++msgIt) {
		delete(msgIt->second);
	}

	fi->mMsgs.clear();
}

bool p3FeedReader::removeFeed(const std::string &feedId)
{
	std::list<std::string> removedFeedIds;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeFeed - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		removedFeedIds.push_back(feedId);

		RsFeedReaderFeed *fi = feedIt->second;
		mFeeds.erase(feedIt);

		if (fi->flag & RS_FEED_FLAG_FOLDER) {
			std::list<std::string> feedIds;
			feedIds.push_back(fi->feedId);
			while (!feedIds.empty()) {
				std::string parentId = feedIds.front();
				feedIds.pop_front();

				std::map<std::string, RsFeedReaderFeed*>::iterator feedIt1;
				for (feedIt1 = mFeeds.begin(); feedIt1 != mFeeds.end(); ) {
					RsFeedReaderFeed *fi1 = feedIt1->second;

					if (fi1->parentId == parentId) {
						removedFeedIds.push_back(fi1->feedId);

						std::map<std::string, RsFeedReaderFeed*>::iterator tempIt = feedIt1;
						++feedIt1;
						mFeeds.erase(tempIt);

						if (fi1->flag & RS_FEED_FLAG_FOLDER) {
							feedIds.push_back(fi->feedId);
						}

						deleteAllMsgs_locked(fi1);
						delete(fi1);

						continue;
					}
					++feedIt1;
				}
			}
		}

		deleteAllMsgs_locked(fi);
		delete(fi);
	}

	IndicateConfigChanged();

	if (mNotify) {
		/* only notify remove of feed */
		std::list<std::string>::iterator it;
		for (it = removedFeedIds.begin(); it != removedFeedIds.end(); ++it) {
			mNotify->feedChanged(*it, NOTIFY_TYPE_DEL);
		}
	}

	return true;
}

void p3FeedReader::getFeedList(const std::string &parentId, std::list<FeedInfo> &feedInfos)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt;
	for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
		RsFeedReaderFeed *fi = feedIt->second;

		if (fi->parentId == parentId) {
			FeedInfo feedInfo;
			feedToInfo(fi, feedInfo);
			feedInfos.push_back(feedInfo);
		}
	}
}

bool p3FeedReader::getFeedInfo(const std::string &feedId, FeedInfo &feedInfo)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedInfo - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	feedToInfo(feedIt->second, feedInfo);

	return true;
}

bool p3FeedReader::getMsgInfo(const std::string &feedId, const std::string &msgId, FeedMsgInfo &msgInfo)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getMsgInfo - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	msgIt = fi->mMsgs.find(msgId);
	if (msgIt == fi->mMsgs.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getMsgInfo - msg " << msgId << " not found" << std::endl;
#endif
		return false;
	}

	feedMsgToInfo(msgIt->second, msgInfo);

	return true;
}

bool p3FeedReader::removeMsg(const std::string &feedId, const std::string &msgId)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsg - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		msgIt = fi->mMsgs.find(msgId);
		if (msgIt == fi->mMsgs.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsg - msg " << msgId << " not found" << std::endl;
#endif
			return false;
		}

		msgIt->second->flag |= RS_FEEDMSG_FLAG_DELETED;
	}

	IndicateConfigChanged();
	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
		mNotify->msgChanged(feedId, msgId, NOTIFY_TYPE_DEL);
	}

	return true;
}

bool p3FeedReader::removeMsgs(const std::string &feedId, const std::list<std::string> &msgIds)
{
	std::list<std::string> removedMsgs;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsgs - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::list<std::string>::const_iterator idIt;
		for (idIt = msgIds.begin(); idIt != msgIds.end(); ++idIt) {
			std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
			msgIt = fi->mMsgs.find(*idIt);
			if (msgIt == fi->mMsgs.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::removeMsgs - msg " << *idIt << " not found" << std::endl;
#endif
				continue;
			}

			msgIt->second->flag |= RS_FEEDMSG_FLAG_DELETED;

			removedMsgs.push_back(*idIt);
		}
	}

	if (mNotify && !removedMsgs.empty()) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);

		std::list<std::string>::iterator it;
		for (it = removedMsgs.begin(); it != removedMsgs.end(); ++it) {
			mNotify->msgChanged(feedId, *it, NOTIFY_TYPE_DEL);
		}
	}

	return true;
}

bool p3FeedReader::getMessageCount(const std::string &feedId, uint32_t *msgCount, uint32_t *newCount, uint32_t *unreadCount)
{
	if (msgCount) *msgCount = 0;
	if (unreadCount) *unreadCount = 0;
	if (newCount) *newCount = 0;

	if (!msgCount && !unreadCount && !newCount) {
		return true;
	}

	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getMessageCount - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->mMsgs.begin(); msgIt != fi->mMsgs.end(); ++msgIt) {
		RsFeedReaderMsg *mi = msgIt->second;

		if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
			continue;
		}

		if (msgCount) ++(*msgCount);
		if (newCount && (mi->flag & RS_FEEDMSG_FLAG_NEW)) ++(*newCount);
		if (unreadCount && (mi->flag & RS_FEEDMSG_FLAG_READ) == 0) ++(*unreadCount);
	}

	return true;
}

bool p3FeedReader::getFeedMsgList(const std::string &feedId, std::list<FeedMsgInfo> &msgInfos)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedMsgList - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->mMsgs.begin(); msgIt != fi->mMsgs.end(); ++msgIt) {
		RsFeedReaderMsg *mi = msgIt->second;

		if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
			continue;
		}

		FeedMsgInfo msgInfo;
		feedMsgToInfo(mi, msgInfo);
		msgInfos.push_back(msgInfo);
	}

	return true;
}

static bool canProcessFeed(RsFeedReaderFeed *fi)
{
	if (fi->flag & RS_FEED_FLAG_DEACTIVATED) {
		/* deactivated */
		return false;
	}

	if (fi->workstate != RsFeedReaderFeed::WAITING) {
		/* should be working */
		return false;
	}

	if (fi->flag & RS_FEED_FLAG_FOLDER) {
		/* folder */
		return false;
	}

	return true;
}

bool p3FeedReader::processFeed(const std::string &feedId)
{
	std::list<std::string> feedToDownload;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt;

		if (feedId.empty()) {
			/* process all feeds */
			for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
				RsFeedReaderFeed *fi = feedIt->second;

				if (!canProcessFeed(fi)) {
					continue;
				}

				/* add to download list */
				feedToDownload.push_back(fi->feedId);
				fi->workstate = RsFeedReaderFeed::WAITING_TO_DOWNLOAD;
				fi->content.clear();

#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::processFeed - starting feed " << fi->feedId << " (" << fi->name << ")" << std::endl;
#endif
			}
		} else {
			feedIt = mFeeds.find(feedId);
			if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::processFeed - feed " << feedId << " not found" << std::endl;
#endif
				return false;
			}

			RsFeedReaderFeed *fi = feedIt->second;
			if (fi->flag & RS_FEED_FLAG_FOLDER) {
				std::list<std::string> feedIds;
				feedIds.push_back(fi->feedId);
				while (!feedIds.empty()) {
					std::string parentId = feedIds.front();
					feedIds.pop_front();

					std::map<std::string, RsFeedReaderFeed*>::iterator feedIt1;
					for (feedIt1 = mFeeds.begin(); feedIt1 != mFeeds.end(); ++feedIt1) {
						RsFeedReaderFeed *fi1 = feedIt1->second;

						if (fi1->parentId == parentId) {
							if (fi1->flag & RS_FEED_FLAG_FOLDER) {
								feedIds.push_back(fi1->feedId);
							} else {
								if (canProcessFeed(fi1)) {
									fi1->workstate = RsFeedReaderFeed::WAITING_TO_DOWNLOAD;
									fi1->content.clear();

									feedToDownload.push_back(fi1->feedId);
#ifdef FEEDREADER_DEBUG
									std::cerr << "p3FeedReader::processFeed - starting feed " << fi1->feedId << " (" << fi1->name << ")" << std::endl;
#endif
								}
							}
						}
					}
				}
			} else {
				if (canProcessFeed(fi)) {
					fi->workstate = RsFeedReaderFeed::WAITING_TO_DOWNLOAD;
					fi->content.clear();

					feedToDownload.push_back(fi->feedId);
#ifdef FEEDREADER_DEBUG
					std::cerr << "p3FeedReader::processFeed - starting feed " << fi->feedId << " (" << fi->name << ")" << std::endl;
#endif
				}
			}
		}
	}

	std::list<std::string> notifyIds;
	std::list<std::string>::iterator it;

	if (!feedToDownload.empty()) {
		RsStackMutex stack(mDownloadMutex); /******* LOCK STACK MUTEX *********/

		for (it = feedToDownload.begin(); it != feedToDownload.end(); ++it) {
			if (std::find(mDownloadFeeds.begin(), mDownloadFeeds.end(), *it) == mDownloadFeeds.end()) {
				mDownloadFeeds.push_back(*it);
				notifyIds.push_back(*it);
			}
		}
	}

	if (mNotify) {
		for (it = notifyIds.begin(); it != notifyIds.end(); ++it) {
			mNotify->feedChanged(*it, NOTIFY_TYPE_MOD);
		}
	}

	return true;
}

bool p3FeedReader::setMessageRead(const std::string &feedId, const std::string &msgId, bool read)
{
	bool changed = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setMessageRead - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		msgIt = fi->mMsgs.find(msgId);
		if (msgIt == fi->mMsgs.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setMessageRead - msg " << msgId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderMsg *mi = msgIt->second;
		uint32_t oldFlag = mi->flag;
		mi->flag &= ~RS_FEEDMSG_FLAG_NEW;
		if (read) {
			/* remove flag new */
			mi->flag |= RS_FEEDMSG_FLAG_READ;
		} else {
			mi->flag &= ~RS_FEEDMSG_FLAG_READ;
		}

		changed = (mi->flag != oldFlag);
	}

	if (changed) {
		IndicateConfigChanged();
		if (mNotify) {
			mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
			mNotify->msgChanged(feedId, msgId, NOTIFY_TYPE_MOD);
		}
	}

	return true;
}

/***************************************************************************/
/****************************** p3Service **********************************/
/***************************************************************************/

int p3FeedReader::tick()
{
	/* clean feeds */
	cleanFeeds();

	time_t currentTime = time(NULL);
	std::list<std::string> feedToDownload;
	std::map<std::string, RsFeedReaderFeed*>::iterator feedIt;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
			RsFeedReaderFeed *fi = feedIt->second;

			if (!canProcessFeed(fi)) {
				continue;
			}

			uint32_t updateInterval;
			if (fi->flag & RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL) {
				updateInterval = mStandardUpdateInterval;
			} else {
				updateInterval = fi->updateInterval;
			}
			if (fi->lastUpdate == 0 || fi->lastUpdate + (long) updateInterval <= currentTime) {
				/* add to download list */
				feedToDownload.push_back(fi->feedId);
				fi->workstate = RsFeedReaderFeed::WAITING_TO_DOWNLOAD;
				fi->content.clear();

#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::tick - starting feed " << fi->feedId << " (" << fi->name << ")" << std::endl;
#endif
			}
		}
	}

	std::list<std::string> notifyIds;
	std::list<std::string>::iterator it;

	if (!feedToDownload.empty()) {
		RsStackMutex stack(mDownloadMutex); /******* LOCK STACK MUTEX *********/

		for (it = feedToDownload.begin(); it != feedToDownload.end(); ++it) {
			if (std::find(mDownloadFeeds.begin(), mDownloadFeeds.end(), *it) == mDownloadFeeds.end()) {
				mDownloadFeeds.push_back(*it);
				notifyIds.push_back(*it);
			}
		}
	}

	if (mNotify) {
		for (it = notifyIds.begin(); it != notifyIds.end(); ++it) {
			mNotify->feedChanged(*it, NOTIFY_TYPE_MOD);
		}
	}

	return 0;
}

void p3FeedReader::cleanFeeds()
{
	time_t currentTime = time(NULL);

	if (mLastClean == 0 || mLastClean + FEEDREADER_CLEAN_INTERVAL <= currentTime) {
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::list<std::pair<std::string, std::string> > removedMsgIds;
		std::map<std::string, RsFeedReaderFeed*>::iterator feedIt;
		for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
			RsFeedReaderFeed *fi = feedIt->second;

			uint32_t storageTime = 0;
			if (fi->flag & RS_FEED_FLAG_STANDARD_STORAGE_TIME) {
				storageTime = mStandardStorageTime;
			} else {
				storageTime = fi->storageTime;
			}
			if (storageTime > 0) {
				uint32_t removedMsgs = 0;

				std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
				for (msgIt = fi->mMsgs.begin(); msgIt != fi->mMsgs.end(); ) {
					RsFeedReaderMsg *mi = msgIt->second;

					if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
						if (mi->pubDate < currentTime - (long) storageTime) {
							removedMsgIds.push_back(std::pair<std::string, std::string> (fi->feedId, mi->msgId));
							delete(mi);
							std::map<std::string, RsFeedReaderMsg*>::iterator deleteIt = msgIt++;
							fi->mMsgs.erase(deleteIt);
							++removedMsgs;
							continue;
						}
					}
					++msgIt;
				}
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::tick - feed " << fi->feedId << " (" << fi->name << ") cleaned, " << removedMsgs << " messages removed" << std::endl;
#endif
			}
		}
		mLastClean = currentTime;

		if (removedMsgIds.size()) {
			IndicateConfigChanged();

			std::list<std::pair<std::string, std::string> >::iterator it;
			for (it = removedMsgIds.begin(); it != removedMsgIds.end(); ++it) {
				mNotify->msgChanged(it->first, it->second, NOTIFY_TYPE_DEL);
			}
		}
	}
}

/***************************************************************************/
/****************************** p3Config ***********************************/
/***************************************************************************/

RsSerialiser *p3FeedReader::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFeedReaderSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}

bool p3FeedReader::saveList(bool &cleanup, std::list<RsItem *> & saveData)
{
	mFeedReaderMtx.lock(); /*********************** LOCK *******/

	cleanup = false;

	RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

	RsTlvKeyValue kv;
	kv.key = "StandardStorageTime";
	rs_sprintf(kv.value, "%u", mStandardStorageTime);
	rskv->tlvkvs.pairs.push_back(kv);

	kv.key = "StandardUpdateInterval";
	rs_sprintf(kv.value, "%u", mStandardUpdateInterval);
	rskv->tlvkvs.pairs.push_back(kv);

	kv.key = "StandardUseProxy";
	rs_sprintf(kv.value, "%hu", mStandardUseProxy ? 1 : 0);
	rskv->tlvkvs.pairs.push_back(kv);

	kv.key = "StandardProxyAddress";
	rs_sprintf(kv.value, "%s", mStandardProxyAddress.c_str());
	rskv->tlvkvs.pairs.push_back(kv);

	kv.key = "StandardProxyPort";
	rs_sprintf(kv.value, "%hu", mStandardProxyPort);
	rskv->tlvkvs.pairs.push_back(kv);

	/* Add KeyValue to saveList */
	saveData.push_back(rskv);

	std::map<std::string, RsFeedReaderFeed *>::iterator it1;
	for (it1 = mFeeds.begin(); it1 != mFeeds.end(); ++it1) {
		RsFeedReaderFeed *fi = it1->second;
		saveData.push_back(fi);

		std::map<std::string, RsFeedReaderMsg*>::iterator it2;
		for (it2 = fi->mMsgs.begin(); it2 != fi->mMsgs.end(); ++it2) {
			saveData.push_back(it2->second);
		}
	}

	/* list completed! */
	return true;
}

void p3FeedReader::saveDone()
{ 
	mFeedReaderMtx.unlock(); /*********************** UNLOCK *******/
	return; 
}

bool p3FeedReader::loadList(std::list<RsItem *>& load)
{
	std::list<RsItem *>::iterator it;
	RsFeedReaderFeed *fi;
	RsFeedReaderMsg *mi;
	RsConfigKeyValueSet *rskv;

#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReader::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	mNextFeedId = 1;
	mNextMsgId = 1;

	std::map<std::string, RsFeedReaderMsg*> msgs;

	for (it = load.begin(); it != load.end(); ++it) {
		/* switch on type */
		if (NULL != (fi = dynamic_cast<RsFeedReaderFeed*>(*it))) {
			uint32_t feedId = 0;
			if (sscanf(fi->feedId.c_str(), "%u", &feedId) == 1) {
				RsStackMutex stack(mFeedReaderMtx); /********** STACK LOCKED MTX ******/
				if (mFeeds.find(fi->feedId) != mFeeds.end()) {
					/* feed with the same id exists */
					delete mFeeds[fi->feedId];
				}
				mFeeds[fi->feedId] = fi;

				if (feedId + 1 > mNextFeedId) {
					mNextFeedId = feedId + 1;
				}
			} else {
				/* invalid feed id */
				delete(*it);
			}
		} else if (NULL != (mi = dynamic_cast<RsFeedReaderMsg*>(*it))) {
			if (msgs.find(mi->msgId) != msgs.end()) {
				delete msgs[mi->msgId];
			}
			msgs[mi->msgId] = mi;
		} else if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet*>(*it))) {
			std::list<RsTlvKeyValue>::iterator kit;
			for(kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); kit++) {
				if (kit->key == "StandardStorageTime") {
					uint32_t value;
					if (sscanf(kit->value.c_str(), "%u", &value) == 1) {
						mStandardStorageTime = value;
					}
				} else if (kit->key == "StandardUpdateInterval") {
					uint32_t value;
					if (sscanf(kit->value.c_str(), "%u", &value) == 1) {
						mStandardUpdateInterval = value;
					}
				} else if (kit->key == "StandardUseProxy") {
					uint16_t value;
					if (sscanf(kit->value.c_str(), "%hu", &value) == 1) {
						mStandardUseProxy = value == 1 ? true : false;
					}
				} else if (kit->key == "StandardProxyAddress") {
					mStandardProxyAddress = kit->value;
				} else if (kit->key == "StandardProxyPort") {
					uint16_t value;
					if (sscanf(kit->value.c_str(), "%hu", &value) == 1) {
						mStandardProxyPort = value;
					}
				}
			}
		} else {
			/* cleanup */
			delete(*it);
		}
	}

	/* now sort msgs into feeds */
	RsStackMutex stack(mFeedReaderMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, RsFeedReaderMsg*>::iterator it1;
	for (it1 = msgs.begin(); it1 != msgs.end(); ++it1) {
		uint32_t msgId = 0;
		if (sscanf(it1->first.c_str(), "%u", &msgId) == 1) {
			std::map<std::string, RsFeedReaderFeed*>::iterator it2 = mFeeds.find(it1->second->feedId);
			if (it2 == mFeeds.end()) {
				/* feed does not exist exists */
				delete it1->second;
				continue;
			}
			it2->second->mMsgs[it1->first] = it1->second;
			if (msgId + 1 > mNextMsgId) {
				mNextMsgId = msgId + 1;
			}
		} else {
			/* invalid msg id */
			delete(it1->second);
		}
	}

	return true;
}

/***************************************************************************/
/****************************** internal ***********************************/
/***************************************************************************/

bool p3FeedReader::getFeedToDownload(RsFeedReaderFeed &feed)
{
	std::string feedId;

	{
		RsStackMutex stack(mDownloadMutex); /******* LOCK STACK MUTEX *********/

		if (mDownloadFeeds.empty()) {
			/* nothing to download */
			return false;
		}

		/* get next feed id to download */
		feedId = mDownloadFeeds.front();
		mDownloadFeeds.pop_front();
	}

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::getFeedToDownload - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		if (it->second->workstate != RsFeedReaderFeed::WAITING_TO_DOWNLOAD) {
			std::cerr << "p3FeedReader::getFeedToDownload - feed in wrong work state for download " << it->second->workstate << std::endl;
			return false;
		}

		/* set state to downloading */
		it->second->workstate = RsFeedReaderFeed::DOWNLOADING;

		/* return a copy of the feed */
		feed = *(it->second);

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedToDownload - feed " << it->second->feedId << " (" << it->second->name << ") is starting to download" << std::endl;
#endif
	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return true;
}

void p3FeedReader::onDownloadSuccess(const std::string &feedId, const std::string &content, std::string &icon)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onDownloadSuccess - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		fi->workstate = RsFeedReaderFeed::WAITING_TO_PROCESS;
		fi->content = content;

		if (fi->icon != icon) {
			fi->icon = icon;

			IndicateConfigChanged();
		}

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onDownloadSuccess - feed " << fi->feedId << " (" << fi->name << ") add to process" << std::endl;
#endif
	}

	{
		RsStackMutex stack(mProcessMutex); /******* LOCK STACK MUTEX *********/

		if (std::find(mProcessFeeds.begin(), mProcessFeeds.end(), feedId) == mProcessFeeds.end()) {
			mProcessFeeds.push_back(feedId);
		}

	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

void p3FeedReader::onDownloadError(const std::string &feedId, p3FeedReaderThread::DownloadResult result, const std::string &error)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onDownloadError - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		fi->workstate = RsFeedReaderFeed::WAITING;
		fi->lastUpdate = time(NULL);
		fi->content.clear();

		long todo; // sort error codes
		switch (result) {
		case p3FeedReaderThread::DOWNLOAD_SUCCESS:
			/* this should not happen */
			std::cerr << "p3FeedReader::onDownloadError - success given as error" << std::endl;
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR;
			break;
		case p3FeedReaderThread::DOWNLOAD_ERROR:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_ERROR;
			break;
		case p3FeedReaderThread::DOWNLOAD_ERROR_INIT:
		case p3FeedReaderThread::DOWNLOAD_INTERNAL_ERROR:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR;
			break;
		case p3FeedReaderThread::DOWNLOAD_UNKNOWN_CONTENT_TYPE:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_UNKNOWN_CONTENT_TYPE;
			break;
		case p3FeedReaderThread::DOWNLOAD_NOT_FOUND:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_NOT_FOUND;
			break;
		case p3FeedReaderThread::DOWNLOAD_UNKOWN_RESPONSE_CODE:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_UNKOWN_RESPONSE_CODE;
			break;
		default:
			fi->errorState = RS_FEED_ERRORSTATE_DOWNLOAD_INTERNAL_ERROR;
		}

		fi->errorString = error;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onDownloadError - feed " << fi->feedId << " (" << fi->name << ") error download, result = " << result << ", errorState = " << fi->errorState << ", error = " << error << std::endl;
#endif

		IndicateConfigChanged();
	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

bool p3FeedReader::getFeedToProcess(RsFeedReaderFeed &feed)
{
	std::string feedId;

	{
		RsStackMutex stack(mProcessMutex); /******* LOCK STACK MUTEX *********/

		if (mProcessFeeds.empty()) {
			/* nothing to process */
			return false;
		}

		/* get next feed id to process */
		feedId = mProcessFeeds.front();
		mProcessFeeds.pop_front();
	}

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::getFeedToProcess - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		if (it->second->workstate != RsFeedReaderFeed::WAITING_TO_PROCESS) {
			std::cerr << "p3FeedReader::getFeedToProcess - feed in wrong state for process " << it->second->workstate << std::endl;
			return false;
		}

		RsFeedReaderFeed *fi = it->second;

		/* set state to processing */
		fi->workstate = RsFeedReaderFeed::PROCESSING;
		fi->errorState = RS_FEED_ERRORSTATE_OK;
		fi->errorString.clear();

		/* return a copy of the feed */
		feed = *fi;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedToProcess - feed " << it->second->feedId << " (" << it->second->name << ") is starting to process" << std::endl;
#endif
	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return true;
}

void p3FeedReader::onProcessSuccess(const std::string &feedId, std::list<RsFeedReaderMsg*> &msgs)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReader::onProcessSuccess - feed " << feedId << " got " << msgs.size() << " messages" << std::endl;
#endif

	std::list<std::string> addedMsgs;
	std::string forumId;
	std::list<RsFeedReaderMsg> forumMsgs;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessSuccess - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		bool forum = (fi->flag & RS_FEED_FLAG_FORUM);
		uint32_t errorState = RS_FEED_ERRORSTATE_OK;

		if (forum) {
			if (fi->forumId.empty()) {
				/* create new forum */
				long todo; // search for existing forum?

				/* search for existing own forum */
//				std::list<ForumInfo> forumList;
//				if (rsForums->getForumList(forumList)) {
//					std::wstring wName = StringToWString(name);
//					for (std::list<ForumInfo>::iterator it = forumList.begin(); it != forumList.end(); ++it) {
//						if (it->forumName == wName) {
//							std::cout << "DEBUG_RSS2FORUM: Found existing forum " << it->forumId << " for " << name << std::endl;
//							return it->forumId;
//						}
//					}
//				}

				std::wstring forumName;
				librs::util::ConvertUtf8ToUtf16(fi->name, forumName);
				std::wstring forumDescription;
				librs::util::ConvertUtf8ToUtf16(fi->description, forumDescription);
				/* create anonymous public forum */
				fi->forumId = rsForums->createForum(forumName, forumDescription, RS_DISTRIB_PUBLIC | RS_DISTRIB_AUTHEN_ANON);

				forumId = fi->forumId;

				if (fi->forumId.empty()) {
					errorState = RS_FEED_ERRORSTATE_FORUM_CREATE;

#ifdef FEEDREADER_DEBUG
					std::cerr << "p3FeedReader::onProcessSuccess - can't create forum for feed " << feedId << " (" << it->second->name << ") - ignore all messages" << std::endl;
				} else {
					std::cerr << "p3FeedReader::onProcessSuccess - forum " << forumId << " (" << fi->name << ") created" << std::endl;
#endif
				}
			} else {
				/* check forum */
				ForumInfo forumInfo;
				if (rsForums->getForumInfo(fi->forumId, forumInfo)) {
					if ((forumInfo.subscribeFlags & RS_DISTRIB_ADMIN) == 0) {
						errorState = RS_FEED_ERRORSTATE_FORUM_NO_ADMIN;
					} else if ((forumInfo.forumFlags & RS_DISTRIB_AUTHEN_REQ) || (forumInfo.forumFlags & RS_DISTRIB_AUTHEN_ANON) == 0) {
						errorState = RS_FEED_ERRORSTATE_FORUM_NO_ANONYMOUS_FORUM;
					} else {
						forumId = fi->forumId;
					}
				} else {
					errorState = RS_FEED_ERRORSTATE_FORUM_NOT_FOUND;
				}
			}
		}

		/* process msgs */
#ifdef FEEDREADER_DEBUG
		uint32_t newMsgs = 0;
#endif

		if (errorState == RS_FEED_ERRORSTATE_OK) {
			std::list<RsFeedReaderMsg*>::iterator newMsgIt;
			for (newMsgIt = msgs.begin(); newMsgIt != msgs.end(); ) {
				RsFeedReaderMsg *miNew = *newMsgIt;
				/* search for exisiting msg */
				std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
				for (msgIt = fi->mMsgs.begin(); msgIt != fi->mMsgs.end(); ++msgIt) {
					RsFeedReaderMsg *mi = msgIt->second;
					if (mi->title == miNew->title && mi->link == mi->link && mi->author == mi->author) {
						/* msg exist */
						break;
					}
				}
				if (msgIt == fi->mMsgs.end()) {
					/* add new msg */
					rs_sprintf(miNew->msgId, "%lu", mNextMsgId++);
					if (forum) {
						miNew->flag = RS_FEEDMSG_FLAG_DELETED;
						forumMsgs.push_back(*miNew);
//						miNew->description.clear();
					} else {
						miNew->flag = RS_FEEDMSG_FLAG_NEW;
						addedMsgs.push_back(miNew->msgId);
					}
					fi->mMsgs[miNew->msgId] = miNew;
					newMsgIt = msgs.erase(newMsgIt);

#ifdef FEEDREADER_DEBUG
					++newMsgs;
#endif
				} else {
					/* msg was updated */
					++newMsgIt;
				}
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::onProcessSuccess - feed " << fi->feedId << " (" << fi->name << ") added " << newMsgs << "/" << msgs.size() << " messages" << std::endl;
#endif
			}
		}

		fi->workstate = RsFeedReaderFeed::WAITING;
		fi->content.clear();
		fi->lastUpdate = time(NULL);
		fi->errorState = errorState;
		fi->errorString.clear();

		IndicateConfigChanged();
	}

	if (!forumId.empty() && !forumMsgs.empty()) {
		/* add messages as forum messages */
		std::list<RsFeedReaderMsg>::iterator msgIt;
		for (msgIt = forumMsgs.begin(); msgIt != forumMsgs.end(); ++msgIt) {
			RsFeedReaderMsg &mi = *msgIt;

			/* convert to forum messages */
			ForumMsgInfo forumMsgInfo;
			forumMsgInfo.forumId = forumId;
			librs::util::ConvertUtf8ToUtf16(mi.title, forumMsgInfo.title);

			std::string description = mi.description;
			/* add link */
			if (!mi.link.empty()) {
				description += "<br><a href=\"" + mi.link + "\">" + mi.link + "</a>";
			}
			librs::util::ConvertUtf8ToUtf16(description, forumMsgInfo.msg);

			if (rsForums->ForumMessageSend(forumMsgInfo)) {
				/* set to new */
				rsForums->setMessageStatus(forumMsgInfo.forumId, forumMsgInfo.msgId, 0, FORUM_MSG_STATUS_MASK);
			} else {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::onProcessSuccess - can't add forum message " << mi.title << " for feed " << forumId << std::endl;
#endif
			}
		}
	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);

		std::list<std::string>::iterator it;
		for (it = addedMsgs.begin(); it != addedMsgs.end(); ++it) {
			mNotify->msgChanged(feedId, *it, NOTIFY_TYPE_ADD);
		}
	}
}

void p3FeedReader::onProcessError(const std::string &feedId, p3FeedReaderThread::ProcessResult result)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessError - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		fi->workstate = RsFeedReaderFeed::WAITING;
		fi->lastUpdate = time(NULL);
		fi->content.clear();

		long todo; // sort error codes
		switch (result) {
		case p3FeedReaderThread::PROCESS_SUCCESS:
			/* this should not happen */
			std::cerr << "p3FeedReader::onProcessError - success given as error" << std::endl;
			fi->errorState = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
			break;
		case p3FeedReaderThread::PROCESS_ERROR_INIT:
			fi->errorState = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
			break;
		default:
			fi->errorState = RS_FEED_ERRORSTATE_PROCESS_INTERNAL_ERROR;
		}

//		fi->errorString = error;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onProcessError - feed " << fi->feedId << " (" << fi->name << ") error process, result = " << result << ", errorState = " << fi->errorState << std::endl;
#endif

		IndicateConfigChanged();
	}

	if (mNotify) {
		mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

void p3FeedReader::setFeedInfo(const std::string &feedId, const std::string &name, const std::string &description)
{
	bool changed = false;
	std::string forumId;
	ForumInfo forumInfo;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<std::string, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeedInfo - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		if (fi->name != name) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeedInfo - feed " << fi->feedId << " changed name from " << fi->name << " to " << name << std::endl;
#endif
			fi->name = name;
			changed = true;
		}
		if (fi->description != description) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeedInfo - feed " << fi->feedId << " changed description from " << fi->description << " to " << description << std::endl;
#endif
			fi->description = description;
			changed = true;
		}

		if (changed && (fi->flag & RS_FEED_FLAG_FORUM) && (fi->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO) && !fi->forumId.empty()) {
			/* change forum too */
			forumId = fi->forumId;
			librs::util::ConvertUtf8ToUtf16(fi->name, forumInfo.forumName);
			librs::util::ConvertUtf8ToUtf16(fi->description, forumInfo.forumDesc);
		}
	}

	if (changed) {
		IndicateConfigChanged();

		if (mNotify) {
			mNotify->feedChanged(feedId, NOTIFY_TYPE_MOD);
		}
	}

	if (!forumId.empty()) {
		/* name or description changed, update forum */
		if (!rsForums->setForumInfo(forumId, forumInfo)) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - can't change forum " << forumId << std::endl;
#endif
		}
	}
}
