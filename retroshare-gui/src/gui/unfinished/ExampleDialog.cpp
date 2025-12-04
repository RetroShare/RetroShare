/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#include <QFile>
#include <QFileInfo>

#include "rshare.h"
#include "ExampleDialog.h"
#include <retroshare/rsiface.h>

#include <iostream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>

#include "util/RsQtVersion.h"

/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_CHAT               ":/images/chat.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/im-user.png"
#define IMAGE_OFFLINE            ":/images/im-user-offline.png"

/** Constructor */
ExampleDialog::ExampleDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.peertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( peertreeWidgetCostumPopupMenu( QPoint ) ) );

  /* hide the Tree +/- */
  ui.peertreeWidget -> setRootIsDecorated( false );
  
    /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.peertreeWidget->header () ;
	QHeaderView_setSectionResizeModeColumn(_header, 0, QHeaderView::Custom);
	QHeaderView_setSectionResizeModeColumn(_header, 1, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 2, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 3, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 4, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 5, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 6, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 7, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 8, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 9, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 10, QHeaderView::Interactive);
	QHeaderView_setSectionResizeModeColumn(_header, 11, QHeaderView::Interactive);

	_header->resizeSection ( 0, 25 );
	_header->resizeSection ( 1, 100 );
	_header->resizeSection ( 2, 100 );
	_header->resizeSection ( 3, 100 );
	_header->resizeSection ( 4, 100 );
	_header->resizeSection ( 5, 200 );
	_header->resizeSection ( 6, 100 );
	_header->resizeSection ( 7, 100 );
	_header->resizeSection ( 8, 100 );
	_header->resizeSection ( 9, 100 );
	_header->resizeSection ( 10, 100 );
}

void ExampleDialog::peertreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      voteupAct = new QAction(QIcon(), tr( "Vote Up" ), this );
      connect( voteupAct , SIGNAL( triggered() ), this, SLOT( voteup() ) );

      votedownAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Vote Down" ), this );
      connect( votedownAct , SIGNAL( triggered() ), this, SLOT( votedown() ) );
      
      contextMnu.clear();
      contextMnu.addAction(voteupAct);
      contextMnu.addSeparator(); 
      contextMnu.addAction(votedownAct);
      contextMnu.exec( mevent->globalPos() );
}



/* get the list of peers from the RsIface.  */
void  ExampleDialog::insertExample()
{

#if 0
        rsiface->lockData(); /* Lock Interface */

        std::map<RsCertId,NeighbourInfo>::const_iterator it;
        const std::map<RsCertId,NeighbourInfo> &friends =
                                rsiface->getFriendMap();

        /* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;

        /* remove old items ??? */
	peerWidget->clear();
	peerWidget->setColumnCount(12);
	


        QList<QTreeWidgetItem *> items;
	for(it = friends.begin(); it != friends.end(); it++)
	{
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* First 5 (1-5) Key Items */
		/* () Status Icon */
		item -> setText(0, "");
		
		/* (0) Status */		
		item -> setText(1, QString::fromStdString(
						it->second.statusString));

		/* (1) Person */
		item -> setText(2, QString::fromStdString(it->second.name));

		/* (2) Auto Connect */
		item -> setText(3, QString::fromStdString(
						it->second.connectString));

		/* (3) Trust Level */
		item -> setText(4, QString::fromStdString(it->second.trustString));
		/* (4) Peer Address */
		item -> setText(5, QString::fromStdString(it->second.peerAddress));

		/* less important ones */
		/* () Last Contact */
		item -> setText(6, QString::fromStdString(it->second.lastConnect));

		/* () Org */
		item -> setText(7, QString::fromStdString(it->second.org));
		/* () Location */
		item -> setText(8, QString::fromStdString(it->second.loc));
		/* () Country */
		item -> setText(9, QString::fromStdString(it->second.country));
	
	
		/* Hidden ones: */
		/* ()  RsCertId */
		{
			std::ostringstream out;
			out << it -> second.id;
			item -> setText(10, QString::fromStdString(out.str()));
		}

		/* ()  AuthCode */	
                item -> setText(11, QString::fromStdString(it->second.authCode));

		/* change background */
		int i;
                if (it->second.statusString == "Online")
		{
			/* bright green */
			for(i = 1; i < 12; i++)
			{
			  item -> setBackground(i,QBrush(Qt::green));
			  item -> setIcon(0,(QIcon(IMAGE_ONLINE)));
			}
		}
		else
		{
                	if (it->second.lastConnect != "Never")
			{
				for(i = 1; i < 12; i++)
				{
				  item -> setBackground(i,QBrush(Qt::lightGray));
				  item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
				}
			}
			else
			{
				for(i = 1; i < 12; i++)
				{
				  item -> setBackground(i,QBrush(Qt::gray));
				  item -> setIcon(0,(QIcon(IMAGE_OFFLINE)));
				}
			}
		}
			


		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	peerWidget->insertTopLevelItems(0, items);

	rsiface->unlockData(); /* UnLock Interface */

	peerWidget->update(); /* update display */
#endif
}

QTreeWidgetItem *ExampleDialog::getCurrentLine()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
		std::cerr << "Invalid Current Item" << std::endl;
		return NULL;
	}

	/* Display the columns of this item. */
	//std::ostringstream out;
        //out << "CurrentPeerItem: " << std::endl;

	for(int i = 1; i < 6; i++)
	{
		QString txt = item -> text(i);
		//out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	//std::cerr << out.str();
	return item;
}

void ExampleDialog::voteup()
{
	QTreeWidgetItem *c = getCurrentLine();
	std::cerr << "ExampleDialog::voteup()" << std::endl;
}

void ExampleDialog::votedown()
{
	QTreeWidgetItem *c = getCurrentLine();
	std::cerr << "ExampleDialog::votedown()" << std::endl;

	/* need to get the input address / port */
	/*
 	std::string addr;
	unsigned short port;
	rsServer->FriendSetAddress(getPeerRsCertId(c), addr, port);
	*/
}



