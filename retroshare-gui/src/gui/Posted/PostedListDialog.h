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

#ifndef MRK_POSTED_LIST_DIALOG_H
#define MRK_POSTED_LIST_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_PostedListDialog.h"

#include <retroshare/rsposted.h>

#include <map>

#include "gui/Posted/PostedItem.h"
#include "gui/common/GroupTreeWidget.h"

#include "util/TokenQueue.h"

#include "retroshare-gui/RsAutoUpdatePage.h"

/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

#define 	POSTEDDIALOG_LISTING			1
#define 	POSTEDDIALOG_CURRENTFORUM		2
#define 	POSTEDDIALOG_INSERTTHREADS		3
#define 	POSTEDDIALOG_INSERTCHILD		4
#define 	POSTEDDIALOG_INSERT_POST		5
#define 	POSTEDDIALOG_REPLY_MESSAGE		6

class PostedListDialog : public RsAutoUpdatePage, public PostedHolder, public TokenResponse
{
  Q_OBJECT

public:
	PostedListDialog(QWidget *parent = 0);

    virtual void deletePostedItem(PostedItem *, uint32_t ptype) { return; }
    virtual void notifySelection(PostedItem *item, int ptype) { return; }
    virtual void requestComments(std::string threadId);

signals:
    void loadComments( std::string );

private slots:

    void    groupListCustomPopupMenu( QPoint /*point*/ );
    void    changedTopic(const QString &id);

    void newTopic();
    void showGroupDetails();
    void newPost();

private:

    void 	clearPosts();

    void 	updateDisplay();
    void 	loadPost(const RsPostedPost &post);

    void 	insertGroups();
    void 	requestGroupSummary();
    void        acknowledgeGroup(const uint32_t &token);
    void 	loadGroupSummary(const uint32_t &token);

    void	requestGroupSummary_CurrentForum(const std::string &forumId);
    void 	loadGroupSummary_CurrentForum(const uint32_t &token);


    void        acknowledgeMsg(const uint32_t &token);
    void        loadPostData(const uint32_t &token);
    void 	insertThreads();
    void 	loadCurrentTopicThreads(const std::string &forumId);
    void 	requestGroupThreadData_InsertThreads(const std::string &forumId);
    void 	loadGroupThreadData_InsertThreads(const uint32_t &token);


    void 	insertGroupData(const std::list<RsGroupMetaData> &groupList);
    void 	groupInfoToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo);

    void loadRequest(const TokenQueue *queue, const TokenRequest &req);


private:

    QTreeWidgetItem *yourTopics;
    QTreeWidgetItem *subscribedTopics;
    QTreeWidgetItem *popularTopics;
    QTreeWidgetItem *otherTopics;

    QAbstractButton *mSortButton;

    bool mThreadLoading;
    RsGxsGroupId mCurrTopicId;

    QMap<RsGxsGroupId, RsPostedGroup> mGroups;
    TokenQueue *mPostedQueue;

    /* UI - from Designer */
    Ui::PostedListDialog ui;

};

#endif

