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

#include "diagrampathitem.h"
#include "diagramscene.h"

#define NDEBUG

//! [0]
DiagramPathItem::DiagramPathItem(DiagramType diagramType, QMenu *contextMenu,
             QGraphicsItem *parent, QGraphicsScene *scene)
    : QGraphicsPathItem(parent)
{
    myDiagramType = diagramType;
    myContextMenu = contextMenu;
    myPoints.clear();

    len = 10.0; // Pfeillänge
    breite = 4.0; // Divisor Pfeilbreite

    // standard initialize
    mySelPoint=-1;
    myHandlerWidth = 2.0;
    myHoverPoint=-1;

    setBrush(QBrush(Qt::black));
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setAcceptHoverEvents(true);
}

DiagramPathItem::DiagramPathItem(QMenu *contextMenu,
             QGraphicsItem *parent, QGraphicsScene *scene)
    : QGraphicsPathItem(parent)
{
    myDiagramType = Path;
    myContextMenu = contextMenu;
    myPoints.clear();

    len = 10.0; // Pfeillänge
    breite = 4.0; // Divisor Pfeilbreite

	// standard initialize
	mySelPoint=-1;
	myHandlerWidth = 2.0;
	myHoverPoint=-1;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptHoverEvents(true);
}
//! [0]
DiagramPathItem::DiagramPathItem(const DiagramPathItem& diagram)
{
	QGraphicsPathItem(diagram.parentItem(),diagram.scene());

	// copy from general GraphcsItem
	setBrush(diagram.brush());
	setPen(diagram.pen());
	setTransform(diagram.transform());

	// copy DiagramPathItem
	myDiagramType = diagram.myDiagramType;
	myContextMenu = diagram.myContextMenu;
	myPoints = diagram.myPoints;

	len = diagram.len;

	setPath(diagram.path());
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setAcceptHoverEvents(true);

	// standard initialize
	mySelPoint=-1;
	myHandlerWidth = 2.0;
	myHoverPoint=-1;
}

void DiagramPathItem::createPath()
{
	QPainterPath myPath=getPath();
	if(myPath.elementCount()>0) setPath(myPath);
}

QPainterPath DiagramPathItem::getPath() const
{
	QPainterPath myPath;
	QPointF p1,p2;
	if(myPoints.size()>1)
	{
		for (int i = 1; i < myPoints.size(); ++i) {
			p1=myPoints.at(i-1);
			p2=myPoints.at(i);
			if( (i==1)&&((myDiagramType==Start) || (myDiagramType==StartEnd)) )
			{
				QPainterPath arrow = createArrow(p2,p1);
				myPath.addPath(arrow);
			}

			myPath.moveTo(p2);
			myPath.lineTo(p1);
			myPath.closeSubpath();
		}
		if((myDiagramType==End) or (myDiagramType==StartEnd)){
			QPainterPath arrow = createArrow(p1,p2);
			myPath.addPath(arrow);
		}
	}
	return myPath;
}

QPainterPath DiagramPathItem::createArrow(QPointF p1, QPointF p2) const
{
#define pi 3.141592654
	QPainterPath arrow;
	qreal dx=p1.x()-p2.x();
	qreal dy=p1.y()-p2.y();
	qreal m=sqrt(dx*dx+dy*dy);
	if(m>1){
		arrow.moveTo(p2);
		arrow.lineTo(-len/breite*dy/m+len*dx/m+p2.x(),len/breite*dx/m+len*dy/m+p2.y());
		arrow.lineTo(len/breite*dy/m+len*dx/m+p2.x(),-len/breite*dx/m+len*dy/m+p2.y());
		arrow.closeSubpath();
	}
	return arrow;
}

//! [4]
QPixmap DiagramPathItem::image() const
{
    QPixmap pixmap(250, 250);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    painter.translate(125, 125);
    QPolygonF myPolygon;
    myPolygon << QPointF(-100,-100) << QPointF(0,-100) << QPointF(0,100) << QPointF(100,100) ;;
    painter.drawPolyline(myPolygon);
    return pixmap;
}
//! [4]
QPixmap DiagramPathItem::icon()
{
    QPixmap pixmap(50, 80);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 8));
    myPoints.clear();
    myPoints.append(QPointF(5,40));
    myPoints.append(QPointF(45,40));
    len=10.0;
    breite=1.0;
    painter.drawPath(getPath());
    return pixmap;
}


//! [5]
void DiagramPathItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    myContextMenu->exec(event->screenPos());
}
//! [5]

//! [6]
void DiagramPathItem::append(const QPointF point)
{
	if(myPoints.size()>1)
	{
		prepareGeometryChange();
		updateLast(point);
		myPoints.append(mapFromScene(point));
	}
	else
	{
		/*myPoints.append(point-pos());
		myPoints.append(point-pos());*/
		myPoints.append(mapFromScene(point));
		myPoints.append(mapFromScene(point));
		createPath();
	}
}

void DiagramPathItem::remove()
{
	if(myPoints.size()>1)
	{
		prepareGeometryChange();
		myPoints.removeLast();
		updateLast(mapToScene(myPoints.last()));
	}
}

void DiagramPathItem::updateLast(const QPointF point)
{
	int i = myPoints.size()-1;
	if (i>0){
		prepareGeometryChange();
		myPoints[i]=mapFromScene(point);
		createPath();
	}
}

QVariant DiagramPathItem::itemChange(GraphicsItemChange change,
                     const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        //foreach (Arrow *arrow, arrows) {
        //    arrow->updatePosition();
        //}
    }

    return value;
}
//! [6]
DiagramPathItem* DiagramPathItem::copy()
{
    DiagramPathItem* newDiagramPathItem=new DiagramPathItem(*this);
    return newDiagramPathItem;
}

void DiagramPathItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
           QWidget *)
{
	 painter->setPen(pen());
	 painter->setBrush(brush());
	 painter->drawPath(getPath());
	 // selected
	 if(isSelected()){
		 QBrush selBrush=QBrush(Qt::cyan);
		 QPen selPen=QPen(Qt::cyan);
		 painter->setBrush(selBrush);
		 painter->setPen(selPen);
		 QPointF point;
		 for(int i=0;i<myPoints.count();i++)
		 {
			 point = myPoints.at(i);
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

QRectF DiagramPathItem::boundingRect() const
{
    qreal extra = (pen().width() + 20) / 2.0;

    qreal minx,maxx,miny,maxy;
    bool first=true;
    foreach(QPointF point,myPoints){
    	if(first){
    		minx=point.x();
    		miny=point.y();
    		maxx=point.x();
    		maxy=point.y();
    		first=false;
    	}
    	else{
    		if(point.x()<minx) minx=point.x();
    		if(point.x()>maxx) maxx=point.x();
    		if(point.y()<miny) miny=point.y();
    		if(point.y()>maxy) maxy=point.y();
    	}

    }

    return QRectF(minx,miny,maxx-minx,maxy-miny)
        .adjusted(-extra, -extra, extra, extra);
}

void DiagramPathItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if(isSelected()){
		if (e -> buttons() & Qt::LeftButton) {
			QPointF mouse_point = onGrid(e -> pos());
			for(mySelPoint=0;mySelPoint<myPoints.count();mySelPoint++){
				if(hasClickedOn(mouse_point,myPoints.at(mySelPoint))) break;
			}
			if(mySelPoint==myPoints.count()) mySelPoint=-1;
			else e->accept();
		}
	}
	QGraphicsPathItem::mousePressEvent(e);
}

void DiagramPathItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	// left click
	if ((e -> buttons() & Qt::LeftButton)&&(mySelPoint>-1)) {
		QPointF mouse_point = onGrid(e -> pos());
		myPoints.replace(mySelPoint,onGrid(mouse_point));
		createPath();
	}
}

QPainterPath DiagramPathItem::shape() const {
	QPainterPath myPath = getPath();
	if(isSelected()){
			 foreach (QPointF point, myPoints)
			 {
				 // Rect around valid point
				 myPath.addRect(QRectF(point-QPointF(myHandlerWidth,myHandlerWidth),point+QPointF(myHandlerWidth,myHandlerWidth)));
			 }// foreach
	     }// if
	return myPath;
}

bool DiagramPathItem::hasClickedOn(QPointF press_point, QPointF point) const {
	return (
		press_point.x() >= point.x() - myHandlerWidth &&\
		press_point.x() <  point.x() + myHandlerWidth &&\
		press_point.y() >= point.y() - myHandlerWidth &&\
		press_point.y() <  point.y() + myHandlerWidth
	);
}

QPointF DiagramPathItem::onGrid(QPointF pos)
{
	DiagramScene* myScene = dynamic_cast<DiagramScene*>(scene());
	QPointF result = myScene->onGrid(pos);
	return result;
}

void DiagramPathItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
#ifdef DEBUG
	std::cout << "entered" << std::endl;
	std::cout << e->pos().x() << "/" << e->pos().y() << std::endl;
#endif
	if (isSelected()) {
		QPointF hover_point = onGrid(e -> pos());
		for(myHoverPoint=0;myHoverPoint<myPoints.count();myHoverPoint++){
			if(hasClickedOn(hover_point,myPoints.at(myHoverPoint))) break;
		}//for
		if(myHoverPoint==myPoints.count()) myHoverPoint=-1;
		else update();
	}
	QGraphicsPathItem::hoverEnterEvent(e);
}

void DiagramPathItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
#ifdef DEBUG
	std::cout << "left" << std::endl;
#endif
	if (isSelected()) {
		if(myHoverPoint>-1){
			myHoverPoint=-1;
			update();
		}
	}
	QGraphicsPathItem::hoverLeaveEvent(e);
}
