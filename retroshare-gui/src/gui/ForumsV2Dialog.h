/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _FORUMSV2DIALOG_H
#define _FORUMSV2DIALOG_H

#include <QThread>

#include "mainpage.h"
#include "RsAutoUpdatePage.h"
#include "ui_ForumsV2Dialog.h"

#include <inttypes.h>

#include "util/TokenQueue.h"

#include <retroshare/rsforumsv2.h>

class ForumInfo;


/* These are all the parameters that are required for thread loading. 
 * They are kept static for the load duration.
 */

class ForumsV2ThreadLoadParameters
{
	public:
		
		std::string ForumId;
    		std::string FocusMsgId;

		uint32_t SubscribeFlags;
		int ViewType;
		uint32_t FilterColumn;
	
		std::map<uint32_t, QTreeWidgetItem *> MsgTokens;
    		QList<QTreeWidgetItem*> Items;
    		QList<QTreeWidgetItem*> ItemToExpand;
		
		bool FillComplete;
		bool FlatView;
		bool UseChildTS;
		bool ExpandNewMessages;
};






class ForumsV2Dialog : public RsAutoUpdatePage, public TokenResponse 
{
  Q_OBJECT

public:
    ForumsV2Dialog(QWidget *parent = 0);
    ~ForumsV2Dialog();

    bool navigate(const std::string& forumId, const std::string& msgId);

    /* overloaded from RsAuthUpdatePage */
    virtual void updateDisplay();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    /** Create the context popup menu and it's submenus */
    void forumListCustomPopupMenu( QPoint point );
    void threadListCustomPopupMenu( QPoint point );
    void restoreForumKeys();
    void newforum();

    void changedForum(const QString &id);
    void changedThread();
    void clickedThread (QTreeWidgetItem *item, int column);

    void replytomessage();
	void replyMessageData(const RsForumV2Msg &msg);

    //void print();
    //void printpreview();

    //void removemessage();
    void markMsgAsRead();
    void markMsgAsReadChildren();
    void markMsgAsReadAll();
    void markMsgAsUnread();
    void markMsgAsUnreadAll();
    void markMsgAsUnreadChildren();
    void copyForumLink();
    void copyMessageLink();

    /* handle splitter */
    void togglethreadview();

    void createthread();
    void createmessage();

    void subscribeToForum();
    void unsubscribeToForum();

    void showForumDetails();
    void editForumDetails();

    void previousMessage ();
    void nextMessage ();
	 void nextUnreadMessage();
    void downloadAllFiles();

    void changedViewBox();

    void filterColumnChanged();
    void filterRegExpChanged();
    void clearFilter();

    void generateMassData();

    void fillThreadFinished();
    void fillThreadProgress(int current, int count);

    void shareKey();

private:
    void insertForums();
    void insertThreads();
    void insertPost();
	void insertPostData(const RsForumV2Msg &msg); // Second Half.

    void updateMessageSummaryList(std::string forumId);
    //void forumInfoToGroupItemInfo(const ForumInfo &forumInfo, GroupItemInfo &groupItemInfo);
    void forumInfoToGroupItemInfo(const RsGroupMetaData &forumInfo, GroupItemInfo &groupItemInfo);

    void forumSubscribe(bool subscribe);
    void FillThreads(QList<QTreeWidgetItem *> &ThreadList, bool bExpandNewMessages, QList<QTreeWidgetItem*> &itemToExpand);
    void FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent, bool bExpandNewMessages, QList<QTreeWidgetItem*> &itemToExpand);

    int getSelectedMsgCount(QList<QTreeWidgetItem*> *pRows, QList<QTreeWidgetItem*> *pRowsRead, QList<QTreeWidgetItem*> *pRowsUnread);
    void setMsgAsReadUnread(QList<QTreeWidgetItem*> &Rows, bool bRead);
    void markMsgAsReadUnread(bool bRead, bool bChildren, bool bForum);
    void CalculateIconsAndFonts(QTreeWidgetItem *pItem = NULL);
    void CalculateIconsAndFonts(QTreeWidgetItem *pItem, bool &bHasReadChilddren, bool &bHasUnreadChilddren);

    void processSettings(bool bLoad);
    void togglethreadview_internal();

    void FilterItems();
    bool FilterItem(QTreeWidgetItem *pItem, QString &sPattern, int filterColumn);


	// New Request/Response Loading Functions.
	void insertForumsData(const std::list<RsGroupMetaData> &forumList);
	void insertForumThreads(const RsGroupMetaData &fi);

	void requestGroupSummary();
	void loadGroupSummary(const uint32_t &token);

	void requestGroupSummary_CurrentForum(const std::string &forumId);
	void loadGroupSummary_CurrentForum(const uint32_t &token);

	void loadCurrentForumThreads(const std::string &forumId);
	void requestGroupThreadData_InsertThreads(const std::string &forumId);
	void loadGroupThreadData_InsertThreads(const uint32_t &token);
	void loadForumBaseThread(const RsForumV2Msg &msg);

	void requestChildData_InsertThreads(uint32_t &token, const std::string &parentId);
	void loadChildData_InsertThreads(const uint32_t &token);
	void loadForumChildMsg(const RsForumV2Msg &msg, QTreeWidgetItem *parent);

	void requestMsgData_InsertPost(const std::string &msgId);
	void loadMsgData_InsertPost(const uint32_t &token);
	void requestMsgData_ReplyMessage(const std::string &msgId);
	void loadMsgData_ReplyMessage(const uint32_t &token);

	bool convertMsgToThreadWidget(const RsForumV2Msg &msgInfo, std::string authorName,
                                        bool useChildTS, uint32_t filterColumn, QTreeWidgetItem *item);

	// Callback for all Loads.
	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

	TokenQueue *mForumQueue;


    bool m_bProcessSettings;

    QTreeWidgetItem *yourForums;
    QTreeWidgetItem *subscribedForums;
    QTreeWidgetItem *popularForums;
    QTreeWidgetItem *otherForums;

    std::string mCurrForumId;
    std::string mCurrThreadId;
    int subscribeFlags;

    QFont m_ForumNameFont;
    int lastViewType;
    std::string lastForumID;

    //ForumsV2FillThread *fillThread;

    // New Datatypes to replace the FillThread.
    bool mThreadLoading;
    ForumsV2ThreadLoadParameters mThreadLoad;
   

    /** Qt Designer generated object */
    Ui::ForumsV2Dialog ui;
};

#if 0
class ForumsV2FillThread : public QThread
{
    Q_OBJECT

public:
    ForumsV2FillThread(ForumsV2Dialog *parent);
    ~ForumsV2FillThread();

    void run();
    void stop();
    bool wasStopped() { return stopped; }

signals:
    void progress(int current, int count);

public:
    std::string forumId;
    int filterColumn;
    int subscribeFlags;
    bool fillComplete;
    int viewType;
    bool expandNewMessages;
    std::string focusMsgId;

    QList<QTreeWidgetItem*> Items;
    QList<QTreeWidgetItem*> ItemToExpand;

private:
    volatile bool stopped;
};

#endif


#endif

