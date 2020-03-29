/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsMessageFramePostWidget.cpp                    *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
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

#include <algorithm>
#include <QApplication>

#include "GxsMessageFramePostWidget.h"
#include "gui/common/UIStateHelper.h"

#include "retroshare/rsgxsifacehelper.h"

//#define ENABLE_DEBUG 1

GxsMessageFramePostWidget::GxsMessageFramePostWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
    : GxsMessageFrameWidget(ifaceImpl, parent)
{
	mSubscribeFlags = 0;
	mFillThread = NULL;

	mTokenTypeGroupData = nextTokenType();
	mTokenTypeAllPosts = nextTokenType();
	mTokenTypePosts = nextTokenType();
}

GxsMessageFramePostWidget::~GxsMessageFramePostWidget()
{
	if (mFillThread) {
		mFillThread->stop(true);
		delete(mFillThread);
		mFillThread = NULL;
	}
}

void GxsMessageFramePostWidget::groupIdChanged()
{
	mGroupName = groupId().isNull () ? "" : tr("Loading...");
	groupNameChanged(mGroupName);

	emit groupChanged(this);

	updateDisplay(true);
}

QString GxsMessageFramePostWidget::groupName(bool /*withUnreadCount*/)
{
	QString name = groupId().isNull () ? tr("No name") : mGroupName;

//	if (withUnreadCount && mUnreadCount) {
//		name += QString(" (%1)").arg(mUnreadCount);
//	}

	return name;
}

bool GxsMessageFramePostWidget::navigate(const RsGxsMessageId &msgId)
{
	if (msgId.isNull()) {
		return false;
	}

	if (mStateHelper->isLoading(mTokenTypeAllPosts) || mStateHelper->isLoading(mTokenTypePosts)) {
		mNavigatePendingMsgId = msgId;

		/* No information if group is available */
		return true;
	}

	return navigatePostItem(msgId);
}

bool GxsMessageFramePostWidget::isLoading()
{
	if (mStateHelper->isLoading(mTokenTypeAllPosts) || mStateHelper->isLoading(mTokenTypePosts)) {
		return true;
	}

	return GxsMessageFrameWidget::isLoading();
}

void GxsMessageFramePostWidget::updateDisplay(bool complete)
{
	if (complete) {
		/* Fill complete */
		requestGroupData();
		requestAllPosts();
		return;
	}

	if (groupId().isNull()) {
		return;
	}
}

void GxsMessageFramePostWidget::fillThreadAddPost(const QVariant &post, bool related, int current, int count)
{
	if (sender() == mFillThread) {
		fillThreadCreatePost(post, related, current, count);
	}
}

void GxsMessageFramePostWidget::fillThreadFinished()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::fillThreadFinished()" << std::endl;
#endif

	/* Thread has finished */
	GxsMessageFramePostThread *thread = dynamic_cast<GxsMessageFramePostThread*>(sender());
	if (thread) {
		if (thread == mFillThread) {
			/* Current thread has finished */
			mFillThread = NULL;

			mStateHelper->setLoading(mTokenTypeAllPosts, false);
			emit groupChanged(this);

			if (!mNavigatePendingMsgId.isNull()) {
				navigate(mNavigatePendingMsgId);

				mNavigatePendingMsgId.clear();
			}
		}

#ifdef ENABLE_DEBUG
		if (thread->stopped()) {
			// thread was stopped
			std::cerr << "GxsMessageFramePostWidget::fillThreadFinished() Thread was stopped" << std::endl;
		}
#endif

#ifdef ENABLE_DEBUG
		std::cerr << "GxsMessageFramePostWidget::fillThreadFinished() Delete thread" << std::endl;
#endif

		thread->deleteLater();
		thread = NULL;
	}

#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::fillThreadFinished done()" << std::endl;
#endif
}

/**************************************************************/
/** Request / Response of Data ********************************/
/**************************************************************/

void GxsMessageFramePostWidget::requestGroupData()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestGroupData()";
	std::cerr << std::endl;
#endif

	mSubscribeFlags = 0;

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeGroupData);

	if (groupId().isNull()) {
		mStateHelper->setActive(mTokenTypeGroupData, false);
		mStateHelper->setLoading(mTokenTypeGroupData, false);
		mStateHelper->clear(mTokenTypeGroupData);

		mGroupName.clear();
		groupNameChanged(mGroupName);

		emit groupChanged(this);

		return;
	}

	mStateHelper->setLoading(mTokenTypeGroupData, true);
	emit groupChanged(this);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId());

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, mTokenTypeGroupData);
}

void GxsMessageFramePostWidget::loadGroupData(const uint32_t &token)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadGroupData()";
	std::cerr << std::endl;
#endif

	RsGroupMetaData metaData;
	bool ok = insertGroupData(token, metaData);

	mStateHelper->setLoading(mTokenTypeGroupData, false);

	if (ok) {
		mSubscribeFlags = metaData.mSubscribeFlags;

		mGroupName = QString::fromUtf8(metaData.mGroupName.c_str());
		groupNameChanged(mGroupName);
	} else {
		std::cerr << "GxsMessageFramePostWidget::loadGroupData() ERROR Not just one Group";
		std::cerr << std::endl;

		mStateHelper->clear(mTokenTypeGroupData);

		mGroupName.clear();
		groupNameChanged(mGroupName);
	}

	mStateHelper->setActive(mTokenTypeGroupData, ok);

	emit groupChanged(this);
}

void GxsMessageFramePostWidget::requestAllPosts()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestAllPosts()";
	std::cerr << std::endl;
#endif

	mNavigatePendingMsgId.clear();

	/* Request all posts */

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeAllPosts);

	if (mFillThread) {
		/* Stop current fill thread */
		GxsMessageFramePostThread *thread = mFillThread;
		mFillThread = NULL;
		thread->stop(false);

		mStateHelper->setLoading(mTokenTypeAllPosts, false);
	}

	clearPosts();

	if (groupId().isNull()) {
		mStateHelper->setActive(mTokenTypeAllPosts, false);
		mStateHelper->setLoading(mTokenTypeAllPosts, false);
		mStateHelper->clear(mTokenTypeAllPosts);
		emit groupChanged(this);
		return;
	}

	mStateHelper->setLoading(mTokenTypeAllPosts, true);
	emit groupChanged(this);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId());

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, mTokenTypeAllPosts);
}

void GxsMessageFramePostWidget::loadAllPosts(const uint32_t &token)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadAllPosts()";
	std::cerr << std::endl;
#endif

	mStateHelper->setActive(mTokenTypeAllPosts, true);

	if (useThread()) {
		/* Create fill thread */
		mFillThread = new GxsMessageFramePostThread(token, this);

		// connect thread
		connect(mFillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
		connect(mFillThread, SIGNAL(addPost(QVariant,bool,int,int)), this, SLOT(fillThreadAddPost(QVariant,bool,int,int)), Qt::BlockingQueuedConnection);

#ifdef ENABLE_DEBUG
		std::cerr << "GxsMessageFramePostWidget::loadAllPosts() Start fill thread" << std::endl;
#endif

		/* Start thread */
		mFillThread->start();
	} else {
		insertAllPosts(token, NULL);

		mStateHelper->setLoading(mTokenTypeAllPosts, false);

		if (!mNavigatePendingMsgId.isNull()) {
			navigate(mNavigatePendingMsgId);

			mNavigatePendingMsgId.clear();
		}
	}

	emit groupChanged(this);
}

void GxsMessageFramePostWidget::requestPosts(const std::set<RsGxsMessageId> &msgIds)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestPosts()";
	std::cerr << std::endl;
#endif

	mNavigatePendingMsgId.clear();

	mTokenQueue->cancelActiveRequestTokens(mTokenTypePosts);

	if (groupId().isNull()) {
		mStateHelper->setActive(mTokenTypePosts, false);
		mStateHelper->setLoading(mTokenTypePosts, false);
		mStateHelper->clear(mTokenTypePosts);
		emit groupChanged(this);
		return;
	}

	if (msgIds.empty()) {
		return;
	}

	mStateHelper->setLoading(mTokenTypePosts, true);
	emit groupChanged(this);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	GxsMsgReq requestMsgIds;
	requestMsgIds[groupId()] = msgIds;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, requestMsgIds, mTokenTypePosts);
}

void GxsMessageFramePostWidget::loadPosts(const uint32_t &token)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadPosts()";
	std::cerr << std::endl;
#endif

	mStateHelper->setActive(mTokenTypePosts, true);

	insertPosts(token);

	mStateHelper->setLoading(mTokenTypePosts, false);
	emit groupChanged(this);

	if (!mNavigatePendingMsgId.isNull()) {
		navigate(mNavigatePendingMsgId);

		mNavigatePendingMsgId.clear();
	}
}

void GxsMessageFramePostWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mTokenQueue)
	{
		if (req.mUserType == mTokenTypeGroupData) {
			loadGroupData(req.mToken);
			return;
		}

		if (req.mUserType == mTokenTypeAllPosts) {
			loadAllPosts(req.mToken);
			return;
		}

		if (req.mUserType == mTokenTypePosts) {
			loadPosts(req.mToken);
			return;
		}
	}

	GxsMessageFrameWidget::loadRequest(queue, req);
}

/**************************************************************/
/** GxsMessageFramePostThread *********************************/
/**************************************************************/

GxsMessageFramePostThread::GxsMessageFramePostThread(uint32_t token, GxsMessageFramePostWidget *parent)
    : QThread(parent), mToken(token), mParent(parent)
{
	mStopped = false;
}

GxsMessageFramePostThread::~GxsMessageFramePostThread()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostThread::~GxsMessageFramePostThread" << std::endl;
#endif
}

void GxsMessageFramePostThread::stop(bool waitForStop)
{
	if (waitForStop) {
		disconnect();
	}

	mStopped = true;
	QApplication::processEvents();

	if (waitForStop) {
		wait();
	}
}

void GxsMessageFramePostThread::run()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostThread::run()" << std::endl;
#endif

	mParent->insertAllPosts(mToken, this);

#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostThread::run() stopped: " << (stopped() ? "yes" : "no") << std::endl;
#endif
}

void GxsMessageFramePostThread::emitAddPost(const QVariant &post, bool related, int current, int count)
{
	emit addPost(post, related, current, count);
}
