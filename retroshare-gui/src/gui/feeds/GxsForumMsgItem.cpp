/*******************************************************************************
 * gui/feeds/GxsForumMsgItem.cpp                                               *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QTimer>
#include <QFileInfo>
#include <QStyle>

#include "rshare.h"
#include "GxsForumMsgItem.h"
#include "ui_GxsForumMsgItem.h"

#include "FeedHolder.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"

#include <retroshare/rsidentity.h>

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

GxsForumMsgItem::GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsGxsForums, autoUpdate)
{
	setup();

	requestGroup();
	requestMessage();
}

GxsForumMsgItem::GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumGroup &group, const RsGxsForumMsg &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsGxsForums, autoUpdate)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::GxsForumMsgItem() Direct Load";
	std::cerr << std::endl;
#endif

	setup();

	setGroup(group, false);
	setMessage(post);
}

GxsForumMsgItem::GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsForumMsg &post, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, post.mMeta.mGroupId, post.mMeta.mMsgId, isHome, rsGxsForums, autoUpdate)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::GxsForumMsgItem() Direct Load";
	std::cerr << std::endl;
#endif

	setup();

	requestGroup();
	setMessage(post);
}

GxsForumMsgItem::~GxsForumMsgItem()
{
	delete(ui);
}

void GxsForumMsgItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsForumMsgItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mInFill = false;
	mCloseOnRead = false;
	mTokenTypeParentMessage = nextTokenType();

	/* clear ui */
	ui->titleLabel->setText(tr("Loading"));
	ui->subjectLabel->clear();
	ui->timestamplabel->clear();
	ui->parentNameLabel->clear();
	ui->nameLabel->clear();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribeForum()));

	ui->subjectLabel->setMinimumWidth(20);

	ui->expandFrame->hide();
	ui->parentFrame->hide();
}

bool GxsForumMsgItem::isTop()
{
//	if (mMessage.mMeta.mMsgId == mMessage.mMeta.mThreadId || mMessage.mMeta.mThreadId.isNull()) {
	if (mMessage.mMeta.mParentId.isNull()) {
		return true;
	}

	return false;
}

bool GxsForumMsgItem::setGroup(const RsGxsForumGroup &group, bool doFill)
{
	if (groupId() != group.mMeta.mGroupId) {
		std::cerr << "GxsForumMsgItem::setGroup() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mGroup = group;

	if (doFill) {
		fill();
	}

	return true;
}

bool GxsForumMsgItem::setMessage(const RsGxsForumMsg &msg, bool doFill)
{
	if (groupId() != msg.mMeta.mGroupId || messageId() != msg.mMeta.mMsgId) {
		std::cerr << "GxsForumMsgItem::setPost() - Wrong id, cannot set post";
		std::cerr << std::endl;
		return false;
	}

	mMessage = msg;

	if (!isTop()) {
		requestParentMessage(mMessage.mMeta.mParentId);
	} else {
		if (doFill) {
			fill();
		}
	}

	return true;
}

QString GxsForumMsgItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsForumMsgItem::loadGroup(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumGroupItem::loadGroup()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumGroup> groups;
	if (!rsGxsForums->getGroupData(token, groups))
	{
		std::cerr << "GxsForumGroupItem::loadGroup() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "GxsForumGroupItem::loadGroup() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setGroup(groups[0]);
}

void GxsForumMsgItem::loadMessage(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::loadMessage()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (!rsGxsForums->getMsgData(token, msgs))
	{
		std::cerr << "GxsForumMsgItem::loadMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}
	
	if (msgs.size() != 1)
	{
		std::cerr << "GxsForumMsgItem::loadMessage() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	setMessage(msgs[0]);
}

void GxsForumMsgItem::loadParentMessage(const uint32_t &token)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::loadParentMessage()";
	std::cerr << std::endl;
#endif

	std::vector<RsGxsForumMsg> msgs;
	if (!rsGxsForums->getMsgData(token, msgs))
	{
		std::cerr << "GxsForumMsgItem::loadParentMessage() ERROR getting data";
		std::cerr << std::endl;
		return;
	}

	if (msgs.size() != 1)
	{
		std::cerr << "GxsForumMsgItem::loadParentMessage() Wrong number of Items";
		std::cerr << std::endl;
		return;
	}

	mParentMessage = msgs[0];

	fill();
}

void GxsForumMsgItem::fill()
{
	/* fill in */

	if (isLoading()) {
		/* Wait for all requests */
		return;
	}

#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::fill()";
	std::cerr << std::endl;
#endif

	mInFill = true;

	if (!mIsHome)
	{
		if (mCloseOnRead && !IS_MSG_NEW(mMessage.mMeta.mMsgStatus)) {
			removeItem();
		}
	}

	QString title = tr("Forum Feed") + ": ";
	RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_FORUM, mMessage.mMeta.mGroupId, groupName());
	title += link.toHtml();
	ui->titleLabel->setText(title);

	if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroup.mMeta.mSubscribeFlags)) {
		ui->unsubscribeButton->setEnabled(true);

		setReadStatus(IS_MSG_NEW(mMessage.mMeta.mMsgStatus), IS_MSG_UNREAD(mMessage.mMeta.mMsgStatus) || IS_MSG_NEW(mMessage.mMeta.mMsgStatus));
	} else {
		ui->unsubscribeButton->setEnabled(false);
	}

	if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags)) {
		ui->iconLabel->setPixmap(QPixmap(":/icons/png/forums.png"));
	} else {
		ui->iconLabel->setPixmap(QPixmap(":/icons/png/forums-default.png"));
	}

	if (!mIsHome) {
		if (IS_MSG_NEW(mMessage.mMeta.mMsgStatus)) {
			mCloseOnRead = true;
		}
	}
	
	RsIdentityDetails idDetails ;
	rsIdentity->getIdDetails(mMessage.mMeta.mAuthorId,idDetails);
		
	QPixmap pixmap ;

	if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
				pixmap = GxsIdDetails::makeDefaultIcon(mMessage.mMeta.mAuthorId,GxsIdDetails::SMALL);
			
	ui->avatar->setPixmap(pixmap);

	ui->nameLabel->setId(mMessage.mMeta.mAuthorId);

//	ui->avatar->setId(msg.srcId, true);

//	if (rsPeers->getPeerName(msg.srcId) != "") {
//		RetroShareLink linkMessage;
//		linkMessage.createMessage(msg.srcId, "");
//		nameLabel->setText(linkMessage.toHtml());
//	}
//	else
//	{
//		nameLabel->setText(tr("Anonymous"));
//	}

	RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, mMessage.mMeta.mGroupId, mMessage.mMeta.mMsgId, messageName());
	ui->subLabel->setText(msgLink.toHtml());
	if (wasExpanded() || ui->expandFrame->isVisible()) {
		fillExpandFrame();
	}

	ui->timestamplabel->setText(DateTime::formatLongDateTime(mMessage.mMeta.mPublishTs));

	if (isTop()) {
		ui->parentFrame->hide();
	} else {
//		ui->parentAvatar->setId(msgParent.srcId, true);

		RetroShareLink linkParent = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, mParentMessage.mMeta.mGroupId, mParentMessage.mMeta.mMsgId, QString::fromUtf8(mParentMessage.mMeta.mMsgName.c_str()));
		ui->parentSubLabel->setText(linkParent.toHtml());
		ui->parentMsgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mParentMessage.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));

		ui->parentNameLabel->setId(mParentMessage.mMeta.mAuthorId);
		
		RsIdentityDetails idDetails ;
		rsIdentity->getIdDetails(mParentMessage.mMeta.mAuthorId,idDetails);
		
		QPixmap pixmap ;

		if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
				pixmap = GxsIdDetails::makeDefaultIcon(mParentMessage.mMeta.mAuthorId,GxsIdDetails::SMALL);
			
		ui->parentAvatar->setPixmap(pixmap);

//		if (rsPeers->getPeerName(msgParent.srcId) !="")
//		{
//			RetroShareLink linkMessage;
//			linkMessage.createMessage(msgParent.srcId, "");
//			ui->parentNameLabel->setText(linkMessage.toHtml());
//		}
//		else
//		{
//			ui->parentNameLabel->setText(tr("Anonymous"));
//		}
	}

	/* header stuff */
	ui->subjectLabel->setText(msgLink.toHtml());

	if (mIsHome)
	{
		/* disable buttons */
		ui->clearButton->setEnabled(false);

		ui->clearButton->hide();
	}

	mInFill = false;
}

void GxsForumMsgItem::fillExpandFrame()
{
	ui->msgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mMessage.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
}

QString GxsForumMsgItem::messageName()
{
	return QString::fromUtf8(mMessage.mMeta.mMsgName.c_str());
}

void GxsForumMsgItem::doExpand(bool open)
{
	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		ui->expandFrame->show();
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/up-arrow.png")));
		ui->expandButton->setToolTip(tr("Hide"));

		if (!mParentMessage.mMeta.mMsgId.isNull()) {
			ui->parentFrame->show();
		}

		setAsRead();
	}
	else
	{
		ui->expandFrame->hide();
		ui->parentFrame->hide();
		ui->expandButton->setIcon(QIcon(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
	}
}

void GxsForumMsgItem::expandFill(bool first)
{
	GxsFeedItem::expandFill(first);

	if (first) {
		fillExpandFrame();
	}
}

void GxsForumMsgItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void GxsForumMsgItem::setReadStatus(bool isNew, bool /*isUnread*/)
{
	ui->frame->setProperty("new", isNew);
	ui->frame->style()->unpolish(ui->frame);
	ui->frame->style()->polish(  ui->frame);
}

void GxsForumMsgItem::requestParentMessage(const RsGxsMessageId &msgId)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::requestMessage()";
	std::cerr << std::endl;
#endif

	if (!initLoadQueue()) {
		return;
	}

	if (mLoadQueue->activeRequestExist(mTokenTypeParentMessage)) {
		/* Request already running */
		return;
	}

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	GxsMsgReq msgIds;
	std::set<RsGxsMessageId> &vect_msgIds = msgIds[groupId()];
	vect_msgIds.insert(msgId);

	uint32_t token;
	mLoadQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, mTokenTypeParentMessage);
}

void GxsForumMsgItem::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsFeedItem::loadRequest()";
	std::cerr << std::endl;
#endif

	if (queue == mLoadQueue) {
		if (req.mUserType == mTokenTypeParentMessage) {
			loadParentMessage(req.mToken);
			return;
		}
	}

	GxsFeedItem::loadRequest(queue, req);
}

bool GxsForumMsgItem::isLoading()
{
	if (GxsFeedItem::isLoading()) {
		return true;
	}

	if (mLoadQueue && mLoadQueue->activeRequestExist(mTokenTypeParentMessage)) {
		return true;
	}

	return false;
}

/*********** SPECIFIC FUNCTIONS ***********************/

void GxsForumMsgItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	setAsRead();
	removeItem();
}

void GxsForumMsgItem::unsubscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::unsubscribeForum()";
	std::cerr << std::endl;
#endif

	unsubscribe();
}

void GxsForumMsgItem::setAsRead()
{
	if (mInFill) {
		return;
	}

	mCloseOnRead = false;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	uint32_t token;
	rsGxsForums->setMessageReadStatus(token, msgPair, true);

	setReadStatus(false, false);
}
