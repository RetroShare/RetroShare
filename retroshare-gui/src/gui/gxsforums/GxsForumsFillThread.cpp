/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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

#include <QApplication>
#include <QTreeWidgetItem>

#include "GxsForumsFillThread.h"
#include "GxsForumThreadWidget.h"

#include "retroshare/rsgxsflags.h"
#include <retroshare/rsgxsforums.h>

#include <iostream>
#include <algorithm>

//#define DEBUG_FORUMS

#define PROGRESSBAR_MAX 100

GxsForumsFillThread::GxsForumsFillThread(GxsForumThreadWidget *parent)
	: QThread(parent), mParent(parent)
{
	mStopped = false;
	mCompareRole = NULL;

	mExpandNewMessages = true;
	mFillComplete = false;

	mFilterColumn = 0;
	mSubscribeFlags = 0;

	mViewType = 0;
	mFlatView = false;
	mUseChildTS = false;
}

GxsForumsFillThread::~GxsForumsFillThread()
{
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::~GxsForumsFillThread" << std::endl;
#endif

	// remove all items (when items are available, the thread was terminated)
	QList<QTreeWidgetItem *>::iterator item;
	for (item = mItems.begin (); item != mItems.end (); item++) {
		if (*item) {
			delete (*item);
		}
	}
	mItems.clear();

	mItemToExpand.clear();
}

void GxsForumsFillThread::stop()
{
	disconnect();
	mStopped = true;
	QApplication::processEvents();
	wait();
}

void GxsForumsFillThread::calculateExpand(const RsGxsForumMsg &msg, QTreeWidgetItem *item)
{
	if (mFillComplete && mExpandNewMessages && IS_MSG_UNREAD(msg.mMeta.mMsgStatus)) {
		QTreeWidgetItem *parentItem = item;
		while ((parentItem = parentItem->parent()) != NULL) {
			if (std::find(mItemToExpand.begin(), mItemToExpand.end(), parentItem) == mItemToExpand.end()) {
				mItemToExpand.push_back(parentItem);
			}
		}
	}
}

void GxsForumsFillThread::run()
{
	RsTokenService *service = rsGxsForums->getTokenService();

	emit status(tr("Waiting"));

	/* get all messages of the forum */
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	std::list<std::string> grpIds;
	grpIds.push_back(mForumId);

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() forum id " << mForumId << std::endl;
#endif

	uint32_t token;
	service->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds);

	/* wait for the answer */
	uint32_t requestStatus;
	while (!wasStopped()) {
		requestStatus = service->requestStatus(token);
		if (requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED ||
			requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE) {
			break;
		}
		msleep(100);
	}

	if (wasStopped()) {
#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() thread stopped, cancel request" << std::endl;
#endif

		/* cancel request */
		service->cancelRequest(token);
		return;
	}

	if (requestStatus == RsTokenService::GXS_REQUEST_V2_STATUS_FAILED) {
//#TODO
		return;
	}

//#TODO
//	if (failed) {
//		mService->cancelRequest(token);
//		return;
//	}

	emit status(tr("Retrieving"));

	/* get messages */
	std::vector<RsGxsForumMsg> msgs;
	if (!rsGxsForums->getMsgData(token, msgs)) {
		return;
	}

	emit status(tr("Loading"));

	int count = msgs.size();
	int pos = 0;
	int steps = count / PROGRESSBAR_MAX;
	int step = 0;
	QList<QPair<std::string, QTreeWidgetItem*> > threadList;
	QPair<std::string, QTreeWidgetItem*> threadPair;

	/* add all threads */
	std::vector<RsGxsForumMsg>::iterator msgIt;
	for (msgIt = msgs.begin(); msgIt != msgs.end(); ) {
		if (wasStopped()) {
			break;
		}

		const RsGxsForumMsg &msg = *msgIt;

		if (!msg.mMeta.mParentId.empty()) {
			++msgIt;
			continue;
		}

#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsFillThread::run() Adding TopLevel Thread: mId: " << msg.mMeta.mMsgId << std::endl;
#endif

		QTreeWidgetItem *item = mParent->convertMsgToThreadWidget(msg, mUseChildTS, mFilterColumn);
		threadList.push_back(QPair<std::string, QTreeWidgetItem*>(msg.mMeta.mMsgId, item));
		calculateExpand(msg, item);

		mItems.append(item);

		if (++step >= steps) {
			step = 0;
			emit progress(++pos, PROGRESSBAR_MAX);
		}

		msgIt = msgs.erase(msgIt);
	}

	/* process messages */
	while (msgs.size()) {
		while (threadList.size() > 0) {
			if (wasStopped()) {
				break;
			}

			threadPair = threadList.front();
			threadList.pop_front();

#ifdef DEBUG_FORUMS
			std::cerr << "GxsForumsFillThread::run() Getting Children of : " << threadPair.first << std::endl;
#endif
			/* iterate through child */
			for (msgIt = msgs.begin(); msgIt != msgs.end(); ) {
				const RsGxsForumMsg &msg = *msgIt;

				if (msg.mMeta.mParentId != threadPair.first) {
					++msgIt;
					continue;
				}

#ifdef DEBUG_FORUMS
				std::cerr << "GxsForumsFillThread::run() adding " << msg.mMeta.mMsgId << std::endl;
#endif

				QTreeWidgetItem *item = mParent->convertMsgToThreadWidget(msg, mUseChildTS, mFilterColumn);
				if (mFlatView) {
					mItems.append(item);
				} else {
					threadPair.second->addChild(item);
				}

				calculateExpand(msg, item);

				/* add item to process list */
				threadList.push_back(QPair<std::string, QTreeWidgetItem*>(msg.mMeta.mMsgId, item));

				if (++step >= steps) {
					step = 0;
					emit progress(++pos, PROGRESSBAR_MAX);
				}

				msgIt = msgs.erase(msgIt);
			}
		}

		if (wasStopped()) {
			break;
		}

		/* process missing messages */

		/* search for a message with missing parent */
		for (msgIt = msgs.begin(); msgIt != msgs.end(); ++msgIt) {
			const RsGxsForumMsg &msg = *msgIt;

			/* search for parent */
			std::vector<RsGxsForumMsg>::iterator msgIt1;
			for (msgIt1 = msgs.begin(); msgIt1 != msgs.end(); ++msgIt1) {
				if (wasStopped()) {
					break;
				}

				const RsGxsForumMsg &msg1 = *msgIt1;

				if (msg.mMeta.mParentId == msg1.mMeta.mMsgId) {
					/* found parent */
					break;
				}
			}

			if (wasStopped()) {
				break;
			}

			if (msgIt1 != msgs.end()) {
				/* parant found */
				continue;
			}

			/* add dummy item */
			QTreeWidgetItem *item = mParent->generateMissingItem(msg.mMeta.mParentId);
			threadList.push_back(QPair<std::string, QTreeWidgetItem*>(msg.mMeta.mParentId, item));

			mItems.append(item);
			break;
		}
	}

#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsFillThread::run() stopped: " << (wasStopped() ? "yes" : "no") << std::endl;
#endif
}
