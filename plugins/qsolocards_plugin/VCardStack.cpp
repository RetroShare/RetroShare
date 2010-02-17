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

#include "VCardStack.h"
#include "CardPixmaps.h"
#include <QtGui/QPainter>
#include <QtGui/QGraphicsScene>

#include <iostream>

const qreal VCardStack::ExposedPrecentFaceUp=.23;
const qreal VCardStack::ExposedPrecentFaceDown=.10; 

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
VCardStack::VCardStack()
    :m_bRectVector(),
     m_compressValue(CompressNormal),
     m_percentScene(1)
{
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
VCardStack::~VCardStack()
{
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void VCardStack::setStackBottom(qreal percentScene)
{
    if(percentScene>=0 && percentScene<=1)
    {
	m_percentScene=percentScene;
    }
    else
    {
	m_percentScene=1;
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
QPointF VCardStack::getGlobalCardAddPt() const
{
    QPointF pt(0,0);

    const PlayingCardVector & cardVector=this->getCardVector();

    // We are going to do this by the bounding rects and not by actual cards
    // in the stacks.  That way we can add something before doing animations
    // and then update the display when the animation is complete
    if (m_bRectVector.size()>0 && cardVector.size()>=m_bRectVector.size())
    {
	pt=m_bRectVector[m_bRectVector.size()-1].topLeft();

	pt.ry()+=getOverlapIncrement(cardVector[m_bRectVector.size()-1],
				     m_compressValue);
    }

    return mapToScene(pt);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
QPointF VCardStack::getGlobalLastCardPt() const
{
    QPointF pt(0,0);

    const PlayingCardVector & cardVector=this->getCardVector();

    // We are going to do this by the bounding rects and not by actual cards
    // in the stacks.  That way we can add something before doing animations
    // and then update the display when the animation is complete
    if (m_bRectVector.size()>0 && cardVector.size()>=m_bRectVector.size())
    {
	pt=m_bRectVector[m_bRectVector.size()-1].topLeft();
    }

    return mapToScene(pt);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
QPointF VCardStack::getGlobalCardPt(int index) const
{
    QPointF pt(0,0);

    if (index>=0 && index<(int)m_bRectVector.size())
    {
	pt=m_bRectVector[index].topLeft();
    }

    return mapToScene(pt);    
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void VCardStack::updateStack()
{
    QSize size;
    const PlayingCardVector & cardVector=this->getCardVector();

    // the compress value decresses how much of cards are exposed.
    // The preferred exposed value is normal.  But we will decrease the
    // values down to a quarter of the desired to make the stack fit on
    // the screen.  This function uses the m_precentScene value to determine
    // the bottom point of the stack
    for(m_compressValue=CompressNormal;m_compressValue<CompressMax;m_compressValue+=.05)
    {
	calcPixmapSize(cardVector,size,m_compressValue);

	// see if the stack is going to fit on the screen
	if ((this->scene()->sceneRect().bottom()*m_percentScene)>mapToScene(QPointF(0,size.height())).y())
	{
	    break;
	}
    }

    QPixmap * pPixmap=NULL;

    // draw the card and calc the bounding rectangles while we are at it.
    if (this->isFlipAniRunning())
    {
	PlayingCardVector newCardVector(cardVector);

	// the stack should have at least one card ie the one being flipped
	// but make sure.
	if (cardVector.size()>0)
	{
	    newCardVector.pop_back();
	}

	// draw the card and calc the bounding rectangles while we are at it.
	pPixmap=getStackPixmap(newCardVector,isHighlighted(),
			       hintHighlightIndex(),m_compressValue,
			       &m_bRectVector);
    }
    else
    {
	// draw the card and calc the bounding rectangles while we are at it.
	pPixmap=getStackPixmap(cardVector,isHighlighted(),
			       hintHighlightIndex(),m_compressValue,
			       &m_bRectVector);
    }

    if (NULL!=pPixmap)
    {
	this->setPixmap(*pPixmap);
	delete pPixmap;
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
QPixmap * VCardStack::getStackPixmap(const PlayingCardVector & cardVector,
				     bool highlighted,
				     int hintHighlightIndex)
{
    return getStackPixmap(cardVector,highlighted,
			  hintHighlightIndex,CompressNormal,NULL);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
bool VCardStack::getCardIndex(const QPointF & pos,unsigned int & index)
{
    bool rc=false;
    
    unsigned int i;

    for(i=0;i<this->m_bRectVector.size();i++)
    {
	if (this->m_bRectVector[i].contains(pos))
	{
	    index=i;
	    rc=true;
	    break;
	}
    }
    
    return rc;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
void VCardStack::calcPixmapSize(const PlayingCardVector & cardVector,
				QSize & size,qreal compressValue)
{
    unsigned int i;

    if (0==cardVector.size())
    {
	size=CardPixmaps::getInst().getCardSize();
    }
    else
    { 
	unsigned int sizeY=0;
	
	for (i=0;i<cardVector.size();i++)
	{
	    // if it is the last card we just want the full size
	    // of the card.
	    if ((cardVector.size()-1)==i)
	    {
		sizeY+=CardPixmaps::getInst().getCardSize().height();
	    }
	    else
	    {
		sizeY+=getOverlapIncrement(cardVector[i],compressValue);
	    }
	}

	size.setWidth(CardPixmaps::getInst().getCardSize().width());
	size.setHeight(sizeY);
    }
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
unsigned int VCardStack::getOverlapIncrement(const PlayingCard & card,
					     qreal compressValue) const
{
    unsigned int rc;
    if (card.isFaceUp())
    {
	rc=(CardPixmaps::getInst().getCardSize().height()*ExposedPrecentFaceUp)/compressValue;
    }
    else
    {
	rc=(CardPixmaps::getInst().getCardSize().height()*ExposedPrecentFaceDown)/compressValue;
    }

    return rc;
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
QPixmap * VCardStack::getStackPixmap(const PlayingCardVector & cardVector,
				     bool highlighted,
				     int hintHighlightIndex,
				     qreal compressValue,
				     CardBRectVect * pBRectVector)
{

    QPixmap * pPixmap=NULL;

    if (pBRectVector)
    {
	pBRectVector->clear();
    }

    // first handle the case that there are no cards in the stack
    if (0==cardVector.size())
    {
	bool hl=(highlighted || HintHighlightNoCards==hintHighlightIndex);
	pPixmap=new QPixmap(CardPixmaps::getInst().getCardNonePixmap(hl,this->showRedealCircle()));
    }
    else
    {
	unsigned int i;
	QSize cardSize(CardPixmaps::getInst().getCardSize());
	QSize pixmapSize(0,0);
	
	this->calcPixmapSize(cardVector,pixmapSize,compressValue);

	pPixmap =new QPixmap(pixmapSize);
	// for linux the transparent fill must be done before
	// we associate the pixmap with the painter
	pPixmap->fill(Qt::transparent);

	QPainter painter(pPixmap);


	QPoint pt(0,0);

	for (i=0;i<cardVector.size();i++)
	{
	    bool hl=((hintHighlightIndex>=0 && hintHighlightIndex<=(int)i) || 
		     ((cardVector.size()-1==i) && highlighted));

	    if (cardVector[i].isFaceUp())
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardPixmap(cardVector[i],hl));
	    }
	    else
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardBackPixmap(hl));
	    }

	    unsigned int incrementValue=getOverlapIncrement(cardVector[i],
							    compressValue);

	    if (pBRectVector)
	    {
		// if it is the last card we just want the full size
		// of the card.
		if (cardVector.size()-1==i)
		{
		    pBRectVector->push_back(QRectF(QPointF(0,pt.y()),cardSize));
		}
		else
		{
		    pBRectVector->push_back(QRectF(QPointF(0,pt.y()),QSize(cardSize.width(),incrementValue)));
		}
	    }

	    // increment the point that we are going to paint the
	    // card pixmap onto this pixmap
	    pt.ry()+=incrementValue;

	}
    }
    return pPixmap;
}
