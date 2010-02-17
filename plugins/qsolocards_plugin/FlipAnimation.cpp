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

#include "FlipAnimation.h"
#include "CardStack.h"
#include "CardAnimationLock.h"

#include <QtGui/QGraphicsScene>


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
FlipAnimation::FlipAnimation()
    :m_pSrc(NULL),
     m_card(),
     m_timeLine(),
     m_pItemAni(NULL),
     m_pPixmapItem(NULL),
     m_flipPtReached(false),
     m_aniRunning(false)
{
    // connect up the slot so we will know when it is finished.
    this->connect(&m_timeLine,SIGNAL(finished()),
		  this,SLOT(slotAniFinished()));

    this->connect(&m_timeLine,SIGNAL(valueChanged(qreal)),
		  this,SLOT(slotFlipAniProgress(qreal)));
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
FlipAnimation::~FlipAnimation()
{
    delete m_pItemAni;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void FlipAnimation::flipCard(CardStack * pSrc,int duration)
{
    // if animation is off just immediately emit the flipComplete 
    // signal
    if (!CardAnimationLock::getInst().animationsEnabled() ||
	pSrc->isEmpty())
    {
	emit flipComplete(pSrc);
    }
    else
    {
	// first if we have an animation running stop it.
	this->stopAni();
	m_pSrc=pSrc;
	runAnimation(duration);
    }

}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void FlipAnimation::stopAni()
{
    if (this->isAniRunning())
    {
	m_aniRunning=false;

	this->m_timeLine.stop();

	// remove the animation object
	m_pSrc->scene()->removeItem(m_pPixmapItem);
	delete m_pPixmapItem;
	m_pPixmapItem=NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
bool FlipAnimation::isAniRunning() const
{
    return m_aniRunning;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void FlipAnimation::slotAniFinished()
{
    m_aniRunning=false;

    emit flipComplete(m_pSrc);	
    
    // remove the animation object
    m_pSrc->scene()->removeItem(m_pPixmapItem);
    delete m_pPixmapItem;
    m_pPixmapItem=NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void FlipAnimation::slotFlipAniProgress(qreal currProgress)
{
    if (currProgress>=.5 && !m_flipPtReached)
    {
	m_flipPtReached=true;

	m_card.setFaceUp(!m_card.isFaceUp());
	
	PlayingCardVector cardVector;

	cardVector.push_back(m_card);

	QPixmap * pPixmap=m_pSrc->getStackPixmap(cardVector);

	if (pPixmap)
	{
	    m_pPixmapItem->setPixmap(*pPixmap);
	    delete pPixmap;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void FlipAnimation::runAnimation(int duration)
{
    m_flipPtReached=false;

    delete m_pItemAni;
    delete m_pPixmapItem;

    m_timeLine.setDuration(duration);
    m_pItemAni=new QGraphicsItemAnimation;
    m_pPixmapItem=new QGraphicsPixmapItem;

    m_card=m_pSrc->getCardVector()[m_pSrc->getCardVector().size()-1];

    PlayingCardVector cardVector;
    
    cardVector.push_back(m_card);
    
    QPixmap * pPixmap=m_pSrc->getStackPixmap(cardVector);
    
    if (pPixmap)
    {
	m_pPixmapItem->setPixmap(*pPixmap);
	delete pPixmap;
    }

    m_pPixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

    QPointF initPt(m_pSrc->getGlobalLastCardPt());

    QPoint halfWayPt(initPt.x()+pPixmap->width()/2,initPt.y()+pPixmap->height()/2);

    // set the z value to 2 so it will be on top of the
    // stacks.
    m_pPixmapItem->setZValue(2);

    // add the item to the scene and move it over the stack in the
    // place of the card we are going to flip
    m_pSrc->scene()->addItem(m_pPixmapItem);
    m_pPixmapItem->setPos(initPt);
    
    // setup the animation
    m_pItemAni->setItem(m_pPixmapItem);
    m_pItemAni->setTimeLine(&m_timeLine);
    
    m_pItemAni->setPosAt (0, initPt);
    m_pItemAni->setPosAt (.5, halfWayPt);
    m_pItemAni->setPosAt (1, initPt);
    
    m_pItemAni->setScaleAt( 0, 1, 1 );
    m_pItemAni->setScaleAt( 0.5, 0.0, 1 );
    m_pItemAni->setScaleAt( 1, 1, 1 );
    

    m_aniRunning=true;
    m_timeLine.start();
}
