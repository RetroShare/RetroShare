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

#include "StackToStackFlipAni.h"
#include "CardAnimationLock.h"
#include "CardPixmaps.h"

#include <QtGui/QGraphicsScene>
#include <QtGui/QPainter>

#include <iostream>

const qreal StackToStackFlipAni::ExposedPrecentShownCards=.18;
const qreal StackToStackFlipAni::ExposedPrecentHiddenCards=.02; 

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
StackToStackFlipAni::StackToStackFlipAni()
    :m_pDst(NULL),
     m_pSrc(NULL),
     m_cardVector(),
     m_firstCardToShow(0),
     m_flipPtReached(false),
     m_moveRecord(),
     m_pTimeLine(NULL),
     m_pItemAni(NULL),
     m_pPixmapItem(NULL),
     m_aniRunning(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
StackToStackFlipAni::~StackToStackFlipAni()
{
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void StackToStackFlipAni::moveCards(CardStack * pDst,CardStack * pSrc,
				    int numCards,int cardsShown,
				    int duration)
{
    // first if we have an animation running stop it.
    this->stopAni();

    // setup the next animation.
    m_pDst=pDst;
    m_pSrc=pSrc;
    m_cardVector.clear();
    m_moveRecord.clear();
    m_flipPtReached=false;

    while( !m_pSrc->isEmpty() && numCards>0)
    {
	PlayingCard card(m_pSrc->removeTopCard(m_moveRecord));
	
	m_cardVector.insert(m_cardVector.begin(),card);
	numCards--;
    }

    // by default we will show all cards
    m_firstCardToShow=0;

    // if we are not showing all cards starting with index 0
    // then figure out the index of the first card that will
    // be shown.
    if (m_cardVector.size()>0 && cardsShown>=0 &&
	cardsShown<(int)m_cardVector.size())
    {
	// ok if the value is 0 or 1.  We will just show
	// the last card.  0 doesn't make any sense if you
	// are flipping cards.  So, we will assume one for
	// that case as well.
	if (cardsShown<=1)
	{
	    m_firstCardToShow=m_cardVector.size()-1;
	}
	else
	{
	    m_firstCardToShow=m_cardVector.size()-cardsShown;
	}
    }


    // if animation is off just immediately add the cards to the new
    // stack.  Call updateStack to redraw the stacks.  And emit the
    // signal that the cards have been moved.
    if (!CardAnimationLock::getInst().animationsEnabled())
    {
	this->flipCards();
	m_pSrc->updateStack();

	m_pDst->addCards(m_cardVector,m_moveRecord);
	m_pDst->updateStack();
	emit cardsMoved(m_moveRecord);
    }
    else
    {
	CardAnimationLock::getInst().lock();
	runAnimation(duration);
    }

}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void StackToStackFlipAni::slotAniFinished(bool emitSignal)
{
    if (m_aniRunning)
    {
	m_aniRunning=false;
	this->m_pTimeLine->stop();

	m_pDst->addCards(m_cardVector,m_moveRecord);
	m_pDst->updateStack();

	// remove the animation object
	m_pSrc->scene()->removeItem(m_pPixmapItem);
	delete m_pPixmapItem;
	m_pPixmapItem=NULL;
	
	CardAnimationLock::getInst().unlock();

	if (emitSignal)
	{
	    emit cardsMoved(m_moveRecord);	
	}
    }
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void StackToStackFlipAni::slotAniProgress(qreal currProgress)
{
    if (currProgress>=.6 && !m_flipPtReached)
    {
	m_flipPtReached=true;

	this->flipCards();

	QPixmap * pPixmap=this->getAniPixmap();

	if (pPixmap)
	{
	    m_pPixmapItem->setPixmap(*pPixmap);
	    delete pPixmap;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void StackToStackFlipAni::runAnimation(int duration)
{
    m_aniRunning=true;

    delete m_pTimeLine;
    delete m_pItemAni;
    delete m_pPixmapItem;

    
    m_pTimeLine=new QTimeLine(duration);
    m_pItemAni=new QGraphicsItemAnimation;
    m_pPixmapItem=new QGraphicsPixmapItem;
    

    QPixmap * pPixmap=this->getAniPixmap();
    
    if (pPixmap)
    {
	m_pPixmapItem->setPixmap(*pPixmap);
	delete pPixmap;
    }

    // set the z value to 2 so it will be on top of the
    // stacks.
    m_pPixmapItem->setZValue(2);

    // add the item to the scene and move it over the stack in the
    // place of the cards we are going to move
    m_pSrc->scene()->addItem(m_pPixmapItem);
    m_pPixmapItem->setPos(m_pSrc->getGlobalLastCardPt());
    
    
    // setup the animation
    m_pItemAni->setItem(m_pPixmapItem);
    m_pItemAni->setTimeLine(m_pTimeLine);

    // set the start and end point
    m_pItemAni->setPosAt (0, m_pSrc->getGlobalLastCardPt());
    m_pItemAni->setPosAt (1, m_pDst->getGlobalCardAddPt());

    // set the scaling to give the flipping effect.
    m_pItemAni->setScaleAt( 0, 1, 1 );
    m_pItemAni->setScaleAt( 0.6, 0, 1 );
    m_pItemAni->setScaleAt( 1, 1, 1 );

    // connect up the slot so we will know when it is finished.
    this->connect(m_pTimeLine,SIGNAL(finished()),
		  this,SLOT(slotAniFinished()));

    this->connect(m_pTimeLine,SIGNAL(valueChanged(qreal)),
		  this,SLOT(slotAniProgress(qreal)));

    // update the src stack behind the flip item we just added
    m_pSrc->updateStack();
    
    m_pTimeLine->start();
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
QPixmap * StackToStackFlipAni::getAniPixmap()
{
    QPixmap * pPixmap=NULL;

    if (m_cardVector.size()>0)
    {
	unsigned int i;

	pPixmap =new QPixmap(this->calcPixmapSize());
	// for linux the transparent fill must be done before
	// we associate the pixmap with the painter
	pPixmap->fill(Qt::transparent);

	QPainter painter(pPixmap);


	QPoint pt(0,0);

	for (i=0;i<m_cardVector.size();i++)
	{
	    if (m_cardVector[i].isFaceUp())
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardPixmap(m_cardVector[i]));
	    }
	    else
	    {
		painter.drawPixmap(pt,CardPixmaps::getInst().getCardBackPixmap());
	    }

	    pt.rx()+=getOverlapIncrement(i);
	}
    }
    else
    {
	pPixmap=new QPixmap(CardPixmaps::getInst().getTransparentPixmap());
    }

    return pPixmap;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
QSize StackToStackFlipAni::calcPixmapSize()
{
    QSize cardSize(CardPixmaps::getInst().getCardSize());
    QSize size(0,cardSize.height());
    
    unsigned int i;

    for(i=0;i<m_cardVector.size();i++)
    {
	if (m_cardVector.size()-1==i)
	{
	    size.rwidth()+=cardSize.width();
	}
	else
	{
	    size.rwidth()+=getOverlapIncrement(i);
	}
    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
int StackToStackFlipAni::getOverlapIncrement(unsigned int index)
{
    int increment=0;

    if (index<m_cardVector.size())
    {
	if (index>=m_firstCardToShow && m_cardVector[index].isFaceUp())
	{
	    increment=CardPixmaps::getInst().getCardSize().width()*ExposedPrecentShownCards;
	}
	else
	{
	    increment=CardPixmaps::getInst().getCardSize().width()*ExposedPrecentHiddenCards;
	}
    }

    return increment;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void StackToStackFlipAni::flipCards()
{

    if (m_cardVector.size()>0)
    {
	int i;
    
	PlayingCardVector flipVector;

	for(i=m_cardVector.size()-1;i>=0;i--)
	{
	    m_cardVector[i].setFaceUp(!m_cardVector[i].isFaceUp());
	    flipVector.push_back(m_cardVector[i]);
	}
	
	m_cardVector.clear();
	m_cardVector=flipVector;
    }
}
