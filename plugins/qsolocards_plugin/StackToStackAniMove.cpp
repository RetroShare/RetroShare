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

#include "StackToStackAniMove.h"
#include "CardAnimationLock.h"

#include <QtGui/QGraphicsScene>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMoveItem::StackToStackAniMoveItem(const CardMoveRecord & startMoveRecord,int duration)
    :m_pSrc(NULL),
     m_pDst(NULL),
     m_pFlipStack(NULL),
     m_flipIndex(-2),
     m_srcTopCardIndex(-1),    
     m_cardVector(),
     m_duration(duration),
     m_moveRecord(startMoveRecord)
{
    CardMoveRecord moveRecord(startMoveRecord);

    while(!moveRecord.empty())
    {
        CardMoveRecordItem currItem(moveRecord.back());
        CardStack * pStack=CardStack::getStackByName(currItem.stackName());
        const PlayingCardVector & cardVector=currItem.cardVector();

        CardMoveRecordItem::MoveType moveType=currItem.moveType();

        switch(moveType)
        {
            case CardMoveRecordItem::RemoveCards:
            {
		m_pSrc=pStack;
		if (NULL!=m_pSrc)
		{
		    m_srcTopCardIndex=m_pSrc->getCardVector().size()-cardVector.size();
		}
		m_cardVector=cardVector;
            }
            break;
            case CardMoveRecordItem::AddCards:
            {
		m_pDst=pStack;
            }
            break;
            // for this case we are just going to flip the card over
            case CardMoveRecordItem::FlipCard:
            {
		m_flipIndex=currItem.flipIndex();
		m_pFlipStack=pStack;
            }
            break;
        };

        moveRecord.pop_back();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMoveItem::StackToStackAniMoveItem(const  StackToStackAniMoveItem & rh)
    :m_pSrc(NULL),
     m_pDst(NULL),
     m_pFlipStack(NULL),
     m_flipIndex(-1),
     m_srcTopCardIndex(-1),    
     m_cardVector(),
     m_duration(0),
     m_moveRecord()
{
    *this=rh;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMoveItem::StackToStackAniMoveItem()
    :m_pSrc(NULL),
     m_pDst(NULL),
     m_pFlipStack(NULL),
     m_flipIndex(-1),
     m_srcTopCardIndex(-1),    
     m_cardVector(),
     m_duration(0),
     m_moveRecord()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMoveItem::~StackToStackAniMoveItem()
{
}

StackToStackAniMoveItem & StackToStackAniMoveItem::operator=(const StackToStackAniMoveItem & rh)
{
    m_pSrc=rh.m_pSrc;
    m_pDst=rh.m_pDst;
    m_pFlipStack=rh.m_pFlipStack;
    m_flipIndex=rh.m_flipIndex;
    m_srcTopCardIndex=rh.m_srcTopCardIndex;    
    m_cardVector=rh.m_cardVector;
    m_duration=rh.m_duration;
    m_moveRecord=rh.m_moveRecord;

    return *this;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMove::StackToStackAniMove()
    :m_pTimeLine(NULL),
     m_flipDelayTimer(),
     m_pItemAni(NULL),
     m_pPixmapItem(NULL),
     m_aniMoveItem(),
     m_aniRunning(false)
{
    m_flipDelayTimer.setSingleShot(true);
    m_flipDelayTimer.setInterval(250);
    this->connect(&m_flipDelayTimer,SIGNAL(timeout()),
		  this,SLOT(slotWaitForFlipComplete()));
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
StackToStackAniMove::~StackToStackAniMove()
{
    delete m_pTimeLine;
    delete m_pItemAni;
    delete m_pPixmapItem;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void StackToStackAniMove::moveCards(const CardMoveRecord & moveRecord,int duration)
{
    // if animation is off just process the move record as normal
    if (!CardAnimationLock::getInst().animationsEnabled())
    {
	CardStack::processCardMoveRecord(CardStack::RedoMove,moveRecord);
	emit cardsMoved(moveRecord);
    }
    else
    {
	// first if we have an animation running stop it.
	slotAniFinished();
	m_aniMoveItem=StackToStackAniMoveItem(moveRecord,duration);
	CardAnimationLock::getInst().lock();
	runAnimation();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void StackToStackAniMove::stopAni()
{
    slotAniFinished(false);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void StackToStackAniMove::slotAniFinished(bool emitSignal)
{
    if (m_aniRunning)
    {
	m_pItemAni->timeLine()->stop();

	// add the cards to the destination
	m_aniMoveItem.dst()->addCards(m_aniMoveItem.getCardVector());

	// now update the destination
	m_aniMoveItem.dst()->updateStack();
	
	// remove the animation object
	m_aniMoveItem.dst()->scene()->removeItem(m_pPixmapItem);
	
	delete m_pPixmapItem;
	m_pPixmapItem=NULL;

	
	
	// use the emit signal to know whether or not to disable the animation.
	if (m_aniMoveItem.flipIndex()>-2)
	{
	    m_aniMoveItem.flipStack()->flipCard(m_aniMoveItem.flipIndex(),emitSignal);
	}

	CardAnimationLock::getInst().unlock();    

	m_aniRunning=false;
	
	if (emitSignal)
	{
	    // emit a signal that the move is complete
	    emit cardsMoved(m_aniMoveItem.moveRecord());
	}
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void StackToStackAniMove::slotWaitForFlipComplete()
{
    this->runAnimation();
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void StackToStackAniMove::runAnimation()
{
    m_aniRunning=true;

    // if the flip animation is running for the src or dst
    // give it 1/2 a second to complete.
    if (m_aniMoveItem.src()->isFlipAniRunning() ||
	m_aniMoveItem.dst()->isFlipAniRunning())
    {	
	m_flipDelayTimer.start();
	return;
    }

    delete m_pTimeLine;
    delete m_pItemAni;
    delete m_pPixmapItem;

    
    m_pTimeLine=new QTimeLine(m_aniMoveItem.duration());
    m_pItemAni=new QGraphicsItemAnimation;
    m_pPixmapItem=new QGraphicsPixmapItem;
    
    QPixmap * pPixmap=m_aniMoveItem.src()->getStackPixmap(m_aniMoveItem.getCardVector());
    
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
    m_aniMoveItem.src()->scene()->addItem(m_pPixmapItem);
    m_pPixmapItem->setPos(m_aniMoveItem.src()->getGlobalCardPt(m_aniMoveItem.srcTopCardIndex()));
    
    
    // setup the animation
    m_pItemAni->setItem(m_pPixmapItem);
    m_pItemAni->setTimeLine(m_pTimeLine);
    
    m_pItemAni->setPosAt (0, m_aniMoveItem.src()->getGlobalCardPt(m_aniMoveItem.srcTopCardIndex()));
    m_pItemAni->setPosAt (1, m_aniMoveItem.dst()->getGlobalCardAddPt());

    // connect up the slot so we will know when it is finished.
    this->connect(m_pTimeLine,SIGNAL(finished()),
		  this,SLOT(slotAniFinished()));
    
    for (unsigned int i=0;i<m_aniMoveItem.getCardVector().size();i++)
    {
	m_aniMoveItem.src()->removeTopCard();
    }
    
    // redraw the source stack and start the animation.
    m_aniMoveItem.src()->updateStack();
    
    m_pTimeLine->start();
}
