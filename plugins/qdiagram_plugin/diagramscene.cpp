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

#include "diagramscene.h"

#define NDEBUG

//! [0]
DiagramScene::DiagramScene(QMenu *itemMenu, QObject *parent)
    : QGraphicsScene(parent)
{
    myItemMenu = itemMenu;
    myMode = MoveItem;
    myItemType = DiagramItem::Step;
    line = 0;
    textItem = 0;
    insertedItem = 0;
    insertedDrawItem = 0;
    insertedPathItem = 0;
    copiedItems = 0;
    myDx=0.0;
    myDy=0.0;
    maxZ=0;
    myItemColor = Qt::white;
    myTextColor = Qt::black;
    myLineColor = Qt::black;
    myArrow=DiagramPathItem::Path;
    myGrid=10.0;
    myGridVisible=false;
    myGridScale=1;
    // no Item in Moveitems
    myMoveItems.clear();
    // initialisiere Cursor
    myCursorWidth = 4.0;
    myCursor.setRect(QRectF(-myCursorWidth/2,-myCursorWidth/2,myCursorWidth,myCursorWidth));
    myCursor.setPen(QPen(Qt::gray));
    myCursor.setZValue(10.0);
    addItem(&myCursor);
}
//! [0]

//! [1]
void DiagramScene::setLineColor(const QColor &color)
{
    myLineColor = color;
    if(insertedPathItem!=0) insertedPathItem->setPen(myLineColor);
    if(insertedItem!=0) insertedItem->setPen(myLineColor);
    if(insertedDrawItem!=0) insertedDrawItem->setPen(myLineColor);
    if (!selectedItems().empty()){
    	foreach(QGraphicsItem* item,selectedItems()){
    		switch(item->type()){
    		case QGraphicsItem::UserType+3:
				// Textitem does not possess Linecolor !
				break;
    		case QGraphicsItem::UserType+6:
				qgraphicsitem_cast<DiagramPathItem*>(item)->setPen(myLineColor);
				qgraphicsitem_cast<DiagramPathItem*>(item)->setBrush(myLineColor);
				break;
    		default:
    			dynamic_cast<DiagramItem*>(item)->setPen(myLineColor);
    			break;
    		}
    	}
    }
}
//! [1]

//! [2]
void DiagramScene::setTextColor(const QColor &color)
{
    myTextColor = color;
    if (isItemChange(DiagramTextItem::Type)) {
        DiagramTextItem *item =
            qgraphicsitem_cast<DiagramTextItem *>(selectedItems().first());
        item->setDefaultTextColor(myTextColor);
    }
}
//! [2]

//! [3]
void DiagramScene::setItemColor(const QColor &color)
{
    myItemColor = color;
    if(insertedItem!=0) insertedItem->setBrush(myItemColor);
    if(insertedDrawItem!=0) insertedDrawItem->setBrush(myItemColor);
    if (!selectedItems().empty()){
    	foreach(QGraphicsItem* item,selectedItems()){
    		switch(item->type()){
    		case QGraphicsItem::UserType+3:
    		// Textitem does not possess Linecolor !
    		break;
    		case QGraphicsItem::UserType+6:
    		// Path does not need Backgroundcolor
    		break;
    		default:
    			dynamic_cast<DiagramItem*>(item)->setBrush(myItemColor);
    			break;
    		}
    	}
    }
       // old code by Trolltech
    /*if (isItemChange(DiagramItem::Type)) {
        DiagramItem *item =
            qgraphicsitem_cast<DiagramItem *>(selectedItems().first());
        item->setBrush(myItemColor);
    }*/
}
//! [3]

//! [4]
void DiagramScene::setFont(const QFont &font)
{
    myFont = font;

    if (isItemChange(DiagramTextItem::Type)) {
        QGraphicsTextItem *item =
            qgraphicsitem_cast<DiagramTextItem *>(selectedItems().first());
        item->setFont(myFont);
        // testing
        /*
        item->setTextWidth(200.0);
        QTextCursor  cursor = item->textCursor();
        cursor.select(QTextCursor::Document);
          QTextBlockFormat bfmt = cursor.blockFormat();
          bfmt.setAlignment(Qt::AlignHCenter);
          cursor.setBlockFormat(bfmt);
          item->setTextCursor(cursor);
          */
    }
}
//! [4]

void DiagramScene::setMode(Mode mode,bool m_abort)
{
	if(m_abort) abort(true);
	/*
	if(myMode==InsertLine){
		if(insertedPathItem!=0){
			insertedPathItem->remove();
			insertedPathItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
			insertedPathItem=0;
		}
	}
	*/
    myMode = mode;
    switch (mode) {
		case MoveItem:
			enableAllItems(true);
			break;
		case MoveItems:
			enableAllItems(true);
			break;
		case CopyItem:
			enableAllItems(true);
			break;
		case CopyingItem:
			enableAllItems(false);
			break;
		case Zoom:
			enableAllItems(false);
			break;
		default:
			enableAllItems(false);
			break;
	}
}

void DiagramScene::enableAllItems(bool enable)
{
	foreach(QGraphicsItem* item,items()){
		item->setEnabled(enable);
	}
}

/*enum DiagramScene::getMode()
{
    return mode;
}*/

void DiagramScene::setItemType(DiagramItem::DiagramType type)
{
    myItemType = type;
}
void DiagramScene::setItemType(DiagramDrawItem::DiagramType type)
{
    myDrawItemType = type;
}


//! [5]
void DiagramScene::editorLostFocus(DiagramTextItem *item)
{
	//Debug
#ifdef DEBUG
	std::cout << "textItem:" << int(textItem) << std::endl;
	if(textItem){
		std::cout << "Focus ?:" << textItem->hasFocus() << std::endl;
	}
#endif
    QTextCursor cursor = item->textCursor();
    cursor.clearSelection();
    item->setTextCursor(cursor);

#ifdef DEBUG
    std::cout << item->toPlainText().toStdString() << std::endl;
#endif

    if (item->toPlainText().isEmpty()) {
        removeItem(item);
        item->deleteLater();
    }
    // avoid double lose focus, which erroneously activates shortcuts for second textitem
    if((textItem==item)or(textItem==0)){
    	textItem=0;
    	emit editorHasLostFocus();
    }

}
void DiagramScene::editorReceivedFocus(DiagramTextItem *item)
{
    emit editorHasReceivedFocus();
}
//! [5]

//! [6]
void DiagramScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
#ifdef DEBUG
	std::cout << "click" << myMode << std::endl;
#endif
	if (mouseEvent->button() == Qt::RightButton){
		switch (myMode) {
			case InsertItem:
				if (insertedItem != 0){
					insertedItem->rotate(90);
				}
				break;
			default:
				;
		}
	    return;
	}

	if (mouseEvent->button() == Qt::MidButton){
			switch (myMode) {
				case InsertLine:
					if (insertedPathItem != 0){
						insertedPathItem->updateLast(onGrid(mouseEvent->scenePos()));
						insertedPathItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
						insertedPathItem->setEnabled(false);
						insertedPathItem = 0;
					}
					break;
				default:
					;
			}
		    return;
		}

    if (mouseEvent->button() != Qt::LeftButton)
        return;

    //DiagramItem *item;
    switch (myMode) {
        case InsertItem:
        	if (insertedItem == 0){
				insertedItem = new DiagramItem(myItemType, myItemMenu);
				insertedItem->setBrush(myItemColor);
				insertedItem->setPen(myLineColor);
				insertedItem->setZValue(maxZ);
				maxZ+=0.1;
				addItem(insertedItem);
            }
            insertedItem->setPos(onGrid(mouseEvent->scenePos()));
            insertedItem->setEnabled(false);
            emit itemInserted(insertedItem);
            insertedItem = 0;

            break;
//! [6] //! [7]
        case InsertDrawItem:
        	if (insertedDrawItem == 0){
        		insertedDrawItem = new DiagramDrawItem(myDrawItemType, myItemMenu);
        		insertedDrawItem->setBrush(myItemColor);
        		insertedDrawItem->setPen(myLineColor);
        		insertedDrawItem->setZValue(maxZ);
        		maxZ+=0.1;
        		addItem(insertedDrawItem);
        		insertedDrawItem->setPos(onGrid(mouseEvent->scenePos()));
        	}
        	else
        	{
        		insertedDrawItem->setPos2(onGrid(mouseEvent->scenePos()));
        		insertedDrawItem->setEnabled(false);
        		insertedDrawItem = 0;
        	}

        	break;
        case MoveItems:
        {
        	QPointF point=onGrid(mouseEvent->scenePos());
        	if(!myMoveItems.isEmpty()){
        		qreal dx=point.rx()-myDx;
        		qreal dy=point.ry()-myDy;
        		foreach(QGraphicsItem* item,myMoveItems){
        			if(item->parentItem()!=0){
        				if(!item->parentItem()->isSelected()) item->moveBy(-dx,-dy);
        			}
        			else {
        				item->moveBy(dx,dy);
        			}
        		}
        		myMoveItems.clear();
        		myMode=MoveItem;
        	}
        	else
        	{
        		if(!selectedItems().isEmpty()){
        			// lösche doppelte Verweise (Child&selected)
        			myMoveItems=selectedItems();
        			foreach(QGraphicsItem* item,myMoveItems){
        				if(item->parentItem())
        					if(item->parentItem()->isSelected()) {
        						item->setSelected(false);
        						myMoveItems.removeOne(item);
        					}
        			}
        			// speichere Referenzpunkt
        			myDx=point.rx();
        			myDy=point.ry();
        		}
        	}
        	break;
        }
        case InsertLine:
        	if (insertedPathItem == 0){
        		insertedPathItem = new DiagramPathItem(myArrow,myItemMenu);
        		insertedPathItem->setPen(myLineColor);
        		insertedPathItem->setBrush(myLineColor);
        		insertedPathItem->setZValue(maxZ);
        		maxZ+=0.1;
        		addItem(insertedPathItem);
        		insertedPathItem->setPos(onGrid(mouseEvent->scenePos()));
        		//insertedPathItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        	}
        	insertedPathItem->append(onGrid(mouseEvent->scenePos()));
            break;
//! [7] //! [8]
        case InsertText:
        	emit editorHasReceivedFocus();
            textItem = new DiagramTextItem();
            textItem->setFont(myFont);
            textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
            textItem->setZValue(maxZ);
            maxZ+=0.1;
            connect(textItem, SIGNAL(lostFocus(DiagramTextItem *)),
                    this, SLOT(editorLostFocus(DiagramTextItem *)));
            connect(textItem, SIGNAL(receivedFocus(DiagramTextItem *)),
                                this, SLOT(editorReceivedFocus(DiagramTextItem *)));
            connect(textItem, SIGNAL(selectedChange(QGraphicsItem *)),
                    this, SIGNAL(itemSelected(QGraphicsItem *)));
            addItem(textItem);
            textItem->setDefaultTextColor(myTextColor);
            textItem->setFocus();
            textItem->setCenterPoint(onGrid(mouseEvent->scenePos()));
            textItem->activateEditor();
            emit textInserted(textItem);
            mouseEvent->setAccepted(true);
            break;
//! [8] //! [9]
        case CopyItem:
				if (!selectedItems().empty()){
						copiedItems=new QList<QGraphicsItem*>;
						copiedItems->clear();
						// lösche doppelte Verweise (Child&selected)
						QList<QGraphicsItem*> myList=selectedItems();
						foreach(QGraphicsItem* item,myList){
							if(item->parentItem())
								if(item->parentItem()->isSelected()) {
									item->setSelected(false);
									myList.removeOne(item);
								}
						}
						// Vorbereitung copy
						QGraphicsItem *insItem;
						insItem=myList.first();
						QPointF point=onGrid(mouseEvent->scenePos());
						myDx=insItem->pos().rx()-point.rx();
						myDy=insItem->pos().ry()-point.ry();
						// copy
						foreach(QGraphicsItem* item,myList){
							insItem=copy(item);
							addItem(insItem);
							insItem->setPos(item->pos());
							copiedItems->append(item);
							item->setZValue(maxZ);
							maxZ+=0.1;
							//check for children
							if(item->childItems().count()>0){
								foreach(QGraphicsItem* item_l1,item->childItems()){
									QGraphicsItem* addedItem=copy(item_l1);
									addItem(addedItem);
									addedItem->setParentItem(insItem);
									addedItem->setPos(item_l1->pos());
								}
							}
							//move original to knew position
							item->setSelected(true);
#ifdef DEBUG
							std::cout<< item->pos().rx()<< ","<<item->pos().ry() << std::endl;
#endif
						}
						myMode=CopyingItem;
				}
				break;
        case CopyingItem:
				if (copiedItems->count() > 0){
					insertedItem=static_cast<DiagramItem*>(copiedItems->first());
					QPointF point=onGrid(mouseEvent->scenePos());
					qreal dx=insertedItem->pos().rx()-point.rx()-myDx;
					qreal dy=insertedItem->pos().ry()-point.ry()-myDy;
					foreach(QGraphicsItem* item,*copiedItems){
						if(item->parentItem()!=0){
							if(!item->parentItem()->isSelected()) item->moveBy(-dx,-dy);
						}
						else {
							item->moveBy(-dx,-dy);
						}
					}
					clearSelection();
					insertedItem=0;
					myDx=0.0;
					myDy=0.0;
					myMode=MoveItem;
				}
				break;
    default:
    	;
    }
if(!mouseEvent->isAccepted()) QGraphicsScene::mousePressEvent(mouseEvent);
}
//! [9]

//! [10]
void DiagramScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    // move cursor
	myCursor.setPos(onGrid(mouseEvent->scenePos()));

	// preview der Zeichnung
    switch (myMode){
    case InsertLine:
    	if (insertedPathItem != 0) {
    	        insertedPathItem->updateLast(onGrid(mouseEvent->scenePos()));
    	}
        break;
    case MoveItem:
    	QGraphicsScene::mouseMoveEvent(mouseEvent);
    	checkOnGrid();
    	break;
    case MoveItems:
    {
    	QPointF point=onGrid(mouseEvent->scenePos());
    	qreal dx=point.rx()-myDx;
    	qreal dy=point.ry()-myDy;
    	foreach(QGraphicsItem* item,myMoveItems){
    		if(item->parentItem()!=0){
    			if(!item->parentItem()->isSelected()) item->moveBy(dx,dy);
    		}
    		else {
    			item->moveBy(dx,dy);
    		}
    	}
    	myDx=point.rx();
    	myDy=point.ry();
    	break;
    }
    case InsertItem:
    	if (insertedItem == 0){
			insertedItem = new DiagramItem(myItemType, myItemMenu);
			insertedItem->setBrush(myItemColor);
			insertedItem->setPen(myLineColor);
			insertedItem->setSelected(true);
			insertedItem->setZValue(maxZ);
			maxZ+=0.1;
			addItem(insertedItem);
		}
    	insertedItem->setPos(onGrid(mouseEvent->scenePos()));
		break;
    case InsertDrawItem:
		if (insertedDrawItem != 0){
			insertedDrawItem->setPos2(onGrid(mouseEvent->scenePos()));
		}
		break;
    case CopyingItem:
    	if (copiedItems->count() > 0){
        	//copiedItems->setPos(onGrid(mouseEvent->scenePos()));
    		insertedItem=static_cast<DiagramItem*>(copiedItems->first());
    		QPointF point=onGrid(mouseEvent->scenePos());
			qreal dx=insertedItem->pos().rx()-point.rx()-myDx;
			qreal dy=insertedItem->pos().ry()-point.ry()-myDy;
			foreach(QGraphicsItem* item,*copiedItems){
				if(item->parentItem()!=0){
					if(!item->parentItem()->isSelected()) item->moveBy(-dx,-dy);
				}
				else {
					item->moveBy(-dx,-dy);
				}
			}
		}
		break;

    default:
    	;
    }
}
//! [10]

//! [11]
void DiagramScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (myMode == Zoom) {
		emit zoomRect(mouseEvent->scenePos(),mouseEvent->lastScenePos());
		return;
	}
//! [12] //! [13]
    line = 0;
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}
//! [13]
void DiagramScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
#ifdef DEBUG
	std::cout << "d click" << myMode << std::endl;
#endif
	switch (myMode){
	case InsertLine:
		//insertedPathItem->updateLast(onGrid(mouseEvent->scenePos()));
		insertedPathItem->remove();
		insertedPathItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
		insertedPathItem->setEnabled(false);
		insertedPathItem=0;
		break;
	default:
		if(!selectedItems().isEmpty()){
			QGraphicsItem *item = selectedItems().first();//itemAt(mouseEvent->scenePos());
			if(item){
				QList<QGraphicsItem *> children = item->childItems();
				if (children.count()>0){
					if (QGraphicsTextItem *child = (QGraphicsTextItem*) item->childItems().first()) {
#ifdef DEBUG
						std::cout << "edit:" << child->toPlainText().toStdString() << std::endl;
#endif
						if (child->textInteractionFlags() == Qt::NoTextInteraction)
							child->setTextInteractionFlags(Qt::TextEditorInteraction);
						emit editorHasReceivedFocus();
						child->setFocus();
					}
				}
				else {
					if(item->type()!=DiagramTextItem::Type){
						// added
						// text wird per Doppelclick eingefuegt
						textItem = new DiagramTextItem();
						textItem->setFont(myFont);
						textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
						textItem->setZValue(1000.0);
						connect(textItem, SIGNAL(lostFocus(DiagramTextItem *)),
								this, SLOT(editorLostFocus(DiagramTextItem *)));
						connect(textItem, SIGNAL(receivedFocus(DiagramTextItem *)),
								this, SLOT(editorReceivedFocus(DiagramTextItem *)));
						connect(textItem, SIGNAL(selectedChange(QGraphicsItem *)),
								this, SIGNAL(itemSelected(QGraphicsItem *)));
						addItem(textItem);
						textItem->setParentItem(item);
						textItem->setDefaultTextColor(myTextColor);
						QPointF mPos=QPointF(0,0);
						// mPos adaptieren je nach DiaType ...
						switch (item->type()) {
							case DiagramDrawItem::Type:
								mPos+=dynamic_cast<DiagramDrawItem*>(item)->getDimension()/2;
								break;
							default:
								break;
						}
						//textItem->setPos(mPos);
						textItem->setCenterPoint(mPos);
						emit editorHasReceivedFocus();
						emit textInserted(textItem);
						textItem->setFocus(Qt::OtherFocusReason);
					}
					else
						QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
				}
			}
		}
	}
}

void DiagramScene::wheelEvent(QGraphicsSceneWheelEvent *mouseEvent)
{
	if(mouseEvent->modifiers()==Qt::ControlModifier){
		int i =  mouseEvent->delta();
		qreal factor;
		if(i>=0){
			factor = i/100.0;
		}
		else {
			factor = -100.0/i; // negative Richtung ...
		}
		emit zoom(factor);
		mouseEvent->setAccepted(true);
		return;
	}
    QGraphicsScene::wheelEvent(mouseEvent);
}


//! [14]
void setArrow(const int i)
{
}

bool DiagramScene::isItemChange(int type)
{
    foreach (QGraphicsItem *item, selectedItems()) {
        if (item->type() == type)
            return true;
    }
    return false;
}
//! [14]
void DiagramScene::checkOnGrid()
{
    foreach (QGraphicsItem *item, selectedItems()) {

    	if(item->type()==DiagramTextItem::Type){
    		// not item position but center position needs to be on grid
    		QPointF centerPoint=dynamic_cast<DiagramTextItem*>(item)->centerPoint();
    		std::cout << "rec:" << centerPoint.x() << "/" << centerPoint.y() << std::endl;
    		qreal x = qRound(centerPoint.x()/10)*10.0;
    		qreal y = qRound(centerPoint.y()/10)*10.0;
    		dynamic_cast<DiagramTextItem*>(item)->setCenterPoint(QPointF(x,y));
    	}
    	else
    	{
    		qreal x = qRound(item->x()/10)*10.0;
    		qreal y = qRound(item->y()/10)*10.0;
    		item->setPos(x,y);
    	}
    }
}

QPointF DiagramScene::onGrid(QPointF pos)
{
	qreal x = qRound(pos.x()/myGrid)*myGrid;
	qreal y = qRound(pos.y()/myGrid)*myGrid;
	QPointF result = QPointF(x,y);
	return result;
}

void DiagramScene::abort(bool keepSelection)
{
	switch(myMode){
	case CopyingItem:
		//removeItem(copiedItems);
		//destroyItemGroup(copiedItems);
		//removeItem(insertedItem);
		break;
	case InsertItem:
		removeItem(insertedItem);
		break;
	case InsertDrawItem:
		removeItem(insertedDrawItem);
		break;
	case InsertLine:
		removeItem(insertedPathItem);
		break;
	default:
		;
	}

    insertedItem=0;
    insertedDrawItem=0;
    copiedItems=0;
    myMode=MoveItem;
    if(!keepSelection) clearSelection();
}

QGraphicsItem* DiagramScene::copy(QGraphicsItem* item)
{
#ifdef DEBUG
	std::cout<<"copy: " << item->type()<<std::endl;
#endif
	switch(item->type()){
	case QGraphicsItem::UserType+3:
		return qgraphicsitem_cast<QGraphicsItem*>(qgraphicsitem_cast<DiagramTextItem*>(item)->copy());
		break;
	case QGraphicsItem::UserType+6:
			return qgraphicsitem_cast<QGraphicsItem*>(qgraphicsitem_cast<DiagramPathItem*>(item)->copy());
			break;
	default:
		{
			DiagramItem* newItem=dynamic_cast<DiagramItem*>(item)->copy();
			return dynamic_cast<QGraphicsItem*>(newItem);
		}
		break;
	}
}

void DiagramScene::setArrow(const int i)
{
	myArrow=DiagramPathItem::DiagramType(i);
	if(insertedPathItem!=0){
		insertedPathItem->setDiagramType(myArrow);
	}
    if (!selectedItems().empty()){
    	foreach(QGraphicsItem* item,selectedItems()){
    		switch(item->type()){
    		case QGraphicsItem::UserType+3:
				// Textitem does not possess Linecolor !
				break;
    		case QGraphicsItem::UserType+6:
				qgraphicsitem_cast<DiagramPathItem*>(item)->setDiagramType(myArrow);
				break;
    		default:
    			// nothing to do
    			break;
    		}
    	}
    }


}

bool DiagramScene::event(QEvent *mEvent)
{
	if (mEvent->type()==QEvent::Enter) {
		myCursor.setVisible(true);
		return true;
	}
	if (mEvent->type()==QEvent::Leave) {
		myCursor.setVisible(false);
		return true;
	}
    return QGraphicsScene::event(mEvent);
}

bool DiagramScene::save(QFile *file)
{
#ifdef DEBUG
	std::cout << "save..."  << std::endl;
#endif
	QXmlStreamWriter xmlWriter(file);
	xmlWriter.setAutoFormatting(true);
	xmlWriter.writeStartDocument();
	xmlWriter.writeComment("File for QDiagram");
	xmlWriter.writeStartElement("doc");
	foreach(QGraphicsItem* item, items()){
		if(item->type()>QGraphicsItem::UserType){
			xmlWriter.writeStartElement("Item");
			xmlWriter.writeAttribute("Type",QString::number(item->type()));
			xmlWriter.writeEmptyElement("Pos");
			xmlWriter.writeAttribute("x",QString::number(item->pos().x()));
			xmlWriter.writeAttribute("y",QString::number(item->pos().y()));
			xmlWriter.writeAttribute("z",QString::number(item->zValue()));
			switch (item->type()) {
				case QGraphicsItem::UserType+16:
					{
						DiagramDrawItem *mItem = dynamic_cast<DiagramDrawItem *>(item);
						//xmlWriter.writeComment("DiagramDrawItem");
						xmlWriter.writeEmptyElement("DiagramType");
						xmlWriter.writeAttribute("type",QString::number(mItem->diagramType()));
						xmlWriter.writeEmptyElement("Dimensions");
						xmlWriter.writeAttribute("width",QString::number(mItem->getDimension().x()));
						xmlWriter.writeAttribute("height",QString::number(mItem->getDimension().y()));
						xmlWriter.writeEmptyElement("Pen");
						xmlWriter.writeAttribute("color",mItem->pen().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->pen().color().alpha()));
						xmlWriter.writeEmptyElement("Brush");
						xmlWriter.writeAttribute("color",mItem->brush().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->brush().color().alpha()));
					}
					break;
				case QGraphicsItem::UserType+15:
					{
						DiagramItem *mItem = dynamic_cast<DiagramItem *>(item);
						//xmlWriter.writeComment("DiagramItem");
						xmlWriter.writeEmptyElement("DiagramType");
						xmlWriter.writeAttribute("type",QString::number(mItem->diagramType()));
						xmlWriter.writeEmptyElement("Pen");
						xmlWriter.writeAttribute("color",mItem->pen().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->pen().color().alpha()));
						xmlWriter.writeEmptyElement("Brush");
						xmlWriter.writeAttribute("color",mItem->brush().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->brush().color().alpha()));
					}
					break;
				case QGraphicsItem::UserType+3:
					{
						DiagramTextItem *mItem = dynamic_cast<DiagramTextItem *>(item);
						//xmlWriter.writeComment("DiagramTextItem");
						xmlWriter.writeEmptyElement("DiagramType");
						xmlWriter.writeAttribute("type","0");
						xmlWriter.writeStartElement("Text");
						xmlWriter.writeCharacters(mItem->toHtml());
						xmlWriter.writeEndElement();
					}
					break;
				case QGraphicsItem::UserType+6:
					{
						DiagramPathItem *mItem = dynamic_cast<DiagramPathItem *>(item);
						//xmlWriter.writeComment("DiagramPathItem");
						xmlWriter.writeEmptyElement("DiagramType");
						xmlWriter.writeAttribute("type",QString::number(mItem->diagramType()));
						xmlWriter.writeEmptyElement("Pen");
						xmlWriter.writeAttribute("color",mItem->pen().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->pen().color().alpha()));
						xmlWriter.writeEmptyElement("Brush");
						xmlWriter.writeAttribute("color",mItem->brush().color().name());
						xmlWriter.writeAttribute("alpha",QString::number(mItem->brush().color().alpha()));
						foreach(QPointF point, mItem->getPoints()){
							xmlWriter.writeEmptyElement("Point");
							xmlWriter.writeAttribute("x",QString::number(point.x()));
							xmlWriter.writeAttribute("y",QString::number(point.y()));
						}
					}
					break;
				default:
					break;
			}
			xmlWriter.writeEmptyElement("Transform");
			xmlWriter.writeAttribute("m11",QString::number(item->transform().m11()));
			xmlWriter.writeAttribute("m12",QString::number(item->transform().m12()));
			xmlWriter.writeAttribute("m21",QString::number(item->transform().m21()));
			xmlWriter.writeAttribute("m22",QString::number(item->transform().m22()));
			xmlWriter.writeAttribute("dx",QString::number(item->transform().dx()));
			xmlWriter.writeAttribute("dy",QString::number(item->transform().dy()));
			xmlWriter.writeEndElement();
		}

	}
	xmlWriter.writeEndElement();
	xmlWriter.writeEndDocument();
	return true;
}
bool DiagramScene::load(QFile *file)
{
#ifdef DEBUG
	std::cout << "Load..." << std::endl;
#endif
	QXmlStreamReader xmlReader(file);
	int DiaType = 0;
	QPointF mPos;
	qreal z;
	int type;
	insertedItem=0;
	insertedPathItem=0;
	insertedDrawItem=0;
	textItem=0;
	while(!xmlReader.atEnd()){
		xmlReader.readNext();
#ifdef DEBUG
		std::cout << "name: " << DiaType << ":" << qPrintable(xmlReader.name().toString()) << std::endl;
#endif
		if(xmlReader.isStartDocument()) continue;
		if(xmlReader.isStartElement()){
			if(xmlReader.name()=="doc") continue;
			if(!DiaType and (xmlReader.name()=="Item")){
				bool ok;
				DiaType = xmlReader.attributes().value("Type").toString().toInt(&ok);
				if(!ok)
				{
					xmlReader.raiseError(tr("Item: type number conversion failed"));
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Pos")){
				bool ok,okay;
				okay=true;
				mPos.setX(xmlReader.attributes().value("x").toString().toDouble(&ok));
				okay&=ok;
				mPos.setY(xmlReader.attributes().value("y").toString().toDouble(&ok));
				okay&=ok;
				z=xmlReader.attributes().value("z").toString().toDouble(&ok);
				okay&=ok;
				if(!okay)
				{
					xmlReader.raiseError(tr("Pos: number conversion failed"));
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="DiagramType")){
				bool ok;
				type = xmlReader.attributes().value("type").toString().toInt(&ok);
				if(!ok)
				{
					xmlReader.raiseError(tr("DiagramType: type number conversion failed"));
					continue;
				}
				switch (DiaType) {
					case QGraphicsItem::UserType+15:
						insertedItem = new DiagramItem(DiagramItem::DiagramType(type),myItemMenu);
						addItem(insertedItem);
#ifdef DEBUG
						std::cout << "DiagramItem" << std::endl;
#endif
						insertedItem->setPos(mPos);
						insertedItem->setZValue(z);
						break;
					case QGraphicsItem::UserType+16:
						insertedDrawItem = new DiagramDrawItem(DiagramDrawItem::DiagramType(type),myItemMenu);
						addItem(insertedDrawItem);
#ifdef DEBUG
						std::cout << "DiagramDrawItem" << std::endl;
#endif
						insertedDrawItem->setPos(mPos);
						insertedDrawItem->setZValue(z);
						break;
					case QGraphicsItem::UserType+6:
						insertedPathItem = new DiagramPathItem(DiagramPathItem::DiagramType(type),myItemMenu);
						addItem(insertedPathItem);
#ifdef DEBUG
						std::cout << "DiagramPathItem" << std::endl;
#endif
						insertedPathItem->setPos(mPos);
						insertedPathItem->setZValue(z);
						insertedPathItem->setFlag(QGraphicsItem::ItemIsMovable, true);
						insertedPathItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
						break;
					case QGraphicsItem::UserType+3:
						textItem = new DiagramTextItem();
						textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
						textItem->setZValue(z);
						connect(textItem, SIGNAL(lostFocus(DiagramTextItem *)),
								this, SLOT(editorLostFocus(DiagramTextItem *)));
						connect(textItem, SIGNAL(receivedFocus(DiagramTextItem *)),
								this, SLOT(editorReceivedFocus(DiagramTextItem *)));
						connect(textItem, SIGNAL(selectedChange(QGraphicsItem *)),
								this, SIGNAL(itemSelected(QGraphicsItem *)));
						addItem(textItem);
						textItem->setPos(mPos);
						break;
					default:
						break;
				}
				continue;
			}
			//DiamgramType nicht gesetzt ? -> Fehler
			if(DiaType and !(insertedItem or insertedDrawItem or textItem or insertedPathItem)){
				xmlReader.raiseError(tr("DiagramType definition missing"));
				continue;
			}
		    // weitere Properties setzen (Farbe,Punkte,etc.)
			if(DiaType and (xmlReader.name()=="Pen")){
				QColor color;
				color.setNamedColor(xmlReader.attributes().value("color").toString());
				if(!color.isValid()){
					xmlReader.raiseError(tr("DiagramType: type number conversion failed"));
					continue;
				}
				bool ok;
				color.setAlpha(xmlReader.attributes().value("alpha").toString().toUInt(&ok));
				if(!ok)
				{
					xmlReader.raiseError(tr("DiagramType: type number conversion failed"));
					continue;
				}
				switch (DiaType) {
				case QGraphicsItem::UserType+15:
					insertedItem->setPen(color);
					break;
				case QGraphicsItem::UserType+16:
					insertedDrawItem->setPen(color);
					break;
				case QGraphicsItem::UserType+6:
					insertedPathItem->setPen(color);
					break;
				default:
					break;
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Brush")){
				QColor color;
				bool ok;
				QString colorName = xmlReader.attributes().value("color").toString();
				color.setNamedColor(colorName);
				if(!color.isValid()){
					xmlReader.raiseError(tr("DiagramType: type number conversion failed"));
					continue;
				}
				color.setAlpha(xmlReader.attributes().value("alpha").toString().toUInt(&ok));
				if(!ok)
				{
					xmlReader.raiseError(tr("DiagramType: type number conversion failed"));
					continue;
				}
				switch (DiaType) {
				case QGraphicsItem::UserType+15:
					insertedItem->setBrush(color);
					break;
				case QGraphicsItem::UserType+16:
					insertedDrawItem->setBrush(color);
					break;
				case QGraphicsItem::UserType+6:
					insertedPathItem->setBrush(color);
					break;
				default:
					break;
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Dimensions")){
				QPointF mPos2;
				bool okay,ok;
				okay=true;
				mPos2.setX(xmlReader.attributes().value("width").toString().toDouble(&ok));
				okay&=ok;
				mPos2.setY(xmlReader.attributes().value("height").toString().toDouble(&ok));
				okay&=ok;
				if(!ok)
				{
					xmlReader.raiseError(tr("Dimensions: number conversion failed"));
					continue;
				}
				switch (DiaType) {
				case QGraphicsItem::UserType+16:
					insertedDrawItem->setDimension(mPos2);
				break;
				default:
					break;
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Point")){
				QPointF mPos2;
				bool okay,ok;
				okay=true;
				mPos2.setX(xmlReader.attributes().value("x").toString().toDouble(&ok));
				okay&=ok;
				mPos2.setY(xmlReader.attributes().value("y").toString().toDouble(&ok));
				okay&=ok;
				if(!ok)
				{
					xmlReader.raiseError(tr("Point: number conversion failed"));
					continue;
				}
				switch (DiaType) {
				case QGraphicsItem::UserType+6:
					insertedPathItem->append(mPos2+insertedPathItem->pos());
				break;
				default:
					break;
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Text")){
				QString mText;
				mText = xmlReader.readElementText();
				switch (DiaType) {
				case QGraphicsItem::UserType+3:
					textItem->setHtml(mText);
				break;
				default:
					break;
				}
				continue;
			}
			if(DiaType and (xmlReader.name()=="Transform")){
				bool ok,okay;
				okay=true;
				qreal m11=xmlReader.attributes().value("m11").toString().toDouble(&ok);
				okay&=ok;
				qreal m12=xmlReader.attributes().value("m12").toString().toDouble(&ok);
				okay&=ok;
				qreal m21=xmlReader.attributes().value("m21").toString().toDouble(&ok);
				okay&=ok;
				qreal m22=xmlReader.attributes().value("m22").toString().toDouble(&ok);
				okay&=ok;
				qreal dx=xmlReader.attributes().value("dx").toString().toDouble(&ok);
				okay&=ok;
				qreal dy=xmlReader.attributes().value("dx").toString().toDouble(&ok);
				okay&=ok;
				if(!ok)
				{
					xmlReader.raiseError(tr("Transform: number conversion failed"));
					continue;
				}
				QTransform trans=QTransform(m11,m12,m21,m22,dx,dy);
				switch (DiaType) {
				case QGraphicsItem::UserType+15:
					insertedItem->setTransform(trans);
					break;
				case QGraphicsItem::UserType+16:
					insertedDrawItem->setTransform(trans);
					break;
				case QGraphicsItem::UserType+6:
					insertedPathItem->setTransform(trans);
					break;
				case QGraphicsItem::UserType+3:
					textItem->setTransform(trans);
					break;
				default:
					break;
				}
				continue;
			}
			else {
				xmlReader.raiseError(tr("unexpected start element"));
				continue;
			}

		}
		if(xmlReader.isEndElement()){
			if(DiaType and (xmlReader.name()=="Item")){
				DiaType = 0;
				insertedItem = 0;
				insertedDrawItem = 0;
				insertedPathItem = 0;
				textItem = 0;
			}
		}

	}
	if(xmlReader.hasError()){
		std::cerr << "Error in XML Read-in: "
				  << qPrintable(xmlReader.errorString())
				  << std::endl
				  << "Line: "
				  << xmlReader.lineNumber()
				  << std::endl;
	}
	// Aufräumen
	insertedItem = 0;
	insertedDrawItem = 0;
	insertedPathItem = 0;
	textItem = 0;
	myMode = MoveItem;
	return true;
}

void DiagramScene::clear()
{
	foreach(QGraphicsItem *item,items()){
		if(item!=&myCursor)
		{
			removeItem(item);
			delete item;
		}
	}
}

void DiagramScene::setCursorVisible(bool vis)
{
	if(vis){
		if(myCursor.scene()==0){
			addItem(&myCursor);
		}
	}
	else
	{
		if(myCursor.scene()) removeItem(&myCursor);
	}
}

void DiagramScene::drawBackground(QPainter *p, const QRectF &r) {
	p -> save();

	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);

	// dessine un fond blanc
	p -> setPen(Qt::NoPen);
	p -> setBrush(Qt::white);
	p -> drawRect(r);

	if (myGridVisible) {
		// to ease transition from qelec
		int xGrid=myGridScale * (int) myGrid;
		int yGrid=myGridScale * (int) myGrid;
		// dessine les points de la grille
		p -> setPen(Qt::black);
		p -> setBrush(Qt::NoBrush);
		qreal limite_x = r.x() + r.width();
		qreal limite_y = r.y() + r.height();

		int g_x = (int)ceil(r.x());
		while (g_x % xGrid) ++ g_x;
		int g_y = (int)ceil(r.y());
		while (g_y % yGrid) ++ g_y;

		for (int gx = g_x ; gx < limite_x ; gx += xGrid) {
			for (int gy = g_y ; gy < limite_y ; gy += yGrid) {
				p -> drawPoint(gx, gy);
			}
		}
	}

	p -> restore();
}
