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

#include "CardStack.h"
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <QtGui/QCursor>
#include <QtGui/QGraphicsSceneHoverEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QApplication>
#include "CardPixmaps.h"
#include "CardAnimationLock.h"

#include <iostream>

CardStackMap      CardStack::m_cardStackMap;
bool              CardStack::m_lockUserInteraction=false;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardStack::CardStack()
    :QObject(),QGraphicsPixmapItem(),
     m_hintHighlightIndex(-1),
     m_stackName(),
     m_cardVector(),
     m_highlighted(false),
     m_showRedealCircle(false),
     m_autoFaceUp(false),
     m_mouseMoved(false),
     m_dragStartPos(0,0),
     m_flipAni(),
     m_dragStack(this)
{
    // set the name of this stack and add it to the map of stacks.
    m_stackName=QString::number((qlonglong)this);    
    m_cardStackMap[this->stackName()]=this;

    // accept drops and mouse hover events
    this->setAcceptDrops(true);
    this->setAcceptHoverEvents(true);

    this->setZValue(1);

    this->setCursor(Qt::PointingHandCursor);
    this->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

    // call update stack to set the initial view of the stack as empty.
    this->updateStack();


    this->connect(&m_flipAni,SIGNAL(flipComplete(CardStack *)),
		  this,SLOT(slotFlipComplete(CardStack *)));

    this->connect(&m_dragStack,SIGNAL(cardsMoved(const CardMoveRecord &)),
		  this,SLOT(slotDragCardsMoved(const CardMoveRecord &)));
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardStack::~CardStack()
{
    // remove the pointer from the map of stacks to the class.
    m_cardStackMap.erase(this->stackName());
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::allCardsFaceUp()const
{
    bool rc=true;
    unsigned int i;

    for(i=0;i<this->m_cardVector.size();i++)
    {
	if (!this->m_cardVector[i].isFaceUp())
	{
	    rc=false;
	    break;
	}
    }

    return rc;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::cardsAscendingTopToBottom()const
{
    bool rc=true;
    const PlayingCardVector & cardVector=this->getCardVector();
    int i;

    // see if the cards are in descending order from 
    // index 0 to n.
    for (i=cardVector.size()-1;i>0;i--)
    {
	if (cardVector[i]>cardVector[i-1])
	{
	    rc=false;
	    break;
	}
    }	

    return rc;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::cardsDecendingTopToBottom()const
{
    bool rc=true;
    const PlayingCardVector & cardVector=this->getCardVector();
    int i;

    // see if the cards are in descending order from 
    // index 0 to n.
    for (i=cardVector.size()-1;i>0;i--)
    {
	if (cardVector[i]<cardVector[i-1])
	{
	    rc=false;
	    break;
	}
    }	

    return rc;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::setTopCardUp(bool faceUp)
{
    if (this->m_cardVector.size()>0 && faceUp!=this->m_cardVector[this->m_cardVector.size()-1].isFaceUp())
    {
	flipCard(-1);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::flipCard(int index,bool aniIfEnabled)
{
    bool rc=false;
    if (this->m_cardVector.size()>0)
    {
	// if index is less than 0 assume it is the last card.
	if (index<0)
	{
	    index=this->m_cardVector.size()-1;
	}

	if (index<(int)this->m_cardVector.size())
	{
	    if (aniIfEnabled)
	    {
		m_flipAni.flipCard(this);
	    }
	    this->m_cardVector[index].setFaceUp(!this->m_cardVector[index].isFaceUp());
	    this->updateStack();
	    rc=true;
	}
    }

    return rc;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::flipCard(int index,CardMoveRecord & moveRecord,bool aniIfEnabled)
{
    bool rc=flipCard(index,aniIfEnabled);
    if (rc)
    {
	// if index is less than 0 assume it is the last card.
	if (index<0)
	{
	    index=this->m_cardVector.size()-1;
	}

	moveRecord.push_back(CardMoveRecordItem(this->stackName(),index));
    }

    return rc;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::addCard(const PlayingCard & newCard)
{
    m_cardVector.push_back(newCard);

    if (isFlipAniRunning())
    {
	m_flipAni.stopAni();
	this->updateStack();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::addCards(const PlayingCardVector & cardVector)
{
    for(unsigned int i=0;i<cardVector.size();i++)
    {
        m_cardVector.push_back(cardVector[i]);
    }

    if (isFlipAniRunning())
    {
	m_flipAni.stopAni();
	this->updateStack();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::addCard(const PlayingCard & newCard,CardMoveRecord & moveRecord,bool justUpdateRec)
{
    if (!justUpdateRec)
    {
	this->addCard(newCard);
    }

    PlayingCardVector cardVector;
    cardVector.push_back(newCard);
    moveRecord.push_back(CardMoveRecordItem(this->stackName(),CardMoveRecordItem::AddCards,cardVector));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::addCards(const PlayingCardVector & cardVector,CardMoveRecord & moveRecord,bool justUpdateRec)
{
    if (!justUpdateRec)
    {
	this->addCards(cardVector);
    }
    moveRecord.push_back(CardMoveRecordItem(this->stackName(),CardMoveRecordItem::AddCards,cardVector));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
PlayingCard CardStack::removeTopCard()
{
    PlayingCard card(PlayingCard::MaxSuit,PlayingCard::MaxCardIndex);

    if (!this->m_cardVector.empty())
    {
        card=m_cardVector.back();
        this->m_cardVector.pop_back();
    }
    return card;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
PlayingCard CardStack::removeTopCard(CardMoveRecord & moveRecord)
{
    PlayingCard card(PlayingCard::MaxSuit,PlayingCard::MaxCardIndex);
    card=this->removeTopCard();

    // if the card is valid update the moveRecord
    if (card.isValid())
    {
        PlayingCardVector cardVector;
        cardVector.push_back(card);

        moveRecord.push_back(CardMoveRecordItem(this->stackName(),CardMoveRecordItem::RemoveCards,cardVector));
    }

    return card;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool CardStack::removeCardsStartingAt(unsigned int index,PlayingCardVector & removedCards)
{
    bool rc=false;

    if (index<this->m_cardVector.size())
    {
	rc=true;

	unsigned int i;
	
	removedCards.clear();
	
	for(i=index;i<this->m_cardVector.size();i++)
	{
	    removedCards.push_back(this->m_cardVector[i]);
	}

	while(m_cardVector.size()>index)
	{
	    this->m_cardVector.pop_back();
	}
    }

    return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool CardStack::removeCardsStartingAt(unsigned int index,
				      PlayingCardVector & removedCards,
				      CardMoveRecord & moveRecord,
				      bool justUpdateRec)
{
    bool rc=false;


    if (!justUpdateRec)
    {
	rc=this->removeCardsStartingAt(index,removedCards);
    }
    else if (index<this->m_cardVector.size())
    {
	rc=true;

	unsigned int i;
	
	removedCards.clear();
	
	for(i=index;i<this->m_cardVector.size();i++)
	{
	    removedCards.push_back(this->m_cardVector[i]);
	}
    }

    if (rc)
    {
	// ok now add the move records for removal of the cards from this stack
	// it will be a remove and then a flip of the card before if it is face down
	moveRecord.push_back(CardMoveRecordItem(this->stackName(),CardMoveRecordItem::RemoveCards,removedCards));

	// also if we are in autoflip mode add that to the flip record.
	if (0!=index && this->m_autoFaceUp && !this->m_cardVector[index-1].isFaceUp())
	{
	    moveRecord.push_back(CardMoveRecordItem(this->stackName(),index-1));
	}
    }
	
    return rc;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::removeAllCards()
{
    // stop any flip animation we have going.
    m_flipAni.stopAni();

    m_cardVector.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::getMovableCards(PlayingCardVector & cardVector,
				unsigned int & index) const
{
    bool rc=false;

    if (this->m_cardVector.size()>0)
    {
        int moveIndex=-1;
        // ok let's go up the stack and find the last card that we can move
        for (int i=this->m_cardVector.size()-1;i>=0;i--)
        {
            if (this->canMoveCard((unsigned int)i))
            {
                moveIndex=i;
            }
            else
            {
                break;
            }
        }

        // did we get an index of a card.
        if (moveIndex>=0)
        {
            rc=true;


            for(unsigned int j=(unsigned int)moveIndex;j<this->m_cardVector.size();j++)
            {
                cardVector.push_back(this->m_cardVector[j]);
            }

            index=(unsigned int)moveIndex;
        }
    }

    return rc;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::updateStack()
{
    QPixmap * pPixmap=NULL;

    if (this->isFlipAniRunning())
    {
	PlayingCardVector cardVector(this->m_cardVector);

	// the stack should have at least one card ie the one being flipped
	// but make sure.
	if (this->m_cardVector.size()>0)
	{
	    cardVector.pop_back();
	}

	pPixmap=getStackPixmap(cardVector,m_highlighted,m_hintHighlightIndex);
    }
    else
    {
	pPixmap=getStackPixmap(this->m_cardVector,m_highlighted,m_hintHighlightIndex);
    }

    if (NULL!=pPixmap)
    {
	this->setPixmap(*pPixmap);
	delete pPixmap;
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
QPixmap * CardStack::getStackPixmap(const PlayingCardVector & cardVector,
				    bool highlighted,
				    int hintHighlightIndex)
{
    bool hl=((hintHighlightIndex>=0 || 
	      (HintHighlightNoCards==hintHighlightIndex && 0==cardVector.size()))|| 
	     highlighted);
    
    QPixmap * pPixmap=NULL;
 
    // if the stack is not empty either show a card or the card back
    if (cardVector.size()>0)
    {
	PlayingCard card(cardVector[cardVector.size()-1]);


	if (card.isFaceUp())
	{
	    pPixmap=new QPixmap(CardPixmaps::getInst().getCardPixmap(card,hl));   
	}
	else
	{
	    pPixmap=new QPixmap(CardPixmaps::getInst().getCardBackPixmap(hl));
	}
    }
    // if the stack is empty show the empty pixmap.
    else
    {
	pPixmap=new QPixmap(CardPixmaps::getInst().getCardNonePixmap(hl,this->m_showRedealCircle));
    }

    return pPixmap;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::updateAllStacks()
{
    CardStackMap::iterator it;

    CardPixmaps::getInst().clearPixmapCache();

    for (it=m_cardStackMap.begin();it!=m_cardStackMap.end();it++)
    {
	it->second->updateStack();
    }    
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::clearAllStacks()
{
    CardStackMap::iterator it;

    for (it=m_cardStackMap.begin();it!=m_cardStackMap.end();it++)
    {
	it->second->removeAllCards();
    }    
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardStack * CardStack::getStackByName(const std::string & stackName)
{
    CardStack * pStack=NULL;

    CardStackMap::iterator it=m_cardStackMap.find(stackName);

    if (m_cardStackMap.end()!=it)
    {
        pStack=it->second;
    }

    return pStack;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CardStack::isCardStack(QGraphicsItem * pGraphicsItem)
{
    CardStack * pCardStack=getStackByName(QString::number((qlonglong)pGraphicsItem).toStdString());
    bool rc=false;

    if (NULL!=pCardStack)
    {
	rc=true;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::processCardMoveRecord(ProcessCardMoveRecordType type,
				      CardMoveRecord moveRecord)
{
    while(!moveRecord.empty())
    {
        CardMoveRecordItem currItem(((CardStack::RedoMove==type)?moveRecord.front():moveRecord.back()));
        CardStack * pStack=CardStack::m_cardStackMap[currItem.stackName()];
        const PlayingCardVector & cardVector=currItem.cardVector();
        unsigned int i;

        CardMoveRecordItem::MoveType moveType=currItem.moveType();

        // for undo we are actually undoing something that was done
        // so we need to AddCards if they were removed or RemoveCards
        // if they were added
        if (CardStack::UndoMove==type)
        {
            if (CardMoveRecordItem::RemoveCards==moveType)
            {
                moveType=CardMoveRecordItem::AddCards;
            }
            else if (CardMoveRecordItem::AddCards==moveType)
            {
                moveType=CardMoveRecordItem::RemoveCards;
            }
        }

        switch(moveType)
        {
            // remove cards to a stack
            case CardMoveRecordItem::RemoveCards:
            {
                for (i=0;i<cardVector.size();i++)
                {
                    pStack->removeTopCard();
                }
                pStack->updateStack();
            }
            break;
            // add cards to a stack
            case CardMoveRecordItem::AddCards:
            {
                pStack->addCards(cardVector);
                pStack->updateStack();
            }
            break;

            // for this case we are just going to flip the card over
            case CardMoveRecordItem::FlipCard:
            {
                // in this case we just want to flip the last card
                if (pStack->m_cardVector.size()>0)
                {
                    unsigned int flipIndex=0;
                    bool isValid=false;
                    if (currItem.flipIndex()<0)
                    {
                        flipIndex=pStack->m_cardVector.size()-1;
                        isValid=true;
                    }
                    else if (currItem.flipIndex()<(int)(pStack->m_cardVector.size()))
                    {
                        flipIndex=(unsigned int)currItem.flipIndex();
                        isValid=true;
                    }

                    if (isValid)
                    {
                        pStack->m_cardVector[flipIndex].setFaceUp(!pStack->m_cardVector[flipIndex].isFaceUp());
                        pStack->updateStack();
                    }
                }
            }
            break;
        };

        if (CardStack::RedoMove==type)
	{
	    moveRecord.pop_front();
	}
	else
	{
	    moveRecord.pop_back();
	}
    }
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardStack::showHint(CardStack * pSrc,
			 unsigned int srcCardIndex,
			 CardStack * pDst)
{
    if (NULL!=pSrc && NULL!=pDst)
    {
        pSrc->slotHintHighlight(srcCardIndex);
        pDst->slotDelayedHintHighlight();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::slotDelayedHintHighlight()
{
    QTimer::singleShot(1000,this,SLOT(slotHintHighlight()));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::slotHintHighlight(int index)
{
    // if the index is less than 0 then the last card should be highlighted
    // or the card pad if no cards are in the stack.
    if (index<0)
    {
        this->m_hintHighlightIndex=HintHighlightNoCards;

        if (this->m_cardVector.size()>0)
        {
            this->m_hintHighlightIndex=this->m_cardVector.size()-1;
        }
    }
    else if (index<(int)this->m_cardVector.size())
    {
        this->m_hintHighlightIndex=index;
    }

    this->updateStack();

    QTimer::singleShot(1000,this,SLOT(slotHintHighlightComplete()));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::slotHintHighlightComplete()
{
    this->m_hintHighlightIndex=-1;
    this->updateStack();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::slotFlipComplete(CardStack * pSrc)
{
    Q_UNUSED(pSrc);

    this->updateStack();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::slotDragCardsMoved(const CardMoveRecord & initRecord)
{ 
    CardMoveRecord moveRecord(initRecord);

    // if we are in autotop card up flip the top card.  and emit a signal
    // that we moved the cards.
    if (!this->isEmpty() && this->isAutoTopCardUp())
    {
	int lastIndex=this->m_cardVector.size()-1;

	if (!this->m_cardVector[lastIndex].isFaceUp())
	{
	    this->flipCard(lastIndex,moveRecord);	    
	}
    }

    emit cardsMovedByDragDrop(moveRecord);
}


//////////////////////////////////////////////////////////////////////////
// for this case when all cards are directly stacked on top of each other
// if there are cards in the stack we will always just return the last card.
//////////////////////////////////////////////////////////////////////////
bool CardStack::getCardIndex(const QPointF & pos,unsigned int & index)
{
    Q_UNUSED(pos);

    bool rc=false;

    if (this->m_cardVector.size()>0)
    {
	index=this->m_cardVector.size()-1;
	rc=true;
    }

    return rc;
}
 
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_lockUserInteraction || this->isFlipAniRunning() || CardAnimationLock::getInst().isDemoRunning())
    {
	QGraphicsPixmapItem::mousePressEvent(event);
	return;
    }


    this->m_mouseMoved=false;
    this->m_dragStartPos=event->pos();
    
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_lockUserInteraction || this->isFlipAniRunning() || CardAnimationLock::getInst().isDemoRunning())
    {
	QGraphicsPixmapItem::mouseReleaseEvent(event);
	return;
    }

    if (!m_mouseMoved)
    {

	unsigned int index;

	if (this->isEmpty())
	{
	    emit padClicked(this); 
	}
	else if (this->getCardIndex(event->pos(),index))
	{
	    if (this->canMoveCard(index))
	    {
		PlayingCardVector moveCards;
		CardMoveRecord moveRecord;

		// create a move record and a vector containing the cards
		// but don't remove them.
		if (CardStack::removeCardsStartingAt(index,moveCards,
						     moveRecord,true))
		{
		    emit  movableCardsClicked(this,moveCards,moveRecord);
		}
	    }
	    else
	    {
		emit cardClicked(this,index);
	    }
	}

    }
    else if (m_dragStack.cardDragStarted())
    {
	m_dragStack.mouseReleaseEvent(event);
    }
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_lockUserInteraction || this->isFlipAniRunning() || CardAnimationLock::getInst().isDemoRunning())
    {
	QGraphicsPixmapItem::mouseMoveEvent(event);
	return;
    }


    QPointF posDiff=(event->pos()-this->m_dragStartPos);
    if ( (qAbs(posDiff.x()) + qAbs((int)posDiff.y()))< QApplication::startDragDistance())
    {
	return;
    }

    m_mouseMoved=true;

    if (!m_dragStack.cardDragStarted())
    {
	unsigned int index=0;
	bool haveCardIndex=this->getCardIndex(event->pos(),index);
	
	if (haveCardIndex && this->canMoveCard(index))
	{
	    unsigned int i;
	    PlayingCardVector dragCardVector;
	    
	    for (i=index;i<this->m_cardVector.size();i++)
	    {
		dragCardVector.push_back(this->m_cardVector[i]);
	    }
	    
	    m_dragStack.startCardMove(event->scenePos(),index,dragCardVector);
	}
    }
    else
    {
	m_dragStack.mouseMoveEvent(event);
    }
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CardStack::hoverMoveEvent(QGraphicsSceneHoverEvent * event)
{
    unsigned int index;

    if (this->getCardIndex(event->pos(),index) &&
	this->canMoveCard(index))
    {
	this->setCursor(Qt::OpenHandCursor);
    }
    else
    {
	this->setCursor(Qt::PointingHandCursor);
    }

    QGraphicsPixmapItem::hoverMoveEvent(event);
}
