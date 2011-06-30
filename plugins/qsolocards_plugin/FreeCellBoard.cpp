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

#include "FreeCellBoard.h"
#include "CardPixmaps.h"
#include "CardDeck.h"
#include "CardAnimationLock.h"

#include <iostream>

#include <QtGui/QMessageBox>

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
FreeCellBoard::FreeCellBoard()
    :GameBoard(NULL,QString(tr("Freecell")).trimmed(),QString("Freecell")),
     m_pDeck(NULL),
     m_freeVector(),
     m_stackVector(),
     m_homeVector(),
     m_cheat(false)
{
    this->setHelpFile(":/help/FreeCellHelp.html");
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
FreeCellBoard::~FreeCellBoard()
{
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::undoMove()
{
    GameBoard::undoMove();
    
    this->setNumStackMoveCards();
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::redoMove()
{
    GameBoard::redoMove();

    this->setNumStackMoveCards();
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
bool FreeCellBoard::getHint(CardStack * & pSrc,
			    unsigned int & srcIndex,
			    CardStack * & pDst)
{
    bool rc=false;
    unsigned int i;
    unsigned int j;


    // first see if we have any cards from the free cells to move
    // home
    for (i=0;i<this->m_freeVector.size() && !rc;i++)
    {
	if (!this->m_freeVector[i]->isEmpty())
	{
	    const PlayingCardVector & cardVector=this->m_freeVector[i]->getCardVector();
	    PlayingCardVector moveCards;

	    srcIndex=cardVector.size()-1;
	    
	    moveCards.push_back(cardVector[srcIndex]);

	    for (j=0;j<this->m_homeVector.size();j++)
	    {
		if (this->m_homeVector[j]->canAddCards(moveCards))
		{
		    pSrc=this->m_freeVector[i];
		    pDst=this->m_homeVector[j];
		    rc=true;
		    break;
		}
	    }
	}
    }

    // now see if there are any cards in the stacks to move home
    if (!rc)
    {
	for (i=0;i<this->m_stackVector.size() && !rc;i++)
	{
	    if (!this->m_stackVector[i]->isEmpty())
	    {
		const PlayingCardVector & cardVector=this->m_stackVector[i]->getCardVector();
		PlayingCardVector moveCards;
		
		srcIndex=cardVector.size()-1;
		
		moveCards.push_back(cardVector[srcIndex]);

		for (j=0;j<this->m_homeVector.size();j++)
		{
		    if (this->m_homeVector[j]->canAddCards(moveCards))
		    {
			pSrc=this->m_stackVector[i];
			pDst=this->m_homeVector[j];
			rc=true;
			break;
		    }
		}
	    }
	}
    }

    // now see if we can move cards from the free cells to the stacks
    if (!rc)
    {
	for (i=0;i<this->m_freeVector.size() && !rc;i++)
	{
	    if (!this->m_freeVector[i]->isEmpty())
	    {
		PlayingCardVector moveCards;
		
		if (this->m_freeVector[i]->getMovableCards(moveCards,srcIndex))
		{
		    for (j=0;j<this->m_stackVector.size();j++)
		    {
			if (this->m_stackVector[j]->canAddCards(moveCards))
			{
			    pSrc=this->m_freeVector[i];
			    pDst=this->m_stackVector[j];
			    rc=true;
			    break;
			}
		    }
		}
	    }
	}
    }

    // now look for stack to stack moves
    if (!rc)
    {
	for (i=0;i<this->m_stackVector.size() && !rc;i++)
	{
	    if (!this->m_stackVector[i]->isEmpty())
	    {
		PlayingCardVector moveCards;
		
		if (this->m_stackVector[i]->getMovableCards(moveCards,srcIndex))
		{
		    // look through the stacks if we couldn't move the card home
		    for (j=0;j<this->m_stackVector.size();j++)
		    {
			// make sure not the same stackVector and that we are not moving
			// the last card in a stack to an empty stack.  Moving the last
			// card in a stack to an empty stack doesn't do anything.  And lastly
			// if the cards can be added we have a match.
			if (i!=j &&
			    !(0==srcIndex &&  this->m_stackVector[j]->isEmpty()) &&
			    this->m_stackVector[j]->canAddCards(moveCards))
			{
			    pSrc=this->m_stackVector[i];
			    pDst=this->m_stackVector[j];
			    rc=true;
			    break;
			}
		    }
		}
	    }
	}
    }


    // now look to move something to a free cell if all else has failed
    // for now just going to be a basic move an available card from the first
    // stack with cards in it to the free cell.
    if (!rc)
    {
	// first find an open free cell.  If we don't have one no need to continue.
	pDst=NULL;

	for (i=0;i<this->m_freeVector.size();i++)
	{
	    if (this->m_freeVector[i]->isEmpty())
	    {
		pDst=this->m_freeVector[i];
		break;
	    }
	}

	if (NULL!=pDst)
	{
	    for (i=0;i<this->m_stackVector.size();i++)
	    {
		if (!this->m_stackVector[i]->isEmpty())
		{
		    PlayingCardVector moveCards;
		    
		    
		    if (this->m_stackVector[i]->getMovableCards(moveCards,srcIndex) &&
			pDst->canAddCards(moveCards))
		    {
			pSrc=this->m_stackVector[i];
			rc=true;
			break;
		    }
		}
	    }
	}
    }


    return rc;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::newGame()
{
    // call the base class to clean up
    GameBoard::newGame();

    CardDeck deck;
    unsigned int i;


    // add all the cards to the deck
    while(!deck.isEmpty())
    {
        this->m_pDeck->addCard(deck.next());
    }

    // setup the deal of cards
    DealItemVector dealItemVector;

    // Create the dealItemVector to direct the DealAnimation object on
    // how to deal the cards.
    for (i=0;i<this->m_stackVector.size();i++)
    {
	unsigned int j;
	unsigned int cardsInStack=((i<4)?7:6);
	
	dealItemVector.push_back(DealItem(this->m_stackVector[i],m_pDeck));
	
        for (j=0;j<cardsInStack;j++)
        {
	    dealItemVector[i].addCard(true);
        }
    }
    
    // ok now start the deal.  We don't need a move record for this item.
    m_dealAni.dealCards(dealItemVector,false);    
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::addGameMenuItems(QMenu & menu)
{
    Q_UNUSED(menu);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::loadSettings(const QSettings & settings)
{
    Q_UNUSED(settings);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::saveSettings(QSettings & settings)
{
    Q_UNUSED(settings);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::setCheat(bool cheat)
{
    unsigned int i;

    for (i=0;i<m_stackVector.size();i++)
    {
	m_stackVector[i]->setCheat(cheat);
    }
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::slotCardsMoved(const CardMoveRecord & moveRecord)
{
    GameBoard::slotCardsMoved(moveRecord);

    this->setNumStackMoveCards();
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::slotFreeCardsClicked(CardStack * pCardStack,
					 const PlayingCardVector & cardVector,
					 const CardMoveRecord & startMoveRecord)
{
    Q_UNUSED(pCardStack);

    CardStack * pEmptyStack=NULL;
    CardStack * pMoveStack=NULL;
    unsigned int i;

    // ok look for where we would move the cards to.  We will look first at the home stacks.
    // After the home stacks the next priority will be a non-empty stack and then lastly
    // an empty stack.

    for (i=0;i<this->m_homeVector.size();i++)
    {
	if (this->m_homeVector[i]->canAddCards(cardVector))
	{
	    pMoveStack=this->m_homeVector[i];
	    break;
	}
    }

    // if we did not find a match continue to look
    if (NULL==pMoveStack)
    {
	for (i=0;i<this->m_stackVector.size();i++)
	{
	    if (this->m_stackVector[i]->canAddCards(cardVector))
	    {
		if (this->m_stackVector[i]->isEmpty())
		{
		    pEmptyStack=this->m_stackVector[i];
		}
		else
		{
		    pMoveStack=this->m_stackVector[i];
		    break;
		}
	    }
	}
    }

    if (NULL!=pMoveStack || NULL!=pEmptyStack)
    {
	if (NULL==pMoveStack)
	{
	    pMoveStack=pEmptyStack;
	}

	CardMoveRecord moveRecord(startMoveRecord);
	pMoveStack->addCards(cardVector,moveRecord,true);
	
	// perform the move of the cards and animate it if animations
	// are enabled
	m_sToSAniMove.moveCards(moveRecord);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::slotStackCardsClicked(CardStack * pCardStack,
					  const PlayingCardVector & cardVector,
					  const CardMoveRecord & startMoveRecord)
{
    CardStack * pEmptyStack=NULL;
    CardStack * pMoveStack=NULL;
    unsigned int i;

    // ok look for where we would move the cards to.  We will look first at the home stacks.
    // After the home stacks the next priority will be a non-empty stack then an empty stack.
    // And lastly a free cell.

    for (i=0;i<this->m_homeVector.size();i++)
    {
	if (this->m_homeVector[i]->canAddCards(cardVector))
	{
	    pMoveStack=this->m_homeVector[i];
	    break;
	}
    }

    // if we did not find a match continue to look
    if (NULL==pMoveStack)
    {
	for (i=0;i<this->m_stackVector.size();i++)
	{
	    if (this->m_stackVector[i]!=pCardStack)
	    {
		if (this->m_stackVector[i]->canAddCards(cardVector))
		{
		    if (this->m_stackVector[i]->isEmpty())
		    {
			pEmptyStack=this->m_stackVector[i];
		    }
		    else
		    {
			pMoveStack=this->m_stackVector[i];
			break;
		    }
		}
	    }
	}
    }

    // now look in the free cells
    if (NULL==pMoveStack)
    {
	for (i=0;i<this->m_freeVector.size();i++)
	{
	    if (this->m_freeVector[i]->canAddCards(cardVector))
	    {
		pMoveStack=this->m_freeVector[i];
		break;
	    }
	}
    }    

    if (NULL!=pMoveStack || NULL!=pEmptyStack)
    {
	if (NULL==pMoveStack)
	{
	    pMoveStack=pEmptyStack;
	}

	CardMoveRecord moveRecord(startMoveRecord);
	pMoveStack->addCards(cardVector,moveRecord,true);
	
	// perform the move of the cards and animate it if animations
	// are enabled
	m_sToSAniMove.moveCards(moveRecord);
    }

}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::calcScore()
{
    int score=0;
    
    for(unsigned int i=0;i<this->m_homeVector.size();i++)
    {
        score+=this->m_homeVector[i]->score();
    }
    
    emit scoreChanged(score,"");
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::resizeEvent (QResizeEvent * event)
{
    unsigned int i;

    GameBoard::resizeEvent(event);


    QSize cardSize(CardPixmaps::getInst().getCardSize());

    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);
    
    for (i=0;i<m_freeVector.size();i++)
    {
	m_freeVector[i]->setPos(currPos);
	currPos.rx()+=cardSize.width()+GameBoard::LayoutSpacing;
    }

    currPos.rx()+=(cardSize.width() + GameBoard::LayoutSpacing)/2;

    m_pDeck->setPos(currPos);

    currPos.setX(GameBoard::LayoutSpacing*7 + cardSize.width()*6);


    for (i=0;i<m_homeVector.size();i++)
    {
	m_homeVector[i]->setPos(currPos);
	currPos.rx()+=cardSize.width()+GameBoard::LayoutSpacing;
    }

    currPos.setY(GameBoard::LayoutSpacing*2+cardSize.height());
    currPos.setX(GameBoard::LayoutSpacing*2+cardSize.width());
    for (i=0;i<m_stackVector.size();i++)
    {
	m_stackVector[i]->setPos(currPos);
	currPos.rx()+=cardSize.width()+GameBoard::LayoutSpacing;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool FreeCellBoard::runDemo(bool stopWhenNoMore)
{
    bool rc=true;

    if (!GameBoard::runDemo(false))
    {
	// if we didn't find a move.  Try to just move a card to a freecell.
	CardStack * pDst=NULL;
	bool foundMove=false;
	CardStack * pSrc=NULL;
	unsigned int srcIndex=0;
	PlayingCardVector moveCards;
	unsigned int i;

	for (i=0;i<this->m_freeVector.size();i++)
	{
	    if (this->m_freeVector[i]->isEmpty())
	    {
		pDst=this->m_freeVector[i];
		break;
	    }
	}

	if (NULL!=pDst)
	{
	    for (i=0;i<this->m_stackVector.size();i++)
	    {
		if (!this->m_stackVector[i]->isEmpty())
		{
		    if (this->m_stackVector[i]->getMovableCards(moveCards,srcIndex) &&
			pDst->canAddCards(moveCards))
		    {
			pSrc=this->m_stackVector[i];
			foundMove=true;
			break;
		    }
		}
	    }
	}

	// if we found a move create the move record.  And call the stack to stack
	// animation to move it.
	if (foundMove)
	{
	    CardMoveRecord moveRecord;

	    pSrc->removeCardsStartingAt(srcIndex,moveCards,moveRecord,true);
	    pDst->addCards(moveCards,moveRecord,true);

	    m_sToSAniMove.moveCards(moveRecord,this->getDemoCardAniTime());
	}
	else
	{
	    if (stopWhenNoMore)
	    {
		stopDemo();
		rc=false;
	    }
	    else
	    {
		rc=false;
	    }
	}
    }

    return rc;

}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::createStacks()
{
    this->setCardResizeAlg(10,ResizeByWidth);

    unsigned int i;

    // first create the home widgets where the cards need to be eventually stacked to
    // win the game.
    for(i=0;i<4;i++)
    {
        this->m_homeVector.push_back(new FreeCellHome);
	this->m_scene.addItem(m_homeVector[i]);
    }

    // now create the free cells
    for(i=0;i<4;i++)
    {
        this->m_freeVector.push_back(new FreeCellFree);
	this->m_scene.addItem(m_freeVector[i]);
        this->connect(this->m_freeVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
        this->connect(this->m_freeVector[i],SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotFreeCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));
    }

    // now create the 8 rows for the stacks.
    for (i=0;i<8;i++)
    {
        this->m_stackVector.push_back(new FreeCellStack);
	this->m_scene.addItem(m_stackVector[i]);
        this->connect(this->m_stackVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
        this->connect(this->m_stackVector[i],SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotStackCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));
    }

    this->m_pDeck=new FreeCellDeck;
    this->m_scene.addItem(this->m_pDeck);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
bool FreeCellBoard::isGameWon()const
{
    bool rc=true;

    for (unsigned int i=0;i<this->m_homeVector.size();i++)
    {
        if (!this->m_homeVector[i]->isStackComplete())
        {
            rc=false;
            break;
        }
    }

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
bool FreeCellBoard::isGameWonNotComplete()const
{
    bool rc=true;

    for (unsigned int i=0;i<this->m_stackVector.size();i++)
    {
        if (!this->m_stackVector[i]->cardsAscendingTopToBottom())
        {
            rc=false;
            break;
        }
    }

    return rc;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void FreeCellBoard::setNumStackMoveCards()
{
    // as a convience allow dragging of cards if there would be enough
    // freecells to move the cards
    unsigned int numDragCards=1;
    unsigned int i;

    for (i=0;i<m_stackVector.size();i++)
    {
	if (m_stackVector[i]->isEmpty())
	{
	    numDragCards++;
	}
    }

    // in the case that we might be dragging the cards to a free
    // space, we don't want the free space to count.  It would not
    // be a free space to dump a card.  So, it can't be counted.
    if (numDragCards>1)
    {
	numDragCards--;
    }

    for (i=0;i<m_freeVector.size();i++)
    {
	if (m_freeVector[i]->isEmpty())
	{
	    numDragCards++;
	}
    }

    for (i=0;i<m_stackVector.size();i++)
    {
	m_stackVector[i]->setMaxMoveCards(numDragCards);
    }
}
