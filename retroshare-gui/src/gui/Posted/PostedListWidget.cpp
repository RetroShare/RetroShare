/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedListWidget.cpp                          *
 *                                                                             *
 * Copyright (C) 2013 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include <QMessageBox>

#include "PostedListWidget.h"
#include "ui_PostedListWidget.h"

#include "PostedCreatePostDialog.h"
#include "PostedItem.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rsposted.h>

#define POSTED_DEFAULT_LISTING_LENGTH 10
#define POSTED_MAX_INDEX	      10000

/** Constructor */
PostedListWidget::PostedListWidget(const RsGxsGroupId &postedId, QWidget *parent)
    : GxsMessageFramePostWidget(rsPosted, parent),
      ui(new Ui::PostedListWidget)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	/* Setup UI helper */
	mStateHelper->addWidget(mTokenTypeAllPosts, ui->comboBox);

	mStateHelper->addWidget(mTokenTypePosts, ui->comboBox);

	mStateHelper->addWidget(mTokenTypeGroupData, ui->submitPostButton);
	mStateHelper->addWidget(mTokenTypeGroupData, ui->subscribeToolButton);

	connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(showNext()));
	connect(ui->prevButton, SIGNAL(clicked()), this, SLOT(showPrev()));
	connect(ui->subscribeToolButton, SIGNAL(subscribe(bool)), this, SLOT(subscribeGroup(bool)));
	
	connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(getRankings(int)));


	// default sort method.
	mSortMethod = RsPosted::HotRankType;
	mLastSortMethod = RsPosted::TopRankType; // to be different.
	mPostIndex = 0;
	mPostShow = POSTED_DEFAULT_LISTING_LENGTH;

	mTokenTypeVote = nextTokenType();

	/* fill in the available OwnIds for signing */
	ui->idChooser->loadIds(IDCHOOSER_ID_REQUIRED, RsGxsId());
	
	int S = QFontMetricsF(font()).height() ;

	ui->submitPostButton->setIconSize(QSize(S*1.5,S*1.5));
	ui->comboBox->setIconSize(QSize(S*1.5,S*1.5));

	connect(ui->submitPostButton, SIGNAL(clicked()), this, SLOT(newPost()));
	
	ui->subscribeToolButton->setToolTip(tr( "<p>Subscribing to the links will gather \
	                                        available posts from your subscribed friends, and make the \
	                                        links visible to all other friends.</p><p>Afterwards you can unsubscribe from the context menu of the links list at left.</p>"));

	/* load settings */
	processSettings(true);

	/* Initialize GUI */
	setGroupId(postedId);
}

PostedListWidget::~PostedListWidget()
{
	// save settings
	processSettings(false);

	delete(ui);
}

void PostedListWidget::processSettings(bool /*load*/)
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

QIcon PostedListWidget::groupIcon()
{
	if (mStateHelper->isLoading(mTokenTypeGroupData) || mStateHelper->isLoading(mTokenTypeAllPosts)) {
//		return QIcon(":/images/kalarm.png");
	}

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

void PostedListWidget::openComments(uint32_t /*feed_type*/, const RsGxsGroupId &groupId, const QVector<RsGxsMessageId> &versions, const RsGxsMessageId &msgId, const QString &title)
{
	emit loadComment(groupId, versions,msgId, title);
}

void PostedListWidget::newPost()
{
	if (groupId().isNull())
		return;

	if (!IS_GROUP_SUBSCRIBED(subscribeFlags())) {
		return;
	}

 	PostedCreatePostDialog *cp = new PostedCreatePostDialog(mTokenQueue, rsPosted, groupId(), this);
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
    QString showText ;
	showText += QString::number(mPostIndex + 1);
	showText += "-";
	showText += QString::number(mPostIndex + mPostShow);
	ui->showLabel->setText(showText);
}

void PostedListWidget::getRankings(int i)
{
	if (groupId().isNull())
		return;

	std::cerr << "PostedListWidget::getRankings()";
	std::cerr << std::endl;

	int oldSortMethod = mSortMethod;
	
	switch(i)
	{
	default:
	case 0:
		mSortMethod = RsPosted::HotRankType;
		break;
	case 1:
		mSortMethod = RsPosted::NewRankType;
		break;
	case 2:
		mSortMethod = RsPosted::TopRankType;
		break;
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
	rsPosted->createNewVote(token, vote);
	mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_ACK, mTokenTypeVote);
}

void PostedListWidget::subscribeGroup(bool subscribe)
{
	if (groupId().isNull()) {
		return;
	}

	uint32_t token;
	rsPosted->subscribeToGroup(token, groupId(), subscribe);
//	mTokenQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_TYPE_SUBSCRIBE_CHANGE);
}

/*****************************************************************************************/

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

void PostedListWidget::insertPostedDetails(const RsPostedGroup &group)
{
	mStateHelper->setWidgetEnabled(ui->submitPostButton, IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));
	ui->subscribeToolButton->setSubscribed(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags));
	ui->subscribeToolButton->setHidden(IS_GROUP_SUBSCRIBED(group.mMeta.mSubscribeFlags)) ;
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListWidget::loadPost(const RsPostedPost &post)
{
	/* Group is not always available because of the TokenQueue */
	RsPostedGroup dummyGroup;
	dummyGroup.mMeta.mGroupId = groupId();

	PostedItem *item = new PostedItem(this, 0, dummyGroup, post, true, false);
	connect(item, SIGNAL(vote(RsGxsGrpMsgIdPair,bool)), this, SLOT(submitVote(RsGxsGrpMsgIdPair,bool)));
	mPosts.insert(post.mMeta.mMsgId, item);
	//QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	//alayout->addWidget(item);
	mPostItems.push_back(item);
}

static bool CmpPIHot(const GxsFeedItem *a, const GxsFeedItem *b)
{
	const PostedItem *aa = dynamic_cast<const PostedItem*>(a);
	const PostedItem *bb = dynamic_cast<const PostedItem*>(b);

	if (!aa || !bb) {
		return true;
	}

	const RsPostedPost &postA = aa->getPost();
	const RsPostedPost &postB = bb->getPost();

	if (postA.mHotScore == postB.mHotScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mHotScore > postB.mHotScore);
}

static bool CmpPITop(const GxsFeedItem *a, const GxsFeedItem *b)
{
	const PostedItem *aa = dynamic_cast<const PostedItem*>(a);
	const PostedItem *bb = dynamic_cast<const PostedItem*>(b);

	if (!aa || !bb) {
		return true;
	}

	const RsPostedPost &postA = aa->getPost();
	const RsPostedPost &postB = bb->getPost();

	if (postA.mTopScore == postB.mTopScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mTopScore > postB.mTopScore);
}

static bool CmpPINew(const GxsFeedItem *a, const GxsFeedItem *b)
{
	const PostedItem *aa = dynamic_cast<const PostedItem*>(a);
	const PostedItem *bb = dynamic_cast<const PostedItem*>(b);

	if (!aa || !bb) {
		return true;
	}

	return (aa->getPost().mNewScore > bb->getPost().mNewScore);
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
			qSort(mPostItems.begin(), mPostItems.end(), CmpPIHot);
			break;
		case RsPosted::NewRankType:
			std::cerr << "PostedListWidget::applyRanking() NEW";
			std::cerr << std::endl;
			qSort(mPostItems.begin(), mPostItems.end(), CmpPINew);
			break;
		case RsPosted::TopRankType:
			std::cerr << "PostedListWidget::applyRanking() TOP";
			std::cerr << std::endl;
			qSort(mPostItems.begin(), mPostItems.end(), CmpPITop);
			break;
	}
	mLastSortMethod = mSortMethod;

	std::cerr << "PostedListWidget::applyRanking() Sorted mPostList";
	std::cerr << std::endl;

	/* go through list (skipping out-of-date items) to get */
	QLayout *alayout = ui->scrollAreaWidgetContents->layout();
	int counter = 0;
	time_t min_ts = 0;
	foreach (PostedItem *item, mPostItems)
	{
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
		++counter;
	}

	std::cerr << "PostedListWidget::applyRanking() Loaded New Order";
	std::cerr << std::endl;

	// trigger a redraw.
	ui->scrollAreaWidgetContents->update();
}

void PostedListWidget::blank()
{
    clearPosts();
}
void PostedListWidget::clearPosts()
{
	/* clear all messages */
	foreach (PostedItem *item, mPostItems) {
		delete(item);
	}
	mPostItems.clear();
	mPosts.clear();
}

bool PostedListWidget::navigatePostItem(const RsGxsMessageId & /*msgId*/)
{
	//TODO
	return false;
}

void PostedListWidget::shallowClearPosts()
{
	std::cerr << "PostedListWidget::shallowClearPosts()" << std::endl;

	std::list<PostedItem *> postedItems;
	std::list<PostedItem *>::iterator pit;

	QLayout *alayout = ui->scrollAreaWidgetContents->layout();
	int count = alayout->count();
	for(int i = 0; i < count; ++i)
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

	for(pit = postedItems.begin(); pit != postedItems.end(); ++pit)
	{
		PostedItem *item = *pit;
		alayout->removeWidget(item);
	}
}

bool PostedListWidget::insertGroupData(const uint32_t &token, RsGroupMetaData &metaData)
{
	std::vector<RsPostedGroup> groups;
	rsPosted->getGroupData(token, groups);

	if (groups.size() == 1)
	{
		insertPostedDetails(groups[0]);
		metaData = groups[0].mMeta;
		return true;
	}

	return false;
}

void PostedListWidget::insertAllPosts(const uint32_t &token, GxsMessageFramePostThread */*thread*/)
{
	std::vector<RsPostedPost> posts;
	rsPosted->getPostData(token, posts);

	std::vector<RsPostedPost>::iterator vit;
	for(vit = posts.begin(); vit != posts.end(); ++vit)
	{
		RsPostedPost& p = *vit;
		loadPost(p);
	}

	applyRanking();
}

void PostedListWidget::insertPosts(const uint32_t &token)
{
	std::vector<RsPostedPost> posts;
	rsPosted->getPostData(token, posts);

	std::vector<RsPostedPost>::iterator vit;
	for(vit = posts.begin(); vit != posts.end(); ++vit)
	{
		RsPostedPost& p = *vit;

		// modify post content
		if(mPosts.find(p.mMeta.mMsgId) != mPosts.end())
		{
			std::cerr << "PostedListWidget::updateCurrentDisplayComplete() updating MsgId: " << p.mMeta.mMsgId;
			std::cerr << std::endl;

			mPosts[p.mMeta.mMsgId]->setPost(p);
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
	for(pit = mPosts.begin(); pit != mPosts.end(); ++pit)
	{
		(*pit)->post().calculateScores(now);
	}

	applyRanking();
}

void PostedListWidget::setAllMessagesReadDo(bool read, uint32_t &token)
{
	if (groupId().isNull() || !IS_GROUP_SUBSCRIBED(subscribeFlags())) {
		return;
	}

	foreach (PostedItem *item, mPostItems) {
		RsGxsGrpMsgIdPair msgPair = std::make_pair(item->groupId(), item->messageId());

		rsPosted->setMessageReadStatus(token, msgPair, read);
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListWidget::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PostedListWidget::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mTokenQueue)
	{
		/* now switch on req */
		if (req.mUserType == mTokenTypeVote) {
			switch(req.mAnsType)
			{
				case RS_TOKREQ_ANSTYPE_ACK:
					acknowledgeVoteMsg(req.mToken);
					break;
				default:
					std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
					break;
			}
			return;
		}
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
	}

	GxsMessageFramePostWidget::loadRequest(queue, req);
}
