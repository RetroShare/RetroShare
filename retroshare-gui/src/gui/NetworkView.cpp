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

#include "NetworkView.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsdisc.h"

#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsItem>

#include <iostream>

/** Constructor */
NetworkView::NetworkView(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  mScene = new QGraphicsScene();
  ui.graphicsView->setScene(mScene);


  //connect( ui.linkTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( linkTreeWidgetCostumPopupMenu( QPoint ) ) );


  /* link combos */
//  connect( ui.rankComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortRank( int ) ) );
//  connect( ui.periodComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortPeriod( int ) ) );
//  connect( ui.fromComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortFrom( int ) ) );
//  connect( ui.topComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( changedSortTop( int ) ) );

  /* add button */
  connect( ui.refreshButton, SIGNAL( clicked( void ) ), this, SLOT( insertPeers( void ) ) );
  connect( mScene, SIGNAL( changed ( const QList<QRectF> & ) ), this, SLOT ( changedScene( void ) ) );
  /* hide the Tree +/- */
//  ui.linkTreeWidget -> setRootIsDecorated( false );
  
    /* Set header resize modes and initial section sizes */
//	QHeaderView * _header = ui.linkTreeWidget->header () ;
//   	_header->setResizeMode (0, QHeaderView::Interactive);
//	_header->setResizeMode (1, QHeaderView::Interactive);
//	_header->setResizeMode (2, QHeaderView::Interactive);
//
//	_header->resizeSection ( 0, 400 );
//	_header->resizeSection ( 1, 50 );
//	_header->resizeSection ( 2, 150 );

}

void NetworkView::peerCustomPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      //QAction * voteupAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Vote Link Up" ), this );
      //connect( voteupAct , SIGNAL( triggered() ), this, SLOT( voteup() ) );

      contextMnu.clear();
      //contextMnu.addAction(voteupAct);
      contextMnu.exec( mevent->globalPos() );
}

void NetworkView::changedFoFCheckBox( )
{
	insertPeers();
}

void NetworkView::changedDrawSignatures( )
{
	insertPeers();
}

void NetworkView::changedDrawFriends( )
{
	insertPeers();
}

void  NetworkView::clearPeerItems()
{
	std::map<std::string, QGraphicsItem *>::iterator pit;

	for(pit = mPeerItems.begin(); pit != mPeerItems.end(); pit++)
	{
		//mScene->removeItem(pit->second);
		mScene->destroyItemGroup((QGraphicsItemGroup *) pit->second);
		//delete (pit->second);
	}
	mPeerItems.clear();
}


void  NetworkView::clearOtherItems()
{
	std::list<QGraphicsItem *>::iterator oit;

	for(oit = mOtherItems.begin(); oit != mOtherItems.end(); oit++)
	{
		mScene->removeItem(*oit);
		delete (*oit);
	}
	mOtherItems.clear();
}


void  NetworkView::clearLineItems()
{
	std::list<QGraphicsItem *>::iterator oit;

	for(oit = mLineItems.begin(); oit != mLineItems.end(); oit++)
	{
		mScene->removeItem(*oit);
		delete (*oit);
	}
	mLineItems.clear();
}


void  NetworkView::insertPeers()
{
	/* clear graphics scene */
	clearPeerItems();
	clearOtherItems();

	/* add all friends */
	std::list<std::string> ids;
	std::list<std::string>::iterator it;
 	rsPeers->getOthersList(ids);
 	ids.push_back(rsPeers->getOwnId()); /* add yourself too */

	std::cerr << "NetworkView::insertPeers()" << std::endl;

	int i = 0;
	for(it = ids.begin(); it != ids.end(); it++, i++)
	{
		/* *** */

		QString name = QString::fromStdString(rsPeers->getPeerName(*it));
		QGraphicsTextItem *gti = new QGraphicsTextItem(name);
		mScene->addItem(gti);

		//QPointF textPoint( i * 10, i * 20);
		//gti->setPos(textPoint);
		gti->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
		gti->setZValue(20);

		QRectF textBound = gti->boundingRect();

		/* work out bounds for circle */
		qreal diameter = textBound.width() + 10;
		if (diameter < 40)
			diameter = 40;

		qreal x = textBound.left() + (textBound.width() / 2.0) - (diameter/ 2.0);
		qreal y = textBound.top() + (textBound.height() / 2.0) - (diameter/ 2.0);

		QGraphicsEllipseItem *gei = new QGraphicsEllipseItem(x, y, diameter, diameter);
		mScene->addItem(gei);
		gei->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
		gei->setZValue(10);

		/* colour depends on Friend... */
		if (*it == rsPeers->getOwnId())
		{
			gei->setBrush(QBrush(Qt::green));
		}
		else if (rsPeers->isFriend(*it))
		{
			gei->setBrush(QBrush(Qt::blue));
		}
		else
		{
			gei->setBrush(QBrush(Qt::red));
		}

		QGraphicsItemGroup *gig = new QGraphicsItemGroup();
		mScene->addItem(gig);

		gig->addToGroup(gei);
		gig->addToGroup(gti);
		gig->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);





		//, const QPen & pen = QPen(), const QBrush & brush = QBrush() )

		mPeerItems[*it] = gig;
		mOtherItems.push_back(gti);
		mOtherItems.push_back(gei);

		std::cerr << "NetworkView::insertPeers() Added Friend: " << *it << std::endl;
		std::cerr << "\t At: " << i*5 << "," << i*10 << std::endl;

	}

	insertConnections();
}


void  NetworkView::insertConnections()
{
	clearLineItems();

	/* iterate through all peerItems .... and find any proxies */
	std::map<std::string, QGraphicsItem *>::const_iterator pit, pit2;

	int i = 0;
	for(pit = mPeerItems.begin(); pit != mPeerItems.end(); pit++, i++)
	{
		std::list<std::string> friendList;
		std::list<std::string>::iterator it;

		rsDisc->getDiscFriends(pit->first, friendList);
		int j = 0;

		for(it = friendList.begin(); it != friendList.end(); it++)
		{
			//pit2  = std::find(pit, mPeerItems.end(), *it);
			pit2 = mPeerItems.find(*it);
			if (pit2 == mPeerItems.end())
			{
				std::cerr << " Failed to Find: " << *it;
				std::cerr << std::endl;
				continue; /* missing */
			}

			if (pit == pit2)
				continue; /* skip same one */

			std::cerr << " Connecting: " << pit->first << " to " << pit2->first;
			std::cerr << std::endl;

			QPointF pos1 = (pit->second)->pos();
			QRectF  bound1 = (pit->second)->boundingRect();

			QPointF pos2 = (pit2->second)->pos();
			QRectF  bound2 = (pit2->second)->boundingRect();

			pos1 += QPointF(bound1.width() / 2.0, bound1.height() / 2.0);
			pos2 += QPointF(bound2.width() / 2.0, bound2.height() / 2.0);

			QLineF  line(pos1, pos2);

			QGraphicsLineItem *gli = mScene->addLine(line);

			mLineItems.push_back(gli);
		}
	}

/* debugging all lines */
#if 0
	/* iterate through all peerItems .... and find any proxies */
	std::map<std::string, QGraphicsItem *>::const_iterator pit, pit2;
	int i = 0;
	for(pit = mPeerItems.begin(); pit != mPeerItems.end(); pit++, i++)
	{
		int j = 0;
		for(pit2 = mPeerItems.begin(); (pit2 != mPeerItems.end()) && (j < i); pit2++, j++)
		{
			if (pit == pit2)
				continue; /* skip same one */

			std::cerr << " Connecting: " << pit->first << " to " << pit2->first;
			std::cerr << std::endl;

			QPointF pos1 = (pit->second)->pos();
			QPointF pos2 = (pit2->second)->pos();
			QLineF  line(pos1, pos2);

			QGraphicsLineItem *gli = mScene->addLine(line);

			mLineItems.push_back(gli);
		}
	}
#endif

	mLineChanged = true;
}


void  NetworkView::changedScene()
{
	std::cerr << "NetworkView::changedScene()" << std::endl;

	QList<QGraphicsItem *> items = mScene->selectedItems();

	std::cerr << "NetworkView::changedScene() Items selected: " << items.size() << std::endl;

	/* if an item was selected and moved - then redraw lines */
	if (mLineChanged)
	{
		mLineChanged = false;
		return;
	}

	if (items.size() > 0)
	{
		mScene->clearSelection();
		insertConnections();
	}

}


