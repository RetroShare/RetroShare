/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 RetroShare Team
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
#include "ChanMsgDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"

#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>


/** Constructor */
ChanMsgDialog::ChanMsgDialog(bool msg, QWidget *parent, Qt::WFlags flags)
: mIsMsg(msg), QMainWindow(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  RshareSettings config;
  config.loadWidgetInformation(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );
  
  //connect( ui.channelstreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( channelstreeViewCostumPopupMenu( QPoint ) ) );
  //
 
  // connect up the buttons. 
  connect( ui.actionSend, SIGNAL( triggered (bool)), this, SLOT( sendMessage( ) ) );
  //connect( ui.actionReply, SIGNAL( triggered (bool)), this, SLOT( replyMessage( ) ) );

  /* if Msg */
  if (mIsMsg)
  {
    connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
      this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));
  }
  else
  {
    connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
      this, SLOT(toggleChannelItem( QTreeWidgetItem *, int ) ));
  }

  connect(ui.msgFileList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
    this, SLOT(toggleRecommendItem( QTreeWidgetItem *, int ) ));
    
  /* hide the Tree +/- */
  ui.msgSendList -> setRootIsDecorated( false );
  ui.msgFileList -> setRootIsDecorated( false );
  
   /* to hide the header  */
  //ui.msgSendList->header()->hide(); 

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


void ChanMsgDialog::channelstreeViewCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      deletechannelAct = new QAction( tr( "Delete Channel" ), this );
      connect( deletechannelAct , SIGNAL( triggered() ), this, SLOT( deletechannel() ) );
      
      createchannelmsgAct = new QAction( tr( "Create Channel MSG" ), this );
      connect( createchannelmsgAct , SIGNAL( triggered() ), this, SLOT( createchannelmsg() ) );


      contextMnu.clear();
      contextMnu.addAction( deletechannelAct);
      contextMnu.addAction( createchannelmsgAct);
      contextMnu.exec( mevent->globalPos() );
}

void ChanMsgDialog::closeEvent (QCloseEvent * event)
{
	RshareSettings config;
	config.saveWidgetInformation(this);

    hide();
    event->ignore();
}


void ChanMsgDialog::deletechannel()
{


}


void ChanMsgDialog::createchannelmsg()
{


}



void  ChanMsgDialog::insertSendList()
{
	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}
		
	std::list<std::string> peers;
	std::list<std::string>::iterator it;
		
	rsPeers->getFriendList(peers);
		
        /* get a link to the table */
        QTreeWidget *sendWidget = ui.msgSendList;

        QList<QTreeWidgetItem *> items;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		
		RsPeerDetails detail;
		if (!rsPeers->getPeerDetails(*it, detail))
		{
			continue; /* BAD */
		}
		
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);


		/* add all the labels */
		/* (0) Person */
		item -> setText(0, QString::fromStdString(detail.name));
		/* () Org */
		//item -> setText(1, QString::fromStdString(detail.org));
		/* () Location */
		//item -> setText(2, QString::fromStdString(detail.location));
		/* () Country */
		//item -> setText(3, QString::fromStdString(detail.email));
		/* () Id */
		item -> setText(1, QString::fromStdString(detail.id));

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item -> setCheckState(0, Qt::Unchecked);

		if (rsicontrol->IsInMsg(detail.id))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* add to the list */
		items.append(item);
	}

        /* remove old items ??? */
	sendWidget->clear();
	sendWidget->setColumnCount(1);

	/* add the items in! */
	sendWidget->insertTopLevelItems(0, items);

	sendWidget->update(); /* update display */
}


void  ChanMsgDialog::insertChannelSendList()
{
        rsiface->lockData(); /* Lock Interface */

        std::map<RsChanId,ChannelInfo>::const_iterator it;
        const std::map<RsChanId,ChannelInfo> &chans =
                                rsiface->getChannels();

        /* get a link to the table */
        QTreeWidget *sendWidget = ui.msgSendList;

        /* remove old items ??? */
	sendWidget->clear();
	sendWidget->setColumnCount(4);

        QList<QTreeWidgetItem *> items;
	for(it = chans.begin(); it != chans.end(); it++)
	{
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* (0) Title */
		item -> setText(0, QString::fromStdString(it->second.chanName));
		if (it->second.publisher)
		{
			item -> setText(1, "Publisher");
		}
		else
		{
			item -> setText(1, "Listener");
		}

		{
			std::ostringstream out;
			out << it->second.chanId;
			item -> setText(2, QString::fromStdString(out.str()));
		}

		item -> setText(3, "Channel");

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item -> setCheckState(0, Qt::Unchecked);
	
		if (it -> second.inBroadcast)
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	sendWidget->insertTopLevelItems(0, items);

	rsiface->unlockData(); /* UnLock Interface */

	sendWidget->update(); /* update display */
}



/* Utility Fns */
/***
RsCertId getSenderRsCertId(QTreeWidgetItem *i)
{
	RsCertId id = (i -> text(3)).toStdString();
	return id;
}
***/


	/* get the list of peers from the RsIface.  */
void  ChanMsgDialog::insertFileList()
{
	rsiface->lockData();   /* Lock Interface */


        const std::list<FileInfo> &recList = rsiface->getRecommendList();
	std::list<FileInfo>::const_iterator it;

	/* get a link to the table */
	QTreeWidget *tree = ui.msgFileList;

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
			
		item -> setText(3, QString::fromStdString(it->hash));
			
		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

		if (it -> inRecommend)
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	tree->insertTopLevelItems(0, items);

	rsiface->unlockData(); /* UnLock Interface */

	tree->update(); /* update display */
}


void  ChanMsgDialog::newMsg()
{
	/* clear all */
	ui.titleEdit->setText("No Title");
	ui.msgText->setText("");

        /* worker fns */
	if (mIsMsg)
	{
		insertSendList();
	}
	else
	{
		insertChannelSendList();
	}

	insertFileList();
}

void  ChanMsgDialog::insertTitleText(std::string title)
{
	ui.titleEdit->setText(QString::fromStdString(title));
}


void  ChanMsgDialog::insertMsgText(std::string msg)
{
	ui.msgText->setText(QString::fromStdString(msg));
}


void  ChanMsgDialog::sendMessage()
{


	/* construct a message */
	MessageInfo mi;

	mi.title = ui.titleEdit->text().toStdWString();
	mi.msg =   ui.msgText->toPlainText().toStdWString();

	rsiface->lockData();   /* Lock Interface */

        const std::list<FileInfo> &recList = rsiface->getRecommendList();
	std::list<FileInfo>::const_iterator it;
	for(it = recList.begin(); it != recList.end(); it++)
	{
		if (it -> inRecommend)
		{
			mi.files.push_back(*it);
		}
	}

	rsiface->unlockData(); /* UnLock Interface */

	/* get the ids from the send list */
	std::list<std::string> peers;
	std::list<std::string> msgto;
	std::list<std::string>::iterator iit;

	rsPeers->getFriendList(peers);
		
	for(iit = peers.begin(); iit != peers.end(); iit++)
	{
		if (rsicontrol->IsInMsg(*iit))
		{
			mi.msgto.push_back(*iit);
		}
	}

	rsMsgs->MessageSend(mi);

	close();
	return;
}


void  ChanMsgDialog::cancelMessage()
{
	close();
	return;
}



/* Toggling .... Check Boxes.....
 * This is dependent on whether we are a 
 *
 * Chan or Msg Dialog.
 */


/* First the Msg (People) ones */
void ChanMsgDialog::togglePersonItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "TogglePersonItem()" << std::endl;

        /* extract id */
        std::string id = (item -> text(1)).toStdString();

        /* get state */
        bool inMsg = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInMsg(id, inMsg);
        return;
}

/* Second  the Channel ones */
void ChanMsgDialog::toggleChannelItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "ToggleChannelItem()" << std::endl;

        /* extract id */
        std::string id = (item -> text(2)).toStdString();

        /* get state */
        bool inBroad = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInBroadcast(id, inBroad);
        return;
}

/* This is actually for both */
void ChanMsgDialog::toggleRecommendItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "ToggleRecommendItem()" << std::endl;

        /* extract name */
        std::string id = (item -> text(0)).toStdString();

        /* get state */
        bool inRecommend = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInRecommend(id, inRecommend);
        return;
}

