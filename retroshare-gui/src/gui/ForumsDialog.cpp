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
#include "gui/forums/CreateForum.h"
#include "gui/forums/CreateForumMsg.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"
#include "rsiface/rsforums.h"
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QDateTime>
#include <QHeaderView>
#include <QTimer>

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"


/** Constructor */
ForumsDialog::ForumsDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.forumTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( forumListCustomPopupMenu( QPoint ) ) );
  connect( ui.threadTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( threadListCustomPopupMenu( QPoint ) ) );

  connect(ui.newForumButton, SIGNAL(clicked()), this, SLOT(newforum()));


  connect( ui.forumTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this,
  	SLOT( changedForum( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

  //connect( ui.threadTreeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem *, QTreeWidgetItem *) ), this,
  //	SLOT( changedThread( QTreeWidgetItem *, QTreeWidgetItem * ) ) );

  connect( ui.threadTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( changedThread2() ) );
  connect( ui.viewBox, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( changedThread2() ) );
  
  connect(ui.expandButton, SIGNAL(clicked()), this, SLOT(togglefileview()));

  QTimer *timer = new QTimer(this);
  timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
  timer->start(1000);


#if 0
  connect( ui.msgWidget, SIGNAL( itemClicked ( QTreeWidgetItem *, int) ), this, SLOT( updateMessages ( QTreeWidgetItem *, int) ) );
  connect( ui.listWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeBox ( int) ) );
  
  connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));
  connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
  //connect(ui.printbutton, SIGNAL(clicked()), this, SLOT(print()));
  //connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
  //connect(ui.actionPrintPreview, SIGNAL(triggered()), this, SLOT(printpreview()));

  connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(getallrecommended()));
  

  mCurrCertId = "";
  mCurrMsgId  = "";
  
  /* hide the Tree +/- */
  ui.msgList->setRootIsDecorated( false );
  ui.msgWidget->setRootIsDecorated( false );
  
  /* Set header resize modes and initial section sizes */
  QHeaderView * msgwheader = ui.msgWidget->header () ;
  msgwheader->setResizeMode (0, QHeaderView::Custom);
  msgwheader->setResizeMode (3, QHeaderView::Interactive);
    
  msgwheader->resizeSection ( 0, 24 );
  msgwheader->resizeSection ( 2, 250 );
  msgwheader->resizeSection ( 3, 140 );
  
    /* Set header resize modes and initial section sizes */
	QHeaderView * msglheader = ui.msgList->header () ;
   	msglheader->setResizeMode (0, QHeaderView::Interactive);
	msglheader->setResizeMode (1, QHeaderView::Interactive);
	msglheader->setResizeMode (2, QHeaderView::Interactive);
	msglheader->setResizeMode (3, QHeaderView::Interactive);
  
	msglheader->resizeSection ( 0, 200 );
	msglheader->resizeSection ( 1, 100 );
	msglheader->resizeSection ( 2, 100 );
	msglheader->resizeSection ( 3, 200 );
	
	ui.newmessageButton->setIcon(QIcon(QString(":/images/folder-draft24-pressed.png")));
    ui.replymessageButton->setIcon(QIcon(QString(":/images/replymail-pressed.png")));
    ui.removemessageButton->setIcon(QIcon(QString(":/images/deletemail-pressed.png")));
    ui.printbutton->setIcon(QIcon(QString(":/images/print24.png")));
    
    /*Disabled Reply Button */
    ui.replymessageButton->setEnabled(false);
    
    QMenu * printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printbutton->setMenu(printmenu);

#endif

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ForumsDialog::forumListCustomPopupMenu( QPoint point )
{
      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      QAction *subForumAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "Subscribe to Forum" ), this );
      connect( subForumAct , SIGNAL( triggered() ), this, SLOT( newmessage() ) );
      
      QAction *unsubForumAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Unsubscribe to Forum" ), this );
      connect( unsubForumAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );
      
      QAction *newForumAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "New Forum" ), this );
      connect( newForumAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );

      QAction *delForumAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Delete Forum" ), this );
      connect( delForumAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );

      contextMnu.clear();
      contextMnu.addAction( subForumAct );
      contextMnu.addAction( unsubForumAct );
      contextMnu.addAction( newForumAct );
      contextMnu.addAction( delForumAct );
      contextMnu.exec( mevent->globalPos() );

}

void ForumsDialog::threadListCustomPopupMenu( QPoint point )
{
      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      QAction *replyAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Reply" ), this );
      connect( replyAct , SIGNAL( triggered() ), this, SLOT( createmessage() ) );
      
      QAction *viewAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "View Whole Thread" ), this );
      connect( viewAct , SIGNAL( triggered() ), this, SLOT( showthread() ) );

      contextMnu.clear();
      contextMnu.addAction( replyAct);
      contextMnu.addAction( viewAct);
      contextMnu.exec( mevent->globalPos() );

}

void ForumsDialog::newmessage()
{
#if 0
    ChanMsgDialog *nMsgDialog = new ChanMsgDialog(true);

    /* fill it in */
    //std::cerr << "ForumsDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();
    nMsgDialog->show();
    nMsgDialog->activateWindow();

#endif

    /* window will destroy itself! */
}

void ForumsDialog::replytomessage()
{
	/* put msg on msgBoard, and switch to it. */


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
	}
	else
	{
		/* no change */
		int nlistSize = (totalSize / 2);
		int nMsgSize = (totalSize / 2);
		newSizeList.push_back(nlistSize);
		newSizeList.push_back(nMsgSize);
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


void ForumsDialog::changeBox( int newrow )
{
#if 0
	//std::cerr << "ForumsDialog::changeBox()" << std::endl;
	insertMessages();
	insertMsgTxtAndFiles();
#endif
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

	mCurrForumId = "";
	mCurrPostId = "";

        QList<QTreeWidgetItem *> AdminList;
        QList<QTreeWidgetItem *> SubList;
        QList<QTreeWidgetItem *> PopList;
        QList<QTreeWidgetItem *> OtherList;
	std::multimap<uint32_t, std::string> popMap;

	for(it = forumList.begin(); it != forumList.end(); it++)
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->forumFlags;

		if (flags & RS_FORUM_ADMIN)
		{
			/* own */

			/* Name, 
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			item -> setText(0, QString::fromStdWString(it->forumName));
			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setText(1, QString::fromStdString(out.str()));
			}
			item -> setText(2, QString::fromStdWString(it->forumName));

			// Date 
			{
				QDateTime qtime;
				qtime.setTime_t(it->lastPost);
				QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
				item -> setText(3, timestamp);
			}
			// Id.
			item -> setText(4, QString::fromStdString(it->forumId));
			AdminList.append(item);
		}
		else if (flags & RS_FORUM_SUBSCRIBED)
		{
			/* subscribed forum */

			/* Name, 
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			item -> setText(0, QString::fromStdWString(it->forumName));
			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setText(1, QString::fromStdString(out.str()));
			}
			item -> setText(2, QString::fromStdWString(it->forumName));

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
		uint32_t flags = it->forumFlags;

		if (flags & RS_FORUM_ADMIN)
		{
			continue;
		}
		else if (flags & RS_FORUM_SUBSCRIBED)
		{
			continue;
		}
		else
		{
			/* Name, 
			 * Type,
			 * Rank,
			 * LastPost
			 * ForumId,
			 */

           		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

			item -> setText(0, QString::fromStdWString(it->forumName));
			/* (1) Popularity */
			{
				std::ostringstream out;
				out << it->pop;
				item -> setText(1, QString::fromStdString(out.str()));
			}
			item -> setText(2, QString::fromStdWString(it->forumName));

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
        QList<QTreeWidgetItem *> TopList;
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
	item -> setText(0, tr("Your Forums"));
	item -> addChildren(AdminList);
	TopList.append(item);

        item = new QTreeWidgetItem((QTreeWidget*)0);
	item -> setText(0, tr("Subscribed Forums"));
	item -> addChildren(SubList);
	TopList.append(item);

        item = new QTreeWidgetItem((QTreeWidget*)0);
	item -> setText(0, tr("Popular Forums"));
	item -> addChildren(PopList);
	TopList.append(item);

        item = new QTreeWidgetItem((QTreeWidget*)0);
	item -> setText(0, tr("Other Forums"));
	item -> addChildren(OtherList);
	TopList.append(item);

	/* add the items in! */
	ui.forumTreeWidget->clear();
	ui.forumTreeWidget->insertTopLevelItems(0, TopList);

	return;
}

void ForumsDialog::changedForum( QTreeWidgetItem *curr, QTreeWidgetItem *prev )
{
	insertThreads();
}

void ForumsDialog::changedThread( QTreeWidgetItem *curr, QTreeWidgetItem *prev )
{
	/* just grab the ids of the current item */
	if ((!curr) || (!curr->isSelected()))
	{
		mCurrPostId = "";
	}
	else
	{
		mCurrPostId = (curr->text(5)).toStdString();
		insertPost();
	}
}

void ForumsDialog::changedThread2()
{
	QTreeWidgetItem *curr = ui.threadTreeWidget->currentItem();

	/* just grab the ids of the current item */
	if ((!curr) || (!curr->isSelected()))
	{
		mCurrPostId = "";
	}
	else
	{
		mCurrPostId = (curr->text(5)).toStdString();
		insertPost();
	}
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
		std::cerr << "ForumsDialog::insertThreads() Current Thread Invalid" << std::endl;

		return;
	}

	/* store forumId */
	mCurrForumId = (forumItem->text(4)).toStdString();
	ui.forumName->setText(forumItem->text(0));
	mCurrPostId = "";
	std::string fId = mCurrForumId;

	bool flatView = false;
	if (ui.viewBox->currentIndex() == 1)
	{
		flatView = true;
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
			qtime.setTime_t(tit->ts);
			QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
			item -> setText(0, timestamp);
		}
		item->setText(1, QString::fromStdWString(tit->title));
		item->setText(2, "author");
		item->setText(3, "signed");
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
					}
					else
					{
						child = new QTreeWidgetItem(parent);
					}
		
					{
						QDateTime qtime;
						qtime.setTime_t(mit->ts);
						QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
						child -> setText(0, timestamp);
					}
					child->setText(1, QString::fromStdWString(mit->title));
					child->setText(2, "author");
					child->setText(3, "signed");
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

	/* add all messages in! */
	ui.threadTreeWidget->clear();
	ui.threadTreeWidget->insertTopLevelItems(0, items);

}


void ForumsDialog::updateMessages( QTreeWidgetItem * item, int column )
{
#if 0
	//std::cerr << "ForumsDialog::insertMsgTxtAndFiles()" << std::endl;
	insertMsgTxtAndFiles();
#endif
}


void ForumsDialog::insertPost()
{
	if ((mCurrForumId == "") || (mCurrPostId == ""))
	{
		/* 
		 */

		ui.postText->setText("");
		return;
	}

	/* get the Post */
	ForumMsgInfo msg;
	if (!rsForums->getForumMessage(mCurrForumId, mCurrPostId, msg))
	{
		ui.postText->setText("");
		return;
	}

	ui.postText->setHtml(QString::fromStdWString(msg.msg));
}


bool ForumsDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
	return false;
}


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
	CreateForum *cf = new CreateForum(NULL);
	cf->show();

	//insertForums();
}


void ForumsDialog::createmessage()
{
	if (mCurrForumId == "")
	{
		return;
	}

	CreateForumMsg *cfm = new CreateForumMsg(mCurrForumId, mCurrPostId);
	cfm->show();
}


void ForumsDialog::showthread()
{
	if (mCurrForumId == "")
	{
		return;
	}

	CreateForumMsg *cfm = new CreateForumMsg(mCurrForumId, "");
	cfm->show();
}




