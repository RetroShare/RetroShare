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

#include "KlondikeFlipStack.h"
#include "CardPixmaps.h"

#include <QtGui/QPainter>
#include <QtGui/QPixmap>

const qreal KlondikeFlipStack::ExposedPrecent=.18;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
KlondikeFlipStack::KlondikeFlipStack()
    :CardStack(),
     m_bRectVector(),
     m_cardsShown(1),
     m_firstShowCard(0)
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
KlondikeFlipStack::~KlondikeFlipStack()
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
QPointF KlondikeFlipStack::getGlobalCardAddPt() const
{
    QPointF pt(0,0);

    const PlayingCardVector & cardVector=this->getCardVector();

    // We are going to do this by the bounding rects and not by actual cards
    // in the stacks.  That way we can add something before doing animations
    // and then update the display when the animation is complete
    if (m_bRectVector.size()>0 && cardVector.size()>=m_bRectVector.size())
    {
	pt=m_bRectVector[m_bRectVector.size()-1].topLeft();

	pt.rx()+=this->getOverlapIncrement(cardVector,m_bRectVector.size()-1,m_firstShowCard);
    }

    return mapToScene(pt);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
QPointF KlondikeFlipStack::getGlobalLastCardPt() const
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

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
QPointF KlondikeFlipStack::getGlobalCardPt(int index) const
{
    QPointF pt(0,0);

    if (index>=0 && index<(int)m_bRectVector.size())
    {
	pt=m_bRectVector[index].topLeft();
    }

    return mapToScene(pt);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void KlondikeFlipStack::updateStack()
{
    PlayingCardVector cardVector=this->getCardVector();
    m_bRectVector.clear();

    // if the stack has no cards in it just call the base
    // class to render the empty stack.
    if (0==cardVector.size())
    {
	CardStack::updateStack();
	return;
    }

    // figure out the index of the first card to show
    if (m_cardsShown>=cardVector.size())
    {
	m_firstShowCard=0;
    }
    else
    {
	m_firstShowCard=cardVector.size()-m_cardsShown;
    }

    // now calc the size of the pixmap we will need
    QSize pixmapSize;

    this->calcPixmapSize(cardVector,pixmapSize,m_firstShowCard);

    QPixmap pixmap(pixmapSize);
    // for linux the transparent fill must be done before
    // we associate the pixmap with the painter
    pixmap.fill(Qt::transparent);
    
    QPainter painter;
    
    painter.begin(&pixmap);
    
    QPoint pt(0,0);
    QSize cardSize(CardPixmaps::getInst().getCardSize());
    unsigned int i;

    for (i=0;i<cardVector.size();i++)
    {
	unsigned int incrementValue=getOverlapIncrement(cardVector,i,
							m_firstShowCard);

	if (i>=m_firstShowCard)
	{
	    bool hl=((hintHighlightIndex()>=0 && hintHighlightIndex()<=(int)i) || 
		     ((cardVector.size()-1==i) && isHighlighted()));
	    
	    if (cardVector[i].isFaceUp())
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardPixmap(cardVector[i],hl));
	    }
	    else
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardBackPixmap(hl));
	    }
	}

	if (cardVector.size()-1==i)
	{
	    m_bRectVector.push_back(QRectF(QPoint(pt.x(),0),
					   cardSize));
	}
	else
	{
	    m_bRectVector.push_back(QRectF(QPoint(pt.x(),0),
					   QSize(incrementValue,cardSize.height())));
	}
	
	// increment the point that we are going to paint the
	// card pixmap onto this pixmap
	pt.rx()+=incrementValue;
    }

    painter.end();

    this->setPixmap(pixmap);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool KlondikeFlipStack::getCardIndex(const QPointF & pos,unsigned int & index)
{
    bool rc=false;
    unsigned int i;

    // go through the bounding rect backwards.  The ones lower for cards that are
    // not visible are just place holders.
    for(i=m_bRectVector.size();i>0;i--)
    {
	if (m_bRectVector[i-1].contains(pos))
	{
	    index=i-1;
	    rc=true;
	    break;
	}
    }

    return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void KlondikeFlipStack::calcPixmapSize(const PlayingCardVector & cardVector,
				       QSize & size,unsigned int showIndex)
{
    QSize cardSize(CardPixmaps::getInst().getCardSize());
    unsigned int i;

    size.setWidth(0);
    size.setHeight(cardSize.height());

    for(i=0;i<cardVector.size();i++)
    {
	if (cardVector.size()-1==i)
	{
	    size.rwidth()+=cardSize.width();
	}
	else
	{
	    size.rwidth()+=this->getOverlapIncrement(cardVector,i,showIndex);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
int KlondikeFlipStack::getOverlapIncrement(const PlayingCardVector & cardVector,
					   unsigned int index,unsigned int showIndex) const
{
    int increment=0;

    if (index<cardVector.size())
    {
	if (index>=showIndex)
	{
	    increment=CardPixmaps::getInst().getCardSize().width()*ExposedPrecent;
	}
    }

    return increment;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool KlondikeFlipStack::canMoveCard(unsigned int index) const
{
    bool rc=false;

    // ok the only time a card can be moved is if the card is the last in the stack.
    if (!this->isEmpty() && (index==(this->getCardVector().size()-1)))
    {
	rc=true;
    }

    return rc;
}
