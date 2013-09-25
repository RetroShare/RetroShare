/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the example classes of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef NODE_H
#define NODE_H

#include <QApplication>
#if QT_VERSION >= 0x040600
#include <QGraphicsObject>
#else
#include <QGraphicsItem>
#endif
#include <QList>

#include "graphwidget.h"

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
    Node(const std::string& node_string,GraphWidget::NodeType type,GraphWidget::AuthType auth,GraphWidget *graphWidget,const std::string& ssl_id,const std::string& gpg_id);

    void addEdge(Edge *edge);
    const QList<Edge *>& edges() const;

    int type() const { return Type; }
	 const std::string& idString() const { return _gpg_id ; }

    void calculateForces(const double *data,int width,int height,int W,int H,float x,float y,float speedf);
    bool advance();

    QRectF boundingRect() const;
    QPainterPath shape() const;
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

	 static Node *_selected_node ;

	 std::string _ssl_id ;
	 std::string _gpg_id ;

	 static const float MASS_FACTOR = 10 ;
	 static const float FRICTION_FACTOR = 10.8 ;
	 static const float REPULSION_FACTOR = 4 ;
	 static const float NODE_DISTANCE = 130.0 ;
};

#endif
