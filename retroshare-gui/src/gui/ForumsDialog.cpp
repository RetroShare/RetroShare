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

#include "ForumsDialog.h"
#include "forums/CreateForum.h"
#include "forums/CreateForumMsg.h"
#include "forums/ForumDetails.h"
#include "forums/EditForumDetails.h"
#include "msgs/MessageComposer.h"
#include "settings/rsharesettings.h"
#include "common/Emoticons.h"
#include "common/RSItemDelegate.h"
#include "common/PopularityDefs.h"
#include "RetroShareLink.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsforums.h>

#include <algorithm>


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

/** Constructor */
ForumsDialog::ForumsDialog(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;
    m_bIsForumSubscribed = false;
    m_bIsForumAdmin = false;

    connect( ui.forumTreeWidget, SIGNAL( treeCustomContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
    connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

    connect(ui.actionCreate_Forum, SIGNAL(triggered()), this, SLOT(newforum()));
    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
    connect(ui.newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

    connect( ui.forumTreeWidget, SIGNAL( treeCurrentItemChanged(QString) ), this, SLOT( changedForum(QString) ) );

    connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread () ) );
    connect( ui.threadTreeWidget, SIGNAL( itemClicked(QTreeWidgetItem*,int)), this, SLOT( clickedThread (QTreeWidgetItem*,int) ) );
    connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( changedViewBox () ) );

    connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
    connect(ui.previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
    connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));

    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));
    connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterColumnChanged()));

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << 300 << width(); // Qt calculates the right sizes
    ui.splitter->setSizes(sizes);

    /* Set own item delegate */
    RSItemDelegate *itemDelegate = new RSItemDelegate(this);
    itemDelegate->removeFocusRect(COLUMN_THREAD_READ);
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

    QMenu *forummenu = new QMenu();
    forummenu->addAction(ui.actionCreate_Forum);
    forummenu->addSeparator();
    ui.forumpushButton->setMenu(forummenu);

    /* Initialize group tree */
    ui.forumTreeWidget->initDisplayMenu(ui.displayButton);

    /* create forum tree */
    yourForums = ui.forumTreeWidget->addCategoryItem(tr("Your Forums"), QIcon(IMAGE_FOLDER), true);
    subscribedForums = ui.forumTreeWidget->addCategoryItem(tr("Subscribed Forums"), QIcon(IMAGE_FOLDERRED), true);
    popularForums = ui.forumTreeWidget->addCategoryItem(tr("Popular Forums"), QIcon(IMAGE_FOLDERGREEN), false);
    otherForums = ui.forumTreeWidget->addCategoryItem(tr("Other Forums"), QIcon(IMAGE_FOLDERYELLOW), false);

    m_LastViewType = -1;

    ui.clearButton->hide();

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    ttheader->resizeSection (COLUMN_THREAD_READ,  24);
    ttheader->setResizeMode (COLUMN_THREAD_READ, QHeaderView::Fixed);
    ttheader->hideSection (COLUMN_THREAD_CONTENT);

    insertThreads();

    ui.threadTreeWidget->installEventFilter(this);

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

ForumsDialog::~ForumsDialog()
{
    // save settings
    processSettings(false);
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
        int nValue = FilterColumnToComboBox(Settings->value("filterColumn", true).toInt());
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

void ForumsDialog::forumListCustomPopupMenu( QPoint point )
{
    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Forum"), this, SLOT(subscribeToForum()));
    action->setDisabled (mCurrForumId.empty() || m_bIsForumSubscribed);

    action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Forum"), this, SLOT(unsubscribeToForum()));
    action->setEnabled (!mCurrForumId.empty() && m_bIsForumSubscribed);

    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

    action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
    action->setEnabled (!mCurrForumId.empty ());

    action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
    action->setEnabled (!mCurrForumId.empty () && m_bIsForumAdmin);

    QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
    connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );

    restoreKeysAct->setEnabled(!mCurrForumId.empty() && !m_bIsForumAdmin);
    contextMnu.addAction( restoreKeysAct);

    action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
    action->setEnabled(!mCurrForumId.empty());

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark all as read"), this, SLOT(markMsgAsReadAll()));
    action->setEnabled (!mCurrForumId.empty () && m_bIsForumSubscribed);

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark all as unread"), this, SLOT(markMsgAsUnreadAll()));
    action->setEnabled (!mCurrForumId.empty () && m_bIsForumSubscribed);

    contextMnu.exec(QCursor::pos());
}

void ForumsDialog::threadListCustomPopupMenu( QPoint point )
{
    QMenu contextMnu( this );

    QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply" ), &contextMnu );
    connect( replyAct , SIGNAL( triggered() ), this, SLOT( createmessage() ) );

    QAction *newthreadAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Start New Thread" ), &contextMnu );
    newthreadAct->setEnabled (m_bIsForumSubscribed);
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

    if (m_bIsForumSubscribed) {
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

static void CleanupItems (QList<QTreeWidgetItem *> &Items)
{
    QList<QTreeWidgetItem *>::iterator Item;
    for (Item = Items.begin (); Item != Items.end (); Item++) {
        if (*Item) {
            delete (*Item);
        }
    }
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
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++);
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
    if (mCurrForumId.empty() || m_bIsForumSubscribed == false) {
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

        if (m_bIsForumSubscribed == false) {
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

void ForumsDialog::CalculateIconsAndFonts(QTreeWidgetItem *pItem /*= NULL*/)
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

void ForumsDialog::insertThreads()
{
    /* get the current Forum */
    std::cerr << "ForumsDialog::insertThreads()" << std::endl;

    m_bIsForumSubscribed = false;
    m_bIsForumAdmin = false;

    if (mCurrForumId.empty())
    {
        /* not an actual forum - clear */
        ui.threadTreeWidget->clear();
        /* when no Thread selected - clear */
        ui.forumName->clear();
        ui.threadTitle->clear();
        ui.postText->clear();
        /* clear last stored forumID */
        mCurrForumId.erase();
        m_LastForumID.erase();
        std::cerr << "ForumsDialog::insertThreads() Current Thread Invalid" << std::endl;

        ui.newmessageButton->setEnabled (false);
        ui.newthreadButton->setEnabled (false);

        return;
    }

    ForumInfo fi;
    if (rsForums->getForumInfo (mCurrForumId, fi)) {
        if (fi.subscribeFlags & RS_DISTRIB_ADMIN) {
            m_bIsForumAdmin = true;
        }
        if (fi.subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) {
            m_bIsForumSubscribed = true;
        }
    } else {
        return;
    }

    ui.forumName->setText(QString::fromStdWString(fi.forumName));

    ui.newmessageButton->setEnabled (m_bIsForumSubscribed);
    ui.newthreadButton->setEnabled (m_bIsForumSubscribed);

    bool flatView = false;
    bool useChildTS = false;
    int ViewType = ui.viewBox->currentIndex();
    switch(ViewType)
    {
        case VIEW_LAST_POST:
                useChildTS = true;
                break;
        case VIEW_FLAT:
                flatView = true;
                break;
        default:
        case VIEW_THREADED:
                break;
    }

    if (flatView) {
        ui.threadTreeWidget->setRootIsDecorated( true );
    } else {
        ui.threadTreeWidget->setRootIsDecorated( true );
    }

    bool bExpandNewMessages = Settings->getExpandNewMessages();
    std::list<QTreeWidgetItem*> itemToExpand;

    bool bFillComplete = false;
    if (m_LastViewType != ViewType || m_LastForumID != mCurrForumId) {
        bFillComplete = true;
    }

    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());
    uint32_t status;

    std::list<ThreadInfoSummary> threads;
    std::list<ThreadInfoSummary>::iterator tit;
    rsForums->getForumThreadList(mCurrForumId, threads);

    QList<QTreeWidgetItem *> items;
    for(tit = threads.begin(); tit != threads.end(); tit++)
    {
        std::cerr << "ForumsDialog::insertThreads() Adding TopLevel Thread: mId: ";
        std::cerr << tit->msgId << std::endl;

        ForumMsgInfo msginfo;
        if (rsForums->getForumMessage(mCurrForumId, tit->msgId, msginfo) == false) {
            std::cerr << "ForumsDialog::insertThreads() Failed to Get Msg";
            std::cerr << std::endl;
            continue;
        }

        /* add Msg */
        /* setup
         *
         */

        QTreeWidgetItem *item = new QTreeWidgetItem();

        {
            QDateTime qtime;
            if (useChildTS)
                qtime.setTime_t(tit->childTS);
            else
                qtime.setTime_t(tit->ts);

            QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

            QString txt = timestamp;
            if (useChildTS)
            {
                QDateTime qtime2;
                qtime2.setTime_t(tit->ts);
                QString timestamp2 = qtime2.toString("yyyy-MM-dd hh:mm:ss");
                txt += " / ";
                txt += timestamp2;
            }
            item -> setText(COLUMN_THREAD_DATE, txt);
        }

        item->setText(COLUMN_THREAD_TITLE, QString::fromStdWString(tit->title));

        if (rsPeers->getPeerName(msginfo.srcId) !="")
        {
            item->setText(COLUMN_THREAD_AUTHOR, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
        }
        else
        {
            item->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
        }

        if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("signed"));
            item->setIcon(COLUMN_THREAD_SIGNED,(QIcon(":/images/mail-signed.png")));
        }
        else
        {
            item->setText(COLUMN_THREAD_SIGNED, tr("none"));
            item->setIcon(COLUMN_THREAD_SIGNED,(QIcon(":/images/mail-signature-unknown.png")));
        }

        if (nFilterColumn == COLUMN_THREAD_CONTENT) {
            // need content for filter
            QTextDocument doc;
            doc.setHtml(QString::fromStdWString(msginfo.msg));
            item->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
        }

        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(tit->msgId));

        if (m_bIsForumSubscribed && !(msginfo.msgflags & RS_DISTRIB_MISSING_MSG)) {
            rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
        } else {
            // show message as read
            status = FORUM_MSG_STATUS_READ;
        }
        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, (msginfo.msgflags & RS_DISTRIB_MISSING_MSG) ? true : false);

        std::list<QTreeWidgetItem *> threadlist;
        threadlist.push_back(item);

        while (threadlist.size() > 0)
        {
            /* get children */
            QTreeWidgetItem *parent = threadlist.front();
            threadlist.pop_front();
            std::string pId = parent->data(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID).toString().toStdString();

            std::list<ThreadInfoSummary> msgs;
            std::list<ThreadInfoSummary>::iterator mit;

            std::cerr << "ForumsDialog::insertThreads() Getting Children of : " << pId;
            std::cerr << std::endl;

            if (rsForums->getForumThreadMsgList(mCurrForumId, pId, msgs))
            {
                std::cerr << "ForumsDialog::insertThreads() #Children " << msgs.size();
                std::cerr << std::endl;

                /* iterate through child */
                for(mit = msgs.begin(); mit != msgs.end(); mit++)
                {
                    std::cerr << "ForumsDialog::insertThreads() adding " << mit->msgId;
                    std::cerr << std::endl;

                    ForumMsgInfo msginfo;
                    if (rsForums->getForumMessage(mCurrForumId, mit->msgId, msginfo) == false) {
                        std::cerr << "ForumsDialog::insertThreads() Failed to Get Msg";
                        std::cerr << std::endl;
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

                        QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");

                        QString txt = timestamp;
                        if (useChildTS)
                        {
                            QDateTime qtime2;
                            qtime2.setTime_t(mit->ts);
                            QString timestamp2 = qtime2.toString("yyyy-MM-dd hh:mm:ss");
                            txt += " / ";
                            txt += timestamp2;
                        }
                        child -> setText(COLUMN_THREAD_DATE, txt);
                    }

                    child->setText(COLUMN_THREAD_TITLE, QString::fromStdWString(mit->title));


                    if (rsPeers->getPeerName(msginfo.srcId) !="")
                    {
                        child->setText(COLUMN_THREAD_AUTHOR, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
                    }
                    else
                    {
                        child->setText(COLUMN_THREAD_AUTHOR, tr("Anonymous"));
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

                    if (nFilterColumn == COLUMN_THREAD_CONTENT) {
                        // need content for filter
                        QTextDocument doc;
                        doc.setHtml(QString::fromStdWString(msginfo.msg));
                        child->setText(COLUMN_THREAD_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
                    }

                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MSGID, QString::fromStdString(mit->msgId));

                    if (m_bIsForumSubscribed && !(msginfo.msgflags & RS_DISTRIB_MISSING_MSG)) {
                        rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
                    } else {
                        // show message as read
                        status = FORUM_MSG_STATUS_READ;
                    }
                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_MISSING, (msginfo.msgflags & RS_DISTRIB_MISSING_MSG) ? true : false);

                    if (bFillComplete && bExpandNewMessages && IS_UNREAD(status)) {
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
    }

    ui.postText->clear();
    ui.threadTitle->clear();
    /* add all messages in! */
    if (m_LastViewType != ViewType || m_LastForumID != mCurrForumId) {
        ui.threadTreeWidget->clear();
        m_LastViewType = ViewType;
        m_LastForumID = mCurrForumId;
        ui.threadTreeWidget->insertTopLevelItems(0, items);
    } else {
        FillThreads (items, bExpandNewMessages, itemToExpand);

        CleanupItems (items);
    }

    if (ui.filterPatternLineEdit->text().isEmpty() == false) {
        FilterItems();
    }

    std::list<QTreeWidgetItem*>::iterator Item;
    for (Item = itemToExpand.begin(); Item != itemToExpand.end(); Item++) {
        if ((*Item)->isHidden() == false) {
            (*Item)->setExpanded(true);
        }
    }

    insertPost ();
    CalculateIconsAndFonts();
}

void ForumsDialog::FillThreads(QList<QTreeWidgetItem *> &ThreadList, bool bExpandNewMessages, std::list<QTreeWidgetItem*> &itemToExpand)
{
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
            FillChildren (Thread, *NewThread, bExpandNewMessages, itemToExpand);
        } else {
            // add new thread
            ui.threadTreeWidget->addTopLevelItem (*NewThread);
            Thread = *NewThread;
            *NewThread = NULL;
        }

        uint32_t status = Thread->data (COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        if (bExpandNewMessages && IS_UNREAD(status)) {
            QTreeWidgetItem *pParent = Thread;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }
}

void ForumsDialog::FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent, bool bExpandNewMessages, std::list<QTreeWidgetItem*> &itemToExpand)
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
            FillChildren (Child, NewChild, bExpandNewMessages, itemToExpand);
        } else {
            // add new child
            Child = NewParent->takeChild(NewIndex);
            Parent->addChild (Child);
            NewIndex--;
            NewCount--;
        }

        uint32_t status = Child->data (COLUMN_THREAD_DATA, ROLE_THREAD_STATUS).toUInt();
        if (bExpandNewMessages && IS_UNREAD(status)) {
            QTreeWidgetItem *pParent = Child;
            while ((pParent = pParent->parent()) != NULL) {
                if (std::find(itemToExpand.begin(), itemToExpand.end(), pParent) == itemToExpand.end()) {
                    itemToExpand.push_back(pParent);
                }
            }
        }
    }
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
    }

    ui.newmessageButton->setEnabled (m_bIsForumSubscribed && mCurrThreadId.empty() == false);

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

    QString extraTxt = RsHtml::formatText(QString::fromStdWString(msg.msg), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS);

    ui.postText->setHtml(extraTxt);
    ui.threadTitle->setText(QString::fromStdWString(msg.title));
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

    if (changedItems.size()) {
        for (std::list<QTreeWidgetItem*>::iterator it = changedItems.begin(); it != changedItems.end(); it++) {
            CalculateIconsAndFonts(*it);
        }
        updateMessageSummaryList(mCurrForumId);
    }
}

void ForumsDialog::markMsgAsReadUnread (bool bRead, bool bChildren, bool bForum)
{
    if (mCurrForumId.empty() || m_bIsForumSubscribed == false) {
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
        RetroShareLink link(RetroShareLink::TYPE_FORUM, QString::fromStdWString(fi.forumName), QString::fromStdString(fi.forumId), "");
        if (link.valid() && link.type() == RetroShareLink::TYPE_FORUM) {
            std::vector<RetroShareLink> urls;
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
        RetroShareLink link(RetroShareLink::TYPE_FORUM, QString::fromStdWString(fi.forumName), QString::fromStdString(mCurrForumId), QString::fromStdString(mCurrThreadId));
        if (link.valid() && link.type() == RetroShareLink::TYPE_FORUM) {
            std::vector<RetroShareLink> urls;
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
    if (mCurrForumId.empty () || m_bIsForumSubscribed == false) {
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
    cfm->setWindowTitle(tr("Start New Thread"));
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
        nMsgDialog->insertTitleText(QString::fromStdWString(msgInfo.title), MessageComposer::REPLY);

        QTextDocument doc ;
        doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;

        nMsgDialog->insertPastedText(doc.toPlainText());
        nMsgDialog->addRecipient(MessageComposer::TO, msgInfo.srcId, false);
        nMsgDialog->show();
        nMsgDialog->activateWindow();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply a Anonymous Author"));
    }
}

void ForumsDialog::filterRegExpChanged()
{
//    QRegExp regExp(ui.filterPatternLineEdit->text(),  Qt::CaseInsensitive , QRegExp::FixedString);
//    proxyModel->setFilterRegExp(regExp);

    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.clearButton->hide();
    } else {
        ui.clearButton->show();
    }

    FilterItems();
}

/* clear Filter */
void ForumsDialog::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
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

void ForumsDialog::filterColumnChanged()
{
    if (m_bProcessSettings) {
        return;
    }

    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());
    if (nFilterColumn == COLUMN_THREAD_CONTENT) {
        // need content ... refill
        insertThreads();
    } else {
        FilterItems();
    }

    // save index
    Settings->setValueToGroup("ForumsDialog", "filterColumn", nFilterColumn);
}

void ForumsDialog::FilterItems()
{
    QString sPattern = ui.filterPatternLineEdit->text();
    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());

    int nCount = ui.threadTreeWidget->topLevelItemCount ();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        FilterItem(ui.threadTreeWidget->topLevelItem(nIndex), sPattern, nFilterColumn);
    }
}

bool ForumsDialog::FilterItem(QTreeWidgetItem *pItem, QString &sPattern, int nFilterColumn)
{
    bool bVisible = true;

    if (sPattern.isEmpty() == false) {
        if (pItem->text(nFilterColumn).contains(sPattern, Qt::CaseInsensitive) == false) {
            bVisible = false;
        }
    }

    int nVisibleChildCount = 0;
    int nCount = pItem->childCount();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        if (FilterItem(pItem->child(nIndex), sPattern, nFilterColumn)) {
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
