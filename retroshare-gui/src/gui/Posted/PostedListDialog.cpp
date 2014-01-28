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

#include "PostedListDialog.h"

#include "PostedGroupDialog.h"
#include "PostedCreatePostDialog.h"
#include "PostedDialog.h"
#include "PostedItem.h"
#include "PostedUserTypes.h"

#include <iostream>

#include <QMenu>
#include <QMessageBox>

/****************************************************************
 */

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"

/* Images for TreeWidget */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"

// token types to deal with

#define POSTED_DEFAULT_LISTING_LENGTH 10
#define POSTED_MAX_INDEX              10000

/** Constructor */
PostedListDialog::PostedListDialog(QWidget *parent)
: RsGxsUpdateBroadcastPage(rsPosted, parent), GxsServiceDialog(dynamic_cast<GxsCommentContainer *>(parent))
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* Setup Queue */
	mPostedQueue = new TokenQueue(rsPosted->getTokenService(), this);

	connect( ui.groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupListCustomPopupMenu(QPoint)));
	connect( ui.groupTreeWidget, SIGNAL(treeCurrentItemChanged(QString)), this, SLOT(changedTopic(QString)));

	connect(ui.hotSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui.newSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui.topSortButton, SIGNAL(clicked()), this, SLOT(getRankings()));
	connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(showNext()));
	connect(ui.prevButton, SIGNAL(clicked()), this, SLOT(showPrev()));

	// default sort method.
	mSortMethod = RsPosted::HotRankType;
	mLastSortMethod = RsPosted::TopRankType; // to be different.
	mPostIndex = 0;
	mPostShow = POSTED_DEFAULT_LISTING_LENGTH;
	
	/* Initialize group tree */
	QToolButton *newTopicButton = new QToolButton(this);
	newTopicButton->setIcon(QIcon(":/images/posted_add_24.png"));
	newTopicButton->setToolTip(tr("Create New Topic"));
	connect(newTopicButton, SIGNAL(clicked()), this, SLOT(newTopic()));
	ui.groupTreeWidget->addToolButton(newTopicButton);

	/* create posted tree */
	yourTopics = ui.groupTreeWidget->addCategoryItem(tr("My Topics"), QIcon(IMAGE_FOLDER), true);
	subscribedTopics = ui.groupTreeWidget->addCategoryItem(tr("Subscribed Topics"), QIcon(IMAGE_FOLDERRED), true);
	popularTopics = ui.groupTreeWidget->addCategoryItem(tr("Popular Topics"), QIcon(IMAGE_FOLDERGREEN), false);
	otherTopics = ui.groupTreeWidget->addCategoryItem(tr("Other Topics"), QIcon(IMAGE_FOLDERYELLOW), false);

	ui.hotSortButton->setChecked(true);

	connect(ui.submitPostButton, SIGNAL(clicked()), this, SLOT(newPost()));
}

void PostedListDialog::showNext()
{
	mPostIndex += mPostShow;
	if (mPostIndex > POSTED_MAX_INDEX)
		mPostIndex = POSTED_MAX_INDEX;
	applyRanking();
	updateShowText();
}

void PostedListDialog::showPrev()
{
	mPostIndex -= mPostShow;
	if (mPostIndex < 0)
		mPostIndex = 0;
	applyRanking();
	updateShowText();
}

void PostedListDialog::updateShowText()
{
	QString showText = tr("Showing");
	showText += " ";
	showText += QString::number(mPostIndex + 1);
	showText += "-";
	showText += QString::number(mPostIndex + mPostShow);
	ui.showLabel->setText(showText);
}

void PostedListDialog::getRankings()
{
	if(mCurrTopicId.empty())
		return;

	std::cerr << "PostedListDialog::getRankings()";
	std::cerr << std::endl;

	int oldSortMethod = mSortMethod;

	QObject* button = sender();
	if(button == ui.hotSortButton)
	{
		mSortMethod = RsPosted::HotRankType;
	}
	else if(button == ui.topSortButton)
	{
		mSortMethod = RsPosted::TopRankType;
	}
	else if(button == ui.newSortButton)
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

void PostedListDialog::groupListCustomPopupMenu(QPoint /*point*/)
{
	if (mCurrTopicId.empty())
	{
		return;
	}

	uint32_t subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mCurrTopicId));

	QMenu contextMnu(this);

	//bool isAdmin = IS_GROUP_ADMIN(subscribeFlags);
	//bool isPublisher = IS_GROUP_PUBLISHER(subscribeFlags);
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);

	QAction *action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Submit Post"), this, SLOT(newPost()));
	action->setEnabled(isSubscribed);
	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Subscribe"), this, SLOT(subscribeTopic()));
	action->setEnabled(!isSubscribed);
	action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Unsubscribe"), this, SLOT(unsubscribeTopic()));
	action->setEnabled(isSubscribed);

	contextMnu.exec(QCursor::pos());
}

void PostedListDialog::newPost()
{
	if(mCurrTopicId.empty())
		return;

	uint32_t subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mCurrTopicId));
	bool isSubscribed = IS_GROUP_SUBSCRIBED(subscribeFlags);

	if (isSubscribed)
	{
		PostedCreatePostDialog cp(mPostedQueue, rsPosted, mCurrTopicId, this);
		cp.exec();
	}
}

void PostedListDialog::unsubscribeTopic()
{
	std::cerr << "PostedListDialog::unsubscribeTopic()";
	std::cerr << std::endl;

	if(mCurrTopicId.empty())
		return;

	uint32_t token;
	rsPosted->subscribeToGroup(token, mCurrTopicId, false);
	mPostedQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_SUBSCRIBE_CHANGE);
}

void PostedListDialog::subscribeTopic()
{
	std::cerr << "PostedListDialog::subscribeTopic()";
	std::cerr << std::endl;

	if(mCurrTopicId.empty())
		return;

	uint32_t token;
	rsPosted->subscribeToGroup(token, mCurrTopicId, true);
	mPostedQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, TOKEN_USER_TYPE_SUBSCRIBE_CHANGE);
}

void PostedListDialog::submitVote(const RsGxsGrpMsgIdPair &msgId, bool up)
{
	/* must grab AuthorId from Layout */
	RsGxsId authorId;
	if (!ui.idChooser->getChosenId(authorId))
	{
		std::cerr << "PostedListDialog::createPost() ERROR GETTING AuthorId!, Vote Failed";
		std::cerr << std::endl;

		QMessageBox::warning(this, tr("RetroShare"),tr("Please create or choose a Signing Id before Voting"), QMessageBox::Ok, QMessageBox::Ok);

		return;
	}

	RsGxsVote vote;

	vote.mMeta.mGroupId = msgId.first;
	vote.mMeta.mThreadId = msgId.second;
	vote.mMeta.mParentId = msgId.second;
	vote.mMeta.mAuthorId = authorId;

	if (up)
	{
		vote.mVoteType = GXS_VOTE_UP;
	}
	else
	{
		vote.mVoteType = GXS_VOTE_DOWN;
	}

	std::cerr << "PostedListDialog::submitVote()";
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
// Overloaded from FeedHolder.
QScrollArea *PostedListDialog::getScrollArea()
{
	return ui.scrollArea;
}

void PostedListDialog::deleteFeedItem(QWidget */*item*/, uint32_t /*type*/)
{
	std::cerr << "PostedListDialog::deleteFeedItem() Nah";
	std::cerr << std::endl;
	return;
}

void PostedListDialog::openChat(std::string /*peerId*/)
{
	std::cerr << "PostedListDialog::openChat() Nah";
	std::cerr << std::endl;
	return;
}

void PostedListDialog::openComments(uint32_t /*feed_type*/, const RsGxsGroupId &groupId, const RsGxsMessageId &msgId, const QString &title)
{
	commentLoad(groupId, msgId, title);
}

/*****************************************************************************************/

void PostedListDialog::updateDisplay(bool /*complete*/)
{
	std::cerr << "rsPosted->updateDisplay()";
	std::cerr << std::endl;

	/* update List */
	insertGroups();

	std::map<RsGxsGroupId, std::vector<RsGxsMessageId> > &msgs = getMsgIds();
	if(!msgs.empty())
	{
		std::cerr << "rsPosted->msgsChanged():";
		std::cerr << std::endl;

		std::map<RsGxsGroupId, std::vector<RsGxsMessageId> >::iterator mit;
		mit = msgs.find(mCurrTopicId);
		if(mit != msgs.end())
		{
			std::cerr << "current Group -> updating Displayed Items";
			std::cerr << std::endl;
			updateDisplayedItems(mit->second);
		}
	}
}

void PostedListDialog::updateDisplayedItems(const std::vector<RsGxsMessageId> &msgIds)
{
	RsTokReqOptions opts;

	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;

	GxsMsgReq msgs;
	msgs[mCurrTopicId] = msgIds;

	std::cerr << "PostedListDialog::updateDisplayedItems(" << mCurrTopicId << ")";
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

void PostedListDialog::changedTopic(const QString &id)
{
	mCurrTopicId = id.toStdString();
	insertThreads();
}

/*********************** **** **** **** ***********************/
/** New / Edit Groups		  ********************************/
/*********************** **** **** **** ***********************/

void PostedListDialog::newTopic()
{
	PostedGroupDialog cf (mPostedQueue, this);
	cf.exec ();
}
	
void PostedListDialog::showGroupDetails()
{
	if (mCurrTopicId.empty()) 
	{
		return;
	}

	PostedGroupDialog cf(mPostedQueue, rsPosted->getTokenService(), GxsGroupDialog::MODE_SHOW, mCurrTopicId, this);
	cf.exec ();
}

void PostedListDialog::insertGroups()
{
	requestGroupSummary();
}

void PostedListDialog::requestGroupSummary()
{
	std::cerr << "PostedListDialog::requestGroupSummary()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	uint32_t token;
	mPostedQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_USER_TYPE_TOPIC);

	/* refresh Id Chooser Too */
	RsGxsId currentId = "";
	ui.idChooser->getChosenId(currentId);
	ui.idChooser->loadIds(IDCHOOSER_ID_REQUIRED, currentId);
}

void PostedListDialog::acknowledgeGroup(const uint32_t &token)
{
	RsGxsGroupId grpId;
	rsPosted->acknowledgeGrp(token, grpId);

	if(!grpId.empty())
	{
		RsTokReqOptions opts;
		opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
		uint32_t reqToken;
		mPostedQueue->requestGroupInfo(reqToken, RS_TOKREQ_ANSTYPE_SUMMARY, opts, TOKEN_USER_TYPE_TOPIC);
	}
}

void PostedListDialog::acknowledgePostMsg(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;

	// just acknowledge, don't load anything
	rsPosted->acknowledgeMsg(token, msgId);
}

void PostedListDialog::loadGroupSummary(const uint32_t &token)
{
	std::cerr << "PostedListDialog::loadGroupSummary()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> groupInfo;
	rsPosted->getGroupSummary(token, groupInfo);

	if (groupInfo.size() > 0)
	{
		insertGroupData(groupInfo);
	}
	else
	{
		std::cerr << "PostedListDialog::loadGroupSummary() ERROR No Groups...";
		std::cerr << std::endl;
	}
}

void PostedListDialog::loadPostData(const uint32_t &token)
{
	loadGroupThreadData_InsertThreads(token);
}

void PostedListDialog::acknowledgeVoteMsg(const uint32_t &token)
{
	RsGxsGrpMsgIdPair msgId;

	rsPosted->acknowledgeVote(token, msgId);
}

void PostedListDialog::loadVoteData(const uint32_t &/*token*/)
{
	return;
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListDialog::requestGroupSummary_CurrentForum(const std::string &forumId)
{
	RsTokReqOptions opts;
	
	std::list<std::string> grpIds;
	grpIds.push_back(forumId);

	std::cerr << "PostedListDialog::requestGroupSummary_CurrentForum(" << forumId << ")";
	std::cerr << std::endl;

	uint32_t token;	
	mPostedQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, POSTEDDIALOG_CURRENTFORUM);
}

void PostedListDialog::loadGroupSummary_CurrentForum(const uint32_t &token)
{
	std::cerr << "PostedListDialog::loadGroupSummary_CurrentForum()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> groupInfo;
	rsPosted->getGroupSummary(token, groupInfo);

	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		//insertForumThreads(fi);
	}
	else
	{
		std::cerr << "PostedListDialog::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
		std::cerr << std::endl;
	}
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void PostedListDialog::insertThreads()
{
	loadCurrentTopicThreads(mCurrTopicId);
}

void PostedListDialog::loadCurrentTopicThreads(const std::string &topicId)
{
	std::cerr << "PostedListDialog::loadCurrentForumThreads(" << topicId << ")";
	std::cerr << std::endl;

	if (topicId.empty())
	{
		std::cerr << "PostedListDialog::loadCurrentForumThreads() Empty GroupId .. ignoring Req";
		std::cerr << std::endl;
		return;
	}

	clearPosts();

	/* initiate loading */
	requestGroupThreadData_InsertThreads(topicId);
}

void PostedListDialog::requestGroupThreadData_InsertThreads(const std::string &groupId)
{
	RsTokReqOptions opts;

	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = RS_TOKREQOPT_MSG_LATEST;
	
	std::list<RsGxsGroupId> grpIds;
	grpIds.push_back(groupId);

	std::cerr << "PostedListDialog::requestGroupThreadData_InsertThreads(" << groupId << ")";
	std::cerr << std::endl;

	uint32_t token;	
	mPostedQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, TOKEN_USER_TYPE_POST);
}

void PostedListDialog::loadGroupThreadData_InsertThreads(const uint32_t &token)
{
	std::cerr << "PostedListDialog::loadGroupThreadData_InsertThreads()";
	std::cerr << std::endl;

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
}

void PostedListDialog::loadPost(const RsPostedPost &post)
{
	PostedItem *item = new PostedItem(this, 0, post, true);
	connect(item, SIGNAL(vote(RsGxsGrpMsgIdPair,bool)), this, SLOT(submitVote(RsGxsGrpMsgIdPair,bool)));
	mPosts.insert(post.mMeta.mMsgId, item);
	//QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	//alayout->addWidget(item);
	mPostList.push_back(item);
}

bool CmpPIHot(const PostedItem *a, const PostedItem *b)
{
	const RsPostedPost &postA = a->getPost();
	const RsPostedPost &postB = b->getPost();

	if (postA.mHotScore == postB.mHotScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mHotScore > postB.mHotScore);
}

bool CmpPITop(const PostedItem *a, const PostedItem *b)
{
	const RsPostedPost &postA = a->getPost();
	const RsPostedPost &postB = b->getPost();

	if (postA.mTopScore == postB.mTopScore)
	{
		return (postA.mNewScore > postB.mNewScore);
	}

	return (postA.mTopScore > postB.mTopScore);
}

bool CmpPINew(const PostedItem *a, const PostedItem *b)
{
	return (a->getPost().mNewScore > b->getPost().mNewScore);
}

void PostedListDialog::applyRanking()
{
	/* uses current settings to sort posts, then add to layout */
	std::cerr << "PostedListDialog::applyRanking()";
	std::cerr << std::endl;

	shallowClearPosts();

	/* sort */
	switch(mSortMethod)
	{
		default:
		case RsPosted::HotRankType:
			std::cerr << "PostedListDialog::applyRanking() HOT";
			std::cerr << std::endl;
			mPostList.sort(CmpPIHot);
			break;
		case RsPosted::NewRankType:
			std::cerr << "PostedListDialog::applyRanking() NEW";
			std::cerr << std::endl;
			mPostList.sort(CmpPINew);
			break;
		case RsPosted::TopRankType:
			std::cerr << "PostedListDialog::applyRanking() TOP";
			std::cerr << std::endl;
			mPostList.sort(CmpPITop);
			break;
	}
	mLastSortMethod = mSortMethod;

	std::cerr << "PostedListDialog::applyRanking() Sorted mPostList";
	std::cerr << std::endl;

	/* go through list (skipping out-of-date items) to get */
	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	int counter = 0;
	time_t min_ts = 0;
	std::list<PostedItem *>::iterator it;
	for(it = mPostList.begin(); it != mPostList.end(); it++)
	{
		PostedItem *item = (*it);
		std::cerr << "PostedListDialog::applyRanking() Item: " << item;
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

	std::cerr << "PostedListDialog::applyRanking() Loaded New Order";
	std::cerr << std::endl;

	// trigger a redraw.
	ui.scrollAreaWidgetContents->update();
}

void PostedListDialog::clearPosts()
{
	std::cerr << "PostedListDialog::clearPosts()" << std::endl;

	std::list<PostedItem *> postedItems;
	std::list<PostedItem *>::iterator pit;

	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PostedListDialog::clearPosts() missing litem";
			std::cerr << std::endl;
			continue;
		}

		PostedItem *item = dynamic_cast<PostedItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PostedListDialog::clearPosts() item: " << item;
			std::cerr << std::endl;

			postedItems.push_back(item);
		}
		else
		{
			std::cerr << "PostedListDialog::clearPosts() Found Child, which is not a PostedItem???";
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

void PostedListDialog::shallowClearPosts()
{
	std::cerr << "PostedListDialog::shallowClearPosts()" << std::endl;

	std::list<PostedItem *> postedItems;
	std::list<PostedItem *>::iterator pit;

	QLayout *alayout = ui.scrollAreaWidgetContents->layout();
	int count = alayout->count();
	for(int i = 0; i < count; i++)
	{
		QLayoutItem *litem = alayout->itemAt(i);
		if (!litem)
		{
			std::cerr << "PostedListDialog::shallowClearPosts() missing litem";
			std::cerr << std::endl;
			continue;
		}

		PostedItem *item = dynamic_cast<PostedItem *>(litem->widget());
		if (item)
		{
			std::cerr << "PostedListDialog::shallowClearPosts() item: " << item;
			std::cerr << std::endl;

			postedItems.push_back(item);
		}
		else
		{
			std::cerr << "PostedListDialog::shallowClearPosts() Found Child, which is not a PostedItem???";
			std::cerr << std::endl;
		}
	}

	for(pit = postedItems.begin(); pit != postedItems.end(); pit++)
	{
		PostedItem *item = *pit;
		alayout->removeWidget(item);
	}
}

void PostedListDialog::updateCurrentDisplayComplete(const uint32_t &token)
{
	std::cerr << "PostedListDialog::updateCurrentDisplayComplete()";
	std::cerr << std::endl;

	std::vector<RsPostedPost> posts;
	rsPosted->getPostData(token, posts);

	std::vector<RsPostedPost>::iterator vit;
	for(vit = posts.begin(); vit != posts.end(); vit++)
	{
		RsPostedPost& p = *vit;

		// modify post content
		if(mPosts.find(p.mMeta.mMsgId) != mPosts.end())
		{
			std::cerr << "PostedListDialog::updateCurrentDisplayComplete() updating MsgId: " << p.mMeta.mMsgId;
			std::cerr << std::endl;

			mPosts[p.mMeta.mMsgId]->setContent(p);
		}
		else
		{
			std::cerr << "PostedListDialog::updateCurrentDisplayComplete() loading New MsgId: " << p.mMeta.mMsgId;
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
}

void PostedListDialog::acknowledgeSubscribeChange(const uint32_t &token)
{
	std::cerr << "PostedListDialog::acknowledgeSubscribeChange()";
	std::cerr << std::endl;

	std::vector<RsPostedPost> posts;
	RsGxsGroupId groupId;
	rsPosted->acknowledgeGrp(token, groupId);

	insertGroups();
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
	
void PostedListDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "PostedListDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
				
	if (queue == mPostedQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case TOKEN_USER_TYPE_TOPIC:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_SUMMARY:
						loadGroupSummary(req.mToken);
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
			case TOKEN_USER_TYPE_POST_RANKINGS:
				switch(req.mAnsType)
				{
					case RS_TOKREQ_ANSTYPE_DATA:
						//loadRankings(req.mToken);
						break;
					default:
						std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
						break;
				}
			case TOKEN_USER_TYPE_SUBSCRIBE_CHANGE:
				acknowledgeSubscribeChange(req.mToken);
				break;
			default:
				std::cerr << "PostedListDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}

	/* now switch on req */
	switch(req.mType)
	{
		case TOKENREQ_GROUPINFO:
			switch(req.mAnsType)
			{
			case RS_TOKREQ_ANSTYPE_ACK:
				acknowledgeGroup(req.mToken);
				break;
			}
			break;
	}
}

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
/**************************** Groups **********************/

void PostedListDialog::groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(groupInfo.mGroupId);
	groupItemInfo.name = QString::fromUtf8(groupInfo.mGroupName.c_str());
	//groupItemInfo.description = QString::fromUtf8(groupInfo.forumDesc);
	groupItemInfo.popularity = groupInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(groupInfo.mLastPost);
	groupItemInfo.subscribeFlags = groupInfo.mSubscribeFlags;
}

void PostedListDialog::insertGroupData(const std::list<RsGroupMetaData> &groupList)
{
	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = groupList.begin(); it != groupList.end(); it++) 
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		groupInfoToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_SUBSCRIBED(flags))
		{
			if (IS_GROUP_ADMIN(flags) || IS_GROUP_PUBLISHER(flags))
			{
				adminList.push_back(groupItemInfo);
			}
			else
			{
				subList.push_back(groupItemInfo);
			}
		}
		else
		{
			popMap.insert(std::make_pair(it->mPop, groupItemInfo));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++) ;
	if (rit != popMap.rend()) {
		popLimit = rit->first;
	}

	for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) {
		if (rit->second.popularity < (int) popLimit) {
			otherList.append(rit->second);
		} else {
			popList.append(rit->second);
		}
	}

	/* now we can add them in as a tree! */
	ui.groupTreeWidget->fillGroupItems(yourTopics, adminList);
	ui.groupTreeWidget->fillGroupItems(subscribedTopics, subList);
	ui.groupTreeWidget->fillGroupItems(popularTopics, popList);
	ui.groupTreeWidget->fillGroupItems(otherTopics, otherList);
}

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
