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

#include "GxsMessageFramePostWidget.h"
#include "GxsFeedItem.h"
#include "gui/common/UIStateHelper.h"

#include "retroshare/rsgxsifacehelper.h"

//#define ENABLE_DEBUG 1

GxsMessageFramePostWidget::GxsMessageFramePostWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent)
    : GxsMessageFrameWidget(ifaceImpl, parent)
{
	mTokenQueue = new TokenQueue(ifaceImpl->getTokenService(), this);

	mSubscribeFlags = 0;
	mNextTokenType = 0;

	mTokenTypeGroupData = nextTokenType();
	mTokenTypePosts = nextTokenType();
	mTokenTypeRelatedPosts = nextTokenType();

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
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

void GxsMessageFramePostWidget::setAllMessagesRead(bool read)
{
	if (mGroupId.isNull() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

	foreach (GxsFeedItem *item, mPostItems) {
		setMessageRead(item, read);
	}
}

void GxsMessageFramePostWidget::clearPosts()
{
	/* clear all messages */
	foreach (GxsFeedItem *item, mPostItems) {
		delete(item);
	}
	mPostItems.clear();
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

	/* Request all posts */
	clearPosts();

	mTokenQueue->cancelActiveRequestTokens(mTokenTypePosts);

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

	insertPosts(token);

	mStateHelper->setLoading(mTokenTypePosts, false);
	emit groupChanged(this);
}

void GxsMessageFramePostWidget::requestRelatedPosts(const std::vector<RsGxsMessageId> &msgIds)
{
#ifdef ENABLE_DEBUG
	std::cerr << "GxsMessageFramePostWidget::requestRelatedPosts()";
	std::cerr << std::endl;
#endif

	mTokenQueue->cancelActiveRequestTokens(mTokenTypeRelatedPosts);

	if (mGroupId.isNull()) {
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

	mStateHelper->setActive(mTokenTypePosts, true);

	insertRelatedPosts(token);

	mStateHelper->setLoading(mTokenTypePosts, false);
	emit groupChanged(this);
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
