/*
 * Retroshare Posted List
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QMessageBox>

#include "PostedListWidget.h"
#include "ui_PostedListWidget.h"

#include "PostedCreatePostDialog.h"
#include "PostedItem.h"
#include "PostedUserTypes.h"
#include "gui/Identity/IdDialog.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rsposted.h>

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

//#define POSTEDDIALOG_LISTING        1
//#define POSTEDDIALOG_CURRENTFORUM   2
#define POSTEDDIALOG_INSERTTHREADS  3
//#define POSTEDDIALOG_INSERTCHILD    4
//#define POSTEDDIALOG_INSERT_POST    5
//#define POSTEDDIALOG_REPLY_MESSAGE  6

/****************************************************************
 */

// token types to deal with

#define POSTED_DEFAULT_LISTING_LENGTH 10
#define POSTED_MAX_INDEX	      10000

/** Constructor */
PostedListWidget::PostedListWidget(const RsGxsGroupId &postedId, QWidget *parent)
    : GxsMessageFrameWidget(rsPosted, parent),
      ui(new Ui::PostedListWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	// No progress yet
//	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui->loadingLabel, UISTATE_LOADING_VISIBLE);
//	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui->progressBar, UISTATE_LOADING_VISIBLE);
//	mStateHelper->addWidget(TOKEN_TYPE_POSTS, ui->progressLabel, UISTATE_LOADING_VISIBLE);

//	mStateHelper->addLoadPlaceholder(TOKEN_TYPE_GROUP_DATA, ui->nameLabel);

//	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui->postButton);
//	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui->logoLabel);
//	mStateHelper->addWidget(TOKEN_TYPE_GROUP_DATA, ui->subscribeToolButton);

	/* Setup Queue */
	mPostedQueue = new TokenQueue(rsPosted->getTokenService(), this);

	mSubscribeFlags = 0;

	connect(ui->hotSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui->newSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui->topSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(showNext()));
	connect(ui->prevButton, SIGNAL(clicked()), this, SLOT(showPrev()));

	connect(ui->toolButton_NewId, SIGNAL(clicked()), this, SLOT(createNewGxsId()));

	// default sort method.
	mSortMethod = RsPosted::HotRankType;
	mLastSortMethod = RsPosted::TopRankType; // to be different.
	mPostIndex = 0;
	mPostShow = POSTED_DEFAULT_LISTING_LENGTH;

	ui->hotSortButton->setChecked(true);

	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());

	connect(ui->submitPostButton, SIGNAL(clicked()), this, SLOT(newPost()));

	/* load settings */
	processSettings(true);

	/* Initialize GUI */
	setGroupId(postedId);
}

PostedListWidget::~PostedListWidget()
{
	// save settings
	processSettings(false);
}

void PostedListWidget::updateDisplay(bool complete)
{
	std::cerr << "rsPosted->updateDisplay()";
	std::cerr << std::endl;

	if (complete) {
		/* Fill complete */
		requestGroupData();
		requestPosts();
		return;
	}

	if (mPostedId.isNull()) {
		return;
	}

	bool updateGroup = false;

	const std::list<RsGxsGroupId> &grpIdsMeta = getGrpIdsMeta();
	if (std::find(grpIdsMeta.begin(), grpIdsMeta.end(), mPostedId) != grpIdsMeta.end()) {
		updateGroup = true;
	}

	const std::list<RsGxsGroupId> &grpIds = getGrpIds();
	if (!mPostedId.isNull() && std::find(grpIds.begin(), grpIds.end(), mPostedId) != grpIds.end()) {
		updateGroup = true;
		/* Do we need to fill all posts? */
		requestPosts();
	} else {
		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > msgs;
		getAllMsgIds(msgs);
		if (!msgs.empty())
		{
			std::cerr << "rsPosted->msgsChanged():";
			std::cerr << std::endl;

			std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::const_iterator mit;
			mit = msgs.find(mPostedId);
			if(mit != msgs.end())
			{
				std::cerr << "current Group -> updating Displayed Items";
				std::cerr << std::endl;
				updateDisplayedItems(mit->second);
			}
		}
	}

	if (updateGroup) {
		requestGroupData();
	}
}

void PostedListWidget::processSettings(bool load)
{
//	Settings->beginGroup(QString("PostedListWidget"));
//
//	if (load) {
//		// load settings
//	} else {
//		// save settings
//	}
//
//	Settings->endGroup();
}

void PostedListWidget::setGroupId(const RsGxsGroupId &groupId)
{
	if (mPostedId == groupId) {
		if (!groupId.isNull()) {
			return;
		}
	}

	mPostedId = groupId;
	mName = mPostedId.isNull () ? "" : tr("Loading");

	emit groupChanged(this);

	fillComplete();
}

QString PostedListWidget::groupName(bool withUnreadCount)
{
	QString name = mPostedId.isNull () ? tr("No name") : mName;

//	if (withUnreadCount && mUnreadCount) {
//		name += QString(" (%1)").arg(mUnreadCount);
//	}

	return name;
}

QIcon PostedListWidget::groupIcon()
{
//	if (mStateHelper->isLoading(TOKEN_TYPE_GROUP_DATA) || mStateHelper->isLoading(TOKEN_TYPE_POSTS)) {
//		return QIcon(":/images/kalarm.png");
//	}

//	if (mNewCount) {
//		return QIcon(":/images/message-state-new.png");
//	}

	return QIcon();
}

/*****************************************************************************************/
// Overloaded from FeedHolder.
QScrollArea *PostedListWidget::getScrollArea()
{
	return ui->scrollArea;
}

void PostedListWidget::deleteFeedItem(QWidget */*item*/, uint32_t /*type*/)
{
	std::cerr << "PostedListWidget::deleteFeedItem() Nah";
	std::cerr << std::endl;
	return;
}

void PostedListWidget::openChat(const RsPeerId & /*peerId*/)
{
	std::cerr << "PostedListWidget::openChat() Nah";
	std::cerr << std::endl;
	return;
}

void PostedListWidget::openComments(uint32_t /*feed_type*/, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title)
{
	emit loadComment(groupId, msgId, title);
}

void PostedListWidget::newPost()
{
	if (mPostedId.isNull())
		return;

	if (!IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
		return;
	}

 	PostedCreatePostDialog *cp = new PostedCreatePostDialog(mPostedQueue, rsPosted, mPostedId, this);
	cp->show();

	/* window will destroy itself! */
}

void PostedListWidget::showNext()
{
	mPostIndex += mPostShow;
	if (mPostIndex > POSTED_MAX_INDEX)
		mPostIndex = POSTED_MAX_INDEX;
	applyRanking();
	updateShowText();
}

void PostedListWidget::showPrev()
{
	mPostIndex -= mPostShow;
	if (mPostIndex < 0)
		mPostIndex = 0;
	applyRanking();
	updateShowText();
}

void PostedListWidget::updateShowText()
{
	QString showText = tr("Showing");
	showText += " ";
	showText += QString::number(mPostIndex + 1);
	showText += "-";
	showText += QString::number(mPostIndex + mPostShow);
	ui->showLabel->setText(showText);
}

void PostedListWidget::getRankings()
{
	if (mPostedId.isNull())
		return;

	std::cerr << "PostedListWidget::getRankings()";
	std::cerr << std::endl;

	int oldSortMethod = mSortMethod;

	QObject* button = sender();
	if(button == ui->hotSortButton)
	{
		mSortMethod = RsPosted::HotRankType;
	}
	else if(button == ui->topSortButton)
	{
		mSortMethod = RsPosted::TopRankType;
	}
	else if(button == ui->newSortButton)
	{
		mSortMethod = RsPosted::NewRankType;
	}
	else
	{
		return;
	}

	if (oldSortMethod != mSortMethod)
	{
		/* Reset Counter */
		mPostIndex = 0;
		updateShowText();
	}

	applyRanking();
}

void PostedListWidget::submitVote(const RsGxsGrpMsgIdPair &msgId, bool up)
{
	/* must grab AuthorId from Layout */
	RsGxsId authorId;
	switch (ui->idChooser->getChosenId(authorId)) {
		case GxsIdChooser::KnowId:
		case GxsIdChooser::UnKnowId:
		break;
		case GxsIdChooser::NoId:
		case GxsIdChooser::None:
		default:
		std::cerr << "PostedListWidget::createPost() ERROR GETTING AuthorId!, Vote Failed";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose a Signing Id before Voting"), QMessageBox::Ok, QMessageBox::Ok);

		return;
	}//switch (ui.idChooser->getChosenId(authorId))

	RsGxsVote vote;

	vote.mMeta.mGroupId = msgId.first;
	vote.mMeta.mThreadId = msgId.second;
	vote.mMeta.mParentId = msgId.second;
	vote.mMeta.mAuthorId = authorId;

	if (up) {
		vote.mVoteType = GXS_VOTE_UP;
	} else { //if (up)
		vote.mVoteType = GXS_VOTE_DOWN;
	}//if (up)

	std::cerr << "PostedListWidget::submitVote()";
	std::cerr << std::endl;

	std::cerr << "GroupId : " << vote.mMeta.mGroupId << std::endl;
	std::cerr << "ThreadId : " << vote.mMeta.mThreadId << std::endl;
	std::cerr << "ParentId : " << vote.mMeta.mParentId << std::endl;
	std::cerr << "AuthorId : " << vote.mMeta.mAuthorId << std::endl;

	uint32_t token;
	rsPosted->createVote(token, vote);
	mPostedQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_VOTE);
}

/*****************************************************************************************/

void PostedListWidget::updateDisplayedItems(const std::vector<RsGxsMessageId> &msgIds)
{
	mPostedQueue->cancelActiveRequestTokens(TOKEN_USER_TYPE_POST_MOD);

	if (mPostedId.isNull()) {
		mStateHelper->setActive(TOKEN_USER_TYPE_POST, false);
		mStateHelper->setLoading(TOKEN_USER_TYPE_POST, false);
		mStateHelper->clear(TOKEN_USER_TYPE_POST);
		emit groupChanged(this);
		return;
	}

	if (msgIds.empty()) {
		return;
	}

	mStateHelper->setLoading(TOKEN_USER_TYPE_POST, true);
	emit groupChanged(this);

	RsTokReqOptions opts;

	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	GxsMsgReq msgs;
	msgs[mPostedId] = msgIds;

	std::cerr << "PostedListWidget::updateDisplayedItems(" << mPostedId << ")";
	std::cerr << std::endl;

	std::vector<RsGxsMessageId>::const_iterator it;
	for(it = msgIds.begin(); it != msgIds.end(); it++)
	{
		std::cerr << "\t\tMsgId: " << *it;
		std::cerr << std::endl;
	}

	uint32_t token;
	mPostedQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgs, TOKEN_USER_TYPE_POST_MOD);
}

void PostedListWidget::createNewGxsId()
{
	IdEditDialog dlg(this);
	dlg.setupNewId(false);
	dlg.exec();
	ui->idChooser->setDefaultId(dlg.getLastIdName());
}

void PostedListWidget::acknowledgeVoteMsg(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;

	rsPosted->acknowledgeVote(token, msgId);
}

void PostedListWidget::loadVoteData(const uint32_t &/*token*/)
{
	return;
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListWidget::requestGroupData()
{
#ifdef DEBUG_POSTED
	std::cerr << "PostedListWidget::requestGroupData()";
	std::cerr << std::endl;
#endif

	mSubscribeFlags = 0;

	mPostedQueue->cancelActiveRequestTokens(TOKEN_USER_TYPE_TOPIC);

	if (mPostedId.isNull()) {
		mStateHelper->setActive(TOKEN_USER_TYPE_TOPIC, false);
		mStateHelper->setLoading(TOKEN_USER_TYPE_TOPIC, false);
		mStateHelper->clear(TOKEN_USER_TYPE_TOPIC);

		emit groupChanged(this);

		return;
	}

	mStateHelper->setLoading(TOKEN_USER_TYPE_TOPIC, true);
	emit groupChanged(this);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mPostedId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	mPostedQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, TOKEN_USER_TYPE_TOPIC);
}

void PostedListWidget::loadGroupData(const uint32_t &token)
{
	std::cerr << "PostedListWidget::loadGroupData()";
	std::cerr << std::endl;

	std::vector<RsPostedGroup> groups;
	rsPosted->getGroupData(token, groups);

	mStateHelper->setLoading(TOKEN_USER_TYPE_TOPIC, false);

	if (groups.size() == 1)
	{
		insertPostedDetails(groups[0]);
	}
	else
	{
		std::cerr << "PostedListWidget::loadGroupData() ERROR Invalid Number of Groups...";
		std::cerr << std::endl;

		mStateHelper->setActive(TOKEN_USER_TYPE_TOPIC, false);
		mStateHelper->clear(TOKEN_USER_TYPE_TOPIC);
	}

	emit groupChanged(this);
}

void PostedListWidget::insertPostedDetails(const RsPostedGroup &group)
{
	mStateHelper->setActive(TOKEN_USER_TYPE_TOPIC, true);

	mSubscribeFlags = group.mMeta.mSubscribeFlags;

	/* set name */
	mName = QString::fromUtf8(group.mMeta.mGroupName.c_str());

	if (mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_PUBLISH)
	{
		mStateHelper->setWidgetEnabled(ui->submitPostButton, true);
	}
	else
	{
		mStateHelper->setWidgetEnabled(ui->submitPostButton, false);
	}

//	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(mSubscribeFlags));
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListWidget::requestPosts()
{
	std::cerr << "PostedListWidget::loadCurrentForumThreads(" << mPostedId << ")";
	std::cerr << std::endl;

	clearPosts();

	mPostedQueue->cancelActiveRequestTokens(TOKEN_USER_TYPE_POST);

	if (mPostedId.isNull())
	{
		std::cerr << "PostedListWidget::loadCurrentForumThreads() Empty GroupId .. ignoring Req";
		std::cerr << std::endl;

		mStateHelper->setActive(TOKEN_USER_TYPE_POST, false);
		mStateHelper->setLoading(TOKEN_USER_TYPE_POST, false);
		mStateHelper->clear(TOKEN_USER_TYPE_POST);
		emit groupChanged(this);
		return;
	}

	mStateHelper->setLoading(TOKEN_USER_TYPE_POST, true);
	emit groupChanged(this);

	/* initiate loading */
	RsTokReqOptions opts;

	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
	
	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(mPostedId);

	std::cerr << "PostedListWidget::requestGroupThreadData_InsertThreads(" << mPostedId << ")";
	std::cerr << std::endl;

	uint32_t token;
	mPostedQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, TOKEN_USER_TYPE_POST);
}

void PostedListWidget::acknowledgePostMsg(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;

	// just acknowledge, don't load anything
	rsPosted->acknowledgeMsg(token, msgId);
}

void PostedListWidget::loadPostData(const uint32_t &token)
{
	std::cerr << "PostedListWidget::loadGroupThreadData_InsertThreads()";
	std::cerr << std::endl;

	mStateHelper->setActive(TOKEN_USER_TYPE_POST, true);
	clearPosts();

	std::vector<RsPostedPost> posts;
	rsPosted->getPostData(token, posts);
	std::vector<RsPostedPost>::iterator vit;

	for(vit = posts.begin(); vit != posts.end(); vit++)
	{
		RsPostedPost& p = *vit;
		loadPost(p);
	}

	applyRanking();

	mStateHelper->setLoading(TOKEN_USER_TYPE_POST, false);
	emit groupChanged(this);
}

void PostedListWidget::loadPost(const RsPostedPost &post)
{
	PostedItem *item = new PostedItem(this, 0, post, true);
	connect(item, SIGNAL(vote(RsGxsGrpMsgIdPair,bool)), this, SLOT(submitVote(RsGxsGrpMsgIdPair,bool)));
	mPosts.insert(post.mMeta.mMsgId, item);
	//QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	//alayout->addWidget(item);
	mPostList.push_back(item);
}

static bool CmpPIHot(const PostedItem *a, const PostedItem *b)
{
	const RsPostedPost &postA = a->getPost();
	const RsPostedPost &postB = b->getPost();

	if (postA.mHotScore == postB.mHotScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mHotScore > postB.mHotScore);
}

static bool CmpPITop(const PostedItem *a, const PostedItem *b)
{
	const RsPostedPost &postA = a->getPost();
	const RsPostedPost &postB = b->getPost();

	if (postA.mTopScore == postB.mTopScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mTopScore > postB.mTopScore);
}

static bool CmpPINew(const PostedItem *a, const PostedItem *b)
{
	return (a->getPost().mNewScore > b->getPost().mNewScore);
}

void PostedListWidget::applyRanking()
{
	/* uses current settings to sort posts, then add to layout */
	std::cerr << "PostedListWidget::applyRanking()";
	std::cerr << std::endl;

	shallowClearPosts();

	/* sort */
	switch(mSortMethod)
	{
		default:
		case RsPosted::HotRankType:
			std::cerr << "PostedListWidget::applyRanking() HOT";
			std::cerr << std::endl;
			mPostList.sort(CmpPIHot);
			break;
		case RsPosted::NewRankType:
			std::cerr << "PostedListWidget::applyRanking() NEW";
			std::cerr << std::endl;
			mPostList.sort(CmpPINew);
			break;
		case RsPosted::TopRankType:
			std::cerr << "PostedListWidget::applyRanking() TOP";
			std::cerr << std::endl;
			mPostList.sort(CmpPITop);
			break;
	}
	mLastSortMethod = mSortMethod;

	std::cerr << "PostedListWidget::applyRanking() Sorted mPostList";
	std::cerr << std::endl;

	/* go through list (skipping out-of-date items) to get */
	QLayout *alayout = ui->scrollAreaWidgetContents->layout();
	int counter = 0;
	time_t min_ts = 0;
	std::list<PostedItem *>::iterator it;
	for(it = mPostList.begin(); it != mPostList.end(); it++)
	{
		PostedItem *item = (*it);
		std::cerr << "PostedListWidget::applyRanking() Item: " << item;
		std::cerr << std::endl;
		
		if (item->getPost().mMeta.mPublishTs < min_ts)
		{
			std::cerr << "\t Skipping OLD";
			std::cerr << std::endl;
			item->hide();
			continue;
		}

		if (counter >= mPostIndex + mPostShow)
		{
			std::cerr << "\t END - Counter too high";
			std::cerr << std::endl;
			item->hide();
		}
		else if (counter >= mPostIndex)
		{
			std::cerr << "\t Adding to Layout";
			std::cerr << std::endl;
			/* add it in! */
			alayout->addWidget(item);
			item->show();
		}
		else
		{
			std::cerr << "\t Skipping to Low";
			std::cerr << std::endl;
			item->hide();
		}
		counter++;
	}

	std::cerr << "PostedListWidget::applyRanking() Loaded New Order";
	std::cerr << std::endl;

	// trigger a redraw.
	ui->scrollAreaWidgetContents->update();
}

void PostedListWidget::clearPosts()
{
	std::cerr << "PostedListWidget::clearPosts()" << std::endl;

	std::list<PostedItem *> postedItems;
	std::list<PostedItem *>::iterator pit;

	QLayout *alayout = ui->scrollAreaWidgetContents->layout();
	int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PostedListWidget::clearPosts() missing litem";
			std::cerr << std::endl;
			continue;
		}

		PostedItem *item = dynamic_cast<PostedItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PostedListWidget::clearPosts() item: " << item;
			std::cerr << std::endl;

			postedItems.push_back(item);
		}
		else
		{
			std::cerr << "PostedListWidget::clearPosts() Found Child, which is not a PostedItem???";
			std::cerr << std::endl;
		}
	}

	for(pit = postedItems.begin(); pit != postedItems.end(); pit++)
	{
		PostedItem *item = *pit;
		alayout->removeWidget(item);
		delete item;
	}

	mPosts.clear();
	mPostList.clear();
}

void PostedListWidget::shallowClearPosts()
{
	std::cerr << "PostedListWidget::shallowClearPosts()" << std::endl;

	std::list<PostedItem *> postedItems;
	std::list<PostedItem *>::iterator pit;

	QLayout *alayout = ui->scrollAreaWidgetContents->layout();
	int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PostedListWidget::shallowClearPosts() missing litem";
			std::cerr << std::endl;
			continue;
		}

		PostedItem *item = dynamic_cast<PostedItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PostedListWidget::shallowClearPosts() item: " << item;
			std::cerr << std::endl;

			postedItems.push_back(item);
		}
		else
		{
			std::cerr << "PostedListWidget::shallowClearPosts() Found Child, which is not a PostedItem???";
			std::cerr << std::endl;
		}
	}

	for(pit = postedItems.begin(); pit != postedItems.end(); pit++)
	{
		PostedItem *item = *pit;
		alayout->removeWidget(item);
	}
}

void PostedListWidget::setAllMessagesRead(bool read)
{
//	if (mPostedId.isNull() || !IS_GROUP_SUBSCRIBED(mSubscribeFlags)) {
//		return;
//	}

//	QList<GxsChannelPostItem *>::iterator mit;
//	for (mit = mChannelPostItems.begin(); mit != mChannelPostItems.end(); ++mit) {
//		GxsChannelPostItem *item = *mit;
//		RsGxsGrpMsgIdPair msgPair = std::make_pair(item->groupId(), item->messageId());

//		uint32_t token;
//		rsGxsChannels->setMessageReadStatus(token, msgPair, read);
//	}
}

//void PostedListWidget::subscribeGroup(bool subscribe)
//{
//	if (mChannelId.isNull()) {
//		return;
//	}

//	uint32_t token;
//	rsGxsChannels->subscribeToGroup(token, mChannelId, subscribe);
////	mChannelQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
//}

void PostedListWidget::updateCurrentDisplayComplete(const uint32_t &token)
{
	std::cerr << "PostedListWidget::updateCurrentDisplayComplete()";
	std::cerr << std::endl;

	std::vector<RsPostedPost> posts;
	rsPosted->getPostData(token, posts);

	mStateHelper->setActive(TOKEN_USER_TYPE_POST, true);

	std::vector<RsPostedPost>::iterator vit;
	for(vit = posts.begin(); vit != posts.end(); vit++)
	{
		RsPostedPost& p = *vit;

		// modify post content
		if(mPosts.find(p.mMeta.mMsgId) != mPosts.end())
		{
			std::cerr << "PostedListWidget::updateCurrentDisplayComplete() updating MsgId: " << p.mMeta.mMsgId;
			std::cerr << std::endl;

			mPosts[p.mMeta.mMsgId]->setContent(p);
		}
		else
		{
			std::cerr << "PostedListWidget::updateCurrentDisplayComplete() loading New MsgId: " << p.mMeta.mMsgId;
			std::cerr << std::endl;
			/* insert new entry */
			loadPost(p);
		}
	}

	time_t now = time(NULL);
	QMap<RsGxsMessageId, PostedItem*>::iterator pit;
	for(pit = mPosts.begin(); pit != mPosts.end(); pit++)
	{
		(*pit)->post().calculateScores(now);
	}

	applyRanking();

	mStateHelper->setLoading(TOKEN_USER_TYPE_POST, false);
	emit groupChanged(this);
}

//void PostedListWidget::acknowledgeSubscribeChange(const uint32_t &token)
//{
//	std::cerr << "PostedListWidget::acknowledgeSubscribeChange()";
//	std::cerr << std::endl;

//	std::vector<RsPostedPost> posts;
//	RsGxsGroupId groupId;
//	rsPosted->acknowledgeGrp(token, groupId);

//	insertGroups();
//}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PostedListWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mPostedQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case TOKEN_USER_TYPE_TOPIC:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						loadGroupData(req.mToken);
						break;
					default:
						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
						break;
				}
				break;
			case TOKEN_USER_TYPE_POST:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_ACK:
						acknowledgePostMsg(req.mToken);
						break;
					case RS_TOKREQ_ANSTYPE_DATA:
						loadPostData(req.mToken);
						break;
					default:
						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
						break;
				}
				break;
			case TOKEN_USER_TYPE_VOTE:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_ACK:
						acknowledgeVoteMsg(req.mToken);
						break;
					default:
						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
						break;
				}
				break;
			case TOKEN_USER_TYPE_POST_MOD:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						updateCurrentDisplayComplete(req.mToken);
						break;
					default:
						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
						break;
				}
				break;
//			case TOKEN_USER_TYPE_POST_RANKINGS:
//				switch(req.mAnsType)
//				{
//					case RS_TOKREQ_ANSTYPE_DATA:
//						//loadRankings(req.mToken);
//						break;
//					default:
//						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
//						break;
//				}
//			case TOKEN_USER_TYPE_SUBSCRIBE_CHANGE:
//				acknowledgeSubscribeChange(req.mToken);
//				break;
			default:
				std::cerr << "PostedListWidget::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
