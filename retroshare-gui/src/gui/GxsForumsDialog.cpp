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

#include <QMenu>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include <QKeyEvent>
#include <QProgressBar>

#include "GxsForumsDialog.h"

#include "gxs/GxsForumGroupDialog.h"

#include "gxsforums/CreateGxsForumMsg.h"

#include "msgs/MessageComposer.h"
#include "settings/rsharesettings.h"
#include "common/Emoticons.h"
#include "common/RSItemDelegate.h"
#include "common/PopularityDefs.h"
#include "RetroShareLink.h"
#include "channels/ShareKey.h"
#include "notifyqt.h"
#include "util/HandleRichText.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsgxsforums.h>

// These should be in retroshare/ folder.
#include "gxs/rsgxsflags.h"

#include <algorithm>

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
#define IMAGE_FORUM          ":/images/konversation.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"

#define VIEW_LAST_POST	0
#define VIEW_THREADED	1
#define VIEW_FLAT	2

/* Thread constants */
#define COLUMN_THREAD_COUNT    6
#define COLUMN_THREAD_TITLE    0
#define COLUMN_THREAD_READ     1
#define COLUMN_THREAD_DATE     2
#define COLUMN_THREAD_AUTHOR   3
#define COLUMN_THREAD_SIGNED   4
#define COLUMN_THREAD_CONTENT  5

#define COLUMN_THREAD_DATA     0 // column for storing the userdata like msgid and parentid

#define ROLE_THREAD_MSGID           Qt::UserRole
#define ROLE_THREAD_STATUS          Qt::UserRole + 1
#define ROLE_THREAD_MISSING         Qt::UserRole + 2
// no need to copy, don't count in ROLE_THREAD_COUNT
#define ROLE_THREAD_READCHILDREN    Qt::UserRole + 3
#define ROLE_THREAD_UNREADCHILDREN  Qt::UserRole + 4

#define ROLE_THREAD_COUNT           3


static int FilterColumnFromComboBox(int nIndex)
{
    switch (nIndex) {
    case 0:
        return COLUMN_THREAD_DATE;
    case 1:
        return COLUMN_THREAD_TITLE;
    case 2:
        return COLUMN_THREAD_AUTHOR;
    case 3:
        return COLUMN_THREAD_CONTENT;
    }

    return COLUMN_THREAD_TITLE;
}

static int FilterColumnToComboBox(int nIndex)
{
    switch (nIndex) {
    case COLUMN_THREAD_DATE:
        return 0;
    case COLUMN_THREAD_TITLE:
        return 1;
    case COLUMN_THREAD_AUTHOR:
        return 2;
    case COLUMN_THREAD_CONTENT:
        return 3;
    }

    return FilterColumnToComboBox(COLUMN_THREAD_TITLE);
}


/*
 * Transformation Notes:
 *   there are still a couple of things that the new forums differ from Old version.
 *   these will need to be addressed in the future.
 *     -> Missing Messages are not handled yet.
 *     -> Child TS (for sorting) is not handled by GXS, this will probably have to be done in the GUI.
 *     -> Need to handle IDs properly.
 *     -> Popularity not handled in GXS yet.
 *     -> Much more to do.
 */

/** Constructor */
GxsForumsDialog::GxsForumsDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;
    subscribeFlags = 0;
    inMsgAsReadUnread = false;


	/* Setup Queue */
        mForumQueue = new TokenQueue(rsGxsForums->getTokenService(), this);



    connect( ui.forumTreeWidget, SIGNAL( treeCustomContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
    connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

	connect(ui.newForumButton, SIGNAL(clicked()), this, SLOT(newforum()));
    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
    connect(ui.newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

    connect( ui.forumTreeWidget, SIGNAL( treeCurrentItemChanged(QString) ), this, SLOT( changedForum(QString) ) );

    connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread () ) );
    connect( ui.threadTreeWidget, SIGNAL( itemClicked(QTreeWidgetItem*,int)), this, SLOT( clickedThread (QTreeWidgetItem*,int) ) );
    connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( changedViewBox () ) );

    connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
    connect(ui.previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
    connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));
	 connect(ui.nextUnreadButton, SIGNAL(clicked()), this, SLOT(nextUnreadMessage()));

    connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(downloadAllFiles()));

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)));
    connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterColumnChanged()));

    connect(NotifyQt::getInstance(), SIGNAL(forumMsgReadSatusChanged(QString,QString,int)), this, SLOT(forumMsgReadSatusChanged(QString,QString,int)));

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << 300 << width(); // Qt calculates the right sizes
    //ui.splitter->setSizes(sizes);

    /* Set own item delegate */
    RSItemDelegate *itemDelegate = new RSItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    ui.threadTreeWidget->setItemDelegate(itemDelegate);

    /* Set header resize modes and initial section sizes */
    QHeaderView * ttheader = ui.threadTreeWidget->header () ;
    ttheader->setResizeMode (COLUMN_THREAD_TITLE, QHeaderView::Interactive);
    ttheader->resizeSection (COLUMN_THREAD_DATE,  140);
    ttheader->resizeSection (COLUMN_THREAD_TITLE, 290);

    ui.threadTreeWidget->sortItems( COLUMN_THREAD_DATE, Qt::DescendingOrder );

    /* Set text of column "Read" to empty - without this the column has a number as header text */
    QTreeWidgetItem *headerItem = ui.threadTreeWidget->headerItem();
    headerItem->setText(COLUMN_THREAD_READ, "");

    m_ForumNameFont = QFont("Times", 12, QFont::Bold);
    ui.forumName->setFont(m_ForumNameFont);
    ui.threadTitle->setFont(m_ForumNameFont);

    /* Initialize group tree */
    ui.forumTreeWidget->initDisplayMenu(ui.displayButton);

    /* create forum tree */
    yourForums = ui.forumTreeWidget->addCategoryItem(tr("Your Forums"), QIcon(IMAGE_FOLDER), true);
    subscribedForums = ui.forumTreeWidget->addCategoryItem(tr("Subscribed Forums"), QIcon(IMAGE_FOLDERRED), true);
    popularForums = ui.forumTreeWidget->addCategoryItem(tr("Popular Forums"), QIcon(IMAGE_FOLDERGREEN), false);
    otherForums = ui.forumTreeWidget->addCategoryItem(tr("Other Forums"), QIcon(IMAGE_FOLDERYELLOW), false);

    lastViewType = -1;

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    ttheader->resizeSection (COLUMN_THREAD_READ,  24);
    ttheader->setResizeMode (COLUMN_THREAD_READ, QHeaderView::Fixed);
    ttheader->hideSection (COLUMN_THREAD_CONTENT);

    ui.progressBar->hide();
    ui.progLayOutTxt->hide();
    ui.progressBarLayOut->setEnabled(false);

    mThreadLoading = false;

    insertThreads();

    ui.threadTreeWidget->installEventFilter(this);

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

GxsForumsDialog::~GxsForumsDialog()
{
    // save settings
    processSettings(false);
}

void GxsForumsDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *pHeader = ui.threadTreeWidget->header () ;

    Settings->beginGroup(QString("GxsForumsDialog"));

    if (bLoad) {
        // load settings

        // expandFiles
        bool bValue = Settings->value("expandButton", true).toBool();
        ui.expandButton->setChecked(bValue);
        togglethreadview_internal();

        // filterColumn
        int nValue = FilterColumnToComboBox(Settings->value("filterColumn", COLUMN_THREAD_TITLE).toInt());
        ui.filterColumnComboBox->setCurrentIndex(nValue);

        // index of viewBox
        ui.viewBox->setCurrentIndex(Settings->value("viewBox", VIEW_THREADED).toInt());

        // state of thread tree
        pHeader->restoreState(Settings->value("ThreadTree").toByteArray());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.threadSplitter->restoreState(Settings->value("threadSplitter").toByteArray());
    } else {
        // save settings

        // state of thread tree
        Settings->setValue("ThreadTree", pHeader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
        Settings->setValue("threadSplitter", ui.threadSplitter->saveState());
    }

    ui.forumTreeWidget->processSettings(Settings, bLoad);

    Settings->endGroup();
    m_bProcessSettings = false;
}

void GxsForumsDialog::forumListCustomPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Forum"), this, SLOT(subscribeToForum()));
    action->setDisabled (mCurrForumId.empty() || IS_GROUP_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Forum"), this, SLOT(unsubscribeToForum()));
    action->setEnabled (!mCurrForumId.empty() && IS_GROUP_SUBSCRIBED(subscribeFlags));

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

    action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
    action->setEnabled (!mCurrForumId.empty ());

    action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
    action->setEnabled (!mCurrForumId.empty () && IS_GROUP_ADMIN(subscribeFlags));

    QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
    connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
    shareKeyAct->setEnabled(!mCurrForumId.empty() && IS_GROUP_ADMIN(subscribeFlags));
    contextMnu.addAction( shareKeyAct);

    QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
    connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
    restoreKeysAct->setEnabled(!mCurrForumId.empty() && !IS_GROUP_ADMIN(subscribeFlags));
    contextMnu.addAction( restoreKeysAct);

    action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
    action->setEnabled(!mCurrForumId.empty());

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsReadAll()));
    action->setEnabled (!mCurrForumId.empty () && IS_GROUP_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnreadAll()));
    action->setEnabled (!mCurrForumId.empty () && IS_GROUP_SUBSCRIBED(subscribeFlags));

#ifdef DEBUG_FORUMS
    contextMnu.addSeparator();
    action = contextMnu.addAction("Generate mass data", this, SLOT(generateMassData()));
    action->setEnabled (!mCurrForumId.empty() && IS_GROUP_SUBSCRIBED(subscribeFlags));
#endif

    contextMnu.exec(QCursor::pos());
}

void GxsForumsDialog::threadListCustomPopupMenu( QPoint /*point*/ )
{
    if (mThreadLoading) {
        return;
    }

    QMenu contextMnu( this );

    QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply" ), &contextMnu );
    connect( replyAct , SIGNAL( triggered() ), this, SLOT( createmessage() ) );

    QAction *newthreadAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Start New Thread" ), &contextMnu );
    newthreadAct->setEnabled (IS_GROUP_SUBSCRIBED(subscribeFlags));
    connect( newthreadAct , SIGNAL( triggered() ), this, SLOT( createthread() ) );

    QAction *replyauthorAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply to Author" ), &contextMnu );
    connect( replyauthorAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );

    QAction* expandAll = new QAction(tr( "Expand all" ), &contextMnu );
    connect( expandAll , SIGNAL( triggered() ), ui.threadTreeWidget, SLOT (expandAll()) );

    QAction* collapseAll = new QAction(tr( "Collapse all" ), &contextMnu );
    connect( collapseAll , SIGNAL( triggered() ), ui.threadTreeWidget, SLOT(collapseAll()) );

    QAction *markMsgAsRead = new QAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read"), &contextMnu);
    connect(markMsgAsRead , SIGNAL(triggered()), this, SLOT(markMsgAsRead()));

    QAction *markMsgAsReadChildren = new QAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsReadChildren, SIGNAL(triggered()), this, SLOT(markMsgAsReadChildren()));

    QAction *markMsgAsUnread = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread"), &contextMnu);
    connect(markMsgAsUnread , SIGNAL(triggered()), this, SLOT(markMsgAsUnread()));

    QAction *markMsgAsUnreadChildren = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsUnreadChildren , SIGNAL(triggered()), this, SLOT(markMsgAsUnreadChildren()));

    if (IS_GROUP_SUBSCRIBED(subscribeFlags)) {
        QList<QTreeWidgetItem*> Rows;
        QList<QTreeWidgetItem*> RowsRead;
        QList<QTreeWidgetItem*> RowsUnread;
        int nCount = getSelectedMsgCount (&Rows, &RowsRead, &RowsUnread);

        if (RowsUnread.size() == 0) {

            markMsgAsRead->setDisabled(true);
        }
        if (RowsRead.size() == 0) {
            markMsgAsUnread->setDisabled(true);
        }

        bool bHasUnreadChildren = false;
        bool bHasReadChildren = false;
        int nRowCount = Rows.count();
        for (int i = 0; i < nRowCount; i++) {
            if (bHasUnreadChildren || Rows[i]->data(COLUMN_THREAD_DATA, ROLE_THREAD_UNREADCHILDREN).toBool()) {
                bHasUnreadChildren = true;
            }
            if (bHasReadChildren || Rows[i]->data(COLUMN_THREAD_DATA, ROLE_THREAD_READCHILDREN).toBool()) {
                bHasReadChildren = true;
            }
        }
        markMsgAsReadChildren->setEnabled(bHasUnreadChildren);
        markMsgAsUnreadChildren->setEnabled(bHasReadChildren);

        if (nCount == 1) {
            replyAct->setEnabled (true);
            replyauthorAct->setEnabled (true);
        } else {
            replyAct->setDisabled (true);
            replyauthorAct->setDisabled (true);
        }
    } else {
        markMsgAsRead->setDisabled(true);
        markMsgAsReadChildren->setDisabled(true);
        markMsgAsUnread->setDisabled(true);
        markMsgAsUnreadChildren->setDisabled(true);
        replyAct->setDisabled (true);
        replyauthorAct->setDisabled (true);
    }

    contextMnu.addAction( replyAct);
    contextMnu.addAction( newthreadAct);
    contextMnu.addAction( replyauthorAct);
    QAction* action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr( "Copy RetroShare Link"), this, SLOT(copyMessageLink()));
    action->setEnabled(!mCurrForumId.empty() && !mCurrThreadId.empty());
    contextMnu.addSeparator();
    contextMnu.addAction(markMsgAsRead);
    contextMnu.addAction(markMsgAsReadChildren);
    contextMnu.addAction(markMsgAsUnread);
    contextMnu.addAction(markMsgAsUnreadChildren);
    contextMnu.addSeparator();
    contextMnu.addAction( expandAll);
    contextMnu.addAction( collapseAll);

    contextMnu.exec(QCursor::pos());
}

bool GxsForumsDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.threadTreeWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && keyEvent->key() == Qt::Key_Space) {
                // Space pressed
                QTreeWidgetItem *item = ui.threadTreeWidget->currentItem ();
                clickedThread (item, COLUMN_THREAD_READ);
                return true; // eat event
            }
        }
    }
    // pass the event on to the parent class
    return RsAutoUpdatePage::eventFilter(obj, event);
}

void GxsForumsDialog::restoreForumKeys(void)
{
#ifdef TOGXS
	rsGxsForums->groupRestoreKeys(mCurrForumId);
#endif
}

void GxsForumsDialog::togglethreadview()
{
    // save state of button
    Settings->setValueToGroup("GxsForumsDialog", "expandButton", ui.expandButton->isChecked());

    togglethreadview_internal();
}

void GxsForumsDialog::togglethreadview_internal()
{
    if (ui.expandButton->isChecked()) {
        ui.postText->setVisible(true);
        ui.expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
        ui.expandButton->setToolTip(tr("Hide"));
    } else  {
        ui.postText->setVisible(false);
        ui.expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
        ui.expandButton->setToolTip(tr("Expand"));
    }
}

void GxsForumsDialog::updateDisplay()
{
    std::list<std::string> forumIds;
    std::list<std::string>::iterator it;
    if (!rsGxsForums)
        return;

#if 0
	// TODO groupsChanged... HACK XXX.
    if ((rsGxsForums->groupsChanged(forumIds)) || (rsGxsForums->updated()))
    {
        /* update Forums List */
        insertForums();

        it = std::find(forumIds.begin(), forumIds.end(), mCurrForumId);
        if (it != forumIds.end())
        {
            /* update threads as well */
            insertThreads();
        }
    }
#endif

    /* The proper version (above) can be done with a data request -> TODO */
    if (rsGxsForums->updated())
    {
        /* update Forums List */
        insertForums();
        /* update threads as well */
        insertThreads();
    }
}

static void CleanupItems (QList<QTreeWidgetItem *> &items)
{
    QList<QTreeWidgetItem *>::iterator item;
    for (item = items.begin (); item != items.end (); item++) {
        if (*item) {
            delete (*item);
        }
    }
    items.clear();
}

void GxsForumsDialog::forumInfoToGroupItemInfo(const RsGroupMetaData &forumInfo, GroupItemInfo &groupItemInfo)
//void GxsForumsDialog::forumInfoToGroupItemInfo(const ForumInfo &forumInfo, GroupItemInfo &groupItemInfo)
{

    groupItemInfo.id = QString::fromStdString(forumInfo.mGroupId);
    groupItemInfo.name = QString::fromUtf8(forumInfo.mGroupName.c_str());
    //groupItemInfo.description = QString::fromUtf8(forumInfo.forumDesc);
    groupItemInfo.popularity = forumInfo.mPop;
    groupItemInfo.lastpost = QDateTime::fromTime_t(forumInfo.mLastPost);

#if TOGXS
    if (forumInfo.mGroupFlags & RS_DISTRIB_AUTHEN_REQ) {
        groupItemInfo.name += " (" + tr("AUTHD") + ")";
        groupItemInfo.icon = QIcon(IMAGE_FORUMAUTHD);
    } 
    else 
#endif
    {
        groupItemInfo.icon = QIcon(IMAGE_FORUM);
    }

//    groupItemInfo.id = QString::fromStdString(forumInfo.forumId);
//    groupItemInfo.name = QString::fromStdWString(forumInfo.forumName);
//    groupItemInfo.description = QString::fromStdWString(forumInfo.forumDesc);
//    groupItemInfo.popularity = forumInfo.pop;
//    groupItemInfo.lastpost = QDateTime::fromTime_t(forumInfo.lastPost);
//
//    if (forumInfo.forumFlags & RS_DISTRIB_AUTHEN_REQ) {
//        groupItemInfo.name += " (" + tr("AUTHD") + ")";
//        groupItemInfo.icon = QIcon(IMAGE_FORUMAUTHD);
//    } else {
//        groupItemInfo.icon = QIcon(IMAGE_FORUM);
//    }
}


/***** INSERT FORUM LISTS *****/
void GxsForumsDialog::insertForumsData(const std::list<RsGroupMetaData> &forumList)
{
	std::list<RsGroupMetaData>::const_iterator it;

    QList<GroupItemInfo> adminList;
    QList<GroupItemInfo> subList;
    QList<GroupItemInfo> popList;
    QList<GroupItemInfo> otherList;
    std::multimap<uint32_t, GroupItemInfo> popMap;

    for (it = forumList.begin(); it != forumList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

        GroupItemInfo groupItemInfo;
        forumInfoToGroupItemInfo(*it, groupItemInfo);

        if (IS_GROUP_ADMIN(flags)) {
            adminList.push_back(groupItemInfo);
        } else if (IS_GROUP_SUBSCRIBED(flags)) {
			/* subscribed forum */
            subList.push_back(groupItemInfo);
        } else {
			/* rate the others by popularity */
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
    ui.forumTreeWidget->fillGroupItems(yourForums, adminList);
    ui.forumTreeWidget->fillGroupItems(subscribedForums, subList);
    ui.forumTreeWidget->fillGroupItems(popularForums, popList);
    ui.forumTreeWidget->fillGroupItems(otherForums, otherList);

	updateMessageSummaryList("");
}

void GxsForumsDialog::changedForum(const QString &id)
{
    mCurrForumId = id.toStdString();

    insertThreads();
}

void GxsForumsDialog::changedThread ()
{
    if (mThreadLoading) {
        return;
    }

    /* just grab the ids of the current item */
    QTreeWidgetItem *curr = ui.threadTreeWidget->currentItem();

    if ((!curr) || (!curr->isSelected())) {
        mCurrThreadId = "";
    } else {
        mCurrThreadId = curr->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();
    }
    insertPost();
}

void GxsForumsDialog::clickedThread (QTreeWidgetItem *item, int column)
{
    if (mCurrForumId.empty() || !IS_GROUP_SUBSCRIBED(subscribeFlags)) {
        return;
    }

    if (item == NULL) {
        return;
    }

    if (column == COLUMN_THREAD_READ) {
        QList<QTreeWidgetItem*> Rows;
        Rows.append(item);
        uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        setMsgReadStatus(Rows, IS_MSG_UNREAD(status));
        return;
    }
}

void GxsForumsDialog::forumMsgReadStatusChanged(const QString &forumId, const QString &msgId, int status)
{
    if (inMsgAsReadUnread) {
        return;
    }

    if (forumId.toStdString() == mCurrForumId) {
        /* Search exisiting item */
        QTreeWidgetItemIterator itemIterator(ui.threadTreeWidget);
        QTreeWidgetItem *item = NULL;
        while ((item = *itemIterator) != NULL) {
            itemIterator++;

            if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString() == msgId) {
                // update status
                item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

                QTreeWidgetItem *parentItem = item;
                while (parentItem->parent()) {
                    parentItem = parentItem->parent();
                }
                CalculateIconsAndFonts(parentItem);
                break;
            }
        }
    }
    updateMessageSummaryList(forumId.toStdString());
}

void GxsForumsDialog::CalculateIconsAndFonts(QTreeWidgetItem *pItem, bool &bHasReadChilddren, bool &bHasUnreadChilddren)
{
    uint32_t status = pItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

    bool bUnread = IS_MSG_UNREAD(status);
    bool missing = pItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING).toBool();

    // set icon
    if (missing) {
        pItem->setIcon(COLUMN_THREAD_READ, QIcon());
        pItem->setIcon(COLUMN_THREAD_TITLE, QIcon());
    } else {
        if (bUnread) {
            pItem->setIcon(COLUMN_THREAD_READ, QIcon(":/images/message-state-unread.png"));
        } else {
            pItem->setIcon(COLUMN_THREAD_READ, QIcon(":/images/message-state-read.png"));
        }
        if (IS_MSG_UNREAD(status)) {
            pItem->setIcon(COLUMN_THREAD_TITLE, QIcon(":/images/message-state-new.png"));
        } else {
            pItem->setIcon(COLUMN_THREAD_TITLE, QIcon());
        }
    }

    int nItem;
    int nItemCount = pItem->childCount();

    bool bMyReadChilddren = false;
    bool bMyUnreadChilddren = false;

    for (nItem = 0; nItem < nItemCount; nItem++) {
        CalculateIconsAndFonts(pItem->child(nItem), bMyReadChilddren, bMyUnreadChilddren);
    }

    // set font
    for (int i = 0; i < COLUMN_THREAD_COUNT; i++) {
        QFont qf = pItem->font(i);

        if (!IS_GROUP_SUBSCRIBED(subscribeFlags)) {
            qf.setBold(false);
            pItem->setTextColor(i, Qt::black);
        } else if (bUnread) {
            qf.setBold(true);
            pItem->setTextColor(i, Qt::black);
        } else if (bMyUnreadChilddren) {
            qf.setBold(true);
            pItem->setTextColor(i, Qt::gray);
        } else {
            qf.setBold(false);
            pItem->setTextColor(i, Qt::gray);
        }
        if (missing) {
            /* Missing message */
            pItem->setTextColor(i, Qt::darkRed);
        }
        pItem->setFont(i, qf);
    }

    pItem->setData(COLUMN_THREAD_DATA, ROLE_THREAD_READCHILDREN, bHasReadChilddren || bMyReadChilddren);
    pItem->setData(COLUMN_THREAD_DATA, ROLE_THREAD_UNREADCHILDREN, bHasUnreadChilddren || bMyUnreadChilddren);

    bHasReadChilddren = bHasReadChilddren || bMyReadChilddren || !bUnread;
    bHasUnreadChilddren = bHasUnreadChilddren || bMyUnreadChilddren || bUnread;
}

void GxsForumsDialog::CalculateIconsAndFonts(QTreeWidgetItem *pItem /*= NULL*/)
{
    bool bDummy1 = false;
    bool bDummy2 = false;

    if (pItem) {
        CalculateIconsAndFonts(pItem, bDummy1, bDummy2);
        return;
    }

    int nItem;
    int nItemCount = ui.threadTreeWidget->topLevelItemCount();

    for (nItem = 0; nItem < nItemCount; nItem++) {
        bDummy1 = false;
        bDummy2 = false;
        CalculateIconsAndFonts(ui.threadTreeWidget->topLevelItem(nItem), bDummy1, bDummy2);
    }
}

void GxsForumsDialog::fillThreadFinished()
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumsDialog::fillThreadFinished" << std::endl;
#endif

	// This is now only called with a successful Load.
	// cleanup of incomplete is handled elsewhere.

	// current thread has finished, hide progressbar and release thread
	ui.progressBar->hide();
	ui.progLayOutTxt->hide();
	ui.progressBarLayOut->setEnabled(false);
	
#ifdef DEBUG_FORUMS
	std::cerr << "GxsForumsDialog::fillThreadFinished Add messages" << std::endl;
#endif
            ui.threadTreeWidget->setSortingEnabled(false);

		/* add all messages in! */
	if (lastViewType != mThreadLoad.ViewType || lastForumID != mCurrForumId) 
	{
		ui.threadTreeWidget->clear();
		lastViewType = mThreadLoad.ViewType;
		lastForumID = mCurrForumId;
		ui.threadTreeWidget->insertTopLevelItems(0, mThreadLoad.Items);
		
		// clear list
		mThreadLoad.Items.clear();
	} 
	else 
	{
		FillThreads (mThreadLoad.Items, mThreadLoad.ExpandNewMessages, mThreadLoad.ItemToExpand);
		
		// cleanup list
		CleanupItems (mThreadLoad.Items);
	}

            ui.threadTreeWidget->setSortingEnabled(true);

	if (mThreadLoad.FocusMsgId.empty() == false) 
	{
		/* Search exisiting item */
		QTreeWidgetItemIterator itemIterator(ui.threadTreeWidget);
		QTreeWidgetItem *item = NULL;
		while ((item = *itemIterator) != NULL) 
		{
			itemIterator++;
		
			if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == mThreadLoad.FocusMsgId) 
			{
				ui.threadTreeWidget->setCurrentItem(item);
				ui.threadTreeWidget->setFocus();
				break;
			}
		}
	}
		
	QList<QTreeWidgetItem*>::iterator Item;
	for (Item = mThreadLoad.ItemToExpand.begin(); Item != mThreadLoad.ItemToExpand.end(); Item++) 
	{
		if ((*Item)->isHidden() == false) 
		{
			(*Item)->setExpanded(true);
		}
	}
	mThreadLoad.ItemToExpand.clear();
	
            if (ui.filterLineEdit->text().isEmpty() == false) {
                filterItems(ui.filterLineEdit->text());
	}
	
	insertPost ();
	CalculateIconsAndFonts();
	
	ui.newthreadButton->setEnabled (IS_GROUP_SUBSCRIBED(subscribeFlags));

	mThreadLoading = false;

#ifdef DEBUG_FORUMS
    	std::cerr << "GxsForumsDialog::fillThreadFinished done" << std::endl;
#endif
}

void GxsForumsDialog::fillThreadProgress(int current, int count)
{
    // show fill progress
    if (count) {
        ui.progressBar->setValue(current * ui.progressBar->maximum() / count);
    }

}

void GxsForumsDialog::insertThreads()
{
#ifdef DEBUG_FORUMS
	/* get the current Forum */
	std::cerr << "GxsForumsDialog::insertThreads()" << std::endl;
#endif

	subscribeFlags = 0;
	
	ui.newmessageButton->setEnabled (false);
	ui.newthreadButton->setEnabled (false);
	
	ui.postText->clear();
	ui.threadTitle->clear();
	
	if (mCurrForumId.empty())
	{
		/* not an actual forum - clear */
		ui.threadTreeWidget->clear();
		/* when no Thread selected - clear */
		ui.forumName->clear();
		/* clear last stored forumID */
		mCurrForumId.erase();
		lastForumID.erase();
		
#ifdef DEBUG_FORUMS
		std::cerr << "GxsForumsDialog::insertThreads() Current Thread Invalid" << std::endl;
#endif
		
		return;
	}
	
	// Get Current Forum Info... then complete insertForumThreads().
	requestGroupSummary_CurrentForum(mCurrForumId);
}


void GxsForumsDialog::insertForumThreads(const RsGroupMetaData &fi)
{
    subscribeFlags = fi.mSubscribeFlags;
    ui.forumName->setText(QString::fromUtf8(fi.mGroupName.c_str()));

    ui.progressBarLayOut->setEnabled(true);

    ui.progLayOutTxt->show();
    ui.progressBar->reset();
    ui.progressBar->show();

#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumsDialog::insertThreads() Start filling Forum threads" << std::endl;
#endif

    loadCurrentForumThreads(fi.mGroupId);
}




void GxsForumsDialog::FillThreads(QList<QTreeWidgetItem *> &ThreadList, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumsDialog::FillThreads()" << std::endl;
#endif

    int Index = 0;
    QTreeWidgetItem *Thread;
    QList<QTreeWidgetItem *>::iterator NewThread;

    // delete not existing
    while (Index < ui.threadTreeWidget->topLevelItemCount ()) {
        Thread = ui.threadTreeWidget->topLevelItem (Index);

        // search existing new thread
        int Found = -1;
        for (NewThread = ThreadList.begin (); NewThread != ThreadList.end (); NewThread++) {
            if (Thread->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == (*NewThread)->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            Index++;
        } else {
            delete (ui.threadTreeWidget->takeTopLevelItem (Index));
        }
    }

    // iterate all new threads
    for (NewThread = ThreadList.begin (); NewThread != ThreadList.end (); NewThread++) {
        // search existing thread
        int Found = -1;
        int Count = ui.threadTreeWidget->topLevelItemCount ();
        for (Index = 0; Index < Count; Index++) {
            Thread = ui.threadTreeWidget->topLevelItem (Index);
            if (Thread->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == (*NewThread)->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }

        if (Found >= 0) {
            // set child data
            int i;
            for (i = 0; i < COLUMN_THREAD_COUNT; i++) {
                Thread->setText (i, (*NewThread)->text (i));
            }
            for (i = 0; i < ROLE_THREAD_COUNT; i++) {
                Thread->setData (COLUMN_THREAD_DATA, Qt::UserRole + i, (*NewThread)->data (COLUMN_THREAD_DATA, Qt::UserRole + i));
            }

            // fill recursive
            FillChildren (Thread, *NewThread, expandNewMessages, itemToExpand);
        } else {
            // add new thread
            ui.threadTreeWidget->addTopLevelItem (*NewThread);
            Thread = *NewThread;
            *NewThread = NULL;
        }

        uint32_t status = Thread->data (COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        if (expandNewMessages && IS_MSG_UNREAD(status)) {
            QTreeWidgetItem *pParent = Thread;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }

#ifdef DEBUG_FORUMS
    std::cerr << "GxsForumsDialog::FillThreads() done" << std::endl;
#endif
}

void GxsForumsDialog::FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
    int Index = 0;
    int NewIndex;
    int NewCount = NewParent->childCount();

    QTreeWidgetItem *Child;
    QTreeWidgetItem *NewChild;

    // delete not existing
    while (Index < Parent->childCount ()) {
        Child = Parent->child (Index);

        // search existing new child
        int Found = -1;
        int Count = NewParent->childCount();
        for (NewIndex = 0; NewIndex < Count; NewIndex++) {
            NewChild = NewParent->child (NewIndex);
            if (NewChild->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == Child->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            Index++;
        } else {
            delete (Parent->takeChild (Index));
        }
    }

    // iterate all new children
    for (NewIndex = 0; NewIndex < NewCount; NewIndex++) {
        NewChild = NewParent->child (NewIndex);

        // search existing child
        int Found = -1;
        int Count = Parent->childCount();
        for (Index = 0; Index < Count; Index++) {
            Child = Parent->child (Index);
            if (Child->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID) == NewChild->data (COLUMN_THREAD_DATA, ROLE_THREAD_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }

        if (Found >= 0) {
            // set child data
            int i;
            for (i = 0; i < COLUMN_THREAD_COUNT; i++) {
                Child->setText (i, NewChild->text (i));
            }
            for (i = 0; i < ROLE_THREAD_COUNT; i++) {
                Child->setData (COLUMN_THREAD_DATA, Qt::UserRole + i, NewChild->data (COLUMN_THREAD_DATA, Qt::UserRole + i));
            }

            // fill recursive
            FillChildren (Child, NewChild, expandNewMessages, itemToExpand);
        } else {
            // add new child
            Child = NewParent->takeChild(NewIndex);
            Parent->addChild (Child);
            NewIndex--;
            NewCount--;
        }

        uint32_t status = Child->data (COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        if (expandNewMessages && IS_MSG_UNREAD(status)) {
            QTreeWidgetItem *pParent = Child;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }
}

QString GxsForumsDialog::titleFromInfo(const RsMsgMetaData &meta)
{
    // NOTE - NOTE SURE HOW THIS WILL WORK!
#ifdef TOGXS
    if (meta.mMsgStatus & RS_DISTRIB_MISSING_MSG) {
        return QApplication::translate("GxsForumsDialog", "[ ... Missing Message ... ]");
    }
#endif

     return QString::fromUtf8(meta.mMsgName.c_str());
}

QString GxsForumsDialog::messageFromInfo(const RsGxsForumMsg &msg)
{
#ifdef TOGXS
    if (msg.mMeta.mMsgStatus & RS_DISTRIB_MISSING_MSG) {
        return QApplication::translate("GxsForumsDialog", "Placeholder for missing Message");
    }
#endif

    return QString::fromUtf8(msg.mMsg.c_str());
}

void GxsForumsDialog::insertPost()
{
    if ((mCurrForumId == "") || (mCurrThreadId == ""))
    {
        ui.postText->setText("");
        ui.threadTitle->setText("");
        ui.previousButton->setEnabled(false);
        ui.nextButton->setEnabled(false);
        ui.newmessageButton->setEnabled (false);
        return;
    }

    QTreeWidgetItem *curr = ui.threadTreeWidget->currentItem();
    if (curr) {
        QTreeWidgetItem *Parent = curr->parent ();
        int Index = Parent ? Parent->indexOfChild (curr) : ui.threadTreeWidget->indexOfTopLevelItem (curr);
        int Count = Parent ? Parent->childCount () : ui.threadTreeWidget->topLevelItemCount ();
        ui.previousButton->setEnabled (Index > 0);
        ui.nextButton->setEnabled (Index < Count - 1);
    } else {
        // there is something wrong
        ui.previousButton->setEnabled(false);
        ui.nextButton->setEnabled(false);
		return;
    }

    ui.newmessageButton->setEnabled (IS_GROUP_SUBSCRIBED(subscribeFlags) && mCurrThreadId.empty() == false);

	/* blank text, incase we get nothing */
    ui.postText->setText("");
	/* request Post */

	RsGxsGrpMsgIdPair postId = std::make_pair(mCurrForumId, mCurrThreadId);
	requestMsgData_InsertPost(postId);

}



void GxsForumsDialog::insertPostData(const RsGxsForumMsg &msg)
{
	/* As some time has elapsed since request - check that this is still the current msg.
	 * otherwise, another request will fill the data 
	 */

	if ((msg.mMeta.mGroupId != mCurrForumId) || (msg.mMeta.mMsgId != mCurrThreadId))
	{
		std::cerr << "GxsForumsDialog::insertPostData() Ignoring Invalid Data....";
		std::cerr << std::endl;
		std::cerr << "\t CurrForumId: " << mCurrForumId << " != msg.GroupId: " << msg.mMeta.mGroupId;
		std::cerr << std::endl;
		std::cerr << "\t or CurrThdId: " << mCurrThreadId << " != msg.MsgId: " << msg.mMeta.mMsgId;
		std::cerr << std::endl;
		std::cerr << std::endl;
		return;
	}

    QTreeWidgetItem *curr = ui.threadTreeWidget->currentItem();

    bool bSetToReadOnActive = Settings->getForumMsgSetToReadOnActivate();
    uint32_t status = curr->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

    QList<QTreeWidgetItem*> Row;
    Row.append(curr);

    if (bSetToReadOnActive && IS_MSG_UNREAD(status))
    {
            setMsgReadStatus(Row, true);
    }

    QString extraTxt = RsHtml().formatText(ui.postText->document(), messageFromInfo(msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);

    ui.postText->setHtml(extraTxt);
    ui.threadTitle->setText(titleFromInfo(msg.mMeta));
}


void GxsForumsDialog::previousMessage ()
{
    QTreeWidgetItem *Item = ui.threadTreeWidget->currentItem ();
    if (Item == NULL) {
        return;
    }

    QTreeWidgetItem *Parent = Item->parent ();
    int Index = Parent ? Parent->indexOfChild (Item) : ui.threadTreeWidget->indexOfTopLevelItem (Item);
    if (Index > 0) {
        QTreeWidgetItem *Previous = Parent ? Parent->child (Index - 1) : ui.threadTreeWidget->topLevelItem (Index - 1);
        if (Previous) {
            ui.threadTreeWidget->setCurrentItem (Previous);
        }
    }
}

void GxsForumsDialog::nextMessage ()
{
    QTreeWidgetItem *Item = ui.threadTreeWidget->currentItem ();
    if (Item == NULL) {
        return;
    }

    QTreeWidgetItem *Parent = Item->parent ();
    int Index = Parent ? Parent->indexOfChild (Item) : ui.threadTreeWidget->indexOfTopLevelItem (Item);
    int Count = Parent ? Parent->childCount () : ui.threadTreeWidget->topLevelItemCount ();
    if (Index < Count - 1) {
        QTreeWidgetItem *Next = Parent ? Parent->child (Index + 1) : ui.threadTreeWidget->topLevelItem (Index + 1);
        if (Next) {
            ui.threadTreeWidget->setCurrentItem (Next);
        }
    }
}

void GxsForumsDialog::downloadAllFiles()
{
	QStringList urls;
	if (RsHtml::findAnchors(ui.postText->toHtml(), urls) == false) {
		return;
	}

	if (urls.count() == 0) {
		return;
	}

	RetroShareLink::process(urls, RetroShareLink::TYPE_FILE/*, true*/);
}

void GxsForumsDialog::nextUnreadMessage()
{
    QTreeWidgetItem* currentItem = ui.threadTreeWidget->currentItem();
    if( !currentItem )
    {
        currentItem = ui.threadTreeWidget->topLevelItem(0);
        if( !currentItem )
            return;
    }

    do
    {
        uint32_t status = currentItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        if( IS_MSG_UNREAD(status) )
        {
            ui.threadTreeWidget->setCurrentItem(currentItem);
            return;
        }
    } while( (currentItem = ui.threadTreeWidget->itemBelow(currentItem)) != NULL );
}

// TODO
#if 0
void GxsForumsDialog::removemessage()
{
	//std::cerr << "GxsForumsDialog::removemessage()" << std::endl;
	std::string cid, mid;
	if (!getCurrentMsg(cid, mid))
	{
		//std::cerr << "GxsForumsDialog::removemessage()";
		//std::cerr << " No Message selected" << std::endl;
		return;
	}

	rsMsgs -> MessageDelete(mid);
}
#endif

/* get selected messages
   the messages tree is single selected, but who knows ... */
int GxsForumsDialog::getSelectedMsgCount(QList<QTreeWidgetItem*> *pRows, QList<QTreeWidgetItem*> *pRowsRead, QList<QTreeWidgetItem*> *pRowsUnread)
{
    if (pRowsRead) pRowsRead->clear();
    if (pRowsUnread) pRowsUnread->clear();

    QList<QTreeWidgetItem*> selectedItems = ui.threadTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::iterator it = selectedItems.begin(); it != selectedItems.end(); it++) {
        if (pRows) pRows->append(*it);
        if (pRowsRead || pRowsUnread) {
            uint32_t status = (*it)->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
            if (IS_MSG_UNREAD(status)) {
                if (pRowsUnread) pRowsUnread->append(*it);
            } else {
                if (pRowsRead) pRowsRead->append(*it);
            }
        }
    }

    return selectedItems.size();
}

void GxsForumsDialog::setMsgReadStatus(QList<QTreeWidgetItem*> &Rows, bool bRead)
{
    QList<QTreeWidgetItem*>::iterator Row;
    std::list<QTreeWidgetItem*> changedItems;

    inMsgAsReadUnread = true;

    for (Row = Rows.begin(); Row != Rows.end(); Row++) {
        if ((*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING).toBool()) {
            /* Missing message */
            continue;
        }

        uint32_t status = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

        uint32_t statusNew = (status & ~GXS_SERV::GXS_MSG_STATUS_UNREAD); // orig status, without UNREAD.
        if (!bRead) {
		statusNew |= GXS_SERV::GXS_MSG_STATUS_UNREAD;
        }

	if (IS_MSG_UNREAD(status) == bRead) // is it different?
	{
            std::string msgId = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();

		// NB: MUST BE PART OF ACTIVE THREAD--- OR ELSE WE MUST STORE GROUPID SOMEWHERE!.
		// LIKE THIS BELOW...
               	//std::string grpId = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_GROUPID).toString().toStdString();

	    RsGxsGrpMsgIdPair msgPair = std::make_pair(mCurrForumId, msgId);

	    uint32_t token;
	    rsGxsForums->setMessageReadStatus(token, msgPair, bRead);

            (*Row)->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, statusNew);

            QTreeWidgetItem *parentItem = *Row;
            while (parentItem->parent()) {
                parentItem = parentItem->parent();
            }
            if (std::find(changedItems.begin(), changedItems.end(), parentItem) == changedItems.end()) {
                changedItems.push_back(parentItem);
            }
        }
    }

    inMsgAsReadUnread = false;

    if (changedItems.size()) {
        for (std::list<QTreeWidgetItem*>::iterator it = changedItems.begin(); it != changedItems.end(); it++) {
            CalculateIconsAndFonts(*it);
        }
        updateMessageSummaryList(mCurrForumId);
    }
}

void GxsForumsDialog::markMsgAsReadUnread (bool bRead, bool bChildren, bool bForum)
{
    if (mCurrForumId.empty() || !IS_GROUP_SUBSCRIBED(subscribeFlags)) {
        return;
    }

    /* get selected messages */
    QList<QTreeWidgetItem*> Rows;
    if (bForum) {
        int itemCount = ui.threadTreeWidget->topLevelItemCount();
        for (int item = 0; item < itemCount; item++) {
            Rows.push_back(ui.threadTreeWidget->topLevelItem(item));
        }
    } else {
        getSelectedMsgCount (&Rows, NULL, NULL);
    }

    if (bChildren) {
        /* add children */
        QList<QTreeWidgetItem*> AllRows;

        while (Rows.isEmpty() == false) {
            QTreeWidgetItem *pRow = Rows.takeFirst();

            /* add only items with the right state or with not RSGXS_MSG_STATUS_READ */
            uint32_t status = pRow->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
             if (IS_MSG_UNREAD(status) == bRead) {
                AllRows.append(pRow);
            }

            for (int i = 0; i < pRow->childCount(); i++) {
                /* add child to main list and let the main loop do the work */
                Rows.append(pRow->child(i));
            }
        }

        if (AllRows.isEmpty()) {
            /* nothing to do */
            return;
        }

        setMsgReadStatus(AllRows, bRead);

        return;
    }

    setMsgReadStatus(Rows, bRead);
}

void GxsForumsDialog::markMsgAsRead()
{
    markMsgAsReadUnread(true, false, false);
}

void GxsForumsDialog::markMsgAsReadChildren()
{
    markMsgAsReadUnread(true, true, false);
}

void GxsForumsDialog::markMsgAsReadAll()
{
    markMsgAsReadUnread(true, true, true);
}

void GxsForumsDialog::markMsgAsUnread()
{
    markMsgAsReadUnread(false, false, false);
}

void GxsForumsDialog::markMsgAsUnreadChildren()
{
    markMsgAsReadUnread(false, true, false);
}

void GxsForumsDialog::markMsgAsUnreadAll()
{
    markMsgAsReadUnread(false, true, true);
}

void GxsForumsDialog::copyForumLink()
{
    if (mCurrForumId.empty()) {
        return;
    }

// THIS CODE CALLS getForumInfo() to verify that the Ids are valid.
// As we are switching to Request/Response this is now harder to do...
// So not bothering any more - shouldn't be necessary.
// IF we get errors - fix them, rather than patching here.
#if 0
    ForumInfo fi;
    if (rsGxsForums->getForumInfo(mCurrForumId, fi)) {
        RetroShareLink link;
        if (link.createForum(fi.forumId, "")) {
            QList<RetroShareLink> urls;
            urls.push_back(link);
            RSLinkClipboard::copyLinks(urls);
        }
    }
#endif

	{
        	RetroShareLink link;
        	if (link.createForum(mCurrForumId, "")) 
		{
            		QList<RetroShareLink> urls;
            		urls.push_back(link);
            		RSLinkClipboard::copyLinks(urls);
		}
	}
}




void GxsForumsDialog::copyMessageLink()
{
    if (mCurrForumId.empty() || mCurrThreadId.empty()) {
        return;
    }

// SEE NOTE In fn above.
#if 0
    ForumInfo fi;
    if (rsGxsForums->getForumInfo(mCurrForumId, fi)) {
        RetroShareLink link;
        if (link.createForum(mCurrForumId, mCurrThreadId)) {
            QList<RetroShareLink> urls;
            urls.push_back(link);
            RSLinkClipboard::copyLinks(urls);
        }
    }
#endif

	{
        	RetroShareLink link;
        	if (link.createForum(mCurrForumId, mCurrThreadId)) 
		{
            		QList<RetroShareLink> urls;
            		urls.push_back(link);
            		RSLinkClipboard::copyLinks(urls);
		}
	}
}


void GxsForumsDialog::newforum()
{
    GxsForumGroupDialog cf (mForumQueue, this);
    //cf.newGroup();

    cf.exec ();
}

void GxsForumsDialog::createmessage()
{
    if (mCurrForumId.empty () || !IS_GROUP_SUBSCRIBED(subscribeFlags)) {
        return;
    }

    CreateGxsForumMsg *cfm = new CreateGxsForumMsg(mCurrForumId, mCurrThreadId);
    cfm->show();

    /* window will destroy itself! */
}

void GxsForumsDialog::createthread()
{
    if (mCurrForumId.empty ()) {
        QMessageBox::information(this, tr("RetroShare"), tr("No Forum Selected!"));
        return;
    }

    CreateGxsForumMsg *cfm = new CreateGxsForumMsg(mCurrForumId, "");
    cfm->setWindowTitle(tr("Start New Thread"));
    cfm->show();

    /* window will destroy itself! */
}

void GxsForumsDialog::subscribeToForum()
{
	forumSubscribe(true);
}

void GxsForumsDialog::unsubscribeToForum()
{
	forumSubscribe(false);
}

void GxsForumsDialog::forumSubscribe(bool subscribe)
{
	if (mCurrForumId.empty()) {
		return;
	}
	
	uint32_t token;
	rsGxsForums->subscribeToGroup(token, mCurrForumId, subscribe);
}

void GxsForumsDialog::showForumDetails()
{
    if (mCurrForumId.empty()) {
            return;
    }

    RsGxsForumGroup grp;
    grp.mMeta.mGroupId = mCurrForumId;

    GxsForumGroupDialog cf(grp, this);

    cf.exec ();
}

void GxsForumsDialog::editForumDetails()
{
    if (mCurrForumId.empty()) {
            return;
    }

    RsGxsForumGroup grp;
    grp.mMeta.mGroupId = mCurrForumId;

    GxsForumGroupDialog cf(grp, this);

    //GxsForumGroupDialog cf (mForumQueue, this, mCurrForumId, GXS_GROUP_DIALOG_EDIT_MODE);

    cf.exec ();
}

static QString buildReplyHeader(const RsMsgMetaData &meta)
{
    RetroShareLink link;
    link.createMessage(meta.mAuthorId, "");
    QString from = link.toHtml();

    QDateTime qtime;
    qtime.setTime_t(meta.mPublishTs);

    QString header = QString("<span>-----%1-----").arg(QApplication::translate("GxsForumsDialog", "Original Message"));
    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumsDialog", "From"), from);

    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("GxsForumsDialog", "Sent"), qtime.toString(Qt::SystemLocaleLongDate));
    header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(QApplication::translate("GxsForumsDialog", "Subject"), QString::fromUtf8(meta.mMsgName.c_str()));
    header += "<br>";

    header += QApplication::translate("GxsForumsDialog", "On %1, %2 wrote:").arg(qtime.toString(Qt::SystemLocaleShortDate), from);

    return header;
}

void GxsForumsDialog::replytomessage()
{
    if (mCurrForumId.empty() || mCurrThreadId.empty()) {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to a non-existant Message"));
        return;
    }

	RsGxsGrpMsgIdPair postId = std::make_pair(mCurrForumId, mCurrThreadId);
	requestMsgData_ReplyMessage(postId);
}

void GxsForumsDialog::replyMessageData(const RsGxsForumMsg &msg)
{
	if ((msg.mMeta.mGroupId != mCurrForumId) || (msg.mMeta.mMsgId != mCurrThreadId))
	{
		std::cerr << "GxsForumsDialog::replyMessageData() ERROR Message Ids have changed!";
		std::cerr << std::endl;
		return;
	}

	// NB: TODO REMOVE rsPeers references.
    if (rsPeers->getPeerName(msg.mMeta.mAuthorId) !="")
    {
        MessageComposer *nMsgDialog = MessageComposer::newMsg();
        nMsgDialog->setTitleText(QString::fromUtf8(msg.mMeta.mMsgName.c_str()), MessageComposer::REPLY);

        nMsgDialog->setQuotedMsg(QString::fromUtf8(msg.mMsg.c_str()), buildReplyHeader(msg.mMeta));

        nMsgDialog->addRecipient(MessageComposer::TO, msg.mMeta.mAuthorId, false);
        nMsgDialog->show();
        nMsgDialog->activateWindow();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply to an Anonymous Author"));
    }
}

void GxsForumsDialog::changedViewBox()
{
    if (m_bProcessSettings) {
        return;
    }

    // save index
    Settings->setValueToGroup("GxsForumsDialog", "viewBox", ui.viewBox->currentIndex());

    insertThreads();
}

void GxsForumsDialog::filterColumnChanged()
{
    if (m_bProcessSettings) {
        return;
    }

    int filterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());
    if (filterColumn == COLUMN_THREAD_CONTENT) {
        // need content ... refill
        insertThreads();
    } else {
        filterItems(ui.filterLineEdit->text());
    }

    // save index
    Settings->setValueToGroup("GxsForumsDialog", "filterColumn", filterColumn);
}

void GxsForumsDialog::filterItems(const QString& text)
{
    int filterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());

    int nCount = ui.threadTreeWidget->topLevelItemCount ();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        filterItem(ui.threadTreeWidget->topLevelItem(nIndex), text, filterColumn);
    }
}

void GxsForumsDialog::shareKey()
{
    ShareKey shareUi(this, 0, mCurrForumId, FORUM_KEY_SHARE);
    shareUi.exec();
}

bool GxsForumsDialog::filterItem(QTreeWidgetItem *pItem, const QString &text, int filterColumn)
{
    bool bVisible = true;

    if (text.isEmpty() == false) {
        if (pItem->text(filterColumn).contains(text, Qt::CaseInsensitive) == false) {
            bVisible = false;
        }
    }

    int nVisibleChildCount = 0;
    int nCount = pItem->childCount();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        if (filterItem(pItem->child(nIndex), text, filterColumn)) {
            nVisibleChildCount++;
        }
    }

    if (bVisible || nVisibleChildCount) {
        pItem->setHidden(false);
    } else {
        pItem->setHidden(true);
    }

    return (bVisible || nVisibleChildCount);
}

void GxsForumsDialog::updateMessageSummaryList(std::string forumId)
{
    QTreeWidgetItem *items[2] = { yourForums, subscribedForums };

    for (int item = 0; item < 2; item++) {
        int child;
        int childCount = items[item]->childCount();
        for (child = 0; child < childCount; child++) {
            QTreeWidgetItem *childItem = items[item]->child(child);
            std::string childId = ui.forumTreeWidget->itemId(childItem).toStdString();
            if (childId.empty()) {
                continue;
            }

            if (forumId.empty() || childId == forumId) {
                /* calculate unread messages */
                unsigned int newMessageCount = 0;
                unsigned int unreadMessageCount = 0;
                //rsGxsForums->getMessageCount(childId, newMessageCount, unreadMessageCount);
		std::cerr << "IMPLEMENT rsGxsForums->getMessageCount()";
		std::cerr << std::endl;

                ui.forumTreeWidget->setUnreadCount(childItem, unreadMessageCount);

                if (forumId.empty() == false) {
                    /* Calculate only this forum */
                    break;
                }
            }
        }
    }
}

bool GxsForumsDialog::navigate(const std::string& forumId, const std::string& msgId)
{
    if (forumId.empty()) {
        return false;
    }

    if (ui.forumTreeWidget->activateId(QString::fromStdString(forumId), msgId.empty()) == NULL) {
        return false;
    }

    /* Threads are filled in changedForum */
    if (mCurrForumId != forumId) {
        return false;
    }

    if (msgId.empty()) {
        return true;
    }

    if (mThreadLoading) {
        mThreadLoad.FocusMsgId = msgId;
        return true;
    }

    /* Search exisiting item */
    QTreeWidgetItemIterator itemIterator(ui.threadTreeWidget);
    QTreeWidgetItem *item = NULL;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;

        if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == msgId) {
            ui.threadTreeWidget->setCurrentItem(item);
            ui.threadTreeWidget->setFocus();
            return true;
        }
    }

    return false;
}

void GxsForumsDialog::generateMassData()
{
#ifdef DEBUG_FORUMS
    if (mCurrForumId.empty ()) {
        return;
    }

    if (QMessageBox::question(this, "Generate mass data", "Do you really want to generate mass data ?", QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
        return;
    }

    for (int thread = 1; thread < 1000; thread++) {
        ForumMsgInfo threadInfo;
        threadInfo.forumId = mCurrForumId;
        threadInfo.title = QString("Test %1").arg(thread, 3, 10, QChar('0')).toStdWString();
        threadInfo.msg = QString("That is only a test").toStdWString();

        if (rsGxsForums->ForumMessageSend(threadInfo) == false) {
            return;
        }

        for (int msg = 1; msg < 3; msg++) {
            ForumMsgInfo msgInfo;
            msgInfo.forumId = mCurrForumId;
            msgInfo.threadId = threadInfo.msgId;
            msgInfo.parentId = threadInfo.msgId;
            msgInfo.title = threadInfo.title;
            msgInfo.msg = threadInfo.msg;

            if (rsGxsForums->ForumMessageSend(msgInfo) == false) {
                return;
            }
        }
    }
#endif
}



/*********************** **** **** **** ***********************/
/** Request / Response of Data ********************************/
/*********************** **** **** **** ***********************/

#define 	FORUMSV2DIALOG_LISTING			1
#define 	FORUMSV2DIALOG_CURRENTFORUM		2
#define 	FORUMSV2DIALOG_INSERTTHREADS		3
#define 	FORUMSV2DIALOG_INSERTCHILD		4
#define 	FORUMV2DIALOG_INSERT_POST		5
#define 	FORUMV2DIALOG_REPLY_MESSAGE		6


void GxsForumsDialog::insertForums()
{
	requestGroupSummary();
}

void GxsForumsDialog::requestGroupSummary()
{
        std::cerr << "GxsForumsDialog::requestGroupSummary()";
        std::cerr << std::endl;

        std::list<std::string> ids;
        RsTokReqOptions opts;
	uint32_t token;
        mForumQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, ids, FORUMSV2DIALOG_LISTING);
}

void GxsForumsDialog::loadGroupSummary(const uint32_t &token)
{
        std::cerr << "GxsForumsDialog::loadGroupSummary()";
        std::cerr << std::endl;

        std::list<RsGroupMetaData> groupInfo;
        rsGxsForums->getGroupSummary(token, groupInfo);

        if (groupInfo.size() > 0)
        {
		insertForumsData(groupInfo);
        }
        else
        {
                std::cerr << "GxsForumsDialog::loadGroupSummary() ERROR No Groups...";
                std::cerr << std::endl;
        }
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/



void GxsForumsDialog::requestGroupSummary_CurrentForum(const std::string &forumId)
{
	RsTokReqOptions opts;
	
	std::list<std::string> grpIds;
	grpIds.push_back(forumId);

        std::cerr << "GxsForumsDialog::requestGroupSummary_CurrentForum(" << forumId << ")";
        std::cerr << std::endl;

	uint32_t token;	
	mForumQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_SUMMARY, opts, grpIds, FORUMSV2DIALOG_CURRENTFORUM);
}

void GxsForumsDialog::loadGroupSummary_CurrentForum(const uint32_t &token)
{
        std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum()";
        std::cerr << std::endl;

        std::list<RsGroupMetaData> groupInfo;
        rsGxsForums->getGroupSummary(token, groupInfo);

        if (groupInfo.size() == 1)
        {
		RsGroupMetaData fi = groupInfo.front();
		insertForumThreads(fi);
        }
        else
        {
                std::cerr << "GxsForumsDialog::loadGroupSummary_CurrentForum() ERROR Invalid Number of Groups...";
                std::cerr << std::endl;
        }
}

/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/



void GxsForumsDialog::loadCurrentForumThreads(const std::string &forumId)
{

        std::cerr << "GxsForumsDialog::loadCurrentForumThreads(" << forumId << ")";
        std::cerr << std::endl;

	/* if already active -> kill current loading */
	if (mThreadLoading)
	{
		/* Cleanup */
        	std::cerr << "GxsForumsDialog::loadCurrentForumThreads() Cleanup old Threads";
        	std::cerr << std::endl;

		/* Wipe Widget Tree */
		mThreadLoad.Items.clear();
		
		/* Stop all active requests */
		std::map<uint32_t, QTreeWidgetItem *>::iterator it;
		for(it = mThreadLoad.MsgTokens.begin(); it != mThreadLoad.MsgTokens.end(); it++)
		{
        		std::cerr << "GxsForumsDialog::loadCurrentForumThreads() Canceling Request: " << it->first;
        		std::cerr << std::endl;

			mForumQueue->cancelRequest(it->first);
		}

		mThreadLoad.MsgTokens.clear();
		mThreadLoad.ItemToExpand.clear();
	}

	/* initiate loading */
        std::cerr << "GxsForumsDialog::loadCurrentForumThreads() Initiating Loading";
        std::cerr << std::endl;

	mThreadLoading = true;

	mThreadLoad.ForumId = mCurrForumId;
    	mThreadLoad.FilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());
    	mThreadLoad.ViewType = ui.viewBox->currentIndex();
	mThreadLoad.FillComplete = false;

    	if (lastViewType != mThreadLoad.ViewType || lastForumID != mCurrForumId) {
        	mThreadLoad.FillComplete = true;
    	}

   	mThreadLoad.FlatView = false;
    	mThreadLoad.UseChildTS = false;
	mThreadLoad.ExpandNewMessages = Settings->getExpandNewMessages();
	mThreadLoad.SubscribeFlags = subscribeFlags;

	if (mThreadLoad.ViewType == VIEW_FLAT) {
		ui.threadTreeWidget->setRootIsDecorated(false);
	} else {
		ui.threadTreeWidget->setRootIsDecorated(true);
	}

	switch(mThreadLoad.ViewType)
    	{
		case VIEW_LAST_POST:
		mThreadLoad.UseChildTS = true;
            		break;
		case VIEW_FLAT:
		mThreadLoad.FlatView = true;
			break;
		case VIEW_THREADED:
			break;
	}

	requestGroupThreadData_InsertThreads(forumId);
}



void GxsForumsDialog::requestGroupThreadData_InsertThreads(const std::string &forumId)
{
	RsTokReqOptions opts;

	opts.mOptions = RS_TOKREQOPT_MSG_THREAD | RS_TOKREQOPT_MSG_LATEST;
	
	std::list<std::string> grpIds;
	grpIds.push_back(forumId);

        std::cerr << "GxsForumsDialog::requestGroupThreadData_InsertThreads(" << forumId << ")";
        std::cerr << std::endl;

	uint32_t token;	
	mForumQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, FORUMSV2DIALOG_INSERTTHREADS);
}


void GxsForumsDialog::loadGroupThreadData_InsertThreads(const uint32_t &token)
{
	std::cerr << "GxsForumsDialog::loadGroupThreadData_InsertThreads()";
	std::cerr << std::endl;
	
	bool someData = false;

        std::vector<RsGxsForumMsg> msgs;
        std::vector<RsGxsForumMsg>::iterator vit;
        if (rsGxsForums->getMsgData(token, msgs))
	{
		for(vit = msgs.begin(); vit != msgs.end(); vit++)
		{
			std::cerr << "GxsForumsDialog::loadGroupThreadData_InsertThreads() MsgId: " << vit->mMeta.mMsgId;
			std::cerr << std::endl;
		
			loadForumBaseThread(*vit);
			someData = true;
		}
	}

	/* completed with no data */
	if (!someData)
	{
		fillThreadFinished();
	}
}

bool GxsForumsDialog::convertMsgToThreadWidget(const RsGxsForumMsg &msgInfo, std::string authorName, 
					bool useChildTS, uint32_t filterColumn, QTreeWidgetItem *item)
{
        QString text;

        {
            QDateTime qtime;
            if (useChildTS)
                qtime.setTime_t(msgInfo.mMeta.mChildTs);
            else
                qtime.setTime_t(msgInfo.mMeta.mPublishTs);

            text = qtime.toString("yyyy-MM-dd hh:mm:ss");
            if (useChildTS)
            {
                QDateTime qtime2;
                qtime2.setTime_t(msgInfo.mMeta.mPublishTs);
                QString timestamp2 = qtime2.toString("yyyy-MM-dd hh:mm:ss");
                text += " / ";
                text += timestamp2;
            }
            item->setText(COLUMN_THREAD_DATE, text);
        }

        item->setText(COLUMN_THREAD_TITLE, GxsForumsDialog::titleFromInfo(msgInfo.mMeta));

        text = QString::fromUtf8(authorName.c_str());

        if (text.isEmpty())
        {
            item->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
        }
        else
        {
            item->setText(COLUMN_THREAD_AUTHOR, text);
        }

#ifdef TOGXS
        if (msgInfo.mMeta.mMsgFlags & RS_DISTRIB_AUTHEN_REQ)
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("signed"));
            item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signed.png"));
        }
        else
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("none"));
            item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signature-unknown.png"));
        }
#endif

        if (filterColumn == COLUMN_THREAD_CONTENT) {
            // need content for filter
            QTextDocument doc;
            doc.setHtml(QString::fromUtf8(msgInfo.mMsg.c_str()));
            item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
        }

        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(msgInfo.mMeta.mMsgId));

#if 0
        if (IS_GROUP_SUBSCRIBED(subscribeFlags) && !(msginfo.mMsgFlags & RS_DISTRIB_MISSING_MSG)) {
            rsGxsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
        } else {
            // show message as read
            status = RSGXS_MSG_STATUS_READ;
        }
#endif
        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, msgInfo.mMeta.mMsgStatus);

#ifdef TOGXS
        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, (msgInfo.mMeta.mMsgFlags & RS_DISTRIB_MISSING_MSG) ? true : false);
#endif

	return true;
}


void GxsForumsDialog::loadForumBaseThread(const RsGxsForumMsg &msg)
{
        std::string authorName = rsPeers->getPeerName(msg.mMeta.mAuthorId);
	
	QTreeWidgetItem *item = new QTreeWidgetItem(); // no Parent.	
	
	convertMsgToThreadWidget(msg, authorName, mThreadLoad.UseChildTS, mThreadLoad.FilterColumn, item);

	/* request Children Data */
	uint32_t token;
	RsGxsGrpMsgIdPair parentId = std::make_pair(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
	requestChildData_InsertThreads(token, parentId);

	/* store pair of (token, item) */
	mThreadLoad.MsgTokens[token] = item;

	/* add item to final tree */
	mThreadLoad.Items.append(item);	
}



/*********************** **** **** **** ***********************/

void GxsForumsDialog::requestChildData_InsertThreads(uint32_t &token, const RsGxsGrpMsgIdPair &parentId)
{
	RsTokReqOptions opts;

	opts.mOptions = RS_TOKREQOPT_MSG_PARENT |  RS_TOKREQOPT_MSG_LATEST;
	
        std::cerr << "GxsForumsDialog::requestChildData_InsertThreads(" << parentId.first << "," << parentId.second << ")";
        std::cerr << std::endl;

        std::vector<RsGxsGrpMsgIdPair> msgIds;
        msgIds.push_back(parentId);
        mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, FORUMSV2DIALOG_INSERTCHILD);
}


void GxsForumsDialog::loadChildData_InsertThreads(const uint32_t &token)
{
        std::cerr << "GxsForumsDialog::loadChildData_InsertThreads()";
        std::cerr << std::endl;

	/* find the matching *item */
	std::map<uint32_t, QTreeWidgetItem *>::iterator it;
	it = mThreadLoad.MsgTokens.find(token);
	if (it == mThreadLoad.MsgTokens.end())
	{
		std::cerr << "GxsForumsDialog::loadChildData_InsertThreads() ERROR Missing Token->Parent in Map";
		std::cerr << std::endl;
		/* finished with this one */
		return;
	}

	QTreeWidgetItem *parent = it->second;
	// cleanup map.
	mThreadLoad.MsgTokens.erase(it);

	std::cerr << "GxsForumsDialog::loadChildData_InsertThreads()";
	std::cerr << std::endl;
	

        std::vector<RsGxsForumMsg> msgs;
        std::vector<RsGxsForumMsg>::iterator vit;
        if (rsGxsForums->getMsgData(token, msgs))
	{
		for(vit = msgs.begin(); vit != msgs.end(); vit++)
		{
			std::cerr << "GxsForumsDialog::loadChildData_InsertThreads() MsgId: " << vit->mMeta.mMsgId;
			std::cerr << std::endl;
		
			loadForumChildMsg(*vit, parent);
		}
	}
	else
	{
		std::cerr << "GxsForumsDialog::loadChildData_InsertThreads() Error getting MsgData";
		std::cerr << std::endl;
	}

	/* check for completion */
	if (mThreadLoad.MsgTokens.size() == 0)
	{
		/* finished */
		/* push data into GUI */
		fillThreadFinished();
	}
}

void GxsForumsDialog::loadForumChildMsg(const RsGxsForumMsg &msg, QTreeWidgetItem *parent)
{
        std::string authorName = rsPeers->getPeerName(msg.mMeta.mAuthorId);

	QTreeWidgetItem *child = NULL;

	if (mThreadLoad.FlatView)
	{
		child = new QTreeWidgetItem(); // no Parent.	
	}
	else
	{
		child = new QTreeWidgetItem(parent); 
	}

	
	convertMsgToThreadWidget(msg, authorName, mThreadLoad.UseChildTS, mThreadLoad.FilterColumn, child);

	/* request Children Data */
	uint32_t token;
	RsGxsGrpMsgIdPair parentId = std::make_pair(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
	requestChildData_InsertThreads(token, parentId);

	/* store pair of (token, item) */
	mThreadLoad.MsgTokens[token] = child;

	// Leave this here... BUT IT WILL NEED TO BE FIXED.
	if (mThreadLoad.FillComplete && mThreadLoad.ExpandNewMessages && IS_MSG_UNREAD(msg.mMeta.mMsgStatus)) 
	{
		QTreeWidgetItem *pParent = child;
		while ((pParent = pParent->parent()) != NULL) 
		{
			if (std::find(mThreadLoad.ItemToExpand.begin(), mThreadLoad.ItemToExpand.end(), pParent) == mThreadLoad.ItemToExpand.end()) 
			{
				mThreadLoad.ItemToExpand.push_back(pParent);
			}
		}
	}

	if (mThreadLoad.FlatView)
	{
		/* add item to final tree */
		mThreadLoad.Items.append(child);	
	}
}



/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumsDialog::requestMsgData_InsertPost(const RsGxsGrpMsgIdPair &msgId)
{
	RsTokReqOptions opts;
	
        std::cerr << "GxsForumsDialog::requestMsgData_InsertPost(" << msgId.first << "," << msgId.second << ")";
        std::cerr << std::endl;

        std::vector<RsGxsGrpMsgIdPair> msgIds;
        msgIds.push_back(msgId);
	uint32_t token;	
        mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, FORUMV2DIALOG_INSERT_POST);
}


void GxsForumsDialog::loadMsgData_InsertPost(const uint32_t &token)
{
        std::cerr << "GxsForumsDialog::loadMsgData_InsertPost()";
        std::cerr << std::endl;

        std::vector<RsGxsForumMsg> msgs;
        if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
                	std::cerr << "GxsForumsDialog::loadMsgData_InsertPost() ERROR Wrong number of answers";
                	std::cerr << std::endl;
			return;
		}
		insertPostData(msgs[0]);
	}
        else
        {
                std::cerr << "GxsForumsDialog::loadMsgData_InsertPost() ERROR Missing Message Data...";
                std::cerr << std::endl;
        }
}


/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/

void GxsForumsDialog::requestMsgData_ReplyMessage(const RsGxsGrpMsgIdPair &msgId)
{
	RsTokReqOptions opts;
	
        std::cerr << "GxsForumsDialog::requestMsgData_ReplyMessage(" << msgId.first << "," << msgId.second << ")";
        std::cerr << std::endl;

        std::vector<RsGxsGrpMsgIdPair> msgIds;
        msgIds.push_back(msgId);
	uint32_t token;	
        mForumQueue->requestMsgRelatedInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, FORUMV2DIALOG_REPLY_MESSAGE);
}


void GxsForumsDialog::loadMsgData_ReplyMessage(const uint32_t &token)
{
        std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage()";
        std::cerr << std::endl;

        std::vector<RsGxsForumMsg> msgs;
        if (rsGxsForums->getMsgData(token, msgs))
	{
		if (msgs.size() != 1)
		{
                	std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage() ERROR Wrong number of answers";
                	std::cerr << std::endl;
			return;
		}

		replyMessageData(msgs[0]);
	}
        else
        {
                std::cerr << "GxsForumsDialog::loadMsgData_ReplyMessage() ERROR Missing Message Data...";
                std::cerr << std::endl;
        }
}


/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/



/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
/*********************** **** **** **** ***********************/
	
void GxsForumsDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsForumsDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
				
	if (queue == mForumQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case FORUMSV2DIALOG_LISTING:
				loadGroupSummary(req.mToken);
				break;

			case FORUMSV2DIALOG_CURRENTFORUM:
				loadGroupSummary_CurrentForum(req.mToken);
				break;

			case FORUMSV2DIALOG_INSERTTHREADS:
				loadGroupThreadData_InsertThreads(req.mToken);
				break;

			case FORUMSV2DIALOG_INSERTCHILD:
				loadChildData_InsertThreads(req.mToken);
				break;

			case FORUMV2DIALOG_INSERT_POST:
				loadMsgData_InsertPost(req.mToken);
				break;

			case FORUMV2DIALOG_REPLY_MESSAGE:
				loadMsgData_ReplyMessage(req.mToken);
				break;

			default:
				std::cerr << "GxsForumsDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}





























