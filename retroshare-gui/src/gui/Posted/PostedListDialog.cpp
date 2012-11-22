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

#include "gui/Posted/PostedGroupDialog.h"
#include "gui/Posted/PostedCreatePostDialog.h"
#include "gui/Posted/PostedDialog.h"

#include <retroshare/rsposted.h>
#include <gxs/rsgxsflags.h>

#include <iostream>
#include <sstream>
#include <algorithm>

#include <QTimer>
#include <QMenu>
#include <QMessageBox>

/****************************************************************
 */

//#define DEBUG_FORUMS

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"

/* Images for TreeWidget */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"
#define IMAGE_FORUM          ":/images/konversation16.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

/** Constructor */
PostedListDialog::PostedListDialog(CommentHolder *commentHolder, QWidget *parent)
: RsAutoUpdatePage(1000,parent), mCommentHolder(commentHolder)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    /* Setup Queue */
    mPostedQueue = new TokenQueue(rsPosted->getTokenService(), this);

    connect( ui.groupTreeWidget, SIGNAL( treeCustomContextMenuRequested( QPoint ) ), this, SLOT( groupListCustomPopupMenu( QPoint ) ) );

    connect( ui.groupTreeWidget, SIGNAL( treeCurrentItemChanged(QString) ), this, SLOT( changedTopic(QString) ) );

    /* create posted tree */
    yourTopics = ui.groupTreeWidget->addCategoryItem(tr("Your Topics"), QIcon(IMAGE_FOLDER), true);
    subscribedTopics = ui.groupTreeWidget->addCategoryItem(tr("Subscribed Topics"), QIcon(IMAGE_FOLDERRED), true);
    popularTopics = ui.groupTreeWidget->addCategoryItem(tr("Popular Topics"), QIcon(IMAGE_FOLDERGREEN), false);
    otherTopics = ui.groupTreeWidget->addCategoryItem(tr("Other Topics"), QIcon(IMAGE_FOLDERYELLOW), false);

    ui.hotSortButton->setChecked(true);
    mSortButton = ui.hotSortButton;

    connect( ui.newTopicButton, SIGNAL( clicked() ), this, SLOT( newTopic() ) );
}


void PostedListDialog::groupListCustomPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("Submit Post"), this, SLOT(newPost()));
    action->setDisabled (mCurrTopicId.empty());

    contextMnu.exec(QCursor::pos());
}

void PostedListDialog::newPost()
{
    PostedCreatePostDialog cp(mPostedQueue, rsPosted, mCurrTopicId, this);
    cp.exec();
}

void PostedListDialog::showComments(const RsGxsMessageId &threadId)
{
    mCommentHolder->commentLoad(threadId);
}

void PostedListDialog::updateDisplay()
{
    std::list<std::string> groupIds;
    std::list<std::string>::iterator it;
    if (!rsPosted)
        return;

    if (rsPosted->updated())
    {
        /* update Forums List */
        insertGroups();
        insertThreads();
    }

}


void PostedListDialog::changedTopic(const QString &id)
{
    mCurrTopicId = id.toStdString();
    insertThreads();
}


/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void PostedListDialog::newTopic()
{
        PostedGroupDialog cf (mPostedQueue, rsPosted, this);
	cf.exec ();
}
	
void PostedListDialog::showGroupDetails()
{
	if (mCurrTopicId.empty()) 
	{
		return;
	}
	
        PostedGroupDialog cf(mGroups[mCurrTopicId], GXS_GROUP_DIALOG_SHOW_MODE, this);
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

        std::list<std::string> ids;
        RsTokReqOptions opts;
	uint32_t token;
        mPostedQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, ids, POSTEDDIALOG_LISTING);
}

void PostedListDialog::acknowledgeGroup(const uint32_t &token)
{
    RsGxsGroupId grpId;
    rsPosted->acknowledgeGrp(token, grpId);

    if(!grpId.empty())
    {
        std::list<RsGxsGroupId> grpIds;
        grpIds.push_back(grpId);

        RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
        uint32_t reqToken;
        mPostedQueue->requestGroupInfo(reqToken, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, 0);
    }
}

void PostedListDialog::acknowledgeMsg(const uint32_t &token)
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
        mPostedQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, POSTEDDIALOG_INSERTTHREADS);
}


void PostedListDialog::loadGroupThreadData_InsertThreads(const uint32_t &token)
{
    std::cerr << "PostedListDialog::loadGroupThreadData_InsertThreads()";
    std::cerr << std::endl;

    clearPosts();

    PostedPostResult result;
    rsPosted->getPost(token, result);
    std::vector<RsPostedPost>& posts = result[mCurrTopicId];
    std::vector<RsPostedPost>::iterator vit = posts.begin();

    for(; vit != posts.end(); vit++)
    {
        loadPost(*vit);
    }
}

void PostedListDialog::loadPost(const RsPostedPost &post)
{
    PostedItem *item = new PostedItem(this, post);
    QLayout *alayout = ui.scrollAreaWidgetContents->layout();
    alayout->addWidget(item);
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
                switch(req.mType)
		{
                    case TOKENREQ_GROUPINFO:
                        switch(req.mAnsType)
                        {
                            case RS_TOKREQ_ANSTYPE_ACK:
                                acknowledgeGroup(req.mToken);
                                break;
                            case RS_TOKREQ_ANSTYPE_SUMMARY:
                                loadGroupSummary(req.mToken);
                            default:
                                std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
                                break;
                        }
                    break;
                    case TOKENREQ_MSGINFO:
                        switch(req.mAnsType)
                        {
                            case RS_TOKREQ_ANSTYPE_ACK:
                                acknowledgeMsg(req.mToken);
                                break;
                            case RS_TOKREQ_ANSTYPE_DATA:
                                loadPostData(req.mToken);
                                break;
                            default:
                                std::cerr << "Error, unexpected anstype:" << req.mAnsType << std::endl;
                                break;
                        }

                    default:
                            std::cerr << "PostedListDialog::loadRequest() ERROR: INVALID TYPE";
                            std::cerr << std::endl;
                            break;
		}
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


}

void PostedListDialog::insertGroupData(const std::list<RsGroupMetaData> &groupList)
{

    std::list<RsGroupMetaData>::const_iterator it;

    QList<GroupItemInfo> adminList;
    QList<GroupItemInfo> subList;
    QList<GroupItemInfo> popList;
    QList<GroupItemInfo> otherList;
    std::multimap<uint32_t, GroupItemInfo> popMap;

    for (it = groupList.begin(); it != groupList.end(); it++) {
        /* sort it into Publish (Own), Subscribed, Popular and Other */
        uint32_t flags = it->mSubscribeFlags;

    GroupItemInfo groupItemInfo;
    groupInfoToGroupItemInfo(*it, groupItemInfo);

  //  if (flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
    adminList.push_back(groupItemInfo);
  //  } else if (flags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) {
                /* subscribed forum */
 //   subList.push_back(groupItemInfo);
 //   } else {
                /* rate the others by popularity */
//    popMap.insert(std::make_pair(it->mPop, groupItemInfo));
        }
//    }

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


