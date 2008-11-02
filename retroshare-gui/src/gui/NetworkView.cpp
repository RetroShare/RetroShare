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
#include <algorithm>

/** Constructor */
NetworkView::NetworkView(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  //mScene = new QGraphicsScene();
  //ui.graphicsView->setScene(mScene);

  /* add button */
  connect( ui.refreshButton, SIGNAL( clicked( void ) ), this, SLOT( insertPeers( void ) ) );
//connect( mScene, SIGNAL( changed ( const QList<QRectF> & ) ), this, SLOT ( changedScene( void ) ) );

    /* Hide Settings frame */
    shownwSettingsFrame(false);
    connect( ui.nviewsettingsButton, SIGNAL(toggled(bool)), this, SLOT(shownwSettingsFrame(bool)));


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
		//mScene->destroyItemGroup((QGraphicsItemGroup *) pit->second);
	}
	mPeerItems.clear();
}


void  NetworkView::clearOtherItems()
{
	std::list<QGraphicsItem *>::iterator oit;

	for(oit = mOtherItems.begin(); oit != mOtherItems.end(); oit++)
	{
		//mScene->removeItem(*oit);
		delete (*oit);
	}
	mOtherItems.clear();
}


void  NetworkView::clearLineItems()
{
	std::list<QGraphicsItem *>::iterator oit;

	for(oit = mLineItems.begin(); oit != mLineItems.end(); oit++)
	{
		//mScene->removeItem(*oit);
		delete (*oit);
	}
	mLineItems.clear();
}


void  NetworkView::insertPeers()
{
	/* clear graphics scene */
  	ui.graphicsView->clearGraph();

	/* add all friends */
	std::list<std::string> ids;
	std::list<std::string>::iterator it;
 	rsPeers->getOthersList(ids);

	std::cerr << "NetworkView::insertPeers()" << std::endl;

	int i = 0;
	uint32_t type = 0;
	for(it = ids.begin(); it != ids.end(); it++, i++)
	{
		/* *** */
		std::string name = rsPeers->getPeerName(*it);

		if (rsPeers->isFriend(*it))
		{
			type = 2;
		}
		else
		{
			type = 3;
		}

  		ui.graphicsView->addNode(type, *it, name);
		std::cerr << "NetworkView::insertPeers() Added Friend: " << *it << std::endl;
	}

	insertConnections();
}


void  NetworkView::insertConnections()
{
	/* iterate through all friends */
	std::list<std::string> ids;
	std::list<std::string>::iterator it;
	std::map<std::string, std::list<std::string> > connLists;

	std::string ownId = rsPeers->getOwnId();
 	rsPeers->getOthersList(ids);

	std::cerr << "NetworkView::insertConnections()" << std::endl;

	int i = 0;
	for(it = ids.begin(); it != ids.end(); it++, i++)
	{
		if (rsPeers->isFriend(*it))
		{
			/* add own connection (check for it first) */
			std::list<std::string> &refList = connLists[*it];
			if (refList.end() == std::find(refList.begin(), refList.end(), ownId))
			{
				connLists[ownId].push_back(*it);
				connLists[*it].push_back(ownId);

  				ui.graphicsView->addEdge("", *it);
				std::cerr << "NetworkView: Adding Edge: Self -> " << *it;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "NetworkView: Already Edge: Self -> " << *it;
				std::cerr << std::endl;
			}
		}

		std::list<std::string> friendList;
		std::list<std::string>::iterator it2;

		rsDisc->getDiscFriends(*it, friendList);
		int j = 0;

		for(it2 = friendList.begin(); it2 != friendList.end(); it2++)
		{
			if (*it == *it2)
				continue;

			/* check that we haven't added this one already */
			std::list<std::string> &refList = connLists[*it];
			if (refList.end() == std::find(refList.begin(), refList.end(), *it2))
			{
				connLists[*it2].push_back(*it);
				connLists[*it].push_back(*it2);

  				ui.graphicsView->addEdge(*it, *it2);
				std::cerr << "NetworkView: Adding Edge: " << *it << " <-> " << *it2;
				std::cerr << std::endl;
			}
			else
			{
				std::cerr << "NetworkView: Already Edge: " << *it << " <-> " << *it2;
				std::cerr << std::endl;
			}
		}
	}
}


void  NetworkView::changedScene()
{
}

/**
Toggles the Settings pane on and off, changes toggle button text
 */
void NetworkView::shownwSettingsFrame(bool show)
{
    if (show) {
        ui.viewsettingsframe->setVisible(true);
        ui.nviewsettingsButton->setChecked(true);
        ui.nviewsettingsButton->setToolTip(tr("Hide Settings"));
    } else {
        ui.viewsettingsframe->setVisible(false);
        ui.nviewsettingsButton->setChecked(false);
        ui.nviewsettingsButton->setToolTip(tr("Show Settings"));
    }
}

