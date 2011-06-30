/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DragCardStack.h"
#include "CardStack.h"

#include <QtGui/QCursor>
#include <QtGui/QGraphicsScene>
#include <QtGui/QPixmap>
#include <QtGui/QGraphicsSceneMouseEvent>

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
DragCardStack::DragCardStack(CardStack * pSrc)
    :QObject(),QGraphicsPixmapItem(),
     m_cardVector(),
     m_pSrc(pSrc),
     m_size(0,0),
     m_cursorPt(0,0),
     m_dragStarted(false),
     m_pDst(NULL)
{
    this->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    this->setCursor(Qt::ClosedHandCursor);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
DragCardStack::~DragCardStack()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void DragCardStack::startCardMove(const QPointF & globalMousePt,
				  unsigned int startCardIndex,
				  const PlayingCardVector & moveCards)
{
    if (startCardIndex<m_pSrc->getCardVector().size() &&
	moveCards.size()>0)
    {
	this->m_cardVector.clear();
	this->m_cardVector=moveCards;

	QPixmap * pPixmap=m_pSrc->getStackPixmap(this->m_cardVector);

	if (NULL!=pPixmap)
	{
	    this->setPixmap(*pPixmap);
	    m_size=pPixmap->size();
	    delete pPixmap;
	}
	
	// set the z value to 2 so it will be on top of the
	// stacks.
	this->setZValue(2);


	QPointF startPt=m_pSrc->getGlobalCardPt(startCardIndex);

	// this gets the relative position of the mouse on the new object.
	m_cursorPt.setX(globalMousePt.x()-startPt.x());
	m_cursorPt.setY(globalMousePt.y()-startPt.y());

	// add the item to the scene and move it over the stack in the
	// place of the cards that we want to move
	m_pSrc->scene()->addItem(this);
	this->setPos(startPt);

	// remove the cards from the stack and repaint
	unsigned int i;
	
	for(i=m_pSrc->getCardVector().size()-1;i>=startCardIndex && !m_pSrc->isEmpty();i--)
	{
	    m_pSrc->removeTopCard();
	}
	m_pSrc->updateStack();
	m_pSrc->setCursor(Qt::ClosedHandCursor);


	m_dragStarted=true;

	m_pDst=NULL;
    } 
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void DragCardStack::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if (m_dragStarted)
    {
	m_dragStarted=false;
	m_pSrc->scene()->removeItem(this);

	if (NULL!=m_pDst && 
	    this->m_pDst->canAddCards(this->m_cardVector))
	{
	    CardMoveRecord moveRecord;

	    moveRecord.push_back(CardMoveRecordItem(this->m_pSrc->stackName(),
						    CardMoveRecordItem::RemoveCards,
						    this->m_cardVector));

	    m_pDst->addCards(this->m_cardVector,moveRecord);
	    m_pDst->setHighlighted(false);
	    m_pDst->updateStack();
	    
	    emit cardsMoved(moveRecord);

	    m_pDst=NULL;
	}
	else
	{
	    m_pSrc->addCards(this->m_cardVector);
	    m_pSrc->updateStack();
	}
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void DragCardStack::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragStarted)
    {
	// get the topleft from the mouse pos and then adjust for where the mouse
	// is positioned on the card.
	QPointF topLeft(event->scenePos());
	topLeft.rx()-=m_cursorPt.x();
	topLeft.ry()-=m_cursorPt.y();
	
	// move the card to it's new place.
	this->setPos(topLeft);

	// calc our bounding rect.
	QRectF boundingRect(topLeft,QSizeF(m_size.width(),m_size.height()));
	
	// now find which items this item intersects with.
	QList<QGraphicsItem *> intersectItems=this->scene()->items(boundingRect);

	CardStack * pNewDst=NULL;

	qreal largestArea=0;

	for(int i=0;i<intersectItems.size();i++)
	{
	    if (intersectItems[i]!=this && 
		CardStack::isCardStack(intersectItems[i]))
	    {
		CardStack * pCurrDst=(CardStack *)intersectItems[i];

		QRectF boundingCurr=pCurrDst->sceneBoundingRect();

		if (boundingCurr.contains(event->scenePos()))
		{
		    pNewDst=pCurrDst;
		    break;
		}
		else
		{
		    QRectF commonRect(boundingRect.intersected(boundingCurr));
		    
		    qreal currArea=commonRect.width()*commonRect.height();
		    
		    if (currArea>largestArea)
		    {
			largestArea=currArea;
			pNewDst=pCurrDst;
		    }
		}
	    }
	}

	if (pNewDst!=this->m_pDst)
	{
	    if (NULL!=this->m_pDst)
	    {
		this->m_pDst->setHighlighted(false);
		this->m_pDst->updateStack();
	    }
	    this->m_pDst=pNewDst;

	    if (NULL!=this->m_pDst && 
		this->m_pDst->canAddCards(this->m_cardVector))
	    {
		this->m_pDst->setHighlighted(true);
		this->m_pDst->updateStack();
	    }
	}
    }
}
