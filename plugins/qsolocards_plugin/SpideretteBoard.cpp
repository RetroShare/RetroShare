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

#include "SpideretteBoard.h"
#include "CardPixmaps.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
SpideretteBoard::SpideretteBoard()
{
    this->setGameName(QString(tr("Spiderette Solitaire")).trimmed());
    this->setGameId("Spiderette");

    this->setHelpFile(":/help/SpideretteHelp.html");
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
SpideretteBoard::~SpideretteBoard()
{
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::newGame()
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
    }
    
    // ok now start the deal.  We don't need a move record for this item.
    m_dealAni.dealCards(dealItemVector,false);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::addGameMenuItems(QMenu & menu)
{
    Q_UNUSED(menu);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::loadSettings(const QSettings & settings)
{
    Q_UNUSED(settings);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::saveSettings(QSettings & settings)
{
    Q_UNUSED(settings);
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::resizeEvent (QResizeEvent * event)
{
    unsigned int i;

    GameBoard::resizeEvent(event);


    QSize cardSize(CardPixmaps::getInst().getCardSize());

    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);
    
    m_pDeck->setPos(currPos);

    currPos.rx()+=GameBoard::LayoutSpacing*4 + cardSize.width()*4;


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

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void SpideretteBoard::createStacks()
{
    setCardResizeAlg(8,ResizeByWidth);


    unsigned int i;

    // first create the home widgets where the cards need to be eventually stacked to
    // win the game.
    for(i=0;i<4;i++)
    {
        this->m_homeVector.push_back(new SpiderHomeStack);
	this->m_scene.addItem(m_homeVector[i]);

        // get signals when cards are added to the stack.  So, we can add undo info and
        // see when the game is over.
        this->connect(this->m_homeVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
    }

    // now create the 7 rows for the stacks.
    for (i=0;i<7;i++)
    {
        this->m_stackVector.push_back(new SpiderStack);
	this->m_scene.addItem(m_stackVector[i]);

        // get signals when cards are added to the stack.  So, we can add undo info and
        // see when the game is over.
        this->connect(this->m_stackVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
        this->connect(this->m_stackVector[i],SIGNAL(sendSuitHome(SpiderStack *,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotSendSuitHome(SpiderStack*,PlayingCardVector,CardMoveRecord)));
        this->connect(this->m_stackVector[i],SIGNAL(moveCardsToDiffStack(SpiderStack *,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotMoveCardsToDiffStack(SpiderStack*,PlayingCardVector,CardMoveRecord)));
    }

    this->m_pDeck=new CardStack;
    this->m_scene.addItem(this->m_pDeck);

    // get signals when the deck is clicked on so we can deal the next set of cards
    this->connect(this->m_pDeck,SIGNAL(cardClicked(CardStack*,uint)),
                  this,SLOT(slotDealNextCards(CardStack*,uint)));
}
