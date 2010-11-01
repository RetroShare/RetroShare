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
#include <retroshare/rspeers.h>

#include <gpgme.h>

#include <deque>
#include <set>
#include <iostream>
#include <algorithm>

#include "gui/elastic/node.h"

/** Constructor */
NetworkView::NetworkView(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  mScene = new QGraphicsScene();
  ui.graphicsView->setScene(mScene);
  ui.graphicsView->setEdgeLength(ui.edgeLengthSB->value()) ;

  setMaxFriendLevel(ui.maxFriendLevelSB->value()) ;

  /* add button */
  connect( ui.refreshButton, SIGNAL( clicked( void ) ), this, SLOT( insertPeers( void ) ) );
  connect( mScene, SIGNAL( changed ( const QList<QRectF> & ) ), this, SLOT ( changedScene( void ) ) );

  /* Hide Settings frame */
  shownwSettingsFrame(false);
  connect( ui.maxFriendLevelSB, SIGNAL(valueChanged(int)), this, SLOT(setMaxFriendLevel(int)));
  connect( ui.edgeLengthSB, SIGNAL(valueChanged(int)), this, SLOT(setEdgeLength(int)));

  insertPeers();
}

void NetworkView::setEdgeLength(int l)
{
	ui.graphicsView->setEdgeLength(l);
}
void NetworkView::setMaxFriendLevel(int m)
{
	_max_friend_level = m ;
	insertPeers() ;
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
		mScene->destroyItemGroup((QGraphicsItemGroup *) pit->second);
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

class NodeInfo
{
	public:
		NodeInfo(const std::string& id,uint32_t lev) : gpg_id(id),friend_level(lev) {}

		std::string gpg_id ;
		uint32_t friend_level ;
} ;

void  NetworkView::insertPeers()
{
	/* clear graphics scene */
  	ui.graphicsView->clearGraph();

	/* add all friends */
	std::string ownId = rsPeers->getGPGOwnId();
	std::cerr << "NetworkView::insertPeers()" << std::endl;

	int i = 0;

	std::map<std::string,GraphWidget::NodeId> node_ids ;	// node ids of the created nodes. 
	std::deque<NodeInfo> nodes_to_treat ;						// list of nodes to be treated. Used as a queue. The int is the level of friendness
	std::set<std::string> nodes_considered ;					// list of nodes already considered. Eases lookup.

	nodes_to_treat.push_front(NodeInfo(ownId,0)) ;			// initialize queue with own id.
	nodes_considered.insert(ownId) ;

	// compute the list of GPG friends
	
	std::set<std::string> gpg_friends ;
	std::list<std::string> ssl_friends ;

	if(!rsPeers->getFriendList(ssl_friends))
		return ;

	for(std::list<std::string>::const_iterator it(ssl_friends.begin());it!=ssl_friends.end();++it)
	{
		RsPeerDetails d ;
		if(!rsPeers->getPeerDetails(*it,d))
			continue ;

		gpg_friends.insert(d.gpg_id) ;
	}
	std::cerr << "Found " << gpg_friends.size() << " gpg friends." << std::endl ;

	// Put own id in queue, and empty the queue, treating all nodes.
	//
	while(!nodes_to_treat.empty())
	{	
		NodeInfo info(nodes_to_treat.back()) ;
		nodes_to_treat.pop_back() ;

		std::cerr << "  Poped out of queue: " << info.gpg_id << ", with level " << info.friend_level << std::endl ;
		RsPeerDetails detail ;

		if(!rsPeers->getPeerDetails(info.gpg_id,detail))
		{
			std::cerr << "Could not request GPG details for node " << info.gpg_id << std::endl ;
			continue ;
		}

		if(info.friend_level <= _max_friend_level && (info.friend_level != 1 || gpg_friends.find(info.gpg_id) != gpg_friends.end()))
		{

			GraphWidget::NodeType type ;
			GraphWidget::AuthType auth ;

			switch(info.friend_level)
			{
				case 0: type = GraphWidget::ELASTIC_NODE_TYPE_OWN ;
						  break ;
				case 1: type = GraphWidget::ELASTIC_NODE_TYPE_FRIEND ;
						  break ;
				case 2: type = GraphWidget::ELASTIC_NODE_TYPE_F_OF_F ;
						  break ;
				default:
						  type = GraphWidget::ELASTIC_NODE_TYPE_UNKNOWN ;
			}

			switch(detail.validLvl)
			{
				case GPGME_VALIDITY_MARGINAL: auth = GraphWidget::ELASTIC_NODE_AUTH_MARGINAL ; break;
				case GPGME_VALIDITY_FULL:
				case GPGME_VALIDITY_ULTIMATE: auth = GraphWidget::ELASTIC_NODE_AUTH_FULL ; break;
				case GPGME_VALIDITY_UNKNOWN:
				case GPGME_VALIDITY_UNDEFINED: 
				case GPGME_VALIDITY_NEVER:
				default: 							auth = GraphWidget::ELASTIC_NODE_AUTH_UNKNOWN ; break ;
			}

			node_ids[info.gpg_id] = ui.graphicsView->addNode("       "+detail.name, detail.name+"@"+info.gpg_id,type,auth);
			std::cerr << "  inserted node " << info.gpg_id << ", type=" << type << ", auth=" << auth << std::endl ;

			std::cerr << "NetworkView::insertPeers() Added Friend: " << info.gpg_id << std::endl;

			for(std::list<std::string>::const_iterator sit(detail.gpgSigners.begin()); sit != detail.gpgSigners.end(); ++sit)
				if(nodes_considered.find(*sit) == nodes_considered.end())
				{
					std::cerr << "  adding to queue: " << *sit << ", with level " << info.friend_level+1 << std::endl ;
					nodes_to_treat.push_front( NodeInfo(*sit,info.friend_level + 1) ) ;
					nodes_considered.insert(*sit) ;
				}
		}
	}

	/* iterate through all friends */

	std::cerr << "NetworkView::insertSignatures()" << std::endl;

	for(std::map<std::string,GraphWidget::NodeId>::const_iterator it(node_ids.begin()); it != node_ids.end(); it++)
	{
		RsPeerDetails detail;
		if (!rsPeers->getPeerDetails(it->first, detail))
			continue;

		for(std::list<std::string>::const_iterator sit(detail.gpgSigners.begin()); sit != detail.gpgSigners.end(); sit++)
		{
			if(it->first < *sit)
			{
				std::cerr << "NetworkView: Adding Arrow: ";
				std::cerr << *sit << " <-> " << it->first;
				std::cerr << std::endl;

				if(node_ids.find(*sit) != node_ids.end())
					ui.graphicsView->addEdge(node_ids[*sit], it->second);
			}
		}
	}

}


void  NetworkView::insertConnections()
{
#if 0
	/* iterate through all friends */
	std::list<std::string> fids, ids;
	std::list<std::string>::iterator it;

	std::string ownId = rsPeers->getGPGOwnId();
 	rsPeers->getPGPAllList(ids);
	rsPeers->getPGPFriendList(fids);

	std::cerr << "NetworkView::insertConnections()" << std::endl;

	// For the moment, only add friends.
	for(it = fids.begin(); it != fids.end(); it++)
	{
  		ui.graphicsView->addEdge("", *it);
		std::cerr << "NetworkView: Adding Edge: Self -> " << *it;
		std::cerr << std::endl;
	}

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
#endif

}

void  NetworkView::insertSignatures()
{
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
//        ui.viewsettingsframe->setVisible(true);
//        ui.nviewsettingsButton->setChecked(true);
//        ui.nviewsettingsButton->setToolTip(tr("Hide Settings"));
    } else {
//        ui.viewsettingsframe->setVisible(false);
//        ui.nviewsettingsButton->setChecked(false);
//        ui.nviewsettingsButton->setToolTip(tr("Show Settings"));
    }
}

