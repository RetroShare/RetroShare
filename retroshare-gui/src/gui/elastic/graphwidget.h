/*******************************************************************************
 * gui/elastic/graphwidget.h                                                   *
 *                                                                             *
 * Copyright (c) 2012, RetroShare Team <retroshare.project@gmail.com>          *
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

// This code is inspired from http://doc.qt.io/qt-5/qtwidgets-graphicsview-elasticnodes-graphwidget-h.html

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <map>
#include <QGraphicsView>
#include <stdint.h>
#include <retroshare/rstypes.h>

class Node;
class Edge;

class GraphWidget : public QGraphicsView
{
    Q_OBJECT

public:
    GraphWidget(QWidget * = NULL);

	 typedef int NodeId ;
	 typedef int EdgeId ;

	 typedef enum  {
		 	ELASTIC_NODE_TYPE_OWN				= 0x0000,
			ELASTIC_NODE_TYPE_FRIEND 			= 0x0001,
			ELASTIC_NODE_TYPE_F_OF_F 			= 0x0002,
			ELASTIC_NODE_TYPE_UNKNOWN			= 0x0003
	 } NodeType ;

	 typedef enum {
			ELASTIC_NODE_AUTH_FULL			= 0x0000,
			ELASTIC_NODE_AUTH_MARGINAL		= 0x0001,
			ELASTIC_NODE_AUTH_UNKNOWN		= 0x0002
	 } AuthType ;

	 NodeId addNode(const std::string& NodeShortText,const std::string& nodeCompleteText,NodeType type,AuthType auth,const RsPeerId& ssl_id,const RsPgpId& gpg_id) ;
	 EdgeId addEdge(NodeId n1,NodeId n2) ;

	 void snapshotNodesPositions() ;
	 void clearNodesPositions() ;
	 void clearGraph() ;

	void setFreeze(bool freeze);
	bool isFrozen() const;

    virtual void itemMoved();

	 void setEdgeLength(uint32_t l) ;
	 void setNameSearch(QString) ;
	 uint32_t edgeLength() const { return _edge_length ; }

	 void forceRedraw() ;
protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void timerEvent(QTimerEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    //void drawBackground(QPainter *painter, const QRectF &rect);

    void scaleView(qreal scaleFactor);

private:
    int timerId;
    //Node *centerNode;
	 bool mDeterminedBB ;
	bool mIsFrozen;

	 std::vector<Node *> _nodes ;
	 std::map<std::pair<NodeId,NodeId>,Edge *> _edges ;
	 std::map<std::string,QPointF> _node_cached_positions ;

	 uint32_t _edge_length ;
	 float _friction_factor ;
	 NodeId _current_node ;
};

#endif
