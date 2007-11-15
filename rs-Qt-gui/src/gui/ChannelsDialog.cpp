/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#include "rshare.h"
#include "ChannelsDialog.h"
#include "msgs/ChanMsgDialog.h"
#include "msgs/ChanCreateDialog.h"

#include "rsiface/rsiface.h"
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>

/* Images for context menu icons */
#define IMAGE_MESSAGE        ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREMOVE  ":/images/mail_delete.png"
#define IMAGE_DOWNLOAD       ":/images/start.png"
#define IMAGE_DOWNLOADALL    ":/images/startall.png"

/** Constructor */
ChannelsDialog::ChannelsDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.msgWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messageslistWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.msgList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgfilelistWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.msgWidget, SIGNAL( itemClicked ( QTreeWidgetItem *, int) ), this, SLOT( updateChannels ( QTreeWidgetItem *, int) ) );

  /* hide the Tree +/- */
  ui.msgWidget -> setRootIsDecorated( false );
  ui.msgList -> setRootIsDecorated( false );


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ChannelsDialog::messageslistWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      newMsgAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "New Message" ), this );
      connect( newMsgAct , SIGNAL( triggered() ), this, SLOT( newmessage() ) );

      newChanAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "New Channel" ), this );
      connect( newChanAct , SIGNAL( triggered() ), this, SLOT( newchannel() ) );
      
      subChanAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Subscribe To Channel" ), this );
      connect( subChanAct , SIGNAL( triggered() ), this, SLOT( subscribechannel() ) );
      
      unsubChanAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Unsubscribe To Channel" ), this );
      connect( unsubChanAct , SIGNAL( triggered() ), this, SLOT( unsubscribechannel() ) );

      delChanAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Delete Your Channel" ), this );
      connect( delChanAct , SIGNAL( triggered() ), this, SLOT( deletechannel() ) );

      contextMnu.clear();
      contextMnu.addAction( newMsgAct);
      contextMnu.addAction( newChanAct);
      contextMnu.addAction( subChanAct);
      contextMnu.addAction( unsubChanAct);
      contextMnu.addAction( delChanAct);
      contextMnu.exec( mevent->globalPos() );
}


void ChannelsDialog::msgfilelistWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      getRecAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Download" ), this );
      connect( getRecAct , SIGNAL( triggered() ), this, SLOT( getcurrentrecommended() ) );
      
      getAllRecAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Download All" ), this );
      connect( getAllRecAct , SIGNAL( triggered() ), this, SLOT( getallrecommended() ) );
      

      contextMnu.clear();
      contextMnu.addAction( getRecAct);
      contextMnu.addAction( getAllRecAct);
      contextMnu.exec( mevent->globalPos() );
}

void ChannelsDialog::newmessage()
{
    static ChanMsgDialog *createChanMsgDialog = new ChanMsgDialog(false);

    /* fill it in */
    std::cerr << "ChannelsDialog::newmessage()" << std::endl;
    createChanMsgDialog->newMsg();
    createChanMsgDialog->show();
}

void ChannelsDialog::newchannel()
{
	/* put msg on msgBoard, and switch to it. */
    static ChanCreateDialog *createChanDialog = new ChanCreateDialog();

    /* fill it in */
    std::cerr << "ChannelsDialog::newchannel()" << std::endl;
    createChanDialog->newChan();
    createChanDialog->show();

}

void ChannelsDialog::subscribechannel()
{
	/* more work */

}

void ChannelsDialog::unsubscribechannel()
{
	/* more work */

}


void ChannelsDialog::deletechannel()
{
	/* more work */

}


void ChannelsDialog::getcurrentrecommended()
{
   

}


void ChannelsDialog::getallrecommended()
{
   

}

void ChannelsDialog::insertChannels()
{
	rsiface->lockData(); /* Lock Interface */

	/* get a link to the table */
        QTreeWidget *msgWidget = ui.msgWidget;

        QList<QTreeWidgetItem *> items;

	std::map<RsChanId, ChannelInfo>::const_iterator it2;
	std::list<MessageInfo>::const_iterator it;

	const std::map<RsChanId, ChannelInfo> &ourChans = rsiface->getOurChannels();

	for(it2 = ourChans.begin(); it2 != ourChans.end(); it2++)
	{
	  for(it = it2 -> second.msglist.begin(); it != it2 -> second.msglist.end(); it++)
	  {
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* So Text should be:
		 * as above. */

		{
			std::ostringstream out;
			out << "@" << it -> ts;
			item -> setText(0, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << it2 -> second.rank; // "5"; // RANK 
			item -> setText(1, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << "Broadcast on " << it2 -> second.chanName;
			item -> setText(2, QString::fromStdString(out.str()));
		}

		item -> setText(3, QString::fromStdString(it->title));
		item -> setText(4, QString::fromStdString(it->msg));

		{
			std::ostringstream out;
			out << it -> size;
			item -> setText(5, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << it -> count;
			item -> setText(6, QString::fromStdString(out.str()));
		}

		item -> setText(7, "CHAN");
		{
			std::ostringstream out;
			out << it2 -> first;
			item -> setText(8, QString::fromStdString(out.str()));
		}
		{
			std::ostringstream out;
			out << it -> msgId;
			item -> setText(9, QString::fromStdString(out.str()));
		}


		/* add to the list */
		items.append(item);
	  }
	}


	const std::map<RsChanId, ChannelInfo> &chans = rsiface->getChannels();

	for(it2 = chans.begin(); it2 != chans.end(); it2++)
	{
	  for(it = it2 -> second.msglist.begin(); it != it2 -> second.msglist.end(); it++)
	  {
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* So Text should be:
		 * as above. */

		{
			std::ostringstream out;
			out << "@" << it -> ts;
			item -> setText(0, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << it2 -> second.rank; // "5"; // RANK 
			item -> setText(1, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << "Broadcast on " << it2 -> second.chanName;
			item -> setText(2, QString::fromStdString(out.str()));
		}

		item -> setText(3, QString::fromStdString(it->title));
		item -> setText(4, QString::fromStdString(it->msg));

		{
			std::ostringstream out;
			out << it -> size;
			item -> setText(5, QString::fromStdString(out.str()));
		}

		{
			std::ostringstream out;
			out << it -> count;
			item -> setText(6, QString::fromStdString(out.str()));
		}

		item -> setText(7, "CHAN");
		{
			std::ostringstream out;
			out << it2 -> first;
			item -> setText(8, QString::fromStdString(out.str()));
		}
		{
			std::ostringstream out;
			out << it -> msgId;
			item -> setText(9, QString::fromStdString(out.str()));
		}



		/* add to the list */
		items.append(item);
	  }
	}

	/* add the items in! */
	msgWidget->clear();
	msgWidget->insertTopLevelItems(0, items);

	rsiface->unlockData(); /* UnLock Interface */
}

void ChannelsDialog::updateChannels( QTreeWidgetItem * item, int column )
{
	std::cerr << "ChannelsDialog::insertMsgTxtAndFiles()" << std::endl;
	insertMsgTxtAndFiles();
}


void ChannelsDialog::insertMsgTxtAndFiles()
{
	/* Locate the current Message */
	QTreeWidget *msglist = ui.msgWidget;

	std::cerr << "ChannelsDialog::insertMsgTxtAndFiles()" << std::endl;

	/* get its Ids */
	std::string chid;
	std::string mid;

	QTreeWidgetItem *qtwi = msglist -> currentItem();
	if (!qtwi)
	{
		/* blank it */
		ui.msgText->setText("");
		ui.msgList->clear();
		return;
	}
	else
	{
		chid = qtwi -> text(8).toStdString();
		mid = qtwi -> text(9).toStdString();
	}

	std::cerr << "chId: " << chid << std::endl;
	std::cerr << "mId: " << mid << std::endl;

	/* get Message */
	rsiface->lockData();   /* Lock Interface */

	const MessageInfo *mi = NULL; 
	mi = rsiface->getChannelMsg(chid, mid);
	if (!mi)
	{
		rsiface->unlockData();   /* Unlock Interface */
		return;
	}

        const std::list<FileInfo> &recList = mi->files;
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
			
		item -> setText(3, QString::fromStdString(it->path));
			
		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	tree->insertTopLevelItems(0, items);


	/* add the Msg */
	ui.msgText->setText(QString::fromStdString(mi -> msg));

	rsiface->unlockData(); /* Unlock Interface */

}



