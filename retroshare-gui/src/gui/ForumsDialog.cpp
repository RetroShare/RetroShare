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
#include <QItemDelegate>

#include "ForumsDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/forums/CreateForum.h"
#include "gui/forums/CreateForumMsg.h"
#include "gui/forums/ForumDetails.h"
#include "msgs/MessageComposer.h"
#include "gui/settings/rsharesettings.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsforums.h>

#include <sstream>
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

#define VIEW_LAST_POST	0
#define VIEW_THREADED	1
#define VIEW_FLAT	2

/* Forum constants */
#define COLUMN_FORUM_COUNT    2
#define COLUMN_FORUM_TITLE    0
#define COLUMN_FORUM_DATE     1

#define COLUMN_FORUM_DATA     0

#define ROLE_FORUM_ID         Qt::UserRole
#define ROLE_FORUM_TITLE      Qt::UserRole + 1

#define ROLE_FORUM_COUNT      2

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
// no need to copy, don't count in ROLE_THREAD_COUNT
#define ROLE_THREAD_READCHILDREN    Qt::UserRole + 2
#define ROLE_THREAD_UNREADCHILDREN  Qt::UserRole + 3

#define ROLE_THREAD_COUNT           2

#define IS_UNREAD(status) ((status & FORUM_MSG_STATUS_READ) == 0 || (status & FORUM_MSG_STATUS_UNREAD_BY_USER))

class ForumsItemDelegate : public QItemDelegate
{
public:
    ForumsItemDelegate(QObject *parent = 0) : QItemDelegate(parent)
    {
    }

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem ownOption (option);

        if (index.column() == COLUMN_THREAD_READ) {
            ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
        }

        QItemDelegate::paint (painter, ownOption, index);
    }

    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem ownOption (option);

        if (index.column() == COLUMN_THREAD_READ) {
            ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
        }

        return QItemDelegate::sizeHint(ownOption, index);
    }
};

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

    connect( ui.forumTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
    connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

    connect(ui.actionCreate_Forum, SIGNAL(triggered()), this, SLOT(newforum()));
    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
    connect(ui.newthreadButton, SIGNAL(clicked()), this, SLOT(createthread()));

    connect( ui.forumTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( changedForum( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

    connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread () ) );
    connect( ui.threadTreeWidget, SIGNAL( itemClicked(QTreeWidgetItem*,int)), this, SLOT( clickedThread (QTreeWidgetItem*,int) ) );
    connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( changedViewBox () ) );
    connect( ui.postText, SIGNAL( anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

    connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
    connect(ui.previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
    connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));

    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));
    connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterColumnChanged()));

    /* Set own item delegate */
    QItemDelegate *pDelegate = new ForumsItemDelegate(this);
    ui.threadTreeWidget->setItemDelegate(pDelegate);

    /* Set header resize modes and initial section sizes */
    QHeaderView * ftheader = ui.forumTreeWidget->header () ;
    ftheader->setResizeMode (COLUMN_FORUM_TITLE, QHeaderView::Interactive);
    ftheader->resizeSection (COLUMN_FORUM_TITLE, 170);

    /* Set header resize modes and initial section sizes */
    QHeaderView * ttheader = ui.threadTreeWidget->header () ;
    ttheader->setResizeMode (COLUMN_THREAD_TITLE, QHeaderView::Interactive);
    ttheader->resizeSection (COLUMN_THREAD_DATE,  190);
    ttheader->resizeSection (COLUMN_THREAD_TITLE, 170);

    ui.threadTreeWidget->sortItems( COLUMN_THREAD_DATE, Qt::DescendingOrder );

    /* Set text of column "Read" to empty - without this the column has a number as header text */
    QTreeWidgetItem *headerItem = ui.threadTreeWidget->headerItem();
    headerItem->setText(COLUMN_THREAD_READ, "");

    m_ForumNameFont = QFont("Times", 12, QFont::Bold);
    ui.forumName->setFont(m_ForumNameFont);
    ui.threadTitle->setFont(m_ForumNameFont);

    loadForumEmoticons();

    QMenu *forummenu = new QMenu();
    forummenu->addAction(ui.actionCreate_Forum);
    forummenu->addSeparator();
    ui.forumpushButton->setMenu(forummenu);

    ui.postText->setOpenExternalLinks ( false );
    ui.postText->setOpenLinks ( false );

    /* create forum tree */
    m_ItemFont = QFont("ARIAL", 10);
    m_ItemFont.setBold(true);

    QList<QTreeWidgetItem *> TopList;

    YourForums = new QTreeWidgetItem();
    YourForums->setText(COLUMN_FORUM_TITLE, tr("Your Forums"));
    YourForums->setFont(COLUMN_FORUM_TITLE, m_ItemFont);
    YourForums->setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FOLDER)));
    YourForums->setSizeHint(COLUMN_FORUM_TITLE, QSize( 18,18 ) );
    YourForums->setForeground(COLUMN_FORUM_TITLE, QBrush(QColor(79, 79, 79)));
    TopList.append(YourForums);

    SubscribedForums = new QTreeWidgetItem((QTreeWidget*)0);
    SubscribedForums->setText(COLUMN_FORUM_TITLE, tr("Subscribed Forums"));
    SubscribedForums->setFont(COLUMN_FORUM_TITLE, m_ItemFont);
    SubscribedForums->setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FOLDERRED)));
    SubscribedForums->setSizeHint(COLUMN_FORUM_TITLE, QSize( 18,18 ) );
    SubscribedForums->setForeground(COLUMN_FORUM_TITLE, QBrush(QColor(79, 79, 79)));
    TopList.append(SubscribedForums);

    PopularForums = new QTreeWidgetItem();
    PopularForums->setText(COLUMN_FORUM_TITLE, tr("Popular Forums"));
    PopularForums->setFont(COLUMN_FORUM_TITLE, m_ItemFont);
    PopularForums->setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FOLDERGREEN)));
    PopularForums->setSizeHint(COLUMN_FORUM_TITLE, QSize( 18,18 ) );
    PopularForums->setForeground(COLUMN_FORUM_TITLE, QBrush(QColor(79, 79, 79)));
    TopList.append(PopularForums);

    OtherForums = new QTreeWidgetItem();
    OtherForums->setText(COLUMN_FORUM_TITLE, tr("Other Forums"));
    OtherForums->setFont(COLUMN_FORUM_TITLE, m_ItemFont);
    OtherForums->setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FOLDERYELLOW)));
    OtherForums->setSizeHint(COLUMN_FORUM_TITLE, QSize( 18,18 ) );
    OtherForums->setForeground(COLUMN_FORUM_TITLE, QBrush(QColor(79, 79, 79)));
    TopList.append(OtherForums);

    ui.forumTreeWidget->addTopLevelItems(TopList);

    YourForums->setExpanded(true);
    SubscribedForums->setExpanded(true);

    m_LastViewType = -1;

    ui.clearButton->hide();

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    ttheader->resizeSection (COLUMN_THREAD_READ,  24);
    ttheader->setResizeMode (COLUMN_THREAD_READ, QHeaderView::Fixed);
    ttheader->hideSection (COLUMN_THREAD_CONTENT);

    insertThreads();

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

    Settings->endGroup();
    m_bProcessSettings = false;
}

void ForumsDialog::forumListCustomPopupMenu( QPoint point )
{
    QMenu contextMnu( this );

    QAction *subForumAct = new QAction(QIcon(IMAGE_SUBSCRIBE), tr( "Subscribe to Forum" ), &contextMnu );
    subForumAct->setDisabled (m_bIsForumSubscribed);
    connect( subForumAct , SIGNAL( triggered() ), this, SLOT( subscribeToForum() ) );

    QAction *unsubForumAct = new QAction(QIcon(IMAGE_UNSUBSCRIBE), tr( "Unsubscribe to Forum" ), &contextMnu );
    unsubForumAct->setEnabled (m_bIsForumSubscribed);
    connect( unsubForumAct , SIGNAL( triggered() ), this, SLOT( unsubscribeToForum() ) );

    QAction *newForumAct = new QAction(QIcon(IMAGE_NEWFORUM), tr( "New Forum" ), &contextMnu );
    connect( newForumAct , SIGNAL( triggered() ), this, SLOT( newforum() ) );

    QAction *detailsForumAct = new QAction(QIcon(IMAGE_INFO), tr( "Show Forum Details" ), &contextMnu );
    detailsForumAct->setDisabled (true);
    connect( detailsForumAct , SIGNAL( triggered() ), this, SLOT( showForumDetails() ) );

    if (!mCurrForumId.empty ()) {
        detailsForumAct->setEnabled (true);
    }

    contextMnu.addAction( subForumAct );
    contextMnu.addAction( unsubForumAct );
    contextMnu.addSeparator();
    contextMnu.addAction( newForumAct );
    contextMnu.addAction( detailsForumAct );

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

    QAction *markMsgAsReadAll = new QAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsReadAll, SIGNAL(triggered()), this, SLOT(markMsgAsReadAll()));

    QAction *markMsgAsUnread = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread"), &contextMnu);
    connect(markMsgAsUnread , SIGNAL(triggered()), this, SLOT(markMsgAsUnread()));

    QAction *markMsgAsUnreadAll = new QAction(QIcon(":/images/message-mail.png"), tr("Mark as unread") + " (" + tr ("with children") + ")", &contextMnu);
    connect(markMsgAsUnreadAll , SIGNAL(triggered()), this, SLOT(markMsgAsUnreadAll()));

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
        markMsgAsReadAll->setEnabled(bHasUnreadChildren);
        markMsgAsUnreadAll->setEnabled(bHasReadChildren);

        if (nCount == 1) {
            replyAct->setEnabled (true);
            replyauthorAct->setEnabled (true);
        }
    } else {
        markMsgAsRead->setDisabled(true);
        markMsgAsReadAll->setDisabled(true);
        markMsgAsUnread->setDisabled(true);
        markMsgAsUnreadAll->setDisabled(true);
        replyAct->setDisabled (true);
        replyauthorAct->setDisabled (true);
    }

    contextMnu.addAction( replyAct);
    contextMnu.addAction( newthreadAct);
    contextMnu.addAction( replyauthorAct);
    contextMnu.addSeparator();
    contextMnu.addAction(markMsgAsRead);
    contextMnu.addAction(markMsgAsReadAll);
    contextMnu.addAction(markMsgAsUnread);
    contextMnu.addAction(markMsgAsUnreadAll);
    contextMnu.addSeparator();
    contextMnu.addAction( expandAll);
    contextMnu.addAction( collapseAll);

    contextMnu.exec(QCursor::pos());
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
        ui.expandButton->setToolTip("Hide");
    } else  {
        ui.postText->setVisible(false);
        ui.expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
        ui.expandButton->setToolTip("Expand");
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

void ForumsDialog::insertForums()
{
	std::list<ForumInfo> forumList;
	std::list<ForumInfo>::iterator it;
	if (!rsForums)
	{
		return;
	}

	rsForums->getForumList(forumList);

        QList<QTreeWidgetItem *> AdminList;
        QList<QTreeWidgetItem *> SubList;
        QList<QTreeWidgetItem *> PopList;
        QList<QTreeWidgetItem *> OtherList;
	std::multimap<uint32_t, std::string> popMap;

	for(it = forumList.begin(); it != forumList.end(); it++)
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->subscribeFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			/* own */

			/* Name,
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			QString name = QString::fromStdWString(it->forumName);
			if (it->forumFlags & RS_DISTRIB_AUTHEN_REQ)
			{
				name += " (AUTHD)";
                                item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
                                item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUM)));
			}

                        item -> setText(COLUMN_FORUM_TITLE, name);
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_TITLE, name);

			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
                                item -> setToolTip(COLUMN_FORUM_TITLE, tr("Popularity:") + " " + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
                                item -> setText(COLUMN_FORUM_DATE, timestamp);
			}
			// Id.
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_ID, QString::fromStdString(it->forumId));
			AdminList.append(item);
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			/* subscribed forum */

			/* Name,
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			QString name = QString::fromStdWString(it->forumName);
			if (it->forumFlags & RS_DISTRIB_AUTHEN_REQ)
			{
				name += " (AUTHD)";
                                item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
                          item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUM)));
			}

                        item -> setText(COLUMN_FORUM_TITLE, name);
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_TITLE, name);

			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
                                item -> setToolTip(COLUMN_FORUM_TITLE, tr("Popularity:") + " " + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
                                item -> setText(COLUMN_FORUM_DATE, timestamp);
			}
			// Id.
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_ID, QString::fromStdString(it->forumId));
			SubList.append(item);
		}
		else
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->pop, it->forumId));
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
	std::multimap<uint32_t, std::string>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++);
	if (rit != popMap.rend())
	{
		popLimit = rit->first;
	}

	for(it = forumList.begin(); it != forumList.end(); it++)
	{
		/* ignore the ones we've done already */
		uint32_t flags = it->subscribeFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			continue;
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			continue;
		}
		else
		{
      /* popular forum */

			/* Name,
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			QString name = QString::fromStdWString(it->forumName);
			if (it->forumFlags & RS_DISTRIB_AUTHEN_REQ)
			{
				name += " (AUTHD)";
                                item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
                          item -> setIcon(COLUMN_FORUM_TITLE,(QIcon(IMAGE_FORUM)));
			}

                        item -> setText(COLUMN_FORUM_TITLE, name);
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_TITLE, name);


			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
                                item -> setToolTip(COLUMN_FORUM_TITLE, tr("Popularity:") + " " + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
                                item -> setText(COLUMN_FORUM_DATE, timestamp);
			}
			// Id.
                        item -> setData(COLUMN_FORUM_DATA, ROLE_FORUM_ID, QString::fromStdString(it->forumId));

			if (it->pop < popLimit)
			{
				OtherList.append(item);
			}
			else
			{
				PopList.append(item);
			}
		}
	}

	/* now we can add them in as a tree! */
        FillForums (YourForums, AdminList);
        FillForums (SubscribedForums, SubList);
        FillForums (PopularForums, PopList);
        FillForums (OtherForums, OtherList);

        // cleanup
        CleanupItems (AdminList);
        CleanupItems (SubList);
        CleanupItems (PopList);
        CleanupItems (OtherList);

        updateMessageSummaryList("");
}

void ForumsDialog::FillForums(QTreeWidgetItem *Forum, QList<QTreeWidgetItem *> &ChildList)
{
    int ChildIndex;
    int ChildIndexCur = 0;

    QTreeWidgetItem *Child;

    // iterate all new children
    QList<QTreeWidgetItem *>::iterator NewChild;
    for (NewChild = ChildList.begin (); NewChild != ChildList.end (); NewChild++) {
        // search existing child
        int ChildIndexFound = -1;
        int ChildCount = Forum->childCount ();
        for (ChildIndex = ChildIndexCur; ChildIndex < ChildCount; ChildIndex++) {
            Child = Forum->child (ChildIndex);
            if (Child->data (COLUMN_FORUM_DATA, ROLE_FORUM_ID) == (*NewChild)->data (COLUMN_FORUM_DATA, ROLE_FORUM_ID)) {
                // found it
                ChildIndexFound = ChildIndex;
                break;
            }
        }
        if (ChildIndexFound >= 0) {
            // delete all children between
            while (ChildIndexCur < ChildIndexFound) {
                Child = Forum->takeChild (ChildIndexCur);
                delete (Child);
                ChildIndexFound--;
            }

            // set child data
            Child = Forum->child (ChildIndexFound);
            Child->setIcon (COLUMN_FORUM_TITLE, (*NewChild)->icon (COLUMN_FORUM_TITLE));
            Child->setToolTip (COLUMN_FORUM_TITLE, (*NewChild)->toolTip (COLUMN_FORUM_TITLE));

            int i;
            for (i = 0; i < COLUMN_FORUM_COUNT; i++) {
                Child->setText (i, (*NewChild)->text (i));
            }
            for (i = 0; i < ROLE_FORUM_COUNT; i++) {
                Child->setData (COLUMN_FORUM_DATA, Qt::UserRole + i, (*NewChild)->data (COLUMN_FORUM_DATA, Qt::UserRole + i));
            }
        } else {
            // insert new child
            if (ChildIndexCur < ChildCount) {
                Forum->insertChild (ChildIndexCur, *NewChild);
            } else {
                Forum->addChild (*NewChild);
            }
            *NewChild = NULL;
        }
        ChildIndexCur++;
    }

    // delete rest
    while (ChildIndexCur < Forum->childCount ()) {
        Child = Forum->takeChild (ChildIndexCur);
        delete (Child);
    }
}

void ForumsDialog::changedForum( QTreeWidgetItem *curr, QTreeWidgetItem *prev )
{
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

    // set icon
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
        qf.setBold(bUnread || bMyUnreadChilddren);
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

    QTreeWidgetItem *forumItem = ui.forumTreeWidget->currentItem();
    if ((!forumItem) || (forumItem->parent() == NULL))
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

    /* store forumId */
    mCurrForumId = forumItem->data(COLUMN_FORUM_DATA, ROLE_FORUM_ID).toString().toStdString();
    ui.forumName->setText(forumItem->text(COLUMN_FORUM_TITLE));
    std::string fId = mCurrForumId;

    ForumInfo fi;
    if (rsForums->getForumInfo (fId, fi)) {
        if (fi.subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) {
            m_bIsForumSubscribed = true;
        }
    } else {
        return;
    }

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

        /* add the top threads */
        ForumMsgInfo msg;
        if (!rsForums->getForumMessage(fId, tit->threadId, msg))
        {
            std::cerr << "ForumsDialog::insertThreads() Failed to Get TopLevel Msg";
            std::cerr << std::endl;
            continue;
        }

        ForumMsgInfo msginfo;
        if (rsForums->getForumMessage(fId,tit->msgId,msginfo) == false) {
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
        item->setSizeHint(COLUMN_THREAD_TITLE, QSize( 18,18 ) );

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

        if (m_bIsForumSubscribed) {
            rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
        } else {
            // show message as read
            status = FORUM_MSG_STATUS_READ;
        }
        item->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

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

            if (rsForums->getForumThreadMsgList(fId, pId, msgs))
            {
                std::cerr << "ForumsDialog::insertThreads() #Children " << msgs.size();
                std::cerr << std::endl;

                /* iterate through child */
                for(mit = msgs.begin(); mit != msgs.end(); mit++)
                {
                    std::cerr << "ForumsDialog::insertThreads() adding " << mit->msgId;
                    std::cerr << std::endl;

                    ForumMsgInfo msginfo;
                    if (rsForums->getForumMessage(fId,mit->msgId,msginfo) == false) {
                        std::cerr << "ForumsDialog::insertThreads() Failed to Get Msg";
                        std::cerr << std::endl;
                        continue;
                    }

                    QTreeWidgetItem *child = NULL;
                    if (flatView)
                    {
                        child = new QTreeWidgetItem();
                        ui.threadTreeWidget->setRootIsDecorated( true );
                    }
                    else
                    {
                        child = new QTreeWidgetItem(parent);
                        ui.threadTreeWidget->setRootIsDecorated( true );
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
                    child->setSizeHint(COLUMN_THREAD_TITLE, QSize( 17,17 ) );

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

                    if (m_bIsForumSubscribed) {
                        rsForums->getMessageStatus(msginfo.forumId, msginfo.msgId, status);
                    } else {
                        // show message as read
                        status = FORUM_MSG_STATUS_READ;
                    }
                    child->setData(COLUMN_THREAD_DATA, ROLE_THREAD_STATUS, status);

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

    QString extraTxt;
    extraTxt += QString::fromStdWString(msg.msg);

    QHashIterator<QString, QString> i(smileys);
    while(i.hasNext())
    {
        i.next();
        foreach(QString code, i.key().split("|"))
            extraTxt.replace(code, "<img src=\"" + i.value() + "\" />");
    }

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

void ForumsDialog::markMsgAsReadUnread (bool bRead, bool bAll)
{
    if (mCurrForumId.empty() || m_bIsForumSubscribed == false) {
        return;
    }

    /* get selected messages */
    QList<QTreeWidgetItem*> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL);

    if (bAll) {
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
    markMsgAsReadUnread(true, false);
}

void ForumsDialog::markMsgAsReadAll()
{
    markMsgAsReadUnread(true, true);
}

void ForumsDialog::markMsgAsUnread()
{
    markMsgAsReadUnread(false, false);
}

void ForumsDialog::markMsgAsUnreadAll()
{
    markMsgAsReadUnread(false, true);
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
        QMessageBox::information(this, tr("RetroShare"),tr("No Forum Selected!"));
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
    QTreeWidgetItem *forumItem = ui.forumTreeWidget->currentItem();
    if ((!forumItem) || (forumItem->parent() == NULL))
    {
        return;
    }

    /* store forumId */
    std::string fId = forumItem->data(COLUMN_FORUM_DATA, ROLE_FORUM_ID).toString().toStdString();

    rsForums->forumSubscribe(fId, subscribe);
}

void ForumsDialog::showForumDetails()
{
    if (mCurrForumId == "")
    {
            return;
    }

    ForumDetails fui;

    fui.showDetails (mCurrForumId);
    fui.exec ();
}

void ForumsDialog::loadForumEmoticons()
{
	QString sm_codes;
	#if defined(Q_OS_WIN32)
	QFile sm_file(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
	#else
	QFile sm_file(QString(":/smileys/emotes.acs"));
	#endif
	if(!sm_file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Could not open resouce file :/emoticons/emotes.acs" << std::endl ;
		return ;
	}
	sm_codes = sm_file.readAll();
	sm_file.close();
	sm_codes.remove("\n");
	sm_codes.remove("\r");
	int i = 0;
	QString smcode;
	QString smfile;
	while(sm_codes[i] != '{')
	{
		i++;

	}
	while (i < sm_codes.length()-2)
	{
		smcode = "";
		smfile = "";
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while (sm_codes[i] != '\"')
		{
			smcode += sm_codes[i];
			i++;

		}
		i++;

		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
		{
			smfile += sm_codes[i];
			i++;
		}
		i++;
		if(!smcode.isEmpty() && !smfile.isEmpty())
			#if defined(Q_OS_WIN32)
		    smileys.insert(smcode, smfile);
	        #else
			smileys.insert(smcode, ":/"+smfile);
			#endif
	}
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
        MessageComposer *nMsgDialog = new MessageComposer();
        nMsgDialog->newMsg();
        nMsgDialog->insertTitleText( (QString("Re:") + " " + QString::fromStdWString(msgInfo.title)).toStdString()) ;
        nMsgDialog->setWindowTitle(tr("Re:") + " " + QString::fromStdWString(msgInfo.title) ) ;

        QTextDocument doc ;
        doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
        std::string cited_text(doc.toPlainText().toStdString()) ;

        nMsgDialog->insertPastedText(cited_text) ;
        nMsgDialog->addRecipient( msgInfo.srcId ) ;
        nMsgDialog->show();
        nMsgDialog->activateWindow();

        /* window will destroy itself! */
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),tr("You cant reply a Anonymous Author"));
    }
}

void ForumsDialog::anchorClicked (const QUrl& link )
{
    #ifdef FORUM_DEBUG
    std::cerr << "ForumsDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
    #endif

    RetroShareLink::processUrl(link, NULL, RSLINK_PROCESS_NOTIFY_ALL);
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
    QTreeWidgetItem *apToplevelItem[2] = { YourForums, SubscribedForums };
    int nToplevelItem;

    for (nToplevelItem = 0; nToplevelItem < 2; nToplevelItem++) {
        QTreeWidgetItem *pToplevelItem = apToplevelItem[nToplevelItem];

        int nItem;
        int nItemCount = pToplevelItem->childCount();

        for (nItem = 0; nItem < nItemCount; nItem++) {
            QTreeWidgetItem *pItem = pToplevelItem->child(nItem);
            std::string fId = pItem->data(COLUMN_FORUM_DATA, ROLE_FORUM_ID).toString().toStdString();
            if (forumId.empty() || fId == forumId) {
                /* calculating the new messages */
                unsigned int newMessageCount = 0;
                unsigned int unreadMessageCount = 0;
                rsForums->getMessageCount(fId, newMessageCount, unreadMessageCount);

                QString sTitle = pItem->data(COLUMN_FORUM_DATA, ROLE_FORUM_TITLE).toString();
                QFont qf = pItem->font(COLUMN_FORUM_TITLE);
                if (unreadMessageCount) {
                    sTitle += " (" + QString::number(unreadMessageCount) + ")";
                    qf.setBold(true);
                } else {
                    qf.setBold(false);
                }

                pItem->setText(COLUMN_FORUM_TITLE, sTitle);
                pItem->setFont(COLUMN_FORUM_TITLE, qf);

                if (forumId.empty() == false) {
                    /* calculate only this forum */
                    break;
                }
            }
        }
    }
}
