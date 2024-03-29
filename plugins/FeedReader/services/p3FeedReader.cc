/*******************************************************************************
 * plugins/FeedReader/services/p3FeedReader.cc                                 *
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

#include "rsFeedReaderItems.h"
#include "p3FeedReader.h"
#include "p3FeedReaderThread.h"
#include "rsitems/rsconfigitems.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsgxsforums.h"
#include "retroshare/rsposted.h"
#include "util/rsstring.h"
#include "util/rstime.h"
#include "gxs/rsgenexchange.h"

#include <unistd.h>

RsFeedReader *rsFeedReader = NULL;

#define FEEDREADER_CLEAN_INTERVAL 1 * 60 * 60 // check every hour
#define FEEDREADER_FORUM_PREFIX  "RSS: "
#define FEEDREADER_POSTED_PREFIX "RSS: "

#define MAX_REQUEST_AGE 30 // 30 seconds

/*********
 * #define FEEDREADER_DEBUG
 *********/

p3FeedReader::p3FeedReader(RsPluginHandler* pgHandler, RsGxsForums *forums, RsPosted *posted)
	: RsPQIService(RS_SERVICE_TYPE_PLUGIN_FEEDREADER,  5, pgHandler),
	  mFeedReaderMtx("p3FeedReader"), mDownloadMutex("p3FeedReaderDownload"), mProcessMutex("p3FeedReaderProcess"), mImageMutex("p3FeedReaderImage"), mPreviewMutex("p3FeedReaderPreview")
{
	mNextFeedId = 1;
	mNextMsgId = 1;
	mNextPreviewFeedId = -1; // use negative values
	mNextPreviewMsgId = -1; // use negative values
	mStandardUpdateInterval = 60 * 60; // 60 minutes
	mStandardStorageTime = 30 * 60 * 60 * 24; // 30 days
	mStandardUseProxy = false;
	mStandardProxyPort = 0;
	mLastClean = 0;
	mForums = forums;
	mPosted = posted;
	mNotify = NULL;
	mSaveInBackground = false;
	mStopped = false;

	mPreviewDownloadThread = NULL;
	mPreviewProcessThread = NULL;

	/* start download thread */
	p3FeedReaderThread *frt = new p3FeedReaderThread(this, p3FeedReaderThread::DOWNLOAD, 0);
	mThreads.push_back(frt);
	frt->start("fr download");

	/* start process thread */
	frt = new p3FeedReaderThread(this, p3FeedReaderThread::PROCESS, 0);
	mThreads.push_back(frt);
	frt->start("fr process");
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
	info.postedId = feed->postedId;
	info.storageTime = feed->storageTime;
	info.errorState = feed->errorState;
	info.errorString = feed->errorString;

	info.transformationType = feed->transformationType;
	info.xpathsToUse = feed->xpathsToUse.ids;
	info.xpathsToRemove = feed->xpathsToRemove.ids;
	info.xslt = feed->xslt;

	info.flag.folder = (feed->flag & RS_FEED_FLAG_FOLDER);
	info.flag.infoFromFeed = (feed->flag & RS_FEED_FLAG_INFO_FROM_FEED);
	info.flag.standardStorageTime = (feed->flag & RS_FEED_FLAG_STANDARD_STORAGE_TIME);
	info.flag.standardUpdateInterval = (feed->flag & RS_FEED_FLAG_STANDARD_UPDATE_INTERVAL);
	info.flag.standardProxy = (feed->flag & RS_FEED_FLAG_STANDARD_PROXY);
	info.flag.authentication = (feed->flag & RS_FEED_FLAG_AUTHENTICATION);
	info.flag.deactivated = (feed->flag & RS_FEED_FLAG_DEACTIVATED);
	info.flag.forum = (feed->flag & RS_FEED_FLAG_FORUM);
	info.flag.updateForumInfo = (feed->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO);
	info.flag.posted = (feed->flag & RS_FEED_FLAG_POSTED);
	info.flag.updatePostedInfo = (feed->flag & RS_FEED_FLAG_UPDATE_POSTED_INFO);
	info.flag.postedFirstImage = (feed->flag & RS_FEED_FLAG_POSTED_FIRST_IMAGE);
	info.flag.postedOnlyImage = (feed->flag & RS_FEED_FLAG_POSTED_ONLY_IMAGE);
	info.flag.postedShrinkImage = (feed->flag & RS_FEED_FLAG_POSTED_SHRINK_IMAGE);
	info.flag.embedImages = (feed->flag & RS_FEED_FLAG_EMBED_IMAGES);
	info.flag.saveCompletePage = (feed->flag & RS_FEED_FLAG_SAVE_COMPLETE_PAGE);

	info.flag.preview = feed->preview;

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

static void infoToFeed(const FeedInfo &info, RsFeedReaderFeed *feed)
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

	feed->forumId = info.forumId;
	feed->postedId = info.postedId;

	feed->transformationType = info.transformationType;
	feed->xpathsToUse.ids = info.xpathsToUse;
	feed->xpathsToRemove.ids = info.xpathsToRemove;
	feed->xslt = info.xslt;

//	feed->preview = info.flag.preview;

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
	if (info.flag.embedImages) {
		feed->flag |= RS_FEED_FLAG_EMBED_IMAGES;
	}
	if (info.flag.saveCompletePage) {
		feed->flag |= RS_FEED_FLAG_SAVE_COMPLETE_PAGE;
	}
	if (info.flag.folder) {
		feed->flag |= RS_FEED_FLAG_FOLDER;
	}
	if (info.flag.forum) {
		feed->flag |= RS_FEED_FLAG_FORUM;
	}
	if (info.flag.updateForumInfo) {
		feed->flag |= RS_FEED_FLAG_UPDATE_FORUM_INFO;
	}
	if (info.flag.posted) {
		feed->flag |= RS_FEED_FLAG_POSTED;
	}
	if (info.flag.updatePostedInfo) {
		feed->flag |= RS_FEED_FLAG_UPDATE_POSTED_INFO;
	}
	if (info.flag.updatePostedInfo) {
		feed->flag |= RS_FEED_FLAG_UPDATE_POSTED_INFO;
	}
	if (info.flag.postedFirstImage) {
		feed->flag |= RS_FEED_FLAG_POSTED_FIRST_IMAGE;
	}
	if (info.flag.postedOnlyImage) {
		feed->flag |= RS_FEED_FLAG_POSTED_ONLY_IMAGE;
	}
	if (info.flag.postedShrinkImage) {
		feed->flag |= RS_FEED_FLAG_POSTED_SHRINK_IMAGE;
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
	info.descriptionTransformed = msg->descriptionTransformed;
	info.pubDate = msg->pubDate;
	info.attachmentLink = msg->attachmentLink;
	if (!msg->attachment.empty()) {
		p3FeedReaderThread::fromBase64(msg->attachment, info.attachment);
	} else {
		info.attachment.clear();
	}
	info.attachmentMimeType = msg->attachmentMimeType;

	info.flag.isnew = (msg->flag & RS_FEEDMSG_FLAG_NEW);
	info.flag.read = (msg->flag & RS_FEEDMSG_FLAG_READ);
	info.flag.deleted = (msg->flag & RS_FEEDMSG_FLAG_DELETED);
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

RsServiceInfo p3FeedReader::getServiceInfo()
{
    const std::string FEEDREADER_APP_NAME = "FEEDREADER";
    const uint16_t FEEDREADER_APP_MAJOR_VERSION  =       1;
    const uint16_t FEEDREADER_APP_MINOR_VERSION  =       0;
    const uint16_t FEEDREADER_MIN_MAJOR_VERSION  =       1;
    const uint16_t FEEDREADER_MIN_MINOR_VERSION  =       0;

    return RsServiceInfo(RS_SERVICE_TYPE_PLUGIN_FEEDREADER,
                         FEEDREADER_APP_NAME,
                         FEEDREADER_APP_MAJOR_VERSION,
                         FEEDREADER_APP_MINOR_VERSION,
                         FEEDREADER_MIN_MAJOR_VERSION,
                         FEEDREADER_MIN_MINOR_VERSION);
}

void p3FeedReader::setStandardStorageTime(uint32_t storageTime)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	if (mStandardStorageTime != storageTime) {
		mStandardStorageTime = storageTime;
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
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
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
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
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
	}
}

bool p3FeedReader::getSaveInBackground()
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	return mSaveInBackground;
}

void p3FeedReader::setSaveInBackground(bool saveInBackground)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	if (saveInBackground != mSaveInBackground) {
		mSaveInBackground = saveInBackground;
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
	}
}

void p3FeedReader::stop()
{
	mStopped = true;

	{
		RsStackMutex stack(mPreviewMutex); /******* LOCK STACK MUTEX *********/

		stopPreviewThreads_locked();
	}

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* stop threads */
		std::list<p3FeedReaderThread*>::iterator it;
		for (it = mThreads.begin(); it != mThreads.end(); ++it) {
			(*it)->fullstop();
			delete(*it);
		}
		mThreads.clear();
	}
}

void p3FeedReader::stopPreviewThreads_locked()
{
	if (mPreviewDownloadThread) {
		mPreviewDownloadThread->fullstop();
		delete mPreviewDownloadThread;
		mPreviewDownloadThread = NULL;
	}
	if (mPreviewProcessThread) {
		mPreviewProcessThread->fullstop();
		delete mPreviewProcessThread;
		mPreviewProcessThread = NULL;
	}
}

RsFeedResult p3FeedReader::addFolder(uint32_t parentId, const std::string &name, uint32_t &feedId)
{
	feedId = 0;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		if (parentId) {
			/* check parent id */
			std::map<uint32_t, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFolder - parent id " << parentId << " not found" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFolder - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = new RsFeedReaderFeed;
		fi->feedId = mNextFeedId++;
		fi->parentId = parentId;
		fi->name = name;
		fi->flag = RS_FEED_FLAG_FOLDER;
		mFeeds[fi->feedId] = fi;

		feedId = fi->feedId;
	}

	IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_ADD);
	}

	return RS_FEED_RESULT_SUCCESS;
}

RsFeedResult p3FeedReader::setFolder(uint32_t feedId, const std::string &name)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::setFolder - feed id " << feedId << ", name " << name << std::endl;
#endif

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFolder - feed id " << feedId << " not found" << std::endl;
#endif
			return RS_FEED_RESULT_FEED_NOT_FOUND;
		}

		if ((feedIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFolder - feed " << feedIt->second->name << " is no folder" << std::endl;
#endif
			return RS_FEED_RESULT_FEED_IS_NO_FOLDER;
		}

		RsFeedReaderFeed *fi = feedIt->second;
		if (fi->name == name) {
			return RS_FEED_RESULT_SUCCESS;
		}
		fi->name = name;
	}

	IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return RS_FEED_RESULT_SUCCESS;
}

RsFeedResult p3FeedReader::addFeed(const FeedInfo &feedInfo, uint32_t &feedId)
{
	feedId = 0;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::addFeed - add feed " << feedInfo.name << ", url " << feedInfo.url << std::endl;
#endif

		if (feedInfo.parentId) {
			/* check parent id */
			std::map<uint32_t, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(feedInfo.parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFeed - parent id " << feedInfo.parentId << " not found" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::addFeed - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = new RsFeedReaderFeed;
		infoToFeed(feedInfo, fi);
		fi->feedId = mNextFeedId++;

		mFeeds[fi->feedId] = fi;

		feedId = fi->feedId;
	}

	IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_ADD);
	}

	return RS_FEED_RESULT_SUCCESS;
}

RsFeedResult p3FeedReader::setFeed(uint32_t feedId, const FeedInfo &feedInfo)
{
	std::string forumId;
	std::string forumName;
	std::string forumDescription;
	std::string postedId;
	std::string postedName;
	std::string postedDescription;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::setFeed - set feed " << feedInfo.name << ", url " << feedInfo.url << std::endl;
#endif

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - feed id " << feedId << " not found" << std::endl;
#endif
			return RS_FEED_RESULT_FEED_NOT_FOUND;
		}

		if (feedIt->second->flag & RS_FEED_FLAG_FOLDER) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeed - feed " << feedIt->second->name << " is a folder" << std::endl;
#endif
			return RS_FEED_RESULT_FEED_IS_FOLDER;
		}

		if (feedInfo.parentId) {
			/* check parent id */
			std::map<uint32_t, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(feedInfo.parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setFeed - parent id " << feedInfo.parentId << " not found" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setFeed - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = feedIt->second;
		std::string oldForumId = fi->forumId;
		std::string oldPostedId = fi->postedId;
		std::string oldName = fi->name;
		std::string oldDescription = fi->description;

		infoToFeed(feedInfo, fi);

		if ((fi->flag & RS_FEED_FLAG_FORUM) && (fi->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO) && !fi->forumId.empty() &&
		    (fi->forumId != oldForumId || fi->name != oldName || fi->description != oldDescription)) {
			/* name or description changed, update forum */
			forumId = fi->forumId;
			forumName = fi->name;
			forumDescription = fi->description;
			forumName.insert(0, FEEDREADER_FORUM_PREFIX);
		}

		if ((fi->flag & RS_FEED_FLAG_POSTED) && (fi->flag & RS_FEED_FLAG_UPDATE_POSTED_INFO) && !fi->postedId.empty() &&
		    (fi->postedId != oldPostedId || fi->name != oldName || fi->description != oldDescription)) {
			/* name or description changed, update posted */
			postedId = fi->postedId;
			postedName = fi->name;
			postedDescription = fi->description;
			postedName.insert(0, FEEDREADER_POSTED_PREFIX);
		}
	}

	IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	if (!forumId.empty()) {
		RsGxsForumGroup forumGroup;
		if (getForumGroup(RsGxsGroupId(forumId), forumGroup)) {
			updateForumGroup(forumGroup, forumName, forumDescription);
		}
		//TODO: error
	}

	if (!postedId.empty()) {
		RsPostedGroup postedGroup;
		if (getPostedGroup(RsGxsGroupId(postedId), postedGroup)) {
			updatePostedGroup(postedGroup, postedName, postedDescription);
		}
		//TODO: error
	}

	return RS_FEED_RESULT_SUCCESS;
}

RsFeedResult p3FeedReader::setParent(uint32_t feedId, uint32_t parentId)
{
	bool changed = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::setParent - set parent " << parentId << std::endl;
#endif

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setParent - feed id " << feedId << " not found" << std::endl;
#endif
			return RS_FEED_RESULT_FEED_NOT_FOUND;
		}

		if (parentId) {
			/* check parent id */
			std::map<uint32_t, RsFeedReaderFeed*>::iterator parentIt = mFeeds.find(parentId);
			if (parentIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setParent - parent id " << parentId << " not found" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_NOT_FOUND;
			}

			if ((parentIt->second->flag & RS_FEED_FLAG_FOLDER) == 0) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::setParent - parent " << parentIt->second->name << " is no folder" << std::endl;
#endif
				return RS_FEED_RESULT_PARENT_IS_NO_FOLDER;
			}
		}

		RsFeedReaderFeed *fi = feedIt->second;

		if (fi->parentId != parentId) {
			fi->parentId = parentId;
			changed = true;
		}
	}

	if (changed) {
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

		if (mNotify) {
			mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
		}
	}

	return RS_FEED_RESULT_SUCCESS;
}

void p3FeedReader::deleteAllMsgs_locked(RsFeedReaderFeed *fi)
{
	if (!fi) {
		return;
	}

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
		delete(msgIt->second);
	}

	fi->msgs.clear();
}

bool p3FeedReader::removeFeed(uint32_t feedId)
{
	std::list<uint32_t> removedFeedIds;
	bool changed = false;
	bool preview = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeFeed - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		removedFeedIds.push_back(feedId);

		RsFeedReaderFeed *fi = feedIt->second;
		mFeeds.erase(feedIt);
		changed = !fi->preview;
		preview = fi->preview;

		if (fi->flag & RS_FEED_FLAG_FOLDER) {
			std::list<uint32_t> feedIds;
			feedIds.push_back(fi->feedId);
			while (!feedIds.empty()) {
				uint32_t parentId = feedIds.front();
				feedIds.pop_front();

				std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt1;
				for (feedIt1 = mFeeds.begin(); feedIt1 != mFeeds.end(); ) {
					RsFeedReaderFeed *fi1 = feedIt1->second;

					if (fi1->parentId == parentId) {
						removedFeedIds.push_back(fi1->feedId);

						std::map<uint32_t, RsFeedReaderFeed*>::iterator tempIt = feedIt1;
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

	if (changed) {
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
	}

	if (preview) {
		RsStackMutex stack(mPreviewMutex); /******* LOCK STACK MUTEX *********/

		/* only check download thread */
		if (mPreviewDownloadThread && mPreviewDownloadThread->getFeedId() == feedId) {
			stopPreviewThreads_locked();
		}
	}

	if (mNotify) {
		/* only notify remove of feed */
		std::list<uint32_t>::iterator it;
		for (it = removedFeedIds.begin(); it != removedFeedIds.end(); ++it) {
			mNotify->notifyFeedChanged(*it, NOTIFY_TYPE_DEL);
		}
	}

	return true;
}

bool p3FeedReader::addPreviewFeed(const FeedInfo &feedInfo, uint32_t &feedId)
{
	{
		RsStackMutex stack(mPreviewMutex); /******* LOCK STACK MUTEX *********/

		stopPreviewThreads_locked();
	}

	feedId = 0;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::addPreviewFeed - add feed " << feedInfo.name << ", url " << feedInfo.url << std::endl;
#endif

		RsFeedReaderFeed *fi = new RsFeedReaderFeed;
		infoToFeed(feedInfo, fi);
		fi->feedId = mNextPreviewFeedId--;
		fi->preview = true;

		/* process feed */
		fi->workstate = RsFeedReaderFeed::WAITING_TO_DOWNLOAD;
		fi->content.clear();

		/* clear not needed members */
		fi->parentId = 0;
		fi->updateInterval = 0;
		fi->lastUpdate = 0;
		fi->forumId.clear();
		fi->postedId.clear();
		fi->storageTime = 0;

		mFeeds[fi->feedId] = fi;

		feedId = fi->feedId;
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_ADD);
	}

	{
		RsStackMutex stack(mPreviewMutex); /******* LOCK STACK MUTEX *********/

		/* start download thread for preview */
		mPreviewDownloadThread = new p3FeedReaderThread(this, p3FeedReaderThread::DOWNLOAD, feedId);
		mPreviewDownloadThread->start("fr preview dl");

		/* start process thread for preview */
		mPreviewProcessThread = new p3FeedReaderThread(this, p3FeedReaderThread::PROCESS, feedId);
		mPreviewProcessThread->start("fr preview proc");
	}

	return true;
}

void p3FeedReader::getFeedList(uint32_t parentId, std::list<FeedInfo> &feedInfos)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;
	for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
		RsFeedReaderFeed *fi = feedIt->second;

		if (fi->preview) {
			continue;
		}

		if (fi->parentId == parentId) {
			FeedInfo feedInfo;
			feedToInfo(fi, feedInfo);
			feedInfos.push_back(feedInfo);
		}
	}
}

bool p3FeedReader::getFeedInfo(uint32_t feedId, FeedInfo &feedInfo)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedInfo - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	feedToInfo(feedIt->second, feedInfo);

	return true;
}

bool p3FeedReader::getMsgInfo(uint32_t feedId, const std::string &msgId, FeedMsgInfo &msgInfo)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getMsgInfo - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	msgIt = fi->msgs.find(msgId);
	if (msgIt == fi->msgs.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getMsgInfo - msg " << msgId << " not found" << std::endl;
#endif
		return false;
	}

	feedMsgToInfo(msgIt->second, msgInfo);

	return true;
}

bool p3FeedReader::removeMsg(uint32_t feedId, const std::string &msgId)
{
	bool changed = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsg - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;
		changed = !fi->preview;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		msgIt = fi->msgs.find(msgId);
		if (msgIt == fi->msgs.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsg - msg " << msgId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderMsg *mi = msgIt->second;
		mi->flag |= RS_FEEDMSG_FLAG_DELETED | RS_FEEDMSG_FLAG_READ;
		mi->flag &= ~RS_FEEDMSG_FLAG_NEW;
		mi->description.clear();
		mi->descriptionTransformed.clear();
	}

	if (changed) {
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
		mNotify->notifyMsgChanged(feedId, msgId, NOTIFY_TYPE_DEL);
	}

	return true;
}

bool p3FeedReader::removeMsgs(uint32_t feedId, const std::list<std::string> &msgIds)
{
	std::list<std::string> removedMsgs;
	bool changed = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::removeMsgs - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;
		changed = !fi->preview;

		std::list<std::string>::const_iterator idIt;
		for (idIt = msgIds.begin(); idIt != msgIds.end(); ++idIt) {
			std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
			msgIt = fi->msgs.find(*idIt);
			if (msgIt == fi->msgs.end()) {
#ifdef FEEDREADER_DEBUG
				std::cerr << "p3FeedReader::removeMsgs - msg " << *idIt << " not found" << std::endl;
#endif
				continue;
			}

			RsFeedReaderMsg *mi = msgIt->second;
			mi->flag |= RS_FEEDMSG_FLAG_DELETED | RS_FEEDMSG_FLAG_READ;
			mi->flag &= ~RS_FEEDMSG_FLAG_NEW;
			mi->description.clear();
			mi->descriptionTransformed.clear();

			removedMsgs.push_back(*idIt);
		}
	}

	if (changed) {
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
	}

	if (mNotify && !removedMsgs.empty()) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);

		std::list<std::string>::iterator it;
		for (it = removedMsgs.begin(); it != removedMsgs.end(); ++it) {
			mNotify->notifyMsgChanged(feedId, *it, NOTIFY_TYPE_DEL);
		}
	}

	return true;
}

bool p3FeedReader::getMessageCount(uint32_t feedId, uint32_t *msgCount, uint32_t *newCount, uint32_t *unreadCount)
{
	if (msgCount) *msgCount = 0;
	if (unreadCount) *unreadCount = 0;
	if (newCount) *newCount = 0;

	if (!msgCount && !unreadCount && !newCount) {
		return true;
	}

	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	if (feedId == 0) {
		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;
		for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
			RsFeedReaderFeed *fi = feedIt->second;

			std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
			for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
				RsFeedReaderMsg *mi = msgIt->second;

				if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
					continue;
				}

				if (msgCount) ++(*msgCount);
				if (newCount && (mi->flag & RS_FEEDMSG_FLAG_NEW)) ++(*newCount);
				if (unreadCount && (mi->flag & RS_FEEDMSG_FLAG_READ) == 0) ++(*unreadCount);
			}
		}
	} else {
		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::getMessageCount - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
			RsFeedReaderMsg *mi = msgIt->second;

			if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
				continue;
			}

			if (msgCount) ++(*msgCount);
			if (newCount && (mi->flag & RS_FEEDMSG_FLAG_NEW)) ++(*newCount);
			if (unreadCount && (mi->flag & RS_FEEDMSG_FLAG_READ) == 0) ++(*unreadCount);
		}
	}

	return true;
}

bool p3FeedReader::getFeedMsgList(uint32_t feedId, std::list<FeedMsgInfo> &msgInfos)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedMsgList - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
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

bool p3FeedReader::getFeedMsgIdList(uint32_t feedId, std::list<std::string> &msgIds)
{
	RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
	if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedMsgList - feed " << feedId << " not found" << std::endl;
#endif
		return false;
	}

	RsFeedReaderFeed *fi = feedIt->second;

	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
		RsFeedReaderMsg *mi = msgIt->second;

		if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
			continue;
		}

		msgIds.push_back(mi->msgId);
	}

	return true;
}

static bool canProcessFeed(RsFeedReaderFeed *fi)
{
	if (fi->preview) {
		/* preview feed */
		return false;
	}

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

bool p3FeedReader::processFeed(uint32_t feedId)
{
	std::list<uint32_t> feedToDownload;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;

		if (feedId == 0) {
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
				std::list<uint32_t> feedIds;
				feedIds.push_back(fi->feedId);
				while (!feedIds.empty()) {
					uint32_t parentId = feedIds.front();
					feedIds.pop_front();

					std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt1;
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

	std::list<uint32_t> notifyIds;
	std::list<uint32_t>::iterator it;

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
			mNotify->notifyFeedChanged(*it, NOTIFY_TYPE_MOD);
		}
	}

	return true;
}

bool p3FeedReader::setMessageRead(uint32_t feedId, const std::string &msgId, bool read)
{
	bool changed = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setMessageRead - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		msgIt = fi->msgs.find(msgId);
		if (msgIt == fi->msgs.end()) {
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
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_OFTEN);
		if (mNotify) {
			mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
			mNotify->notifyMsgChanged(feedId, msgId, NOTIFY_TYPE_MOD);
		}
	}

	return true;
}

bool p3FeedReader::retransformMsg(uint32_t feedId, const std::string &msgId)
{
	bool msgChanged = false;
	bool feedChanged = false;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setMessageRead - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		msgIt = fi->msgs.find(msgId);
		if (msgIt == fi->msgs.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setMessageRead - msg " << msgId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderMsg *mi = msgIt->second;

		std::string errorString;
		std::string descriptionTransformed = mi->descriptionTransformed;
		if (p3FeedReaderThread::processTransformation(*fi, mi, errorString) == RS_FEED_ERRORSTATE_OK) {
			if (mi->descriptionTransformed != descriptionTransformed) {
				msgChanged = true;
			}
		} else {
			if (!errorString.empty()) {
				fi->errorString = errorString;
				feedChanged = true;
			}
		}
	}

	if (feedChanged || msgChanged) {
		IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		if (mNotify) {
			if (feedChanged) {
				mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
			}
			if (msgChanged) {
				mNotify->notifyMsgChanged(feedId, msgId, NOTIFY_TYPE_MOD);
			}
		}
	}

	return true;
}

bool p3FeedReader::clearMessageCache(uint32_t feedId)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::clearMessageCache - feed id " << feedId << std::endl;
#endif

		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt = mFeeds.find(feedId);
		if (feedIt == mFeeds.end()) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::clearMessageCache - feed id " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		if (feedIt->second->flag & RS_FEED_FLAG_FOLDER) {
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::clearMessageCache - feed " << feedIt->second->name << " is a folder" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = feedIt->second;

		std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
		for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ) {
			RsFeedReaderMsg *mi = msgIt->second;

			if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
				delete(mi);
				std::map<std::string, RsFeedReaderMsg*>::iterator deleteIt = msgIt++;
				fi->msgs.erase(deleteIt);
				continue;
			}
			++msgIt;
		}
	}

	IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

	return true;
}

RsFeedReaderErrorState p3FeedReader::processXPath(const std::list<std::string> &xpathsToUse, const std::list<std::string> &xpathsToRemove, std::string &description, std::string &errorString)
{
	return p3FeedReaderThread::processXPath(xpathsToUse, xpathsToRemove, description, errorString);
}

RsFeedReaderErrorState p3FeedReader::processXslt(const std::string &xslt, std::string &description, std::string &errorString)
{
	return p3FeedReaderThread::processXslt(xslt, description, errorString);
}

/***************************************************************************/
/****************************** p3Service **********************************/
/***************************************************************************/

int p3FeedReader::tick()
{
	/* clean feeds */
	cleanFeeds();

	/* check feeds for update interval */
	time_t currentTime = time(NULL);
	std::list<uint32_t> feedToDownload;
	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;

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

			if (updateInterval == 0) {
				continue;
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

	std::list<uint32_t> notifyIds;
	std::list<uint32_t>::iterator it;

	if (!feedToDownload.empty()) {
		RsStackMutex stack(mDownloadMutex); /******* LOCK STACK MUTEX *********/

		for (it = feedToDownload.begin(); it != feedToDownload.end(); ++it) {
			if (std::find(mDownloadFeeds.begin(), mDownloadFeeds.end(), *it) == mDownloadFeeds.end()) {
				mDownloadFeeds.push_back(*it);
				notifyIds.push_back(*it);
			}
		}
	}

	// check images
	bool imageToOptimze = false;
	{
		RsStackMutex stack(mImageMutex); /******* LOCK STACK MUTEX *********/

		imageToOptimze = !mImages.empty();
	}

	if (mNotify) {
		for (it = notifyIds.begin(); it != notifyIds.end(); ++it) {
			mNotify->notifyFeedChanged(*it, NOTIFY_TYPE_MOD);
		}
		if (imageToOptimze) {
			mNotify->notifyOptimizeImage();
		}
	}

	return 0;
}

void p3FeedReader::cleanFeeds()
{
	time_t currentTime = time(NULL);

	if (mLastClean == 0 || mLastClean + FEEDREADER_CLEAN_INTERVAL <= currentTime) {
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		std::list<std::pair<uint32_t, std::string> > removedMsgIds;
		std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;
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
				for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ) {
					RsFeedReaderMsg *mi = msgIt->second;

					if (mi->flag & RS_FEEDMSG_FLAG_DELETED) {
						if (mi->pubDate < currentTime - (long) storageTime) {
							removedMsgIds.push_back(std::pair<uint32_t, std::string> (fi->feedId, mi->msgId));
							delete(mi);
							std::map<std::string, RsFeedReaderMsg*>::iterator deleteIt = msgIt++;
							fi->msgs.erase(deleteIt);
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
			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);

			if (mNotify) {
				std::list<std::pair<uint32_t, std::string> >::iterator it;
				for (it = removedMsgIds.begin(); it != removedMsgIds.end(); ++it) {
					mNotify->notifyMsgChanged(it->first, it->second, NOTIFY_TYPE_DEL);
				}
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

bool p3FeedReader::saveList(bool &cleanup, std::list<RsItem *> &saveData)
{
	mFeedReaderMtx.lock(); /*********************** LOCK *******/

	if (mSaveInBackground) {
		cleanup = true;
	} else {
		cleanup = false;
	}

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

	kv.key = "SaveInBackground";
	rs_sprintf(kv.value, "%hu", mSaveInBackground ? 1 : 0);
	rskv->tlvkvs.pairs.push_back(kv);

	/* Add KeyValue to saveList */
	saveData.push_back(rskv);
	if (!cleanup) {
		cleanSaveData.push_back(rskv);
	}

	std::map<uint32_t, RsFeedReaderFeed *>::iterator it1;
	for (it1 = mFeeds.begin(); it1 != mFeeds.end(); ++it1) {
		RsFeedReaderFeed *fi = it1->second;
		if (fi->preview) {
			continue;
		}
		if (cleanup) {
			saveData.push_back(new RsFeedReaderFeed(*fi));
		} else {
			saveData.push_back(fi);
		}

		std::map<std::string, RsFeedReaderMsg*>::iterator it2;
		for (it2 = fi->msgs.begin(); it2 != fi->msgs.end(); ++it2) {
			RsFeedReaderMsg *msg = it2->second;

			if (cleanup) {
				saveData.push_back(new RsFeedReaderMsg(*msg));
			} else {
				saveData.push_back(msg);
			}
		}
	}

	if (mSaveInBackground) {
		mFeedReaderMtx.unlock(); /*********************** UNLOCK *******/
	}

	/* list completed! */
	return true;
}

void p3FeedReader::saveDone()
{
	/* clean settings items */
	std::list<RsItem*>::iterator it;
	for (it = cleanSaveData.begin(); it != cleanSaveData.end(); ++it) {
		delete(*it);
	}
	cleanSaveData.clear();

	if (!mSaveInBackground) {
		mFeedReaderMtx.unlock(); /*********************** UNLOCK *******/
	}
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
			RsStackMutex stack(mFeedReaderMtx); /********** STACK LOCKED MTX ******/
			if (mFeeds.find(fi->feedId) != mFeeds.end()) {
				/* feed with the same id exists */
				delete mFeeds[fi->feedId];
			}
			mFeeds[fi->feedId] = fi;

			if (fi->feedId + 1 > mNextFeedId) {
				mNextFeedId = fi->feedId + 1;
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
				} else if (kit->key == "SaveInBackground") {
					uint16_t value;
					if (sscanf(kit->value.c_str(), "%hu", &value) == 1) {
						mSaveInBackground = value == 1 ? true : false;
					}
				}
			}
		} else {
			/* cleanup */
			delete(*it);
		}
	}

	RsStackMutex stack(mFeedReaderMtx); /********** STACK LOCKED MTX ******/

	/* check feeds */
	std::map<uint32_t, RsFeedReaderFeed*>::iterator feedIt;
	for (feedIt = mFeeds.begin(); feedIt != mFeeds.end(); ++feedIt) {
		RsFeedReaderFeed *feed = feedIt->second;
		if (feed->parentId) {
			/* check parent */
			if (mFeeds.find(feed->parentId) == mFeeds.end()) {
				/* parent not found, clear it */
				feed->parentId = 0;
			}
		}
	}

	/* now sort msgs into feeds */
	std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
	for (msgIt = msgs.begin(); msgIt != msgs.end(); ++msgIt) {
		uint32_t msgId = 0;
		if (sscanf(msgIt->first.c_str(), "%u", &msgId) == 1) {
			feedIt = mFeeds.find(msgIt->second->feedId);
			if (feedIt == mFeeds.end()) {
				/* feed does not exist exists */
				delete msgIt->second;
				continue;
			}
			feedIt->second->msgs[msgIt->first] = msgIt->second;
			if (msgId + 1 > mNextMsgId) {
				mNextMsgId = msgId + 1;
			}
		} else {
			/* invalid msg id */
			delete(msgIt->second);
		}
	}

	return true;
}

/***************************************************************************/
/****************************** internal ***********************************/
/***************************************************************************/

bool p3FeedReader::getFeedToDownload(RsFeedReaderFeed &feed, uint32_t neededFeedId)
{
	uint32_t feedId = neededFeedId;

	if (feedId == 0) {
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
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::getFeedToDownload - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		/* check state */
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
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return true;
}

void p3FeedReader::onDownloadSuccess(uint32_t feedId, const std::string &content, std::string &icon)
{
	bool preview;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
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
		preview = fi->preview;

		if (fi->icon != icon) {
			fi->icon = icon;

			if (!preview) {
				IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
			}
		}

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onDownloadSuccess - feed " << fi->feedId << " (" << fi->name << ") add to process" << std::endl;
#endif
	}

	if (!preview) {
		RsStackMutex stack(mProcessMutex); /******* LOCK STACK MUTEX *********/

		if (std::find(mProcessFeeds.begin(), mProcessFeeds.end(), feedId) == mProcessFeeds.end()) {
			mProcessFeeds.push_back(feedId);
		}

	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

void p3FeedReader::onDownloadError(uint32_t feedId, RsFeedReaderErrorState result, const std::string &errorString)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
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

		fi->errorState = result;
		fi->errorString = errorString;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onDownloadError - feed " << fi->feedId << " (" << fi->name << ") error download, result = " << result << ", errorState = " << fi->errorState << ", error = " << errorString << std::endl;
#endif

		if (!fi->preview) {
			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		}
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

bool p3FeedReader::getFeedToProcess(RsFeedReaderFeed &feed, uint32_t neededFeedId)
{
	uint32_t feedId = neededFeedId;

	if (feedId == 0) {
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
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::getFeedToProcess - feed " << feedId << " not found" << std::endl;
#endif
			return false;
		}

		RsFeedReaderFeed *fi = it->second;

		if (fi->workstate != RsFeedReaderFeed::WAITING_TO_PROCESS) {
			std::cerr << "p3FeedReader::getFeedToProcess - feed in wrong state for process " << fi->workstate << std::endl;
			return false;
		}

		/* set state to processing */
		fi->workstate = RsFeedReaderFeed::PROCESSING;
		fi->errorState = RS_FEED_ERRORSTATE_OK;
		fi->errorString.clear();

		/* return a copy of the feed */
		feed = *fi;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::getFeedToProcess - feed " << fi->feedId << " (" << fi->name << ") is starting to process" << std::endl;
#endif
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}

	return true;
}

void p3FeedReader::onProcessSuccess_filterMsg(uint32_t feedId, std::list<RsFeedReaderMsg*> &msgs)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReader::onProcessSuccess_filterMsg - feed " << feedId << " got " << msgs.size() << " messages" << std::endl;
#endif

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessSuccess_filterMsg - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;

		std::list<RsFeedReaderMsg*>::iterator newMsgIt;
		for (newMsgIt = msgs.begin(); newMsgIt != msgs.end(); ) {
			RsFeedReaderMsg *miNew = *newMsgIt;
			/* search for existing msg */
			std::map<std::string, RsFeedReaderMsg*>::iterator msgIt;
			for (msgIt = fi->msgs.begin(); msgIt != fi->msgs.end(); ++msgIt) {
				RsFeedReaderMsg *mi = msgIt->second;
				if (mi->title == miNew->title && mi->link == miNew->link && mi->author == miNew->author) {
					/* msg exist */
					break;
				}
			}
			if (msgIt != fi->msgs.end()) {
				/* msg exists */
				delete(miNew);
				newMsgIt = msgs.erase(newMsgIt);
			} else {
				++newMsgIt;
			}
		}

		fi->content.clear();
		fi->errorString.clear();

		if (!fi->preview) {
			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		}
	}
}

void p3FeedReader::onProcessSuccess_addMsgs(uint32_t feedId, std::list<RsFeedReaderMsg*> &msgs)
{
#ifdef FEEDREADER_DEBUG
	std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - feed " << feedId << " got " << msgs.size() << " messages" << std::endl;
#endif

	RsFeedReaderErrorState errorState = RS_FEED_ERRORSTATE_OK;
	std::list<std::string> addedMsgs;
	std::string forumId;
	RsGxsId forumAuthorId;
	std::list<RsFeedReaderMsg> forumMsgs;
	std::string postedId;
	RsGxsId postedAuthorId;
	std::list<RsFeedReaderMsg> postedMsgs;
	uint32_t feedFlag = 0;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		bool forum = (fi->flag & RS_FEED_FLAG_FORUM) && !fi->preview;
		bool posted = (fi->flag & RS_FEED_FLAG_POSTED) && !fi->preview;
		feedFlag = fi->flag;

		if (forum && !msgs.empty()) {
			if (mForums) {
				if (!fi->forumId.empty()) {
					/* check forum */
					RsGxsForumGroup forumGroup;
					if (getForumGroup(RsGxsGroupId(fi->forumId), forumGroup)) {
						if (IS_GROUP_PUBLISHER(forumGroup.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(forumGroup.mMeta.mSubscribeFlags)) {
							forumId = fi->forumId;
							forumAuthorId = forumGroup.mMeta.mAuthorId;

							if (forumAuthorId.isNull()) {
								errorState = RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_AUTHOR;
							}
						} else {
							errorState = RS_FEED_ERRORSTATE_PROCESS_FORUM_NO_ADMIN;
						}
					} else {
						errorState = RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_FOUND;
					}
				} else {
					std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - forum id is empty (" << fi->name << ")" << std::endl;
					errorState = RS_FEED_ERRORSTATE_PROCESS_FORUM_NOT_FOUND;
				}
			} else {
				std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process forum, member mForums is not set" << std::endl;
			}
		}

		if (posted && !msgs.empty()) {
			if (mPosted) {
				if (!fi->postedId.empty()) {
					/* check posted */
					RsPostedGroup postedGroup;
					if (getPostedGroup(RsGxsGroupId(fi->postedId), postedGroup)) {
						if (IS_GROUP_PUBLISHER(postedGroup.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(postedGroup.mMeta.mSubscribeFlags)) {
							postedId = fi->postedId;
							postedAuthorId = postedGroup.mMeta.mAuthorId;

							if (postedAuthorId.isNull()) {
								errorState = RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_AUTHOR;
							}
						} else {
							errorState = RS_FEED_ERRORSTATE_PROCESS_POSTED_NO_ADMIN;
						}
					} else {
						errorState = RS_FEED_ERRORSTATE_PROCESS_POSTED_NOT_FOUND;
					}
				} else {
					std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - posted id is empty (" << fi->name << ")" << std::endl;
					errorState = RS_FEED_ERRORSTATE_PROCESS_POSTED_NOT_FOUND;
				}
			} else {
				std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process posted, member mPosted is not set" << std::endl;
			}
		}

		/* process msgs */
		if (errorState == RS_FEED_ERRORSTATE_OK) {
			/* process msgs */
#ifdef FEEDREADER_DEBUG
			uint32_t newMsgs = 0;
#endif

			std::list<RsFeedReaderMsg*>::iterator newMsgIt;
			for (newMsgIt = msgs.begin(); newMsgIt != msgs.end(); ) {
				RsFeedReaderMsg *miNew = *newMsgIt;
				/* add new msg */
				if (fi->preview) {
					rs_sprintf(miNew->msgId, "preview%d", mNextPreviewMsgId--);
				} else {
					rs_sprintf(miNew->msgId, "%lu", mNextMsgId++);
				}
				if (forum || posted) {
					miNew->flag = RS_FEEDMSG_FLAG_DELETED;
					if (forum) {
						forumMsgs.push_back(*miNew);
					}
					if (posted) {
						postedMsgs.push_back(*miNew);
					}
					miNew->description.clear();
					miNew->descriptionTransformed.clear();
					miNew->attachmentLink.clear();
					miNew->attachment.clear();
					miNew->attachmentMimeType.clear();
				} else {
					miNew->flag = RS_FEEDMSG_FLAG_NEW;
					addedMsgs.push_back(miNew->msgId);
				}
				fi->msgs[miNew->msgId] = miNew;
				newMsgIt = msgs.erase(newMsgIt);

#ifdef FEEDREADER_DEBUG
				++newMsgs;
#endif
			}
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - feed " << fi->feedId << " (" << fi->name << ") added " << newMsgs << "/" << msgs.size() << " messages" << std::endl;
#endif
		}
	}

	bool postError = false;

	if (errorState == RS_FEED_ERRORSTATE_OK) {
		if (!forumId.empty() && !forumMsgs.empty()) {
			if (mForums) {
				/* a bit tricky */
				RsGenExchange *genExchange = dynamic_cast<RsGenExchange*>(mForums);
				if (genExchange) {
					/* add messages as forum messages */
					std::list<RsFeedReaderMsg>::iterator msgIt;
					for (msgIt = forumMsgs.begin(); msgIt != forumMsgs.end(); ++msgIt) {
						RsFeedReaderMsg &mi = *msgIt;

						/* convert to forum messages */
						RsGxsForumMsg forumMsg;
						forumMsg.mMeta.mGroupId = RsGxsGroupId(forumId);
						forumMsg.mMeta.mMsgName = mi.title;
						forumMsg.mMeta.mAuthorId = forumAuthorId;

						std::string description = mi.descriptionTransformed.empty() ? mi.description : mi.descriptionTransformed;
						/* add link */
						if (!mi.link.empty()) {
							description += "<br><a href=\"" + mi.link + "\">" + mi.link + "</a>";
						}

						if (!mi.attachmentBinary.empty()) {
							if (p3FeedReaderThread::isContentType(mi.attachmentMimeType, "image/")) {
								/* add attachement to description */

								// optimize image
								std::vector<unsigned char> optimizedImage;
								std::string optimizedMimeType;
								if (optimizeImage(FeedReaderOptimizeImageTask::SIZE, mi.attachmentBinary, mi.attachmentBinaryMimeType, optimizedImage, optimizedMimeType)) {
									std::string base64;
									if (p3FeedReaderThread::toBase64(optimizedImage, base64)) {
										std::string imageBase64;
										rs_sprintf(imageBase64, "data:%s;base64,%s", optimizedMimeType.c_str(), base64.c_str());
										description += "<br><img src=\"" + imageBase64 + "\"/>";
									}
								}
							}
						}

						forumMsg.mMsg = description;

						uint32_t token;
						if (mForums->createMsg(token, forumMsg) && waitForToken(mForums, token)) {
							RsGxsGrpMsgIdPair msgPair;
							if (mForums->acknowledgeMsg(token, msgPair)) {
								/* set to new */
								genExchange->setMsgStatusFlags(token, msgPair, GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD, GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD);
							}
						} else {
#ifdef FEEDREADER_DEBUG
							std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't add forum message " << mi.title << " for feed " << forumId << std::endl;
#endif
							postError = true;
						}
					}
				} else {
					std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process forum, member mForums is not derived from RsGenExchange" << std::endl;
				}
			} else {
				std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process forum, member mForums is not set" << std::endl;
			}
		}

		if (!postedId.empty() && !postedMsgs.empty()) {
			if (mPosted) {
				/* a bit tricky */
				RsGenExchange *genExchange = dynamic_cast<RsGenExchange*>(mPosted);
				if (genExchange) {
					/* add messages as posted messages */
					std::list<RsFeedReaderMsg>::iterator msgIt;
					for (msgIt = postedMsgs.begin(); msgIt != postedMsgs.end(); ++msgIt) {
						RsFeedReaderMsg &mi = *msgIt;

						/* convert to posted messages */
						RsPostedPost postedPost;
						postedPost.mMeta.mGroupId = RsGxsGroupId(postedId);
						postedPost.mMeta.mMsgName = mi.title;
						postedPost.mMeta.mAuthorId = postedAuthorId;
						postedPost.mLink = mi.link;

						std::string description;
						if (feedFlag & RS_FEED_FLAG_POSTED_FIRST_IMAGE) {
							if (!mi.postedFirstImage.empty()) {
								/* use first image as image for posted and description without image as notes */
								if (feedFlag & RS_FEED_FLAG_POSTED_SHRINK_IMAGE) {
									// optimize image
									std::vector<unsigned char> optimizedImage;
									std::string optimizedMimeType;
									if (optimizeImage(FeedReaderOptimizeImageTask::POSTED, mi.postedFirstImage, mi.postedFirstImageMimeType, optimizedImage, optimizedMimeType)) {
										postedPost.mImage.copy(optimizedImage.data(), optimizedImage.size());
									}
								} else {
									postedPost.mImage.copy(mi.postedFirstImage.data(), mi.postedFirstImage.size());
								}
								if (feedFlag & RS_FEED_FLAG_POSTED_ONLY_IMAGE) {
									/* ignore description */
								} else {
									description = mi.postedDescriptionWithoutFirstImage;
								}
							} else {
								if (feedFlag & RS_FEED_FLAG_POSTED_ONLY_IMAGE) {
									/* ignore messages without image */
									continue;
								}
								description = mi.descriptionTransformed.empty() ? mi.description : mi.descriptionTransformed;
							}
						} else {
							description = mi.descriptionTransformed.empty() ? mi.description : mi.descriptionTransformed;

							if (!mi.attachmentBinary.empty()) {
								if (p3FeedReaderThread::isContentType(mi.attachmentMimeType, "image/")) {
									/* use attachement as image */

									if (feedFlag & RS_FEED_FLAG_POSTED_SHRINK_IMAGE) {
										// optimize image
										std::vector<unsigned char> optimizedImage;
										std::string optimizedMimeType;
										if (optimizeImage(FeedReaderOptimizeImageTask::POSTED, mi.attachmentBinary, mi.attachmentBinaryMimeType, optimizedImage, optimizedMimeType)) {
											postedPost.mImage.copy(optimizedImage.data(), optimizedImage.size());
										}
									} else {
										postedPost.mImage.copy(mi.attachmentBinary.data(), mi.attachmentBinary.size());
									}
								}
							}
						}

						postedPost.mNotes = description;

						uint32_t token;
						if (mPosted->createPost(token, postedPost) && waitForToken(mPosted, token)) {
							RsGxsGrpMsgIdPair msgPair;
							if (mPosted->acknowledgeMsg(token, msgPair)) {
								/* set to new */
								genExchange->setMsgStatusFlags(token, msgPair, GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD, GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD);
							}
						} else {
#ifdef FEEDREADER_DEBUG
							std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't add posted message " << mi.title << " for feed " << postedId << std::endl;
#endif
							postError = true;
						}
					}
				} else {
					std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process posted, member mPosted is not derived from RsGenExchange" << std::endl;
				}
			} else {
				std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - can't process posted, member mPosted is not set" << std::endl;
			}
		}
	}

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::onProcessSuccess_addMsgs - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;

		if (!fi->preview) {
			fi->workstate = RsFeedReaderFeed::WAITING;
			fi->content.clear();
			if (errorState == RS_FEED_ERRORSTATE_OK && postError) {
				// post error occured
				errorState = RS_FEED_ERRORSTATE_PROCESS_POST;
			}
			fi->errorState = errorState;
			fi->lastUpdate = time(NULL);

			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		}
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);

		std::list<std::string>::iterator it;
		for (it = addedMsgs.begin(); it != addedMsgs.end(); ++it) {
			mNotify->notifyMsgChanged(feedId, *it, NOTIFY_TYPE_ADD);
		}
	}
}

void p3FeedReader::onProcessError(uint32_t feedId, RsFeedReaderErrorState result, const std::string &errorString)
{
	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
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

		fi->errorState = result;
		fi->errorString = errorString;

#ifdef FEEDREADER_DEBUG
		std::cerr << "p3FeedReader::onProcessError - feed " << fi->feedId << " (" << fi->name << ") error process, result = " << result << ", errorState = " << fi->errorState << std::endl;
#endif

		if (!fi->preview) {
			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		}
	}

	if (mNotify) {
		mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
	}
}

void p3FeedReader::setFeedInfo(uint32_t feedId, const std::string &name, const std::string &description)
{
	bool changed = false;
	bool preview;
	std::string forumId;
	std::string forumName;
	std::string forumDescription;
	std::string postedId;
	std::string postedName;
	std::string postedDescription;

	{
		RsStackMutex stack(mFeedReaderMtx); /******* LOCK STACK MUTEX *********/

		/* find feed */
		std::map<uint32_t, RsFeedReaderFeed*>::iterator it = mFeeds.find(feedId);
		if (it == mFeeds.end()) {
			/* feed not found */
#ifdef FEEDREADER_DEBUG
			std::cerr << "p3FeedReader::setFeedInfo - feed " << feedId << " not found" << std::endl;
#endif
			return;
		}

		RsFeedReaderFeed *fi = it->second;
		preview = fi->preview;
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

		if ((fi->flag & RS_FEED_FLAG_FORUM) && (fi->flag & RS_FEED_FLAG_UPDATE_FORUM_INFO) && !fi->forumId.empty() && !preview) {
			/* change forum too */
			forumId = fi->forumId;
			forumName = fi->name;
			forumDescription = fi->description;
			forumName.insert(0, FEEDREADER_FORUM_PREFIX);
		}

		if ((fi->flag & RS_FEED_FLAG_POSTED) && (fi->flag & RS_FEED_FLAG_UPDATE_POSTED_INFO) && !fi->postedId.empty() && !preview) {
			/* change posted too */
			postedId = fi->postedId;
			postedName = fi->name;
			postedDescription = fi->description;
			postedName.insert(0, FEEDREADER_POSTED_PREFIX);
		}
	}

	if (changed) {
		if (!preview) {
			IndicateConfigChanged(RsConfigMgr::CheckPriority::SAVE_NOW);
		}

		if (mNotify) {
			mNotify->notifyFeedChanged(feedId, NOTIFY_TYPE_MOD);
		}
	}

	if (!forumId.empty()) {
		RsGxsForumGroup forumGroup;
		if (getForumGroup(RsGxsGroupId(forumId), forumGroup)) {
			updateForumGroup(forumGroup, forumName, forumDescription);
		}
		//TODO: error
	}

	if (!postedId.empty()) {
		RsPostedGroup postedGroup;
		if (getPostedGroup(RsGxsGroupId(postedId), postedGroup)) {
			updatePostedGroup(postedGroup, postedName, postedDescription);
		}
		//TODO: error
	}
}

bool p3FeedReader::getForumGroup(const RsGxsGroupId &groupId, RsGxsForumGroup &forumGroup)
{
	if (!mForums) {
		std::cerr << "p3FeedReader::getForumGroup - can't get forum group " << groupId.toStdString() << ", member mForums is not set" << std::endl;
		return false;
	}

	if (groupId.isNull()) {
		std::cerr << "p3FeedReader::getForumGroup - group id is not valid" << std::endl;
		return false;
	}

	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(groupId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
	mForums->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds);

	if (!waitForToken(mForums, token)) {
		std::cerr << "p3FeedReader::getForumGroup - waitForToken for request failed" << std::endl;
		return false;
	}

	std::vector<RsGxsForumGroup> groups;
	if (!mForums->getGroupData(token, groups)) {
		std::cerr << "p3FeedReader::getForumGroup - Error getting data" << std::endl;
		return false;
	}

	if (groups.size() != 1) {
		std::cerr << "p3FeedReader::getForumGroup - Wrong number of items" << std::endl;
		return false;
	}

	forumGroup = groups[0];

	return true;
}

bool p3FeedReader::updateForumGroup(const RsGxsForumGroup &forumGroup, const std::string &groupName, const std::string &groupDescription)
{
	if (!mForums) {
		std::cerr << "p3FeedReader::updateForumGroup - can't change forum " << forumGroup.mMeta.mGroupId.toStdString() << ", member mForums is not set" << std::endl;
		return false;
	}

	if (forumGroup.mMeta.mGroupName == groupName && forumGroup.mDescription == groupDescription) {
		/* No change */
		return true;
	}

	RsGxsForumGroup newForumGroup = forumGroup;
	newForumGroup.mMeta.mGroupName = groupName;
	newForumGroup.mDescription = groupDescription;

	uint32_t token;
	if (!mForums->updateGroup(token, newForumGroup)) {
		std::cerr << "p3FeedReader::updateForumGroup - can't change forum " << newForumGroup.mMeta.mGroupId.toStdString() << std::endl;
		return false;
	}

	if (!waitForToken(mForums, token)) {
		std::cerr << "p3FeedReader::updateForumGroup - waitForToken for update failed" << std::endl;
		return false;
	}

	/* Forum updated */
	return true;
}

bool p3FeedReader::getPostedGroup(const RsGxsGroupId &groupId, RsPostedGroup &postedGroup)
{
	if (!mPosted) {
		std::cerr << "p3FeedReader::getPostedGroup - can't get posted group " << groupId.toStdString() << ", member mPosted is not set" << std::endl;
		return false;
	}

	if (groupId.isNull()) {
		std::cerr << "p3FeedReader::getPostedGroup - group id is not valid" << std::endl;
		return false;
	}

	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(groupId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	uint32_t token;
	mPosted->getTokenService()->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds);

	if (!waitForToken(mPosted, token)) {
		std::cerr << "p3FeedReader::getPostedGroup - waitForToken for request failed" << std::endl;
		return false;
	}

	std::vector<RsPostedGroup> groups;
	if (!mPosted->getGroupData(token, groups)) {
		std::cerr << "p3FeedReader::getPostedGroup - Error getting data" << std::endl;
		return false;
	}

	if (groups.size() != 1) {
		std::cerr << "p3FeedReader::getPostedGroup - Wrong number of items" << std::endl;
		return false;
	}

	postedGroup = groups[0];

	return true;
}

bool p3FeedReader::updatePostedGroup(const RsPostedGroup &postedGroup, const std::string &groupName, const std::string &groupDescription)
{
	if (!mPosted) {
		std::cerr << "p3FeedReader::updatePostedGroup - can't change posted " << postedGroup.mMeta.mGroupId.toStdString() << ", member mPosted is not set" << std::endl;
		return false;
	}

	if (postedGroup.mMeta.mGroupName == groupName && postedGroup.mDescription == groupDescription) {
		/* No change */
		return true;
	}

	RsPostedGroup newPostedGroup = postedGroup;
	newPostedGroup.mMeta.mGroupName = groupName;
	newPostedGroup.mDescription = groupDescription;

	uint32_t token;
	if (!mPosted->updateGroup(token, newPostedGroup)) {
		std::cerr << "p3FeedReader::updatePostedGroup - can't change posted " << newPostedGroup.mMeta.mGroupId.toStdString() << std::endl;
		return false;
	}

	if (!waitForToken(mPosted, token)) {
		std::cerr << "p3FeedReader::updatePostedGroup - waitForToken for update failed" << std::endl;
		return false;
	}

	/* Posted updated */
	return true;
}

bool p3FeedReader::waitForToken(RsGxsIfaceHelper *interface, uint32_t token)
{
	if (!interface) {
		return false;
	}

	RsTokenService *service = interface->getTokenService();
	int count = MAX_REQUEST_AGE * 2;

	while (!mStopped) {
		uint32_t status = service->requestStatus(token);
		if (status == RsTokenService::FAILED) {
			break;
		}

		if (status == RsTokenService::COMPLETE) {
			return true;
		}

		if (count-- <= 0) {
			break;
		}

		rstime::rs_usleep(500 * 1000); // sleep for 500 msec
	}

	return false;
}

bool p3FeedReader::getForumGroups(std::vector<RsGxsForumGroup> &groups, bool onlyOwn)
{
	if (!mForums) {
		std::cerr << "p3FeedReader::getForumGroups - can't get groups, member mForums is not set" << std::endl;
		return false;
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	if (!mForums->requestGroupInfo(token, opts)) {
		std::cerr << "p3FeedReader::getForumGroups - can't get group list" << std::endl;
		return false;
	}

	if (!waitForToken(mForums, token)) {
		std::cerr << "p3FeedReader::getForumGroups - waitForToken for list failed" << std::endl;
		return false;
	}

	if (!mForums->getGroupData(token, groups)) {
		std::cerr << "p3FeedReader::getForumGroups - getGroupData failed" << std::endl;
		return false;
	}

	if (onlyOwn) {
		// filter groups
		for (std::vector<RsGxsForumGroup>::iterator it = groups.begin(); it != groups.end(); ) {
			const RsGxsForumGroup &group = *it;
			if (IS_GROUP_PUBLISHER(group.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(group.mMeta.mSubscribeFlags)) {
				++it;
			} else {
				it = groups.erase(it);
			}
		}
	}

	return true;
}

bool p3FeedReader::getPostedGroups(std::vector<RsPostedGroup> &groups, bool onlyOwn)
{
	if (!mPosted) {
		std::cerr << "p3FeedReader::getPostedGroups - can't get groups, member mPosted is not set" << std::endl;
		return false;
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	if (!mPosted->requestGroupInfo(token, opts)) {
		std::cerr << "p3FeedReader::getPostedGroups - can't get group list" << std::endl;
		return false;
	}

	if (!waitForToken(mPosted, token)) {
		std::cerr << "p3FeedReader::getPostedGroups - waitForToken for list failed" << std::endl;
		return false;
	}

	if (!mPosted->getGroupData(token, groups)) {
		std::cerr << "p3FeedReader::getForumGroups - getGroupData failed" << std::endl;
		return false;
	}

	if (onlyOwn) {
		// filter groups
		for (std::vector<RsPostedGroup>::iterator it = groups.begin(); it != groups.end(); ) {
			const RsPostedGroup &group = *it;
			if (IS_GROUP_PUBLISHER(group.mMeta.mSubscribeFlags) && IS_GROUP_ADMIN(group.mMeta.mSubscribeFlags)) {
				++it;
			} else {
				it = groups.erase(it);
			}
		}
	}

	return true;
}

bool p3FeedReader::optimizeImage(FeedReaderOptimizeImageTask::Type type, const std::vector<unsigned char> &image, const std::string &mimeType, std::vector<unsigned char> &resultImage, std::string &resultMimeType)
{
	if (!mNotify) {
		return false;
	}

	FeedReaderOptimizeImageTask *optimizeImageTask = new FeedReaderOptimizeImageTask(type, image, mimeType);
	{
		RsStackMutex stack(mImageMutex); /******* LOCK STACK MUTEX *********/

		mImages.push_back(optimizeImageTask);
	}

	/* Wait until task is complete */
	int nSeconds = 0;
	while (true) {
		if (mStopped) {
			return false;
		}

		rstime::rs_usleep(1000 * 1000); // 1 second

		if (++nSeconds >= 30) {
			// timeout
			std::list<FeedReaderOptimizeImageTask*>::iterator it = std::find(mImages.begin(), mImages.end(), optimizeImageTask);
			if (it != mImages.end()) {
				mImages.erase(it);

				delete(optimizeImageTask);

				return false;
			}

			// not found in mImages?
		}

		{
			RsStackMutex stack(mImageMutex); /******* LOCK STACK MUTEX *********/

			std::list<FeedReaderOptimizeImageTask*>::iterator it = std::find(mResultImages.begin(), mResultImages.end(), optimizeImageTask);
			if (it != mResultImages.end()) {
				mResultImages.erase(it);

				bool result = optimizeImageTask->mResult;
				if (result) {
					resultImage = optimizeImageTask->mImageResult;
					resultMimeType = optimizeImageTask->mMimeTypeResult;
				}

				delete(optimizeImageTask);

				return result;
			}
		}
	}

	return false;
}

FeedReaderOptimizeImageTask *p3FeedReader::getOptimizeImageTask()
{
	RsStackMutex stack(mImageMutex); /******* LOCK STACK MUTEX *********/

	if (mImages.empty()) {
		return NULL;
	}

	FeedReaderOptimizeImageTask *imageResize = mImages.front();
	mImages.pop_front();

	return imageResize;
}

void p3FeedReader::setOptimizeImageTaskResult(FeedReaderOptimizeImageTask *optimizeImageTask)
{
	if (!optimizeImageTask) {
		return;
	}

	RsStackMutex stack(mImageMutex); /******* LOCK STACK MUTEX *********/

	mResultImages.push_back(optimizeImageTask);
}
