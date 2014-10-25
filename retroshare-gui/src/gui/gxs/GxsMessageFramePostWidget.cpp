/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#include "GxsMessageFramePostWidget.h"
#include "gui/common/UIStateHelper.h"

#include "retroshare/rsgxsifacehelper.h"

//#define ENABLE_DEBUG 1

GxsMessageFramePostWidget::GxsMessageFramePostWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
    : GxsMessageFrameWidget(ifaceImpl, parent)
{
	mTokenQueue = new TokenQueue(ifaceImpl->getTokenService(), this);

	mSubscribeFlags = 0;
	mNextTokenType = 0;
	mFillThread = NULL;

	mTokenTypeGroupData = nextTokenType();
	mTokenTypePosts = nextTokenType();
	mTokenTypeRelatedPosts = nextTokenType();

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
}

GxsMessageFramePostWidget::~GxsMessageFramePostWidget()
{
	if (mFillThread) {
		mFillThread->stop(true);
		delete(mFillThread);
		mFillThread = NULL;
	}
	delete(mTokenQueue);
}

void GxsMessageFramePostWidget::setGroupId(const RsGxsGroupId &groupId)
{
	if (mGroupId == groupId) {
		if (!groupId.isNull()) {
			return;
		}
	}

	mGroupId = groupId;
	mGroupName = mGroupId.isNull () ? "" : tr("Loading");
	groupNameChanged(mGroupName);

	emit groupChanged(this);

	fillComplete();
}

RsGxsGroupId GxsMessageFramePostWidget::groupId()
{
	return mGroupId;
}

QString GxsMessageFramePostWidget::groupName(bool withUnreadCount)
{
	QString name = mGroupId.isNull () ? tr("No name") : mGroupName;

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

	if (mStateHelper->isLoading(mTokenTypePosts) || mStateHelper->isLoading(mTokenTypeRelatedPosts)) {
		mNavigatePendingMsgId = msgId;

		/* No information if group is available */
		return true;
	}

	return navigatePostItem(msgId);
}

void GxsMessageFramePostWidget::updateDisplay(bool complete)
{
	if (complete) {
		/* Fill complete */
		requestGroupData();
		requestPosts();
		return;
	}

	bool updateGroup = false;
	if (mGroupId.isNull()) {
		return;
	}

	const std::list<RsGxsGroupId> &grpIdsMeta = getGrpIdsMeta();
	if (std::find(grpIdsMeta.begin(), grpIdsMeta.end(), mGroupId) != grpIdsMeta.end()) {
		updateGroup = true;
	}

	const std::list<RsGxsGroupId> &grpIds = getGrpIds();
	if (!mGroupId.isNull() && std::find(grpIds.begin(), grpIds.end(), mGroupId) != grpIds.end()) {
		updateGroup = true;
		/* Do we need to fill all posts? */
		requestPosts();
	} else {
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;
		getAllMsgIds(msgs);
		if (!msgs.empty()) {
			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mit = msgs.find(mGroupId);
			if (mit != msgs.end()) {
				requestRelatedPosts(mit->second);
			}
		}
	}

	if (updateGroup) {
		requestGroupData();
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

			mStateHelper->setLoading(mTokenTypePosts, false);
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

	if (mGroupId.isNull()) {
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
	groupIds.push_back(mGroupId);

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

void GxsMessageFramePostWidget::requestPosts()
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestPosts()";
	std::cerr << std::endl;
#endif

	mNavigatePendingMsgId.clear();

	/* Request all posts */

	mTokenQueue->cancelActiveRequestTokens(mTokenTypePosts);

	if (mFillThread) {
		/* Stop current fill thread */
		GxsMessageFramePostThread *thread = mFillThread;
		mFillThread = NULL;
		thread->stop(false);

		mStateHelper->setLoading(mTokenTypePosts, false);
	}

	clearPosts();

	if (mGroupId.isNull()) {
		mStateHelper->setActive(mTokenTypePosts, false);
		mStateHelper->setLoading(mTokenTypePosts, false);
		mStateHelper->clear(mTokenTypePosts);
		emit groupChanged(this);
		return;
	}

	mStateHelper->setLoading(mTokenTypePosts, true);
	emit groupChanged(this);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mGroupId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mTokenQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, mTokenTypePosts);
}

void GxsMessageFramePostWidget::loadPosts(const uint32_t &token)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadPosts()";
	std::cerr << std::endl;
#endif

	mStateHelper->setActive(mTokenTypePosts, true);

	if (useThread()) {
		/* Create fill thread */
		mFillThread = new GxsMessageFramePostThread(token, this);

		// connect thread
		connect(mFillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
		connect(mFillThread, SIGNAL(addPost(QVariant,bool,int,int)), this, SLOT(fillThreadAddPost(QVariant,bool,int,int)), Qt::BlockingQueuedConnection);

#ifdef ENABLE_DEBUG
		std::cerr << "GxsMessageFramePostWidget::loadPosts() Start fill thread" << std::endl;
#endif

		/* Start thread */
		mFillThread->start();
	} else {
		insertPosts(token, NULL);

		mStateHelper->setLoading(mTokenTypePosts, false);

		if (!mNavigatePendingMsgId.isNull()) {
			navigate(mNavigatePendingMsgId);

			mNavigatePendingMsgId.clear();
		}
	}

	emit groupChanged(this);
}

void GxsMessageFramePostWidget::requestRelatedPosts(const std::vector<RsGxsMessageId> &msgIds)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestRelatedPosts()";
	std::cerr << std::endl;
#endif

	mNavigatePendingMsgId.clear();

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeRelatedPosts);

	if (mGroupId.isNull()) {
		mStateHelper->setActive(mTokenTypeRelatedPosts, false);
		mStateHelper->setLoading(mTokenTypeRelatedPosts, false);
		mStateHelper->clear(mTokenTypeRelatedPosts);
		emit groupChanged(this);
		return;
	}

	if (msgIds.empty()) {
		return;
	}

	mStateHelper->setLoading(mTokenTypeRelatedPosts, true);
	emit groupChanged(this);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_RELATED_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_VERSIONS;

	uint32_t token;
	std::vector<RsGxsGrpMsgIdPair> relatedMsgIds;
	for (std::vector<RsGxsMessageId>::const_iterator msgIt = msgIds.begin(); msgIt != msgIds.end(); ++msgIt) {
		relatedMsgIds.push_back(RsGxsGrpMsgIdPair(mGroupId, *msgIt));
	}
	mTokenQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, relatedMsgIds, mTokenTypeRelatedPosts);
}

void GxsMessageFramePostWidget::loadRelatedPosts(const uint32_t &token)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::loadRelatedPosts()";
	std::cerr << std::endl;
#endif

	mStateHelper->setActive(mTokenTypeRelatedPosts, true);

	insertRelatedPosts(token);

	mStateHelper->setLoading(mTokenTypeRelatedPosts, false);
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
		} else if (req.mUserType == mTokenTypePosts) {
			loadPosts(req.mToken);
		} else if (req.mUserType == mTokenTypeRelatedPosts) {
			loadRelatedPosts(req.mToken);
		} else {
			std::cerr << "GxsMessageFramePostWidget::loadRequest() ERROR: INVALID TYPE";
			std::cerr << std::endl;
		}
	}
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

	mParent->insertPosts(mToken, this);

#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostThread::run() stopped: " << (stopped() ? "yes" : "no") << std::endl;
#endif
}

void GxsMessageFramePostThread::emitAddPost(const QVariant &post, bool related, int current, int count)
{
	emit addPost(post, related, current, count);
}
