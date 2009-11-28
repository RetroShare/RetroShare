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

#include "diagramtextitem.h"
#include "diagramscene.h"
#include <iostream>

//! [0]
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent, QGraphicsScene *scene)
    : QGraphicsTextItem(parent, scene)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    QObject::connect(this->document(), SIGNAL(contentsChanged()),
                this, SLOT(textChanged()));
}
//! [0]
DiagramTextItem::DiagramTextItem(const DiagramTextItem& textItem)
{
	//QGraphicsTextItem();
	setFont(textItem.font());
	setDefaultTextColor(textItem.defaultTextColor());
	setHtml(textItem.toHtml());
	setTransform(textItem.transform());
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    m_adapt=false;
}


//! [1]
QVariant DiagramTextItem::itemChange(GraphicsItemChange change,
                     const QVariant &value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged)
        emit selectedChange(this);
    if (change == QGraphicsItem::ItemPositionHasChanged)
    {
    	if(!m_adapt) {
    		qreal width=boundingRect().width();
    		qreal height=boundingRect().height();
    		mCenterPoint=mapToParent(mapFromParent(scenePos())+QPointF(width/2,height/2));
    		m_adapt=true;
    		prepareGeometryChange();
    		return mapToParent(mapFromParent(mCenterPoint)+QPointF(-width/2,-height/2));
    	}
    	m_adapt=false;
    	return value.toPointF();
    }
    return value;
}
//! [1]

//! [2]
void DiagramTextItem::focusOutEvent(QFocusEvent *event)
{
    setTextInteractionFlags(Qt::NoTextInteraction);
    emit lostFocus(this);
    QGraphicsTextItem::focusOutEvent(event);
}
void DiagramTextItem::focusInEvent(QFocusEvent *event)
{
    //emit receivedFocus(this);
    QGraphicsTextItem::focusInEvent(event);
}
//! [2]

//! [5]
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (textInteractionFlags() == Qt::NoTextInteraction)
        setTextInteractionFlags(Qt::TextEditorInteraction);
    emit receivedFocus(this);
    QGraphicsTextItem::mouseDoubleClickEvent(event);
}
//! [5]
DiagramTextItem* DiagramTextItem::copy()
{
	DiagramTextItem* newTextItem=new DiagramTextItem(*this);
	return newTextItem;
}

void DiagramTextItem::textChanged()
{
	qreal width=boundingRect().width();
	qreal height=boundingRect().height();
	m_adapt=true;
	prepareGeometryChange();
	setPos(mapToParent(mapFromParent(mCenterPoint)+QPointF(-width/2,-height/2)));
}

void DiagramTextItem::setCenterPoint(const QPointF point)
{
	mCenterPoint=point;
	textChanged();
}

void DiagramTextItem::activateEditor()
{
	if (textInteractionFlags() == Qt::NoTextInteraction)
	        setTextInteractionFlags(Qt::TextEditorInteraction);
	QGraphicsSceneMouseEvent *event=new QGraphicsSceneMouseEvent();
	event->setPos(QPointF(0,0));
	QGraphicsTextItem::mousePressEvent(event);
	delete event;
}
