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

#include "ForumsDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/forums/CreateForum.h"
#include "gui/forums/CreateForumMsg.h"
#include "gui/forums/ForumDetails.h"
#include "msgs/MessageComposer.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"
#include "rsiface/rsforums.h"

#include <sstream>
#include <algorithm>

#include <QtGui>

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

#define COLUMN_COUNT    7
#define COLUMN_DATE     0
#define COLUMN_TITLE    1
#define COLUMN_AUTHOR   2
#define COLUMN_SIGNED   3
#define COLUMN_PARENTID 4
#define COLUMN_MSGID    5
#define COLUMN_CONTENT  6

static int FilterColumnFromComboBox(int nIndex)
{
    switch (nIndex) {
    case 0:
        return COLUMN_DATE;
    case 1:
        return COLUMN_TITLE;
    case 2:
        return COLUMN_AUTHOR;
    case 3:
        return COLUMN_CONTENT;
    }

    return COLUMN_TITLE;
}

static int FilterColumnToComboBox(int nIndex)
{
    switch (nIndex) {
    case COLUMN_DATE:
        return 0;
    case COLUMN_TITLE:
        return 1;
    case COLUMN_AUTHOR:
        return 2;
    case COLUMN_CONTENT:
        return 3;
    }

    return FilterColumnToComboBox(COLUMN_TITLE);
}

/** Constructor */
ForumsDialog::ForumsDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;

    connect( ui.forumTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
    connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

    connect(ui.actionCreate_Forum, SIGNAL(triggered()), this, SLOT(newforum()));
    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
    connect(ui.newthreadButton, SIGNAL(clicked()), this, SLOT(showthread()));

    connect( ui.forumTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this, SLOT( changedForum( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

    connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread () ) );
    connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( changedViewBox () ) );
    connect( ui.postText, SIGNAL( anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

    connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglethreadview()));
    connect(ui.previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
    connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));

    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));
    connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterColumnChanged()));

    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
    timer->start(1000);

    /* Set header resize modes and initial section sizes */
    QHeaderView * ftheader = ui.forumTreeWidget->header () ;
    ftheader->setResizeMode (0, QHeaderView::Interactive);

    ftheader->resizeSection ( 0, 170 );

    /* Set header resize modes and initial section sizes */
    QHeaderView * ttheader = ui.threadTreeWidget->header () ;
    ttheader->setResizeMode (0, QHeaderView::Interactive);

    ttheader->resizeSection ( COLUMN_DATE,  170 );
    ttheader->resizeSection ( COLUMN_TITLE, 170 );
    ttheader->hideSection (COLUMN_CONTENT);


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
    YourForums->setText(0, tr("Your Forums"));
    YourForums->setFont(0, m_ItemFont);
    YourForums->setIcon(0,(QIcon(IMAGE_FOLDER)));
    TopList.append(YourForums);

    SubscribedForums = new QTreeWidgetItem((QTreeWidget*)0);
    SubscribedForums->setText(0, tr("Subscribed Forums"));
    SubscribedForums->setFont(0, m_ItemFont);
    SubscribedForums->setIcon(0,(QIcon(IMAGE_FOLDERRED)));
    TopList.append(SubscribedForums);

    PopularForums = new QTreeWidgetItem();
    PopularForums->setText(0, tr("Popular Forums"));
    PopularForums->setFont(0, m_ItemFont);
    PopularForums->setIcon(0,(QIcon(IMAGE_FOLDERGREEN)));
    TopList.append(PopularForums);

    OtherForums = new QTreeWidgetItem();
    OtherForums->setText(0, tr("Other Forums"));
    OtherForums->setFont(0, m_ItemFont);
    OtherForums->setIcon(0,(QIcon(IMAGE_FOLDERYELLOW)));
    TopList.append(OtherForums);

    ui.forumTreeWidget->addTopLevelItems(TopList);

    YourForums->setExpanded(true);
    SubscribedForums->setExpanded(true);

    m_LastViewType = -1;

    ui.clearButton->hide();

    // load settings
    processSettings(true);

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

    QAction *subForumAct = new QAction(QIcon(IMAGE_SUBSCRIBE), tr( "Subscribe to Forum" ), this );
    subForumAct->setDisabled (true);
    connect( subForumAct , SIGNAL( triggered() ), this, SLOT( subscribeToForum() ) );

    QAction *unsubForumAct = new QAction(QIcon(IMAGE_UNSUBSCRIBE), tr( "Unsubscribe to Forum" ), this );
    unsubForumAct->setDisabled (true);
    connect( unsubForumAct , SIGNAL( triggered() ), this, SLOT( unsubscribeToForum() ) );

    QAction *newForumAct = new QAction(QIcon(IMAGE_NEWFORUM), tr( "New Forum" ), this );
    connect( newForumAct , SIGNAL( triggered() ), this, SLOT( newforum() ) );

    QAction *detailsForumAct = new QAction(QIcon(IMAGE_INFO), tr( "Show Forum Details" ), this );
    detailsForumAct->setDisabled (true);
    connect( detailsForumAct , SIGNAL( triggered() ), this, SLOT( showForumDetails() ) );

    ForumInfo fi;
    if (rsForums && !mCurrForumId.empty ()) {
        if (rsForums->getForumInfo (mCurrForumId, fi)) {
            if ((fi.subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) == 0) {
                subForumAct->setEnabled (true);
            }

            if ((fi.subscribeFlags & (RS_DISTRIB_ADMIN | RS_DISTRIB_SUBSCRIBED)) != 0) {
                unsubForumAct->setEnabled (true);
            }
        }
    }

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

    QAction *replyAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply" ), this );
    replyAct->setDisabled (true);
    connect( replyAct , SIGNAL( triggered() ), this, SLOT( createmessage() ) );

    QAction *viewAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Start New Thread" ), this );
    viewAct->setDisabled (true);
    connect( viewAct , SIGNAL( triggered() ), this, SLOT( showthread() ) );

    QAction *replyauthorAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply to Author" ), this );
    replyauthorAct->setDisabled (true);
    connect( replyauthorAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );

    QAction* expandAll = new QAction(tr( "Expand all" ), this );
    connect( expandAll , SIGNAL( triggered() ), ui.threadTreeWidget, SLOT (expandAll()) );

    QAction* collapseAll = new QAction(tr( "Collapse all" ), this );
    connect( collapseAll , SIGNAL( triggered() ), ui.threadTreeWidget, SLOT(collapseAll()) );

    if (!mCurrForumId.empty ()) {
        viewAct->setEnabled (true);
        if (!mCurrPostId.empty ()) {
            replyAct->setEnabled (true);
            replyauthorAct->setEnabled (true);
        }
    }

    contextMnu.addAction( replyAct);
    contextMnu.addAction( viewAct);
    contextMnu.addAction( replyauthorAct);
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

void ForumsDialog::checkUpdate()
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
				item -> setIcon(0,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
				item -> setIcon(0,(QIcon(IMAGE_FORUM)));
			}

			item -> setText(0, name);

			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setToolTip(0, tr("Popularity: ") + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item -> setText(1, timestamp);
			}
			// Id.
			item -> setText(4, QString::fromStdString(it->forumId));
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
				item -> setIcon(0,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
			  item -> setIcon(0,(QIcon(IMAGE_FORUM)));
			}

			item -> setText(0, name);

			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setToolTip(0, tr("Popularity: ") + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item -> setText(3, timestamp);
			}
			// Id.
			item -> setText(4, QString::fromStdString(it->forumId));
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
				item -> setIcon(0,(QIcon(IMAGE_FORUMAUTHD)));
			}
			else
			{
			  item -> setIcon(0,(QIcon(IMAGE_FORUM)));
			}

			item -> setText(0, name);


			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setToolTip(0, tr("Popularity: ") + QString::fromStdString(out.str()));
			}

			// Date
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item -> setText(3, timestamp);
			}
			// Id.
			item -> setText(4, QString::fromStdString(it->forumId));

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
            if (Child->text (4) == (*NewChild)->text (4)) {
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
            Child->setIcon (0, (*NewChild)->icon (0));
            Child->setToolTip (0, (*NewChild)->toolTip (0));

            for (int i = 0; i <= 4; i++) {
                Child->setText (i, (*NewChild)->text (i));
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
        mCurrPostId = "";
    } else {
        mCurrPostId = (curr->text(COLUMN_MSGID)).toStdString();
    }
    insertPost();
}

void ForumsDialog::insertThreads()
{
    /* get the current Forum */
    std::cerr << "ForumsDialog::insertThreads()" << std::endl;


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
        mCurrForumId = "";
        std::cerr << "ForumsDialog::insertThreads() Current Thread Invalid" << std::endl;

        return;
    }

    /* store forumId */
    mCurrForumId = (forumItem->text(4)).toStdString();
    ui.forumName->setText(forumItem->text(0));
    std::string fId = mCurrForumId;

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

    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());

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
            item -> setText(COLUMN_DATE, txt);
        }
        ForumMsgInfo msginfo ;
        rsForums->getForumMessage(fId,tit->msgId,msginfo) ;

        item->setText(COLUMN_TITLE, QString::fromStdWString(tit->title));

        if (rsPeers->getPeerName(msginfo.srcId) !="")
        {
            item->setText(COLUMN_AUTHOR, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
        }
        else
        {
            item->setText(COLUMN_AUTHOR, tr("Anonymous"));
        }

        if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
        {
            item->setText(COLUMN_SIGNED, tr("signed"));
        }
        else
        {
            item->setText(COLUMN_SIGNED, tr("none"));
        }

        if (nFilterColumn == COLUMN_CONTENT) {
            // need content for filter
            QTextDocument doc;
            doc.setHtml(QString::fromStdWString(msginfo.msg));
            item->setText(COLUMN_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
        }

        item->setText(COLUMN_PARENTID, QString::fromStdString(tit->parentId));
        item->setText(COLUMN_MSGID, QString::fromStdString(tit->msgId));

        std::list<QTreeWidgetItem *> threadlist;
        threadlist.push_back(item);

        while (threadlist.size() > 0)
        {
            /* get children */
            QTreeWidgetItem *parent = threadlist.front();
            threadlist.pop_front();
            std::string pId = (parent->text(COLUMN_MSGID)).toStdString();

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

                    QTreeWidgetItem *child = NULL;
                    if (flatView)
                    {
                        child = new QTreeWidgetItem();
                        ui.threadTreeWidget->setRootIsDecorated( false );
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
                        child -> setText(COLUMN_DATE, txt);
                    }
                    ForumMsgInfo msginfo ;
                    rsForums->getForumMessage(fId,mit->msgId,msginfo) ;

                    child->setText(COLUMN_TITLE, QString::fromStdWString(mit->title));

                    if (rsPeers->getPeerName(msginfo.srcId) !="")
                    {
                        child->setText(COLUMN_AUTHOR, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
                    }
                    else
                    {
                        child->setText(COLUMN_AUTHOR, tr("Anonymous"));
                    }

                    if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
                    {
                        child->setText(COLUMN_SIGNED, tr("signed"));
                    }
                    else
                    {
                        child->setText(COLUMN_SIGNED, tr("none"));
                    }

                    if (nFilterColumn == COLUMN_CONTENT) {
                        // need content for filter
                        QTextDocument doc;
                        doc.setHtml(QString::fromStdWString(msginfo.msg));
                        child->setText(COLUMN_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
                    }

                    child->setText(COLUMN_PARENTID, QString::fromStdString(mit->parentId));
                    child->setText(COLUMN_MSGID, QString::fromStdString(mit->msgId));

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
        FillThreads (items);

        CleanupItems (items);
    }

    if (ui.filterPatternLineEdit->text().isEmpty() == false) {
        FilterItems();
    }

    insertPost ();
}

void ForumsDialog::FillThreads(QList<QTreeWidgetItem *> &ThreadList)
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
            if (Thread->text (COLUMN_MSGID) == (*NewThread)->text (COLUMN_MSGID)) {
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
            if (Thread->text (COLUMN_MSGID) == (*NewThread)->text (COLUMN_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            // set child data
            for (int i = 0; i <= COLUMN_COUNT; i++) {
                Thread->setText (i, (*NewThread)->text (i));
            }

            // fill recursive
            FillChildren (Thread, *NewThread);
        } else {
            // add new thread
            ui.threadTreeWidget->addTopLevelItem (*NewThread);
            *NewThread = NULL;
        }
    }
}

void ForumsDialog::FillChildren(QTreeWidgetItem *Parent, QTreeWidgetItem *NewParent)
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
            if (NewChild->text (COLUMN_MSGID) == Child->text (COLUMN_MSGID)) {
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
            if (Child->text (COLUMN_MSGID) == NewChild->text (COLUMN_MSGID)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            // set child data
            for (int i = 0; i <= COLUMN_COUNT; i++) {
                Child->setText (i, NewChild->text (i));
            }

            // fill recursive
            FillChildren (Child, NewChild);
        } else {
            // add new child
            Parent->addChild (NewParent->takeChild(NewIndex));
            NewIndex--;
            NewCount--;
        }
    }
}

void ForumsDialog::insertPost()
{
    if ((mCurrForumId == "") || (mCurrPostId == ""))
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
    if (!rsForums->getForumMessage(mCurrForumId, mCurrPostId, msg))
    {
        ui.postText->setText("");
        return;
    }

    /* get the Thread */
    ForumMsgInfo title;
    if (!rsForums->getForumMessage(mCurrForumId, mCurrPostId, title))
    {
        ui.threadTitle->setText("");
        return;
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
    ui.threadTitle->setText(QString::fromStdWString(title.title));
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
bool ForumsDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
	return false;
}


// TODO
void ForumsDialog::removemessage()
{
#if 0
	//std::cerr << "ForumsDialog::removemessage()" << std::endl;
	std::string cid, mid;
	if (!getCurrentMsg(cid, mid))
	{
		//std::cerr << "ForumsDialog::removemessage()";
		//std::cerr << " No Message selected" << std::endl;
		return;
	}

	rsMsgs -> MessageDelete(mid);

#endif
	return;
}


// TODO
void ForumsDialog::markMsgAsRead()
{


#if 0
	//std::cerr << "ForumsDialog::markMsgAsRead()" << std::endl;
	std::string cid, mid;
	if (!getCurrentMsg(cid, mid))
	{
		//std::cerr << "ForumsDialog::markMsgAsRead()";
		//std::cerr << " No Message selected" << std::endl;
		return;
	}

	rsMsgs -> MessageRead(mid);
#endif

	return;

}


void ForumsDialog::newforum()
{
    CreateForum cf (this);
    cf.exec ();
}


void ForumsDialog::createmessage()
{
    if (mCurrForumId.empty ()) {
        return;
    }

    CreateForumMsg *cfm = new CreateForumMsg(mCurrForumId, mCurrPostId);
    cfm->show();

    /* window will destroy itself! */
}

void ForumsDialog::showthread()
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
	std::string fId = (forumItem->text(4)).toStdString();

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
    if (mCurrForumId == "")
    {
        return;
    }

    fId = mCurrForumId;
    pId = mCurrPostId;

    ForumMsgInfo msgInfo ;
    rsForums->getForumMessage(fId,pId,msgInfo) ;

    if (rsPeers->getPeerName(msgInfo.srcId) !="")
    {
        MessageComposer *nMsgDialog = new MessageComposer(true);
        nMsgDialog->newMsg();
        nMsgDialog->insertTitleText( (QString("Re: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
        nMsgDialog->setWindowTitle(tr("Re: ") + QString::fromStdWString(msgInfo.title) ) ;

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

	if (link.scheme() == "retroshare")
	{
		RetroShareLink rslnk(link.toString()) ;

		if(rslnk.valid())
		{
			std::list<std::string> srcIds;

			if(rsFiles->FileRequest(rslnk.name().toStdString(), rslnk.hash().toStdString(), rslnk.size(), "", RS_FILE_HINTS_NETWORK_WIDE, srcIds))
			{
				QMessageBox mb(tr("File Request Confirmation"), tr("The file has been added to your download list."),QMessageBox::Information,QMessageBox::Ok,0,0);
				mb.setButtonText( QMessageBox::Ok, "OK" );
				mb.setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
				mb.exec();
			}
			else
			{
				QMessageBox mb(tr("File Request canceled"), tr("The file has not been added to your download list, because you already have it."),QMessageBox::Information,QMessageBox::Ok,0,0);
				mb.setButtonText( QMessageBox::Ok, "OK" );
				mb.exec();
			}
		}
		else
		{
			QMessageBox mb(tr("File Request Error"), tr("The file link is malformed."),QMessageBox::Information,QMessageBox::Ok,0,0);
			mb.setButtonText( QMessageBox::Ok, "OK" );
			mb.setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
			mb.exec();
		}
	}
	else if (link.scheme() == "http")
	{
		QDesktopServices::openUrl(link);
	}
	else if (link.scheme() == "")
	{
		//it's probably a web adress, let's add http:// at the beginning of the link
		QString newAddress = link.toString();
		newAddress.prepend("http://");
		QDesktopServices::openUrl(QUrl(newAddress));
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
    if (nFilterColumn == COLUMN_CONTENT) {
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
