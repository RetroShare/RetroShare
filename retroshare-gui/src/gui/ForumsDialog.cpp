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

#if 0
  connect( ui.msgWidget, SIGNAL( itemClicked ( QTreeWidgetItem *, int) ), this, SLOT( updateMessages ( QTreeWidgetItem *, int) ) );
  connect( ui.listWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeBox ( int) ) );
  
  connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));
  connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
  //connect(ui.printbutton, SIGNAL(clicked()), this, SLOT(print()));
  //connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
  //connect(ui.actionPrintPreview, SIGNAL(triggered()), this, SLOT(printpreview()));

  connect(ui.expandFilesButton, SIGNAL(clicked()), this, SLOT(togglefileview()));
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
      connect( replyAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );
      
      QAction *viewAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "View Post" ), this );
      connect( viewAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );

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
#if 0
	/* if msg header visible -> hide by changing splitter 
	 * three widgets...
	 */

	QList<int> sizeList = ui.msgSplitter->sizes();
	QList<int>::iterator it;

	int listSize = 0;
	int msgSize = 0;
	int recommendSize = 0;
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
		else if (i == 2)
		{
			recommendSize = (*it);
		}
	}

	int totalSize = listSize + msgSize + recommendSize;

	bool toShrink = true;
	if (recommendSize < (int) totalSize / 10)
	{
		toShrink = false;
	}

	QList<int> newSizeList;
	if (toShrink)
	{
		newSizeList.push_back(listSize + recommendSize / 3);
		newSizeList.push_back(msgSize + recommendSize * 2 / 3);
		newSizeList.push_back(0);
	}
	else
	{
		/* no change */
		int nlistSize = (totalSize * 2 / 3) * listSize / (listSize + msgSize);
		int nMsgSize = (totalSize * 2 / 3) - listSize;
		newSizeList.push_back(nlistSize);
		newSizeList.push_back(nMsgSize);
		newSizeList.push_back(totalSize * 1 / 3);
	}

	ui.msgSplitter->setSizes(newSizeList);
#endif
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

void ForumsDialog::insertThreads()
{
#if 0

virtual bool getForumList(std::list<ForumInfo> &forumList) = 0;
virtual bool getForumThreadList(std::string fId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumThreadMsgList(std::string fId, std::string tId, std::list<ThreadInfoSummary> &msgs) = 0;
virtual bool getForumMessage(std::string fId, std::string mId, ForumMsgInfo &msg) = 0;

	std::list<MsgInfoSummary> msgList;
	std::list<MsgInfoSummary>::const_iterator it;

	rsMsgs -> getMessageSummaries(msgList);

	/* get a link to the table */
        QTreeWidget *msgWidget = ui.msgWidget;

	/* get the MsgId of the current one ... */

	std::string cid;
	std::string mid;

	bool oldSelected = getCurrentMsg(cid, mid);
	QTreeWidgetItem *newSelected = NULL;

	/* remove old items ??? */

	int listrow = ui.listWidget -> currentRow();

	//std::cerr << "ForumsDialog::insertMessages()" << std::endl;
	//std::cerr << "Current Row: " << listrow << std::endl;

	/* check the mode we are in */
	unsigned int msgbox = 0;
	switch(listrow)
	{
		case 3:
			msgbox = RS_MSG_SENTBOX;
			break;
		case 2:
			msgbox = RS_MSG_DRAFTBOX;
			break;
		case 1:
			msgbox = RS_MSG_OUTBOX;
			break;
		case 0:
		default:
			msgbox = RS_MSG_INBOX;
			break;
	}

        QList<QTreeWidgetItem *> items;
	for(it = msgList.begin(); it != msgList.end(); it++)
	{
		/* check the message flags, to decide which
		 * group it should go in...
		 *
		 * InBox 
		 * OutBox 
		 * Drafts 
		 * Sent 
		 *
		 * FLAGS = OUTGOING.
		 * 	-> Outbox/Drafts/Sent
		 * 	  + SENT -> Sent
		 *	  + IN_PROGRESS -> Draft.
		 *	  + nuffing -> Outbox.
		 * FLAGS = INCOMING = (!OUTGOING)
		 * 	-> + NEW -> Bold.
		 *
		 */

		if ((it -> msgflags & RS_MSG_BOXMASK) != msgbox)
		{
			//std::cerr << "Msg from other box: " << it->msgflags;
			//std::cerr << std::endl;
			continue;
		}
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* So Text should be:
		 * (1) Msg / Broadcast
		 * (1b) Person / Channel Name
		 * (2) Rank
		 * (3) Date
		 * (4) Title
		 * (5) Msg
		 * (6) File Count
		 * (7) File Total
		 */

		// Date First.... (for sorting)
		{
			QDateTime qtime;
			qtime.setTime_t(it->ts);
			QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
			item -> setText(3, timestamp);
		}

		//  From ....
		{
			item -> setText(1, QString::fromStdString(rsPeers->getPeerName(it->srcId)));
		}

		// Subject
		item -> setText(2, QString::fromStdWString(it->title));
		item -> setIcon(2, (QIcon(":/images/message-mail-read.png")));

		// No of Files.
		{
			std::ostringstream out;
			out << it -> count;
			item -> setText(0, QString::fromStdString(out.str()));
		}

		item -> setText(4, QString::fromStdString(it->srcId));
		item -> setText(5, QString::fromStdString(it->msgId));
		if ((oldSelected) && (mid == it->msgId))
		{
			newSelected = item;
		}

		if (it -> msgflags & RS_MSG_NEW)
		{
			for(int i = 0; i < 10; i++)
			{
				QFont qf = item->font(i);
				qf.setBold(true);
				item->setFont(i, qf);
			    item -> setIcon(2, (QIcon(":/images/message-mail.png")));

				//std::cerr << "Setting Item BOLD!" << std::endl;
			}
		}

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	msgWidget->clear();
	msgWidget->insertTopLevelItems(0, items);

	if (newSelected)
	{
		msgWidget->setCurrentItem(newSelected);
	}
#endif
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
#if 0
	/* Locate the current Message */
	QTreeWidget *msglist = ui.msgWidget;

	std::cerr << "ForumsDialog::insertMsgTxtAndFiles()" << std::endl;


	/* get its Ids */
	std::string cid;
	std::string mid;

	QTreeWidgetItem *qtwi = msglist -> currentItem();
	if (!qtwi)
	{
		/* blank it */
		ui.dateText-> setText("");
		ui.toText->setText("");
		ui.fromText->setText("");
		ui.filesText->setText("");

		ui.subjectText->setText("");
		//ui.msgText->setText("");
		ui.msgList->clear();

		return;
	}
	else
	{
		cid = qtwi -> text(4).toStdString();
		mid = qtwi -> text(5).toStdString(); 
	}

	std::cerr << "ForumsDialog::insertMsgTxtAndFiles() mid:" << mid << std::endl;

	/* Save the Data.... for later */

	mCurrCertId = cid;
	mCurrMsgId  = mid;

	MessageInfo msgInfo;
	if (!rsMsgs -> getMessage(mid, msgInfo))
	{
		std::cerr << "ForumsDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
		return;
	}

        const std::list<FileInfo> &recList = msgInfo.files;
	std::list<FileInfo>::const_iterator it;

	/* get a link to the table */
	QTreeWidget *tree = ui.msgList;

	/* get the MessageInfo */

        tree->clear(); 
        tree->setColumnCount(5); 

        QList<QTreeWidgetItem *> items;
	for(it = recList.begin(); it != recList.end(); it++)
	{
		/* make a widget per person */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
		/* (0) Filename */
		item -> setText(0, QString::fromStdString(it->fname));
		//std::cerr << "Msg FileItem(" << it->fname.length() << ") :" << it->fname << std::endl;
			
		/* (1) Size */
		{
			std::ostringstream out;
			out << it->size;
			item -> setText(1, QString::fromStdString(out.str()));
		}
		/* (2) Rank */
		{
			std::ostringstream out;
			out << it->rank;
			item -> setText(2, QString::fromStdString(out.str()));
		}
			
		item -> setText(3, QString::fromStdString(it->hash));
			
		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	tree->insertTopLevelItems(0, items);

	/* iterate through the sources */
	std::list<std::string>::const_iterator pit;

	QString msgTxt;
	for(pit = msgInfo.msgto.begin(); pit != msgInfo.msgto.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	if (msgInfo.msgcc.size() > 0)
		msgTxt += "\nCc: ";
	for(pit = msgInfo.msgcc.begin(); pit != msgInfo.msgcc.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	if (msgInfo.msgbcc.size() > 0)
		msgTxt += "\nBcc: ";
	for(pit = msgInfo.msgbcc.begin(); pit != msgInfo.msgbcc.end(); pit++)
	{
		msgTxt += QString::fromStdString(*pit);
		msgTxt += " <";
		msgTxt += QString::fromStdString(rsPeers->getPeerName(*pit));
		msgTxt += ">, ";
	}

	{
		QDateTime qtime;
		qtime.setTime_t(msgInfo.ts);
		QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
		ui.dateText-> setText(timestamp);
	}
	ui.toText->setText(msgTxt);
	ui.fromText->setText(QString::fromStdString(rsPeers->getPeerName(msgInfo.srcId)));

	ui.subjectText->setText(QString::fromStdWString(msgInfo.title));
	ui.msgText->setText(QString::fromStdWString(msgInfo.msg));

	{
		std::ostringstream out;
		out << "(" << msgInfo.count << " Files)";
		ui.filesText->setText(QString::fromStdString(out.str()));
	}

	std::cerr << "ForumsDialog::insertMsgTxtAndFiles() Msg Displayed OK!" << std::endl;

	/* finally mark message as read! */
	rsMsgs -> MessageRead(mid);
#endif
}


bool ForumsDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
#if 0
	/* Locate the current Message */
	QTreeWidget *msglist = ui.msgWidget;

	//std::cerr << "ForumsDialog::getCurrentMsg()" << std::endl;

	/* get its Ids */
	QTreeWidgetItem *qtwi = msglist -> currentItem();
	if (qtwi)
	{
		cid = qtwi -> text(4).toStdString();
		mid = qtwi -> text(5).toStdString();
		return true;
	}
#endif
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
	insertForums();
}


