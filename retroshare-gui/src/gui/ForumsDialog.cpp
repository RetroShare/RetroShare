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
#include "msgs/ChanMsgDialog.h"

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




/** Constructor */
ForumsDialog::ForumsDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.forumTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
  connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

  connect(ui.actionCreate_Forum, SIGNAL(triggered()), this, SLOT(newforum()));
  connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(createmessage()));
  connect(ui.newthreadButton, SIGNAL(clicked()), this, SLOT(showthread()));

  connect( ui.forumTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this,
  	SLOT( changedForum( QTreeWidgetItem *, QTreeWidgetItem * ) ) );


  connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread () ) );
  connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( insertThreads() ) );
  connect( ui.postText, SIGNAL( anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

  connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglefileview()));
  connect(ui.previousButton, SIGNAL(clicked()), this, SLOT(previousMessage()));
  connect(ui.nextButton, SIGNAL(clicked()), this, SLOT(nextMessage()));

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

	ttheader->resizeSection ( 0, 170 );
	ttheader->resizeSection ( 1, 170 );


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

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
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

void ForumsDialog::togglefileview()
{
	/* if msg header visible -> hide by changing splitter
	 * three widgets...
	 */

	QList<int> sizeList = ui.msgSplitter->sizes();
	QList<int>::iterator it;

	int listSize = 0;
	int msgSize = 0;
	int i = 0;

	for(it = sizeList.begin(); it != sizeList.end(); it++, i++)
	{
		if (i == 0)
		{
			listSize = (*it);
		}
		else if (i == 1)
		{
			msgSize = (*it);
		}
	}

	int totalSize = listSize + msgSize;

	bool toShrink = true;
	if (msgSize < (int) totalSize / 10)
	{
		toShrink = false;
	}

	QList<int> newSizeList;
	if (toShrink)
	{
		newSizeList.push_back(totalSize);
		newSizeList.push_back(0);
		ui.expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	    	ui.expandButton->setToolTip("Expand");
	}
	else
	{
		/* no change */
		int nlistSize = (totalSize / 2);
		int nMsgSize = (totalSize / 2);
		newSizeList.push_back(nlistSize);
		newSizeList.push_back(nMsgSize);
	    	ui.expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	    	ui.expandButton->setToolTip("Hide");
	}

	ui.msgSplitter->setSizes(newSizeList);
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
        mCurrPostId = (curr->text(5)).toStdString();
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

#define VIEW_LAST_POST	0
#define VIEW_THREADED	1
#define VIEW_FLAT	2

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
			item -> setText(0, txt);
		}
		ForumMsgInfo msginfo ;
		rsForums->getForumMessage(fId,tit->msgId,msginfo) ;

		item->setText(1, QString::fromStdWString(tit->title));

		if (rsPeers->getPeerName(msginfo.srcId) !="")
		{
		item->setText(2, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
		}
		else
		{
		item->setText(2, tr("Anonymous"));
		}

		if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
    {
    item->setText(3, tr("signed"));
    }
    else
    {
    item->setText(3, tr("none"));
    }

		item->setText(4, QString::fromStdString(tit->parentId));
		item->setText(5, QString::fromStdString(tit->msgId));

		std::list<QTreeWidgetItem *> threadlist;
		threadlist.push_back(item);

		while (threadlist.size() > 0)
		{
			/* get children */
			QTreeWidgetItem *parent = threadlist.front();
			threadlist.pop_front();
			std::string pId = (parent->text(5)).toStdString();

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
						child -> setText(0, txt);
					}
					ForumMsgInfo msginfo ;
					rsForums->getForumMessage(fId,mit->msgId,msginfo) ;

          child->setText(1, QString::fromStdWString(mit->title));

          if (rsPeers->getPeerName(msginfo.srcId) !="")
          {
          child->setText(2, QString::fromStdString(rsPeers->getPeerName(msginfo.srcId)));
          }
          else
          {
          child->setText(2, tr("Anonymous"));
          }

          if (msginfo.msgflags & RS_DISTRIB_AUTHEN_REQ)
          {
          child->setText(3, tr("signed"));
          }
          else
          {
          child->setText(3, tr("none"));
          }

					child->setText(4, QString::fromStdString(mit->parentId));
					child->setText(5, QString::fromStdString(mit->msgId));

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
            if (Thread->text (5) == (*NewThread)->text (5)) {
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
            if (Thread->text (5) == (*NewThread)->text (5)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            // set child data
            for (int i = 0; i <= 5; i++) {
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
            if (NewChild->text (5) == Child->text (5)) {
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
            if (Child->text (5) == NewChild->text (5)) {
                // found it
                Found = Index;
                break;
            }
        }
        if (Found >= 0) {
            // set child data
            for (int i = 0; i <= 5; i++) {
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
		/*
		 */

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
        ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);
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
