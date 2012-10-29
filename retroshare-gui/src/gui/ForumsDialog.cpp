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

#include "ForumsDialog.h"
#include "forums/CreateForum.h"
#include "forums/CreateForumMsg.h"
#include "forums/ForumDetails.h"
#include "forums/EditForumDetails.h"
#include "forums/ForumUserNotify.h"
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
#include <retroshare/rsforums.h>

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

#define IS_UNREAD(status) ((status & FORUM_MSG_STATUS_READ) == 0 || (status & FORUM_MSG_STATUS_UNREAD_BY_USER))
#define IS_FORUM_ADMIN(subscribeFlags) (subscribeFlags & RS_DISTRIB_ADMIN)
#define IS_FORUM_SUBSCRIBED(subscribeFlags) (subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED))

/** Constructor */
ForumsDialog::ForumsDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;
    subscribeFlags = 0;
    inMsgAsReadUnread = false;

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
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

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

    /* Initialize group tree */
    ui.forumTreeWidget->initDisplayMenu(ui.displayButton);

    /* create forum tree */
    yourForums = ui.forumTreeWidget->addCategoryItem(tr("Your Forums"), QIcon(IMAGE_FOLDER), true);
    subscribedForums = ui.forumTreeWidget->addCategoryItem(tr("Subscribed Forums"), QIcon(IMAGE_FOLDERRED), true);
    popularForums = ui.forumTreeWidget->addCategoryItem(tr("Popular Forums"), QIcon(IMAGE_FOLDERGREEN), false);
    otherForums = ui.forumTreeWidget->addCategoryItem(tr("Other Forums"), QIcon(IMAGE_FOLDERYELLOW), false);

    lastViewType = -1;

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Title"), COLUMN_THREAD_TITLE);
    ui.filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_THREAD_DATE);
    ui.filterLineEdit->addFilter(QIcon(), tr("Author"), COLUMN_THREAD_AUTHOR);
    ui.filterLineEdit->addFilter(QIcon(), tr("Content"), COLUMN_THREAD_CONTENT);
    ui.filterLineEdit->setCurrentFilter(COLUMN_THREAD_TITLE);
    // can be removed when the actions of the filter line edit have own placeholder text
    ui.filterLineEdit->setPlaceholderText(tr("Search this forum...")) ;

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    ttheader->resizeSection (COLUMN_THREAD_READ,  24);
    ttheader->setResizeMode (COLUMN_THREAD_READ, QHeaderView::Fixed);
    ttheader->hideSection (COLUMN_THREAD_CONTENT);

    ui.progressBar->hide();
    ui.progLayOutTxt->hide();
    ui.progressBarLayOut->setEnabled(false);

    fillThread = NULL;

    insertThreads();

    ui.threadTreeWidget->installEventFilter(this);
    
    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

ForumsDialog::~ForumsDialog()
{
    if (fillThread) {
        fillThread->stop();
        delete(fillThread);
        fillThread = NULL;
    }

    // save settings
    processSettings(false);
}

UserNotify *ForumsDialog::getUserNotify(QObject *parent)
{
    return new ForumUserNotify(parent);
}

void ForumsDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *pHeader = ui.threadTreeWidget->header () ;

    Settings->beginGroup(QString("ForumsDialog"));

    if (bLoad) {
        // load settings

        // expandFiles
        bool bValue = Settings->value("expandButton", true).toBool();
        ui.expandButton->setChecked(bValue);
        togglethreadview_internal();

        // filterColumn
        ui.filterLineEdit->setCurrentFilter(Settings->value("filterColumn", COLUMN_THREAD_TITLE).toInt());

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

void ForumsDialog::forumListCustomPopupMenu( QPoint /*point*/ )
{
    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Forum"), this, SLOT(subscribeToForum()));
    action->setDisabled (mCurrForumId.empty() || IS_FORUM_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Forum"), this, SLOT(unsubscribeToForum()));
    action->setEnabled (!mCurrForumId.empty() && IS_FORUM_SUBSCRIBED(subscribeFlags));

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

    action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
    action->setEnabled (!mCurrForumId.empty ());

    action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
    action->setEnabled (!mCurrForumId.empty () && IS_FORUM_ADMIN(subscribeFlags));

    QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
    connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
    shareKeyAct->setEnabled(!mCurrForumId.empty() && IS_FORUM_ADMIN(subscribeFlags));
    contextMnu.addAction( shareKeyAct);

    QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
    connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
    restoreKeysAct->setEnabled(!mCurrForumId.empty() && !IS_FORUM_ADMIN(subscribeFlags));
    contextMnu.addAction( restoreKeysAct);

    action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
    action->setEnabled(!mCurrForumId.empty());

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsReadAll()));
    action->setEnabled (!mCurrForumId.empty () && IS_FORUM_SUBSCRIBED(subscribeFlags));

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnreadAll()));
    action->setEnabled (!mCurrForumId.empty () && IS_FORUM_SUBSCRIBED(subscribeFlags));

#ifdef DEBUG_FORUMS
    contextMnu.addSeparator();
    action = contextMnu.addAction("Generate mass data", this, SLOT(generateMassData()));
    action->setEnabled (!mCurrForumId.empty() && IS_FORUM_SUBSCRIBED(subscribeFlags));
#endif

    contextMnu.exec(QCursor::pos());
}

void ForumsDialog::threadListCustomPopupMenu( QPoint /*point*/ )
{
    if (fillThread) {
        return;
    }

    QMenu contextMnu( this );

    QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply" ), &contextMnu );
    connect( replyAct , SIGNAL( triggered() ), this, SLOT( createmessage() ) );

    QAction *newthreadAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Start New Thread" ), &contextMnu );
    newthreadAct->setEnabled (IS_FORUM_SUBSCRIBED(subscribeFlags));
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

    if (IS_FORUM_SUBSCRIBED(subscribeFlags)) {
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

bool ForumsDialog::eventFilter(QObject *obj, QEvent *event)
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

void ForumsDialog::restoreForumKeys(void)
{
	rsForums->forumRestoreKeys(mCurrForumId);
}

void ForumsDialog::togglethreadview()
{
    // save state of button
    Settings->setValueToGroup("ForumsDialog", "expandButton", ui.expandButton->isChecked());

    togglethreadview_internal();
}

void ForumsDialog::togglethreadview_internal()
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

void ForumsDialog::updateDisplay()
{
    std::list<std::string> forumIds;
    std::list<std::string>::iterator it;
    if (!rsForums)
        return;

    if (rsForums->forumsChanged(forumIds))
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

void ForumsDialog::forumInfoToGroupItemInfo(const ForumInfo &forumInfo, GroupItemInfo &groupItemInfo)
{
    groupItemInfo.id = QString::fromStdString(forumInfo.forumId);
    groupItemInfo.name = QString::fromStdWString(forumInfo.forumName);
    groupItemInfo.description = QString::fromStdWString(forumInfo.forumDesc);
    groupItemInfo.popularity = forumInfo.pop;
    groupItemInfo.lastpost = QDateTime::fromTime_t(forumInfo.lastPost);

    if (forumInfo.forumFlags & RS_DISTRIB_AUTHEN_REQ) {
        groupItemInfo.name += " (" + tr("AUTHD") + ")";
        groupItemInfo.icon = QIcon(IMAGE_FORUMAUTHD);
    } else {
        groupItemInfo.icon = QIcon(IMAGE_FORUM);
    }
}

void ForumsDialog::insertForums()
{
	std::list<ForumInfo> forumList;
	std::list<ForumInfo>::iterator it;
	if (!rsForums)
	{
		return;
	}

	rsForums->getForumList(forumList);

    QList<GroupItemInfo> adminList;
    QList<GroupItemInfo> subList;
    QList<GroupItemInfo> popList;
    QList<GroupItemInfo> otherList;
    std::multimap<uint32_t, GroupItemInfo> popMap;

    for (it = forumList.begin(); it != forumList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->subscribeFlags;

        GroupItemInfo groupItemInfo;
        forumInfoToGroupItemInfo(*it, groupItemInfo);

        if (flags & RS_DISTRIB_ADMIN) {
            adminList.push_back(groupItemInfo);
        } else if (flags & RS_DISTRIB_SUBSCRIBED) {
			/* subscribed forum */
            subList.push_back(groupItemInfo);
        } else {
			/* rate the others by popularity */
            popMap.insert(std::make_pair(it->pop, groupItemInfo));
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

void ForumsDialog::changedForum(const QString &id)
{
    mCurrForumId = id.toStdString();

    insertThreads();
}

void ForumsDialog::changedThread ()
{
    if (fillThread) {
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

void ForumsDialog::clickedThread (QTreeWidgetItem *item, int column)
{
    if (mCurrForumId.empty() || !IS_FORUM_SUBSCRIBED(subscribeFlags)) {
        return;
    }

    if (item == NULL) {
        return;
    }

    if (column == COLUMN_THREAD_READ) {
        QList<QTreeWidgetItem*> Rows;
        Rows.append(item);
        uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        setMsgAsReadUnread(Rows, IS_UNREAD(status));
        return;
    }
}

void ForumsDialog::forumMsgReadSatusChanged(const QString &forumId, const QString &msgId, int status)
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

void ForumsDialog::CalculateIconsAndFonts(QTreeWidgetItem *pItem, bool &bHasReadChilddren, bool &bHasUnreadChilddren)
{
    uint32_t status = pItem->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

    bool bUnread = IS_UNREAD(status);
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
        if (status & FORUM_MSG_STATUS_READ) {
            pItem->setIcon(COLUMN_THREAD_TITLE, QIcon());
        } else {
            pItem->setIcon(COLUMN_THREAD_TITLE, QIcon(":/images/message-state-new.png"));
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

        if (!IS_FORUM_SUBSCRIBED(subscribeFlags)) {
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

void ForumsDialog::CalculateIconsAndFonts(QTreeWidgetItem *pItem /* = NULL*/)
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

void ForumsDialog::fillThreadFinished()
{
#ifdef DEBUG_FORUMS
    std::cerr << "ForumsDialog::fillThreadFinished" << std::endl;
#endif

    // thread has finished
    ForumsFillThread *thread = dynamic_cast<ForumsFillThread*>(sender());
    if (thread) {
        if (thread == fillThread) {
            // current thread has finished, hide progressbar and release thread
            ui.progressBar->hide();
            ui.progLayOutTxt->hide();
            ui.progressBarLayOut->setEnabled(false);
            fillThread = NULL;
        }

        if (thread->wasStopped()) {
            // thread was stopped
#ifdef DEBUG_FORUMS
            std::cerr << "ForumsDialog::fillThreadFinished Thread was stopped" << std::endl;
#endif
        } else {
#ifdef DEBUG_FORUMS
            std::cerr << "ForumsDialog::fillThreadFinished Add messages" << std::endl;
#endif
            ui.threadTreeWidget->setSortingEnabled(false);

            /* add all messages in! */
            if (lastViewType != thread->viewType || lastForumID != mCurrForumId) {
                ui.threadTreeWidget->clear();
                lastViewType = thread->viewType;
                lastForumID = mCurrForumId;
                ui.threadTreeWidget->insertTopLevelItems(0, thread->items);

                // clear list
                thread->items.clear();
            } else {
                FillThreads (thread->items, thread->expandNewMessages, thread->itemToExpand);

                // cleanup list
                CleanupItems (thread->items);
            }

            ui.threadTreeWidget->setSortingEnabled(true);

            if (thread->focusMsgId.empty() == false) {
                /* Search exisiting item */
                QTreeWidgetItemIterator itemIterator(ui.threadTreeWidget);
                QTreeWidgetItem *item = NULL;
                while ((item = *itemIterator) != NULL) {
                    itemIterator++;

                    if (item->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString() == thread->focusMsgId) {
                        ui.threadTreeWidget->setCurrentItem(item);
                        ui.threadTreeWidget->setFocus();
                        break;
                    }
                }
            }

            QList<QTreeWidgetItem*>::iterator Item;
            for (Item = thread->itemToExpand.begin(); Item != thread->itemToExpand.end(); Item++) {
                if ((*Item)->isHidden() == false) {
                    (*Item)->setExpanded(true);
                }
            }
            thread->itemToExpand.clear();

            if (ui.filterLineEdit->text().isEmpty() == false) {
                filterItems(ui.filterLineEdit->text());
            }

            insertPost ();
            CalculateIconsAndFonts();

            ui.newthreadButton->setEnabled (IS_FORUM_SUBSCRIBED(subscribeFlags));
        }

#ifdef DEBUG_FORUMS
        std::cerr << "ForumsDialog::fillThreadFinished Delete thread" << std::endl;
#endif

        thread->deleteLater();
        thread = NULL;
    }

#ifdef DEBUG_FORUMS
    std::cerr << "ForumsDialog::fillThreadFinished done" << std::endl;
#endif
}

void ForumsDialog::fillThreadProgress(int current, int count)
{
    // show fill progress
    if (count) {
        ui.progressBar->setValue(current * ui.progressBar->maximum() / count);
    }
}

void ForumsDialog::insertThreads()
{
#ifdef DEBUG_FORUMS
    /* get the current Forum */
    std::cerr << "ForumsDialog::insertThreads()" << std::endl;
#endif

    if (fillThread) {
#ifdef DEBUG_FORUMS
        std::cerr << "ForumsDialog::insertThreads() stop current fill thread" << std::endl;
#endif
        // stop current fill thread
        ForumsFillThread *thread = fillThread;
        fillThread = NULL;
        thread->stop();
        delete(thread);

        ui.progressBar->hide();
        ui.progLayOutTxt->hide();
    }

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
        std::cerr << "ForumsDialog::insertThreads() Current Thread Invalid" << std::endl;
#endif

        return;
    }

    ForumInfo fi;
    if (!rsForums->getForumInfo(mCurrForumId, fi)) {
        return;
    }

    subscribeFlags = fi.subscribeFlags;
    ui.forumName->setText(QString::fromStdWString(fi.forumName));

    ui.progressBarLayOut->setEnabled(true);

    ui.progLayOutTxt->show();
    ui.progressBar->reset();
    ui.progressBar->show();

    // create fill thread
    fillThread = new ForumsFillThread(this);

    // set data
    fillThread->forumId = mCurrForumId;
    fillThread->filterColumn = ui.filterLineEdit->currentFilter();
    fillThread->subscribeFlags = subscribeFlags;
    fillThread->viewType = ui.viewBox->currentIndex();
    if (lastViewType != fillThread->viewType || lastForumID != mCurrForumId) {
        fillThread->fillComplete = true;
    }

    if (fillThread->viewType == VIEW_FLAT) {
        ui.threadTreeWidget->setRootIsDecorated(false);
    } else {
        ui.threadTreeWidget->setRootIsDecorated(true);
    }

    // connect thread
    connect(fillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
    connect(fillThread, SIGNAL(progress(int,int)), this, SLOT(fillThreadProgress(int,int)));

#ifdef DEBUG_FORUMS
    std::cerr << "ForumsDialog::insertThreads() Start fill thread" << std::endl;
#endif

    // start thread
    fillThread->start();
}

void ForumsDialog::FillThreads(QList<QTreeWidgetItem *> &ThreadList, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
{
#ifdef DEBUG_FORUMS
    std::cerr << "ForumsDialog::FillThreads()" << std::endl;
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
        if (expandNewMessages && IS_UNREAD(status)) {
            QTreeWidgetItem *pParent = Thread;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }

#ifdef DEBUG_FORUMS
    std::cerr << "ForumsDialog::FillThreads() done" << std::endl;
#endif
}

void ForumsDialog::FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent, bool expandNewMessages, QList<QTreeWidgetItem*> &itemToExpand)
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
        if (expandNewMessages && IS_UNREAD(status)) {
            QTreeWidgetItem *pParent = Child;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }
}

QString ForumsDialog::titleFromInfo(ForumMsgInfo &msgInfo)
{
    if (msgInfo.msgflags & RS_DISTRIB_MISSING_MSG) {
        return QApplication::translate("ForumsDialog", "[ ... Missing Message ... ]");
    }

    return QString::fromStdWString(msgInfo.title);
}

QString ForumsDialog::messageFromInfo(ForumMsgInfo &msgInfo)
{
    if (msgInfo.msgflags & RS_DISTRIB_MISSING_MSG) {
        return QApplication::translate("ForumsDialog", "Placeholder for missing Message");
    }

    return QString::fromStdWString(msgInfo.msg);
}

void ForumsDialog::insertPost()
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

    ui.newmessageButton->setEnabled (IS_FORUM_SUBSCRIBED(subscribeFlags) && mCurrThreadId.empty() == false);

    /* get the Post */
    ForumMsgInfo msg;
    if (!rsForums->getForumMessage(mCurrForumId, mCurrThreadId, msg))
    {
        ui.postText->setText("");
        return;
    }

    bool bSetToReadOnActive = Settings->getForumMsgSetToReadOnActivate();
    uint32_t status = curr->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();

    QList<QTreeWidgetItem*> Row;
    Row.append(curr);
    if (status & FORUM_MSG_STATUS_READ) {
        if (bSetToReadOnActive && (status & FORUM_MSG_STATUS_UNREAD_BY_USER)) {
            /* set to read */
            setMsgAsReadUnread(Row, true);
        }
    } else {
        /* set to read */
        if (bSetToReadOnActive) {
            setMsgAsReadUnread(Row, true);
        } else {
            /* set to unread by user */
            setMsgAsReadUnread(Row, false);
        }
    }

    QString extraTxt = RsHtml().formatText(ui.postText->document(), messageFromInfo(msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);

    ui.postText->setHtml(extraTxt);
    ui.threadTitle->setText(titleFromInfo(msg));
}

void ForumsDialog::previousMessage ()
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

void ForumsDialog::nextMessage ()
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

void ForumsDialog::downloadAllFiles()
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

void ForumsDialog::nextUnreadMessage()
{
    QTreeWidgetItem *currentItem = ui.threadTreeWidget->currentItem();

    while (TRUE) {
        QTreeWidgetItemIterator itemIterator = currentItem ? QTreeWidgetItemIterator(currentItem, QTreeWidgetItemIterator::NotHidden) : QTreeWidgetItemIterator(ui.threadTreeWidget, QTreeWidgetItemIterator::NotHidden);

        QTreeWidgetItem *item;
        while ((item = *itemIterator) != NULL) {
            itemIterator++;

            if (item == currentItem) {
                continue;
            }

            uint32_t status = item->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
            if (IS_UNREAD(status)) {
                ui.threadTreeWidget->setCurrentItem(item);
                ui.threadTreeWidget->scrollToItem(item, QAbstractItemView::EnsureVisible);
                return;
            }
        }

        if (currentItem == NULL) {
            break;
        }

        /* start from top */
        currentItem = NULL;
    }
}

// TODO
#if 0
void ForumsDialog::removemessage()
{
	//std::cerr << "ForumsDialog::removemessage()" << std::endl;
	std::string cid, mid;
	if (!getCurrentMsg(cid, mid))
	{
		//std::cerr << "ForumsDialog::removemessage()";
		//std::cerr << " No Message selected" << std::endl;
		return;
	}

	rsMsgs -> MessageDelete(mid);
}
#endif

/* get selected messages
   the messages tree is single selected, but who knows ... */
int ForumsDialog::getSelectedMsgCount(QList<QTreeWidgetItem*> *pRows, QList<QTreeWidgetItem*> *pRowsRead, QList<QTreeWidgetItem*> *pRowsUnread)
{
    if (pRowsRead) pRowsRead->clear();
    if (pRowsUnread) pRowsUnread->clear();

    QList<QTreeWidgetItem*> selectedItems = ui.threadTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::iterator it = selectedItems.begin(); it != selectedItems.end(); it++) {
        if (pRows) pRows->append(*it);
        if (pRowsRead || pRowsUnread) {
            uint32_t status = (*it)->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
            if (IS_UNREAD(status)) {
                if (pRowsUnread) pRowsUnread->append(*it);
            } else {
                if (pRowsRead) pRowsRead->append(*it);
            }
        }
    }

    return selectedItems.size();
}

void ForumsDialog::setMsgAsReadUnread(QList<QTreeWidgetItem*> &Rows, bool bRead)
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

        /* set always as read ... */
        uint32_t statusNew = status | FORUM_MSG_STATUS_READ;
        if (bRead) {
            /* ... and as read by user */
            statusNew &= ~FORUM_MSG_STATUS_UNREAD_BY_USER;
        } else {
            /* ... and as unread by user */
            statusNew |= FORUM_MSG_STATUS_UNREAD_BY_USER;
        }
        if (status != statusNew) {
            std::string msgId = (*Row)->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();
            rsForums->setMessageStatus(mCurrForumId, msgId, statusNew, FORUM_MSG_STATUS_READ | FORUM_MSG_STATUS_UNREAD_BY_USER);

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

void ForumsDialog::markMsgAsReadUnread (bool bRead, bool bChildren, bool bForum)
{
    if (mCurrForumId.empty() || !IS_FORUM_SUBSCRIBED(subscribeFlags)) {
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

            /* add only items with the right state or with not FORUM_MSG_STATUS_READ */
            uint32_t status = pRow->data(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
             if (IS_UNREAD(status) == bRead || (status & FORUM_MSG_STATUS_READ) == 0) {
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

        setMsgAsReadUnread (AllRows, bRead);

        return;
    }

    setMsgAsReadUnread (Rows, bRead);
}

void ForumsDialog::markMsgAsRead()
{
    markMsgAsReadUnread(true, false, false);
}

void ForumsDialog::markMsgAsReadChildren()
{
    markMsgAsReadUnread(true, true, false);
}

void ForumsDialog::markMsgAsReadAll()
{
    markMsgAsReadUnread(true, true, true);
}

void ForumsDialog::markMsgAsUnread()
{
    markMsgAsReadUnread(false, false, false);
}

void ForumsDialog::markMsgAsUnreadChildren()
{
    markMsgAsReadUnread(false, true, false);
}

void ForumsDialog::markMsgAsUnreadAll()
{
    markMsgAsReadUnread(false, true, true);
}

void ForumsDialog::copyForumLink()
{
    if (mCurrForumId.empty()) {
        return;
    }

    ForumInfo fi;
    if (rsForums->getForumInfo(mCurrForumId, fi)) {
        RetroShareLink link;
        if (link.createForum(fi.forumId, "")) {
            QList<RetroShareLink> urls;
            urls.push_back(link);
            RSLinkClipboard::copyLinks(urls);
        }
    }
}

void ForumsDialog::copyMessageLink()
{
    if (mCurrForumId.empty() || mCurrThreadId.empty()) {
        return;
    }

    ForumInfo fi;
    if (rsForums->getForumInfo(mCurrForumId, fi)) {
        RetroShareLink link;
        if (link.createForum(mCurrForumId, mCurrThreadId)) {
            QList<RetroShareLink> urls;
            urls.push_back(link);
            RSLinkClipboard::copyLinks(urls);
        }
    }
}

void ForumsDialog::newforum()
{
    CreateForum cf (this);
    cf.exec ();
}

void ForumsDialog::createmessage()
{
    if (mCurrForumId.empty () || !IS_FORUM_SUBSCRIBED(subscribeFlags)) {
        return;
    }

    CreateForumMsg *cfm = new CreateForumMsg(mCurrForumId, mCurrThreadId);
    cfm->show();

    /* window will destroy itself! */
}

void ForumsDialog::createthread()
{
    if (mCurrForumId.empty ()) {
        QMessageBox::information(this, tr("RetroShare"), tr("No Forum Selected!"));
        return;
    }

    CreateForumMsg *cfm = new CreateForumMsg(mCurrForumId, "");
    cfm->show();

    /* window will destroy itself! */
}

void ForumsDialog::subscribeToForum()
{
	forumSubscribe(true);
}

void ForumsDialog::unsubscribeToForum()
{
	forumSubscribe(false);
}

void ForumsDialog::forumSubscribe(bool subscribe)
{
    if (mCurrForumId.empty()) {
            return;
    }

    rsForums->forumSubscribe(mCurrForumId, subscribe);
}

void ForumsDialog::showForumDetails()
{
    if (mCurrForumId.empty()) {
            return;
    }

    ForumDetails fui;

    fui.showDetails (mCurrForumId);
    fui.exec ();
}

void ForumsDialog::editForumDetails()
{
    if (mCurrForumId.empty()) {
            return;
    }

    EditForumDetails editUi(mCurrForumId, this);
    editUi.exec();
}

static QString buildReplyHeader(const ForumMsgInfo &msgInfo)
{
    RetroShareLink link;
    link.createMessage(msgInfo.srcId, "");
    QString from = link.toHtml();

    QDateTime qtime;
    qtime.setTime_t(msgInfo.ts);

    QString header = QString("<span>-----%1-----").arg(QApplication::translate("ForumsDialog", "Original Message"));
    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("ForumsDialog", "From"), from);

    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(QApplication::translate("ForumsDialog", "Sent"), qtime.toString(Qt::SystemLocaleLongDate));
    header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(QApplication::translate("ForumsDialog", "Subject"), QString::fromStdWString(msgInfo.title));
    header += "<br>";

    header += QApplication::translate("ForumsDialog", "On %1, %2 wrote:").arg(qtime.toString(Qt::SystemLocaleShortDate), from);

    return header;
}

void ForumsDialog::replytomessage()
{
    if (mCurrForumId.empty()) {
        return;
    }

    std::string fId = mCurrForumId;
    std::string pId = mCurrThreadId;

    ForumMsgInfo msgInfo ;
    rsForums->getForumMessage(fId,pId,msgInfo) ;

    if (rsPeers->getPeerName(msgInfo.srcId) !="")
    {
        MessageComposer *nMsgDialog = MessageComposer::newMsg();
        nMsgDialog->setTitleText(QString::fromStdWString(msgInfo.title), MessageComposer::REPLY);

        nMsgDialog->setQuotedMsg(QString::fromStdWString(msgInfo.msg), buildReplyHeader(msgInfo));
        nMsgDialog->addRecipient(MessageComposer::TO, msgInfo.srcId, false);

        nMsgDialog->show();
        nMsgDialog->activateWindow();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You can't reply an Anonymous Author"));
    }
}

void ForumsDialog::changedViewBox()
{
    if (m_bProcessSettings) {
        return;
    }

    // save index
    Settings->setValueToGroup("ForumsDialog", "viewBox", ui.viewBox->currentIndex());

    insertThreads();
}

void ForumsDialog::filterColumnChanged(int column)
{
    if (m_bProcessSettings) {
        return;
    }

    if (column == COLUMN_THREAD_CONTENT) {
        // need content ... refill
        insertThreads();
    } else {
        filterItems(ui.filterLineEdit->text());
    }

    // save index
    Settings->setValueToGroup("ForumsDialog", "filterColumn", column);
}

void ForumsDialog::filterItems(const QString& text)
{
    int filterColumn = ui.filterLineEdit->currentFilter();

    int nCount = ui.threadTreeWidget->topLevelItemCount ();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        filterItem(ui.threadTreeWidget->topLevelItem(nIndex), text, filterColumn);
    }
}

void ForumsDialog::shareKey()
{
    ShareKey shareUi(this, 0, mCurrForumId, FORUM_KEY_SHARE);
    shareUi.exec();
}

bool ForumsDialog::filterItem(QTreeWidgetItem *pItem, const QString &text, int filterColumn)
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

void ForumsDialog::updateMessageSummaryList(std::string forumId)
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
                rsForums->getMessageCount(childId, newMessageCount, unreadMessageCount);

                ui.forumTreeWidget->setUnreadCount(childItem, unreadMessageCount);

                if (forumId.empty() == false) {
                    /* Calculate only this forum */
                    break;
                }
            }
        }
    }
}

bool ForumsDialog::navigate(const std::string& forumId, const std::string& msgId)
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

    if (fillThread && fillThread->isRunning()) {
        fillThread->focusMsgId = msgId;
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

void ForumsDialog::generateMassData()
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

        if (rsForums->ForumMessageSend(threadInfo) == false) {
            return;
        }

        for (int msg = 1; msg < 3; msg++) {
            ForumMsgInfo msgInfo;
            msgInfo.forumId = mCurrForumId;
            msgInfo.threadId = threadInfo.msgId;
            msgInfo.parentId = threadInfo.msgId;
            msgInfo.title = threadInfo.title;
            msgInfo.msg = threadInfo.msg;

            if (rsForums->ForumMessageSend(msgInfo) == false) {
                return;
            }
        }
    }
#endif
}

// ForumsFillThread
ForumsFillThread::ForumsFillThread(ForumsDialog *parent)
    : QThread(parent)
{
    stopped = false;

    expandNewMessages = Settings->getExpandNewMessages();
    fillComplete = false;

    filterColumn = 0;
    subscribeFlags = 0;
    viewType = 0;
}

ForumsFillThread::~ForumsFillThread()
{
#ifdef DEBUG_FORUMS
    std::cerr << "ForumsFillThread::~ForumsFillThread" << std::endl;
#endif
    // remove all items (when items are available, the thread was terminated)
    CleanupItems (items);
    itemToExpand.clear();
}

void ForumsFillThread::stop()
{
    disconnect();
    stopped = true;
    QApplication::processEvents();
    wait();
}

void ForumsFillThread::run()
{
#ifdef DEBUG_FORUMS
    std::cerr << "ForumsFillThread::run()" << std::endl;
#endif

    uint32_t status;

    std::list<ThreadInfoSummary> threads;
    std::list<ThreadInfoSummary>::iterator tit;
    rsForums->getForumThreadList(forumId, threads);

    bool flatView = false;
    bool useChildTS = false;
    switch(viewType)
    {
        case VIEW_LAST_POST:
            useChildTS = true;
            break;
        case VIEW_FLAT:
            flatView = true;
            break;
        case VIEW_THREADED:
            break;
    }

    int count = threads.size();
    int pos = 0;

    for (tit = threads.begin(); tit != threads.end(); tit++)
    {
        if (stopped) {
            break;
        }

#ifdef DEBUG_FORUMS
        std::cerr << "ForumsFillThread::run() Adding TopLevel Thread: mId: " << tit->msgId << std::endl;
#endif

        ForumMsgInfo msginfo;
        if (rsForums->getForumMessage(forumId, tit->msgId, msginfo) == false) {
#ifdef DEBUG_FORUMS
            std::cerr << "ForumsFillThread::run() Failed to Get Msg" << std::endl;
#endif
            continue;
        }

        /* add Msg */
        /* setup
         *
         */

        QTreeWidgetItem *item = new QTreeWidgetItem();

        QString text;

        {
            QDateTime qtime;
            if (useChildTS)
                qtime.setTime_t(tit->childTS);
            else
                qtime.setTime_t(tit->ts);

            text = qtime.toString("yyyy-MM-dd hh:mm:ss");
            if (useChildTS)
            {
                QDateTime qtime2;
                qtime2.setTime_t(tit->ts);
                QString timestamp2 = qtime2.toString("yyyy-MM-dd hh:mm:ss");
                text += " / ";
                text += timestamp2;
            }
            item->setText(COLUMN_THREAD_DATE, text);
        }

        item->setText(COLUMN_THREAD_TITLE, ForumsDialog::titleFromInfo(msginfo));

        text = QString::fromUtf8(rsPeers->getPeerName(msginfo.srcId).c_str());
        if (text.isEmpty())
        {
            item->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
        }
        else
        {
            item->setText(COLUMN_THREAD_AUTHOR, text);
        }

        if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("signed"));
            item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signed.png"));
        }
        else
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("none"));
            item->setIcon(COLUMN_THREAD_SIGNED, QIcon(":/images/mail-signature-unknown.png"));
        }

        if (filterColumn == COLUMN_THREAD_CONTENT) {
            // need content for filter
            QTextDocument doc;
            doc.setHtml(QString::fromStdWString(msginfo.msg));
            item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
        }

        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(tit->msgId));

        if (IS_FORUM_SUBSCRIBED(subscribeFlags) && !(msginfo.msgflags & RS_DISTRIB_MISSING_MSG)) {
            rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
        } else {
            // show message as read
            status = FORUM_MSG_STATUS_READ;
        }
        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, (msginfo.msgflags & RS_DISTRIB_MISSING_MSG) ? true : false);

        std::list<QTreeWidgetItem*> threadlist;
        threadlist.push_back(item);

        while (threadlist.size() > 0)
        {
            if (stopped) {
                break;
            }

            /* get children */
            QTreeWidgetItem *parent = threadlist.front();
            threadlist.pop_front();
            std::string pId = parent->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();

            std::list<ThreadInfoSummary> msgs;
            std::list<ThreadInfoSummary>::iterator mit;

#ifdef DEBUG_FORUMS
            std::cerr << "ForumsFillThread::run() Getting Children of : " << pId << std::endl;
#endif

            if (rsForums->getForumThreadMsgList(forumId, pId, msgs))
            {
#ifdef DEBUG_FORUMS
                std::cerr << "ForumsFillThread::run() #Children " << msgs.size() << std::endl;
#endif

                /* iterate through child */
                for(mit = msgs.begin(); mit != msgs.end(); mit++)
                {
#ifdef DEBUG_FORUMS
                    std::cerr << "ForumsFillThread::run() adding " << mit->msgId << std::endl;
#endif

                    ForumMsgInfo msginfo;
                    if (rsForums->getForumMessage(forumId, mit->msgId, msginfo) == false) {
#ifdef DEBUG_FORUMS
                        std::cerr << "ForumsFillThread::run() Failed to Get Msg" << std::endl;
#endif
                        continue;
                    }

                    QTreeWidgetItem *child = NULL;
                    if (flatView)
                    {
                        child = new QTreeWidgetItem();
                    }
                    else
                    {
                        child = new QTreeWidgetItem(parent);
                    }

                    {
                        QDateTime qtime;
                        if (useChildTS)
                            qtime.setTime_t(mit->childTS);
                        else
                            qtime.setTime_t(mit->ts);

                        text = qtime.toString("yyyy-MM-dd hh:mm:ss");

                        if (useChildTS)
                        {
                            QDateTime qtime2;
                            qtime2.setTime_t(mit->ts);
                            QString timestamp2 = qtime2.toString("yyyy-MM-dd hh:mm:ss");
                            text += " / ";
                            text += timestamp2;
                        }
                        child->setText(COLUMN_THREAD_DATE, text);
                    }

                    child->setText(COLUMN_THREAD_TITLE, ForumsDialog::titleFromInfo(msginfo));

                    text = QString::fromUtf8(rsPeers->getPeerName(msginfo.srcId).c_str());
                    if (text.isEmpty())
                    {
                        child->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
                    }
                    else
                    {
                        child->setText(COLUMN_THREAD_AUTHOR, text);
                    }

                    if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
                    {
                        child->setText(COLUMN_THREAD_SIGNED, tr("signed"));
                        child->setIcon(COLUMN_THREAD_SIGNED,(QIcon(":/images/mail-signed.png")));
                    }
                    else
                    {
                        child->setText(COLUMN_THREAD_SIGNED, tr("none"));
                        child->setIcon(COLUMN_THREAD_SIGNED,(QIcon(":/images/mail-signature-unknown.png")));
                    }

                    if (filterColumn == COLUMN_THREAD_CONTENT) {
                        // need content for filter
                        QTextDocument doc;
                        doc.setHtml(ForumsDialog::messageFromInfo(msginfo));
                        child->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
                    }

                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(mit->msgId));

                    if (IS_FORUM_SUBSCRIBED(subscribeFlags) && !(msginfo.msgflags & RS_DISTRIB_MISSING_MSG)) {
                        rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
                    } else {
                        // show message as read
                        status = FORUM_MSG_STATUS_READ;
                    }
                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, (msginfo.msgflags & RS_DISTRIB_MISSING_MSG) ? true : false);

                    if (fillComplete && expandNewMessages && IS_UNREAD(status)) {
                        QTreeWidgetItem *pParent = child;
                        while ((pParent = pParent->parent()) != NULL) {
                            if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                                itemToExpand.push_back(pParent);
                            }
                        }
                    }

                    /* setup child */
                    threadlist.push_back(child);

                    if (flatView)
                    {
                        items.append(child);
                    }
                }
            }
        }

        /* add to list */
        items.append(item);

        emit progress(++pos, count);
    }

#ifdef DEBUG_FORUMS
    std::cerr << "ForumsFillThread::run() stopped: " << (wasStopped() ? "yes" : "no") << std::endl;
#endif
}
