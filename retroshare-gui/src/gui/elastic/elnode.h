/*******************************************************************************
 * gui/elastic/elnode.h                                                        *
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

// This code is inspired from http://doc.qt.io/qt-5/qtwidgets-graphicsview-elasticnodes-node-h.html

#ifndef ELNODE_H
#define ELNODE_H

#include "graphwidget.h"

#include <retroshare/rstypes.h>

#include <QApplication>
#if QT_VERSION >= 0x040600
#include <QGraphicsObject>
#else
#include <QGraphicsItem>
#endif
#include <QList>
#include <QPainterPath>

class Edge;
QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

#if QT_VERSION >= 0x040600
class Node : public QGraphicsObject
#else
class Node : public QObject, public QGraphicsItem
#endif
{
	Q_OBJECT

public:
    Node(const std::string& node_string,GraphWidget::NodeType type,GraphWidget::AuthType auth,GraphWidget *graphWidget,const RsPeerId& ssl_id,const RsPgpId& gpg_id);

    void addEdge(Edge *edge);
    const QList<Edge *>& edges() const;

	int type() const { return Type; }
	std::string idString() const { return _gpg_id.toStdString() ; }
	std::string descString() const { return _desc_string ; }

    void calculateForces(const double *data,int width,int height,int W,int H,float x,float y,float speedf);
    bool progress();

    QRectF boundingRect() const;
    QPainterPath shape() const;

	void setNodeDrawSize(int nds, bool repaint_edges) {mNodeDrawSize = nds; sought_for=repaint_edges;}
	int getNodeDrawSize(){return mNodeDrawSize;}

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	 int n_edges() const { return edgeList.size() ; }
protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	 virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *) ;
    
 protected slots:
	 void denyFriend() ;
	 void makeFriend() ;
	 void peerDetails() ;
private:
    QList<Edge *> edgeList;
    QPointF newPos;
    GraphWidget *graph;
	 qreal _speedx,_speedy;
	 int _steps ;
	 std::string _desc_string ;
	 GraphWidget::NodeType _type ;
	 GraphWidget::AuthType _auth ;
	 bool mDeterminedBB ;
	 int mBBWidth ;
	 int mNodeDrawSize;
	 bool sought_for = false;

	 static Node *_selected_node ;

	 RsPeerId _ssl_id ;
	 RsPgpId _gpg_id ;

     static const float MASS_FACTOR;
     static const float FRICTION_FACTOR;
     static const float REPULSION_FACTOR;
     static const float NODE_DISTANCE;
};

#endif
