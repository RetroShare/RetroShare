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

#include "DealAnimation.h"
#include "CardAnimationLock.h"


#include <QtGui/QPixmap>
#include <QtGui/QGraphicsScene>
#include <QtCore/QTimeLine>
#include <QtCore/QTimer>

#include <iostream>

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealItem::DealItem(CardStack * pDst,CardStack * pSrc)
    :m_pDst(pDst),
     m_pSrc(pSrc),
     m_flipList()
{
} 

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealItem::DealItem(const DealItem & rh)
    :m_pDst(NULL),
     m_pSrc(NULL),
     m_flipList()
{
    *this=rh;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealItem::~DealItem()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
bool DealItem::getNextCard()
{
    bool rc=false; 

    if (!m_flipList.empty())
    {
	rc=m_flipList.front();
	m_flipList.pop_front(); 
    }


    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealItem & DealItem::operator=(const DealItem & rh)
{
    m_pSrc=rh.m_pSrc;
    m_pDst=rh.m_pDst;
    m_flipList=rh.m_flipList;

    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealGraphicsItem::DealGraphicsItem(DealItem & dealItem,CardMoveRecord &moveRecord)
    :QObject(),
     QGraphicsPixmapItem(),
     m_dealItem(dealItem),
     m_flipCard(dealItem.getNextCard()),
     m_card(PlayingCard::MaxSuit,PlayingCard::MaxCardIndex),
     m_pGItemAni(new QGraphicsItemAnimation),
     m_moveRecord(moveRecord),
     m_timeLine(),
     m_delayTimer(),
     m_emitFinished(true)
{
    this->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);    
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealGraphicsItem::~DealGraphicsItem()
{
    
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealGraphicsItem::slotAniFinished()
{
    // add the card
    m_dealItem.dst()->addCard(m_card,m_moveRecord);

    // now update the destination
    m_dealItem.dst()->updateStack();

    // remove the item from the scene
    m_dealItem.src()->scene()->removeItem(this);

    // now if the card needs to be flipped.  Flip it.  If we are not
    // emitting a finish signal.  Then disable animation for the flip.
    if (m_flipCard)
    {
	m_dealItem.dst()->flipCard(m_dealItem.dst()->getCardVector().size()-1,m_moveRecord,m_emitFinished);
    }

    delete m_pGItemAni;

    m_pGItemAni=NULL;

    if (m_emitFinished)
    {
	emit aniFinished(this);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealGraphicsItem::slotTimeToStart()
{
    m_timeLine.start();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealGraphicsItem::setupAnimation(unsigned int duration,unsigned int delay,unsigned int zValue)
{
    m_timeLine.setDuration(duration);
    this->connect(&m_timeLine,SIGNAL(finished()),
		  this,SLOT(slotAniFinished()));

    QPointF startPt(m_dealItem.src()->getGlobalLastCardPt());

    this->m_card=m_dealItem.src()->removeTopCard(m_moveRecord);

    PlayingCardVector cardVector;

    cardVector.push_back(this->m_card);

    QPixmap * pPixmap=m_dealItem.src()->getStackPixmap(cardVector);
    
    if (pPixmap)
    {
	this->setPixmap(*pPixmap);
	delete pPixmap;
    }

    // set the z value passed in
    this->setZValue(zValue);
    
    // add the item to the scene and move it over the stack in the
    // place of the cards we are going to move
    m_dealItem.src()->scene()->addItem(this);
    this->setPos(startPt);
    

    
    // setup the animation
    m_pGItemAni->setItem(this);
    m_pGItemAni->setTimeLine(&m_timeLine);
    
    m_pGItemAni->setPosAt (0, startPt);
    m_pGItemAni->setPosAt (1, m_dealItem.dst()->getGlobalCardAddPt());
    m_pGItemAni->setRotationAt (0, 0);

    // change the rotation slightly depending on how far apart in the
    // x direction that stacks are apart.
    if (qAbs((int)(startPt.x()-m_dealItem.dst()->getGlobalCardAddPt().x()))<5)
    {
    }
    else if (startPt.x()>=m_dealItem.dst()->getGlobalCardAddPt().x())
    {
	m_pGItemAni->setRotationAt (.5, 90);
    }
    else
    {
	m_pGItemAni->setRotationAt (.5, -90);
    }

    m_pGItemAni->setRotationAt (1, 0);

    // redraw the source stack.
    m_dealItem.src()->updateStack();

    if (delay>0)
    {
	m_delayTimer.setSingleShot(true);
	m_delayTimer.setInterval(delay);
	this->connect(&m_delayTimer,SIGNAL(timeout()),
		      this,SLOT(slotTimeToStart()));
	m_delayTimer.start();
    }
    else
    {
	this->slotTimeToStart();
    }
}

void DealGraphicsItem::stopAni()
{
    bool stopping=false;
    if (QTimeLine::Running==m_timeLine.state())
    {
	m_timeLine.stop();
	stopping=true;
    }
    
    if (m_delayTimer.isActive())
    {
	m_delayTimer.stop();
	stopping=true;
    }

    if (stopping)
    {
	m_emitFinished=false;
	this->slotAniFinished();
    }    
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealAnimation::DealAnimation()
    :m_moveRecord(),
     m_dealItemVector(),
     m_graphicsItemVector(),
     m_aniRunning(false),
     m_stopAni(false),
     m_duration(DealAnimation::PerDealDuration),
     m_createMoveRecord(true)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
DealAnimation::~DealAnimation()
{
    this->stopAni();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
bool DealAnimation::dealCards(const DealItemVector & itemVector, bool createMoveRecord)
{
    if (m_aniRunning)
    {
	return false;
    }


    m_createMoveRecord=createMoveRecord;


    // clear any previous items in the move record.
    m_moveRecord.clear();

    // clear any previous items in the dealItemVector
    // and set it equal to the new one.
    m_dealItemVector.clear();
    m_dealItemVector=itemVector;

    if (!CardAnimationLock::getInst().animationsEnabled())
    {
	this->noAniStackUdpates();
    }
    else
    {
	this->buildAniStackUpdates();
	CardAnimationLock::getInst().lock();
	    
	this->m_aniRunning=true;    
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::stopAni()
{
    // ok we only want to do this if animation is running.
    if (m_aniRunning)
    {
	m_aniRunning=false;

	unsigned int i;
	
	// different from the case when we are cleaning these up as we go we want a hard update, and then
	// delete of the objects.
	for (i=0;i<m_graphicsItemVector.size();i++)
	{
	    m_graphicsItemVector[i]->stopAni();
	    delete m_graphicsItemVector[i];
	}
	
	m_graphicsItemVector.clear();	
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::slotAniFinished(DealGraphicsItem * pDealGraphicsItem)
{
    if (m_aniRunning)
    {
	// cleanup and finialize addition of items to new stacks.
	this->cleanUpGraphicsItem(pDealGraphicsItem);
	
	if (m_graphicsItemVector.empty())
	{
	    m_aniRunning=false;
	    
	    CardAnimationLock::getInst().unlock();
	    
	    if (!m_createMoveRecord)
	    {
		m_moveRecord.clear();
	    }
	    
	    emit cardsMoved(m_moveRecord);
	}
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
bool DealAnimation::cardsRemaining()
{
    bool rc=false;
    unsigned int i;
    
    for (i=0;i<m_dealItemVector.size();i++)
    {
	if(!m_dealItemVector[i].isEmpty())
	{
	    rc=true;
	    break;
	}
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::cleanUpGraphicsItem(DealGraphicsItem * pDealGraphicsItem)
{
    unsigned int i;
    
    for (i=0;i<m_graphicsItemVector.size();i++)
    {
	if (m_graphicsItemVector[i]==pDealGraphicsItem)
	{
	    m_graphicsItemVector.erase (m_graphicsItemVector.begin()+i);
	    pDealGraphicsItem->deleteLater();
	    break;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::cleanUpGraphicsItems()
{
    unsigned int i;
    
    for (i=0;i<m_graphicsItemVector.size();i++)
    {
	m_graphicsItemVector[i]->deleteLater();
    }

    m_graphicsItemVector.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::noAniStackUdpates()
{
    unsigned int i;
    bool oneNotEmpty=true;

    while (oneNotEmpty)
    {
	oneNotEmpty=false;
	for (i=0;i<m_dealItemVector.size();i++)
	{
	    if (!m_dealItemVector[i].isEmpty())
	    {
		bool flip=m_dealItemVector[i].getNextCard();
		
		PlayingCard card(m_dealItemVector[i].src()->removeTopCard(m_moveRecord));
		
		m_dealItemVector[i].dst()->addCard(card,m_moveRecord);
		
		if (flip)
		{
		    m_dealItemVector[i].dst()->flipCard(-1,m_moveRecord);
		}
		m_dealItemVector[i].src()->updateStack();
		m_dealItemVector[i].dst()->updateStack();

		oneNotEmpty=true;
	    }
	}
    }

    if (!m_createMoveRecord)
    {
	m_moveRecord.clear();
    }	

    if (m_aniRunning)
    {
	emit cardsMoved(m_moveRecord);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
void DealAnimation::buildAniStackUpdates()
{
    unsigned int i;
    unsigned int j=0;
    
    bool oneNotEmpty=true;

    while (oneNotEmpty)
    {
	oneNotEmpty=false;


	for (i=0;i<m_dealItemVector.size();i++)
	{
	    if (!m_dealItemVector[i].isEmpty())
	    {
		
		DealGraphicsItem * pCurrGraphicsItem=new DealGraphicsItem(m_dealItemVector[i],m_moveRecord);
		
		// we want these cards to be above that of any stack and the stacks
		// have a zValue of 1.  And we want the cards to be of different values.
		// So, cards that are going
		// to dealt first will be on top of the ones that are delayed.  A separate var
		// is used for the delay.  Because we may not have cards for all stacks.  And
		// we want the delay between each card to be the same.  And we don't want a delay
		// before dealing the first card.
		pCurrGraphicsItem->setupAnimation(m_duration,j*PerCardDelay,j+2);
		
		this->connect(pCurrGraphicsItem,SIGNAL(aniFinished(DealGraphicsItem *)),
			      this,SLOT(slotAniFinished(DealGraphicsItem *)));
		
		m_graphicsItemVector.push_back(pCurrGraphicsItem);
		
		oneNotEmpty=true;
		j++;
	    }
	}
    }
}
