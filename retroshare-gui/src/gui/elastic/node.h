/****************************************************************************
**
** Copyright (C) 2006-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.0, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** In addition, as a special exception, Trolltech, as the sole copyright
** holder for Qt Designer, grants users of the Qt/Eclipse Integration
** plug-in the right for the Qt/Eclipse Integration to link to
** functionality provided by Qt Designer and its related libraries.
**
** Trolltech reserves all rights not expressly granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef NODE_H
#define NODE_H

#include <QGraphicsItem>
#include <QList>
#include <stdint.h>

#include <string>

#define ELASTIC_NODE_TYPE_OWN		1
#define ELASTIC_NODE_TYPE_FRIEND	2
#define ELASTIC_NODE_TYPE_AUTHED	3
#define ELASTIC_NODE_TYPE_MARGINALAUTH	4
#define ELASTIC_NODE_TYPE_FOF		5

class Edge;
class Arrow;
class GraphWidget;
class QGraphicsSceneMouseEvent;

class Node : public QObject, public QGraphicsItem
{
  Q_OBJECT


public:
    Node(GraphWidget *graphWidget, uint32_t t, std::string id_in, std::string n);

    void addEdge(Edge *edge);
    QList<Edge *> edges() const;

    void addArrow(Arrow *arrow);
    QList<Arrow *> arrows() const;

    enum { Type = UserType + 1 };
    int type() const { return Type; }

    void calculateForces();
    bool advance();

    QRectF boundingRect() const;
    //QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
	void peerdetails();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event); 

private:
    QList<Edge *> edgeList;
    QList<Arrow *> arrowList;
    QPointF newPos;
    GraphWidget *graph;

    /* extra information */
	uint32_t ntype; /* Ourself, friend, fof */
	std::string id;
	std::string name;

    bool mDeterminedBB;
    uint32_t mBBWidth;

};

#endif
