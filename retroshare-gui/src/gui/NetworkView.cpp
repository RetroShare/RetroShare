/*******************************************************************************
 * gui/NetworkView.cpp                                                         *
 *                                                                             *
 * Copyright (c) 2008 Robert Fernie    <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "NetworkView.h"
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

#include <deque>
#include <set>
#include <iostream>
#include <algorithm>

#include "gui/elastic/elnode.h"

/********
* #define DEBUG_NETWORKVIEW
********/

/** Constructor */
NetworkView::NetworkView(QWidget *parent)
: RsAutoUpdatePage(60000,parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  mScene = new QGraphicsScene();
  mScene->setItemIndexMethod(QGraphicsScene::NoIndex);
  mScene->setSceneRect(0, 0, ui.graphicsView->width(), ui.graphicsView->height());

  ui.graphicsView->setScene(mScene);
  ui.graphicsView->setEdgeLength(ui.edgeLengthSB->value()) ;

  setMaxFriendLevel(ui.maxFriendLevelSB->value()) ;

  /* add button */
  connect( ui.refreshButton, SIGNAL( clicked( void ) ), this, SLOT( redraw( void ) ) );
  connect( mScene, SIGNAL( changed ( const QList<QRectF> & ) ), this, SLOT ( changedScene( void ) ) );

  /* Hide Settings frame */
  connect( ui.maxFriendLevelSB, SIGNAL(valueChanged(int)), this, SLOT(setMaxFriendLevel(int)));
  connect( ui.edgeLengthSB, SIGNAL(valueChanged(int)), this, SLOT(setEdgeLength(int)));
  connect( ui.freezeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setFreezeState(bool)));
  connect( ui.nameBox, SIGNAL(textChanged(QString)), this, SLOT(setNameSearch(QString)));

  QString help_text = tr(
  "<h2>Graph scene info</h2>"
  "<p>You could use mouse to deal with the scene here:</p>"
  "<ul>"
  "<li>Left mouse button press on a node: " "selection to see with which nodes it has connections"
    " (you can drag it for some distance)"
  "<li>Right mouse button click on a node: " "to do some actions with it"
  "<li>Drag scene with left mouse button: " "to scroll"
  "</ul>"
  "<p>and keyboard:</p>"
  "<ul>"
  "<li>Arrow keys: " "to scroll"
  "<li>Space/Enter: " "to repaint a scene"
  "<li>+/-: " "to scale"
  "</ul>"
  );
  registerHelpButton(ui.helpButton, help_text, "Graph scene");

  _should_update = true ;
}

NetworkView::~NetworkView()
{
	if(mScene != NULL)
		delete mScene ;
}

void NetworkView::setEdgeLength(int l)
{
	ui.graphicsView->setEdgeLength(l);
}
void NetworkView::setNameSearch(QString s)
{
	ui.graphicsView->setNameSearch(s);
}
void NetworkView::setMaxFriendLevel(int m)
{
	ui.graphicsView->snapshotNodesPositions() ;
	clear() ;
	_max_friend_level = m ;
	updateDisplay() ;
}
void NetworkView::changedFoFCheckBox( )
{
	updateDisplay();
}
void NetworkView::redraw()
{
	ui.graphicsView->clearNodesPositions() ;
	clear() ;
	updateDisplay() ;
}

void  NetworkView::clear()
{
	ui.graphicsView->clearGraph() ;
	_node_ids.clear() ;
	update() ;
}

void NetworkView::setFreezeState(bool freeze)
{
#ifdef DEBUG_NETWORKVIEW
	std::cerr << "  NetworkView::setFreezeState(" << freeze << ")" << std::endl;
#endif

	ui.graphicsView->setFreeze(freeze);
}

class NodeInfo
{
	public:
        NodeInfo(const RsPgpId& _gpg_id,uint32_t lev) : gpg_id(_gpg_id),friend_level(lev) {}

        RsPgpId gpg_id ;
		uint32_t friend_level ;
} ;

void  NetworkView::update()
{
	_should_update = true ;
}
void  NetworkView::updateDisplay()
{
	if(!isVisible())
		return ;
	if(!_should_update)
		return ;

	/* add all friends */
    RsPgpId ownGPGId = rsPeers->getGPGOwnId();
//#ifdef DEBUG_NETWORKVIEW
	std::cerr << "NetworkView::updateDisplay()" << std::endl;
//#endif

	std::deque<NodeInfo> nodes_to_treat ;						// list of nodes to be treated. Used as a queue. The int is the level of friendness
    std::set<RsPgpId> nodes_considered ;					// list of nodes already considered. Eases lookup.

	nodes_to_treat.push_front(NodeInfo(ownGPGId,0)) ;		// initialize queue with own id.
    nodes_considered.insert(rsPeers->getGPGOwnId()) ;

	// Put own id in queue, and empty the queue, treating all nodes.
	//
	while(!nodes_to_treat.empty())
	{	
		NodeInfo info(nodes_to_treat.back()) ;
		nodes_to_treat.pop_back() ;
#ifdef DEBUG_NETWORKVIEW
		std::cerr << "  Poped out of queue: " << info.gpg_id << ", with level " << info.friend_level << std::endl ;
#endif
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

		RsPeerDetails detail ;
        if(!rsPeers->getGPGDetails(info.gpg_id, detail))
			continue ;

		switch(detail.trustLvl)
		{
			case RS_TRUST_LVL_MARGINAL: auth = GraphWidget::ELASTIC_NODE_AUTH_MARGINAL ; break;
			case RS_TRUST_LVL_FULL:
			case RS_TRUST_LVL_ULTIMATE: auth = GraphWidget::ELASTIC_NODE_AUTH_FULL ; break;
			case RS_TRUST_LVL_UNKNOWN:
			case RS_TRUST_LVL_UNDEFINED: 
			case RS_TRUST_LVL_NEVER:
			default: 							auth = GraphWidget::ELASTIC_NODE_AUTH_UNKNOWN ; break ;
		}

		if(info.friend_level <= _max_friend_level && _node_ids.find(info.gpg_id) == _node_ids.end())
		{
            _node_ids[info.gpg_id] = ui.graphicsView->addNode("       "+detail.name, detail.name+"@"+detail.gpg_id.toStdString(),type,auth,RsPeerId(),info.gpg_id);
#ifdef DEBUG_NETWORKVIEW
			std::cerr << "  inserted node " << info.gpg_id << ", type=" << type << ", auth=" << auth << std::endl ;
#endif
		}

        std::list<RsPgpId> friendList;
		rsDisc->getDiscPgpFriends(info.gpg_id, friendList);

#ifdef DEBUG_NETWORKVIEW
		std::cerr << "  Got a list of " << friendList.size() << " friends for this peer." << std::endl ;
#endif

		if(info.friend_level+1 <= _max_friend_level)
            for(std::list<RsPgpId>::const_iterator sit(friendList.begin()); sit != friendList.end(); ++sit)
                if(nodes_considered.find(*sit) == nodes_considered.end())
				{
#ifdef DEBUG_NETWORKVIEW
					std::cerr << "  adding to queue: " << *sit << ", with level " << info.friend_level+1 << std::endl ;
#endif
					nodes_to_treat.push_front( NodeInfo(*sit,info.friend_level + 1) ) ;
					nodes_considered.insert(*sit) ;
				}
	}

	/* iterate through all friends */

#ifdef DEBUG_NETWORKVIEW
	std::cerr << "NetworkView::insertSignatures()" << std::endl;
#endif

    for(std::map<RsPgpId,GraphWidget::NodeId>::const_iterator it(_node_ids.begin()); it != _node_ids.end(); ++it)
	{
        std::list<RsPgpId> friendList ;

		if(rsDisc->getDiscPgpFriends(it->first,friendList)) 
            for(std::list<RsPgpId>::const_iterator sit(friendList.begin()); sit != friendList.end(); ++sit)
			{
#ifdef DEBUG_NETWORKVIEW
					std::cerr << "NetworkView: Adding Edge: ";
					std::cerr << *sit << " <-> " << it->first;
					std::cerr << std::endl;
#endif

					if(_node_ids.find(*sit) != _node_ids.end())
						ui.graphicsView->addEdge(_node_ids[*sit], it->second);
			}
	}

	_should_update = false ;
}

