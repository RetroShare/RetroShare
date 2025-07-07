/*******************************************************************************
 * gui/elastic/edge.cpp                                                        *
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

// This code is inspired from http://doc.qt.io/qt-5/qtwidgets-graphicsview-elasticnodes-edge-cpp.html

#include <QPainter>
#include "gui/settings/rsharesettings.h"

#include "edge.h"
#include "elnode.h"

#include <math.h>

//static const double Pi = 3.14159265358979323846264338327950288419717;

Edge::Edge(Node *sourceNode, Node *destNode)
    : arrowSize(10)
{
    setAcceptedMouseButtons(0);
    source = sourceNode;
    dest = destNode;
    source->addEdge(this);
    dest->addEdge(this);
    adjust();
}

Edge::~Edge()
{
}

Node *Edge::sourceNode() const
{
    return source;
}

void Edge::setSourceNode(Node *node)
{
    source = node;
    adjust();
}

Node *Edge::destNode() const
{
    return dest;
}

void Edge::setDestNode(Node *node)
{
    dest = node;
    adjust();
}

void Edge::adjust()
{
    if (!source || !dest)
        return;

    QLineF line(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0));
    qreal length = line.length();
	 if(length==0.0f)
		 length=1.0 ;
    QPointF edgeOffset((line.dx() * 10) / length, (line.dy() * 10) / length);

    prepareGeometryChange();
    sourcePoint = line.p1() + edgeOffset;
    destPoint = line.p2() - edgeOffset;
}

QRectF Edge::boundingRect() const
{
    if (!source || !dest)
        return QRectF();

    qreal penWidth = 1;
    qreal extra = (penWidth + arrowSize) / 2.0;

    return QRectF(sourcePoint, QSizeF(destPoint.x() - sourcePoint.x(),
                                      destPoint.y() - sourcePoint.y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!source || !dest)
        return;

    // Draw the line itself
    QLineF line(sourcePoint, destPoint);
	if (Settings->getSheetName() == ":Standard_Dark"){
		painter->setPen(QPen(Qt::white, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	} else {
		painter->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	}
    painter->drawLine(line);

	 return ;
    // Draw the arrows if there's enough room

	 float length = line.length() ;
	 float cos_theta,sin_theta ;

	 if(length == 0.0f)
	 {
		 cos_theta = 1.0f ;
		 sin_theta = 0.0f ;
	 }
	 else
	 {
		 cos_theta = line.dx() / length ;
		 sin_theta =-line.dy() / length ;
	 }

	 static const float cos_pi_over_3 = 0.5 ;
	 static const float sin_pi_over_3 = 0.5 * sqrt(3) ;
	 static const float cos_2_pi_over_3 =-0.5 ;
	 static const float sin_2_pi_over_3 = 0.5 * sqrt(3) ;

	 float cos_theta_plus_pi_over_3 = cos_theta * cos_pi_over_3 - sin_theta * sin_pi_over_3 ;
	 float sin_theta_plus_pi_over_3 = sin_theta * cos_pi_over_3 + cos_theta * sin_pi_over_3 ;

	 float cos_theta_mins_pi_over_3 = cos_theta_plus_pi_over_3 + 2 * sin_theta * sin_pi_over_3 ;
	 float sin_theta_mins_pi_over_3 = sin_theta_plus_pi_over_3 - 2 * cos_theta * sin_pi_over_3 ;

	 float cos_theta_plus_2_pi_over_3 = cos_theta * cos_2_pi_over_3 - sin_theta * sin_2_pi_over_3 ;
	 float sin_theta_plus_2_pi_over_3 = sin_theta * cos_2_pi_over_3 + cos_theta * sin_2_pi_over_3 ;

	 float cos_theta_mins_2_pi_over_3 = cos_theta_plus_2_pi_over_3 + 2 * sin_theta * sin_2_pi_over_3 ;
	 float sin_theta_mins_2_pi_over_3 = sin_theta_plus_2_pi_over_3 - 2 * cos_theta * sin_2_pi_over_3 ;

    QPointF sourceArrowP1 = sourcePoint + QPointF(  sin_theta_plus_pi_over_3 * arrowSize,   cos_theta_plus_pi_over_3 * arrowSize);
    QPointF sourceArrowP2 = sourcePoint + QPointF(sin_theta_plus_2_pi_over_3 * arrowSize, cos_theta_plus_2_pi_over_3 * arrowSize);   

    QPointF destArrowP1 = destPoint + QPointF(  sin_theta_mins_pi_over_3 * arrowSize,   cos_theta_mins_pi_over_3 * arrowSize);
    QPointF destArrowP2 = destPoint + QPointF(sin_theta_mins_2_pi_over_3 * arrowSize, cos_theta_mins_2_pi_over_3 * arrowSize);

	if (Settings->getSheetName() == ":Standard_Dark"){
		painter->setBrush(Qt::white);
	}else {
		painter->setBrush(Qt::black);
	}
    painter->drawConvexPolygon(QPolygonF() << line.p1() << sourceArrowP1 << sourceArrowP2);
    painter->drawConvexPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);        
}

