/*******************************************************************************
 * gui/elastic/arrow.h                                                         *
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

// This code is inspired from http://doc.qt.io/qt-5/qtwidgets-graphicsview-diagramscene-arrow-h.html

#ifndef ARROW_H
#define ARROW_H

#include <QGraphicsItem>

class Node;

class Arrow : public QGraphicsItem
{
public:
    Arrow(Node *sourceNode, Node *destNode);
    ~Arrow();

    Node *sourceNode() const;
    void setSourceNode(Node *node);

    Node *destNode() const;
    void setDestNode(Node *node);

    void adjust();

    enum { Type = UserType + 3 };
    int type() const { return Type; }
    
protected:
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    
private:
    Node *source, *dest;

    QPointF sourcePoint;
    QPointF destPoint;
    qreal arrowSize;
};

#endif
