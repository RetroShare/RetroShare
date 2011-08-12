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

#include <QPainter>

#include "edge.h"
#include "node.h"

#include <math.h>

static const double Pi = 3.14159265358979323846264338327950288419717;

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
    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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

    painter->setBrush(Qt::black);
    painter->drawConvexPolygon(QPolygonF() << line.p1() << sourceArrowP1 << sourceArrowP2);
    painter->drawConvexPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);        
}

