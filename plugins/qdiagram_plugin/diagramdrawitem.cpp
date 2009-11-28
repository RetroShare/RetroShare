/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>
#include <iostream>

#include "diagramdrawitem.h"
#include "diagramscene.h"

//! [0]
DiagramDrawItem::DiagramDrawItem(DiagramType diagramType, QMenu *contextMenu,
             QGraphicsItem *parent, QGraphicsScene *scene)
	: DiagramItem(contextMenu,parent,scene)
{
	myPos2=pos();
    myDiagramType = diagramType;
    myContextMenu = contextMenu;

    myPolygon=createPath();
    setPolygon(myPolygon);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    myHoverPoint=-1;
    mySelPoint=-1;
    myHandlerWidth=2.0;
}
//! [0]
DiagramDrawItem::DiagramDrawItem(const DiagramDrawItem& diagram)
	: DiagramItem(diagram.myContextMenu,diagram.parentItem(),0)
{

	myDiagramType=diagram.myDiagramType;
	// copy from general GraphcsItem
	setBrush(diagram.brush());
	setPen(diagram.pen());
	setTransform(diagram.transform());
	myPos2=diagram.myPos2;
	myPolygon=createPath();
	setPolygon(myPolygon);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setAcceptHoverEvents(true);
	myHoverPoint=-1;
	mySelPoint=-1;
	myHandlerWidth=2.0;

}
//! [1]
QPolygonF DiagramDrawItem::createPath()
{
		qreal dx=myPos2.x();
	    qreal dy=myPos2.y();

	    QPainterPath path;
	    QPolygonF polygon;
	    switch (myDiagramType) {
	        case Rectangle:
	            path.moveTo(0, 0);
	            path.lineTo(dx,0);
	            path.lineTo(dx,dy);
	            path.lineTo(0,dy);
	            path.lineTo(0,0);
	            polygon = path.toFillPolygon();
	            break;
	        case Ellipse:
	        	path.addEllipse(0,0,dx,dy);
	        	polygon = path.toFillPolygon();
	            break;
	        default:
	            break;
	            polygon = 0;
	    }
	    return polygon;
}
//! [4]
QPixmap DiagramDrawItem::image() const
{
    QPixmap pixmap(250, 250);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    painter.translate(10, 10);
    painter.drawPolyline(myPolygon);

    return pixmap;
}
//! [4]

//! [5]
void DiagramDrawItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}
//! [5]

//! [6]
QVariant DiagramDrawItem::itemChange(GraphicsItemChange change,
                     const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        ;
    }

    return value;
}
//! [6]
DiagramItem* DiagramDrawItem::copy()
{
    DiagramDrawItem* newDiagramDrawItem=new DiagramDrawItem(*this);
    return dynamic_cast<DiagramItem*>(newDiagramDrawItem);
}

void DiagramDrawItem::setPos2(qreal x,qreal y)
{
	myPos2=mapFromScene(QPointF(x,y));
	myPolygon=createPath();
	setPolygon(myPolygon);
}

void DiagramDrawItem::setPos2(QPointF newPos)
{
	prepareGeometryChange();
	myPos2=mapFromScene(newPos);
	myPolygon=createPath();
	setPolygon(myPolygon);
}

void DiagramDrawItem::setDimension(QPointF newPos)
{
	prepareGeometryChange();
	myPos2=newPos;
	myPolygon=createPath();
	setPolygon(myPolygon);
}

QPointF DiagramDrawItem::getDimension()
{
	return myPos2;
}

void DiagramDrawItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
           QWidget *)
{
	 painter->setPen(pen());
	 painter->setBrush(brush());
	 painter->drawPolygon(polygon());
	 // selected
	 if(isSelected()){
		 // Rect
		 QPen selPen=QPen(Qt::DashLine);
		 selPen.setColor(Qt::black);
		 QBrush selBrush=QBrush(Qt::NoBrush);
		 painter->setBrush(selBrush);
		 painter->setPen(selPen);
		 painter->drawRect(QRectF(QPointF(0,0),myPos2));
		 // Draghandles
		 selBrush=QBrush(Qt::cyan,Qt::SolidPattern);
		 selPen=QPen(Qt::cyan);
		 painter->setBrush(selBrush);
		 painter->setPen(selPen);
		 QPointF point;
		 for(int i=0;i<8;i++)
		 {
			 if(i<3) point=QPointF(myPos2.x()/2*i,0);
			 if(i==3) point=QPointF(myPos2.x(),myPos2.y()/2);
			 if(i>3 && i<7) point=QPointF(myPos2.x()/2*(i-4),myPos2.y());
			 if(i==7) point=QPointF(0,myPos2.y()/2);
			 if(i==myHoverPoint){
				 painter->setBrush(QBrush(Qt::red));
			 }
			 // Rect around valid point
			 painter->drawRect(QRectF(point-QPointF(2,2),point+QPointF(2,2)));
			 if(i==myHoverPoint){
				 painter->setBrush(selBrush);
			 }
		 }// foreach
     }// if
}

void DiagramDrawItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
#ifdef DEBUG
	std::cout << "entered" << std::endl;
	std::cout << e->pos().x() << "/" << e->pos().y() << std::endl;
#endif
	if (isSelected()) {
		QPointF hover_point = e -> pos();
		QPointF point;
		for(myHoverPoint=0;myHoverPoint<8;myHoverPoint++){
			if(myHoverPoint<3) point=QPointF(myPos2.x()/2*myHoverPoint,0);
			if(myHoverPoint==3) point=QPointF(myPos2.x(),myPos2.y()/2);
			if(myHoverPoint>3 && myHoverPoint<7) point=QPointF(myPos2.x()/2*(myHoverPoint-4),myPos2.y());
			if(myHoverPoint==7) point=QPointF(0,myPos2.y()/2);
			if(hasClickedOn(hover_point,point)) break;
		}//for
		if(myHoverPoint==8) myHoverPoint=-1;
		else update();
	}
	DiagramItem::hoverEnterEvent(e);
}

void DiagramDrawItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
#ifdef DEBUG
	std::cout << "left" << std::endl;
#endif
	if (isSelected()) {
		if(myHoverPoint>-1){
			myHoverPoint=-1;
			update();
		}
	}
	DiagramItem::hoverLeaveEvent(e);
}

bool DiagramDrawItem::hasClickedOn(QPointF press_point, QPointF point) const {
	return (
		press_point.x() >= point.x() - myHandlerWidth &&\
		press_point.x() <  point.x() + myHandlerWidth &&\
		press_point.y() >= point.y() - myHandlerWidth &&\
		press_point.y() <  point.y() + myHandlerWidth
	);
}

QPointF DiagramDrawItem::onGrid(QPointF pos)
{
	DiagramScene* myScene = dynamic_cast<DiagramScene*>(scene());
	QPointF result = myScene->onGrid(pos);
	return result;
}

QPainterPath DiagramDrawItem::shape() const {
	QPainterPath myPath;
	myPath.addPolygon(polygon());
	if(isSelected()){
		QPointF point;
		for(int i=0;i<8;i++)
		{
			if(i<3) point=QPointF(myPos2.x()/2*i,0);
			if(i==3) point=QPointF(myPos2.x(),myPos2.y()/2);
			if(i>3 && i<7) point=QPointF(myPos2.x()/2*(i-4),myPos2.y());
			if(i==7) point=QPointF(0,myPos2.y()/2);
			// Rect around valid point
			myPath.addRect(QRectF(point-QPointF(myHandlerWidth,myHandlerWidth),point+QPointF(myHandlerWidth,myHandlerWidth)));
		}// for
	}// if
	return myPath;
}

QRectF DiagramDrawItem::boundingRect() const
{
    qreal extra = pen().width()+20 / 2.0 + myHandlerWidth;
    qreal minx = myPos2.x() < 0 ? myPos2.x() : 0;
    qreal maxx = myPos2.x() < 0 ? 0 : myPos2.x() ;
    qreal miny = myPos2.y() < 0 ? myPos2.y() : 0;
    qreal maxy = myPos2.y() < 0 ? 0 : myPos2.y() ;

    QRectF newRect = QRectF(minx,miny,maxx-minx,maxy-miny)
    .adjusted(-extra, -extra, extra, extra);
    return newRect;
}

void DiagramDrawItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if(isSelected()){
		if (e -> buttons() & Qt::LeftButton) {
			QPointF mouse_point = e -> pos();
			QPointF point;
			for(mySelPoint=0;mySelPoint<8;mySelPoint++){
				if(mySelPoint<3) point=QPointF(myPos2.x()/2*mySelPoint,0);
				if(mySelPoint==3) point=QPointF(myPos2.x(),myPos2.y()/2);
				if(mySelPoint>3 && mySelPoint<7) point=QPointF(myPos2.x()/2*(mySelPoint-4),myPos2.y());
				if(mySelPoint==7) point=QPointF(0,myPos2.y()/2);
				if(hasClickedOn(mouse_point,point)) break;
			}//for
			if(mySelPoint==8) mySelPoint=-1;
			else e->accept();
		}
	}
	DiagramItem::mousePressEvent(e);
}

void DiagramDrawItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	// left click
	if ((e -> buttons() & Qt::LeftButton)&&(mySelPoint>-1)) {
		QPointF mouse_point = onGrid(e -> pos());
#ifdef DEBUG
		std::cout << "Corner: " << mySelPoint << std::endl;
		std::cout << "mouse: " << mouse_point.x() << "/" << mouse_point.y() << std::endl;
		std::cout << "pos2: " << myPos2.x() << "/" << myPos2.y() << std::endl;
#endif
		prepareGeometryChange();
		switch (mySelPoint) {
			case 0:
				myPos2=myPos2-mouse_point;
				setPos(mapToScene(mouse_point));
				break;
			case 1:
				setPos(pos().x(),mapToScene(mouse_point).y());
				myPos2.setY(myPos2.y()-mouse_point.y());
				break;
			case 2:
				myPos2.setX(mouse_point.x());
				setPos(pos().x(),mapToScene(mouse_point).y());
				myPos2.setY(myPos2.y()-mouse_point.y());
				break;
			case 3:
				myPos2.setX(mouse_point.x());
				break;
			case 6:
				myPos2.setX(mouse_point.x());
				myPos2.setY(mouse_point.y());
				break;
			case 5:
				myPos2.setY(mouse_point.y());
				break;
			case 4:
				myPos2.setY(mouse_point.y());
				setPos(mapToScene(mouse_point).x(),pos().y());
				myPos2.setX(myPos2.x()-mouse_point.x());
				break;
			case 7:
				setPos(mapToScene(mouse_point).x(),pos().y());
				myPos2.setX(myPos2.x()-mouse_point.x());
				break;
			default:
				break;
		}
		myPolygon=createPath();
		setPolygon(myPolygon);
	}
	else
	DiagramItem::mouseMoveEvent(e);
}
