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

#include "YukonBoard.h"
#include "CardPixmaps.h"
#include <QtGui/QResizeEvent>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
YukonBoard::YukonBoard()
    :GameBoard(NULL,QString(tr("Yukon Solitaire")).trimmed(),QString("Yukon")),
     m_pDeck(NULL),
     m_homeVector(),
     m_stackVector(),
     m_cheat(false)
{
    this->setHelpFile(":/help/YukonHelp.html");
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
YukonBoard::~YukonBoard()
{
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool YukonBoard::getHint(CardStack * & pSrc,
			 unsigned int & srcStackIndex,
			 CardStack * & pDst)
{
    bool rc=false;
    unsigned int i;

    // ok we will look for a move based on trying to move the first faceup
    // card in a stack.  The basic strategy for this game is to get all cards turned 
    // over.  
    if (!rc)
    {
	for (i=0;i<this->m_stackVector.size() && !rc;i++)
	{
	    unsigned int j;
	    
	    for (j=0;j<this->m_stackVector.size() && !rc;j++)
	    {
		// can't move a stack to itself
		if (i!=j)
		{
		    PlayingCardVector addCardVector(this->m_stackVector[j]->getCardVector());
		    unsigned int k=0;
		    while (!rc && addCardVector.size()>0)
		    {
			// ok find the first card that is faceup.
			if (!addCardVector[0].isFaceUp())
			{
			    addCardVector.erase(addCardVector.begin());
			}
			// no need to do this if a king is already face up and the first card in the stack.
			else if (!(PlayingCard::King==addCardVector[0].getIndex() && 0==k && 
				   this->m_stackVector[i]->isEmpty()) &&
				 this->m_stackVector[i]->canAddCards(addCardVector))
			{
                            pDst=this->m_stackVector[i];
			    srcStackIndex=k;
                            pSrc=this->m_stackVector[j];
			    
			    rc=true;
			}
			else
			{
			    break;
			}
			k++;
		    }
		}
	    }
	}
    }

    // ok let's see if we have an empty stack
    // and a face up king.
    if (!rc)
    {
	CardStack * pEmptyStack=NULL;
	CardStack * pFaceUpKingStack=NULL;
	unsigned int faceUpKingIndex=0;

	for (i=0;i<this->m_stackVector.size() && !rc;i++)
	{
	    if (NULL==pEmptyStack && this->m_stackVector[i]->isEmpty())
	    {
		pEmptyStack=this->m_stackVector[i];
	    }
	    
	    if (NULL==pFaceUpKingStack)
	    {
		const PlayingCardVector cardVector=this->m_stackVector[i]->getCardVector();
		
		unsigned int j;
		
		for(j=0;j<cardVector.size();j++)
		{
		    // look for a king that is not already face up and first in it's stack.
		    if (PlayingCard::King==cardVector[j].getIndex() &&
			     cardVector[j].isFaceUp() && 0!=j)
		    {
			pFaceUpKingStack=m_stackVector[i];
			faceUpKingIndex=j;
		    }
		}
	    }
	    
	    if (NULL!=pEmptyStack && NULL!=pFaceUpKingStack)
	    {
		pSrc=pFaceUpKingStack;
		srcStackIndex=faceUpKingIndex;
		pDst=pEmptyStack;
		
		rc=true;
	    }
	}
    }

    // try to move cards to home stacks.
    if (!rc)
    {
	for (i=0;i<this->m_homeVector.size() && !rc;i++)
	{
	    unsigned int j;
	    
	    for (j=0;j<this->m_stackVector.size() && !rc;j++)
	    {
		const PlayingCardVector cardVector=this->m_stackVector[j]->getCardVector();

		if (cardVector.size()>0)
		{
		    PlayingCardVector addCardVector;

		    addCardVector.push_back(cardVector[cardVector.size()-1]);

		    if (this->m_homeVector[i]->canAddCards(addCardVector))
		    {
			pDst=this->m_homeVector[i];
			srcStackIndex=cardVector.size()-1;
			pSrc=this->m_stackVector[j];
			
			rc=true;
		    }
		}
	    }
	}
    }

    // look at each stack and try to find the first available card that from another
    // stack that can be moved to the stack.
    if (!rc)
    {
	for (i=0;i<this->m_stackVector.size() && !rc;i++)
	{
	    unsigned int j;
	    
	    for (j=0;j<this->m_stackVector.size() && !rc;j++)
	    {
		// can't move a stack to itself
		if (i!=j)
		{
		    PlayingCardVector addCardVector(this->m_stackVector[j]->getCardVector());
		    unsigned int k=0;
		    while (!rc && addCardVector.size()>0)
		    {
			// ok find the first card that is faceup.
			if (!addCardVector[0].isFaceUp())
			{
			    addCardVector.erase(addCardVector.begin());
			}
			else if (this->m_stackVector[i]->canAddCards(addCardVector))
			{
                            pDst=this->m_stackVector[i];
			    srcStackIndex=k;
                            pSrc=this->m_stackVector[j];
			    
			    rc=true;
			}
			else
			{
			    addCardVector.erase(addCardVector.begin());			    
			}
			k++;
		    }		    
		}
	    }
	}
    }



    return rc;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::newGame()
{
    // call the base class
    GameBoard::newGame();

    CardDeck deck;
    unsigned int i;


    while(!deck.isEmpty())
    {
        this->m_pDeck->addCard(deck.next());
    }


    DealItemVector dealItemVector;
    

    // Create the dealItemVector to direct the DealAnimation object on
    // how to deal the cards.
    for (i=0;i<this->m_stackVector.size();i++)
    {
	dealItemVector.push_back(DealItem(this->m_stackVector[i],m_pDeck));

	unsigned int j;
	
        for (j=0;j<i+1;j++)
        {
	    // add the items to tell how to deal the cards to the stack
	    // we want to flip the last card in each stack.
	    if (i==j)
	    {
		dealItemVector[i].addCard(true);		
	    }
	    else
	    {
		dealItemVector[i].addCard(false);
	    }
        }

	// for yukon all but the first stack get 4 more face up cards
	if (i>0)
	{
	    for (j=0;j<4;j++)
	    {
		dealItemVector[i].addCard(true);
	    }
	}
    }
    
    // ok now start the deal.  We don't need a move record for this item.
    m_dealAni.dealCards(dealItemVector,false);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::addGameMenuItems(QMenu & menu)
{
    Q_UNUSED(menu);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::loadSettings(const QSettings & settings)
{
    Q_UNUSED(settings);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::saveSettings(QSettings & settings)
{
    Q_UNUSED(settings);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::setCheat(bool cheat)
{
    this->m_cheat=cheat;

    for(unsigned int i=0;i<this->m_stackVector.size();i++)
    {
        m_stackVector[i]->setCheat(cheat);
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::slotStackCardsClicked(CardStack * pCardStack,
				       const PlayingCardVector & cardVector,
				       const CardMoveRecord & startMoveRecord)
{
    unsigned int i=0;
    CardStack * pFoundStack=NULL;

    if (NULL==pCardStack)
    {
        return;
    }

    // first see if the card can be added to the sent home stack
    if (cardVector.size()==1)
    {
        for(i=0;i<this->m_homeVector.size();i++)
        {
            if (pCardStack!=this->m_homeVector[i] &&
                this->m_homeVector[i]->canAddCards(cardVector))
            {
                pFoundStack=this->m_homeVector[i];
                break;
            }
        }
    }

    // if we did not find a match look at the stacks.
    if (NULL==pFoundStack)
    {
	for(i=0;i<this->m_stackVector.size();i++)
	{
	    if (pCardStack!=this->m_stackVector[i] &&
		this->m_stackVector[i]->canAddCards(cardVector))
	    {
		pFoundStack=this->m_stackVector[i];
		break;
	    }
	}
    }

    if (pFoundStack)
    {
 	CardMoveRecord moveRecord(startMoveRecord);
	pFoundStack->addCards(cardVector,moveRecord,true);
	
	// perform the move of the cards and animate it if animations
	// are enabled
	m_sToSAniMove.moveCards(moveRecord);
    }    
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::calcScore()
{
    int score=0;

    for(unsigned int i=0;i<this->m_homeVector.size();i++)
    {
        score+=this->m_homeVector[i]->score();
    }

    emit scoreChanged(score,"");
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::resizeEvent (QResizeEvent * event)
{
    int i;

    this->setCardResizeAlg(8,ResizeByWidth);
    
    GameBoard::resizeEvent(event);

    QSize cardSize(CardPixmaps::getInst().getCardSize());

    // ok let's see if it fits when we size things by width
    // basically we just need to check if we have room to draw
    // 4 cards vertically.  If we don't have enough room we
    // will try to draw by height.
    if (cardSize.height()*4+GameBoard::LayoutSpacing*5>event->size().height())
    {
	this->setCardResizeAlg(4,ResizeByHeight);
    
	GameBoard::resizeEvent(event);
	
	cardSize=CardPixmaps::getInst().getCardSize();
    }



    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);


    for (i=0;i<static_cast<int>(m_homeVector.size());i++)
    {
	m_homeVector[i]->setPos(currPos);
	currPos.ry()+=GameBoard::LayoutSpacing+cardSize.height();
    }

    currPos.setX(event->size().width()-GameBoard::LayoutSpacing-cardSize.width());
    currPos.setY(GameBoard::LayoutSpacing);
    for (i=m_stackVector.size()-1;i>=0;i--)
    {
	m_stackVector[i]->setPos(currPos);
	currPos.rx()-=cardSize.width()+GameBoard::LayoutSpacing;
    }

    
    currPos.setX(GameBoard::LayoutSpacing*4+cardSize.width()*4);
    currPos.setY(event->size().height()-GameBoard::LayoutSpacing-cardSize.height());

    m_pDeck->setPos(currPos);

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void YukonBoard::createStacks()
{
    unsigned int i;

    // first create the home widgets where the cards need to be eventually stacked to
    // win the game.
    for(i=0;i<4;i++)
    {
        this->m_homeVector.push_back(new FreeCellHome);
	this->m_scene.addItem(m_homeVector[i]);
    }

    // now create the 7 rows for the stacks.
    for (i=0;i<7;i++)
    {
        this->m_stackVector.push_back(new KlondikeStack);
	this->m_scene.addItem(m_stackVector[i]);
        this->connect(this->m_stackVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
        this->connect(this->m_stackVector[i],SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotStackCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));
    }

    this->m_pDeck=new FreeCellDeck;
    this->m_scene.addItem(this->m_pDeck);

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool YukonBoard::isGameWon()const
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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
bool YukonBoard::isGameWonNotComplete()const
{
    bool rc=true;

    for (unsigned int i=0;i<this->m_stackVector.size();i++)
    {
	if (!(this->m_stackVector[i]->cardsAscendingTopToBottom() &&
	      this->m_stackVector[i]->allCardsFaceUp()))
	{
	    rc=false;
	    break;
	}
    }

    return rc;
}
