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

#include "Spider3DeckBoard.h"
#include "CardPixmaps.h"

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
Spider3DeckBoard::Spider3DeckBoard()
    :SpiderBoard()
{
    this->setGameName(QString(tr("Three Deck Spider Solitaire")).trimmed());
    this->setGameId("ThreeDeckSpider");

    this->setHelpFile(":/help/Spider3DeckHelp.html");
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
Spider3DeckBoard::~Spider3DeckBoard()
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::newGame()
{
    // call the base class to do cleanup for us.
    GameBoard::newGame();

    CardDeck deck(3);

    // Put all cards in the m_pDeck stack.  We will deal
    // from this stack.
    while(!deck.isEmpty())
    {
        this->m_pDeck->addCard(deck.next());
    }

    unsigned int i;
    unsigned int j;

    DealItemVector dealItemVector;

    // Create the dealItemVector to direct the DealAnimation object on
    // how to deal the cards.
    for (i=0;i<this->m_stackVector.size();i++)
    {
	unsigned int cardsInStack=((i<6)?5:4);
	
	dealItemVector.push_back(DealItem(this->m_stackVector[i],m_pDeck));
	
        for (j=0;j<cardsInStack;j++)
        {
	    // add the items to tell how to deal the cards to the stack
	    // we want to flip the last card.
	    if (cardsInStack-1==j)
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

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::addGameMenuItems(QMenu & menu)
{
    Q_UNUSED(menu);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::loadSettings(const QSettings & settings)
{
    Q_UNUSED(settings);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::saveSettings(QSettings & settings)
{
    Q_UNUSED(settings);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::resizeEvent (QResizeEvent * event)
{
    unsigned int i;

    GameBoard::resizeEvent(event);


    QSize cardSize(CardPixmaps::getInst().getCardSize());

    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);
    
    m_pDeck->setPos(currPos);

    currPos.rx()+=GameBoard::LayoutSpacing+cardSize.width();

    for (i=0;i<m_homeVector.size();i++)
    {
	m_homeVector[i]->setPos(currPos);
	currPos.setX(currPos.x()+cardSize.width()+GameBoard::LayoutSpacing);
    }

    currPos.setY(GameBoard::LayoutSpacing*2+cardSize.height());
    currPos.setX(GameBoard::LayoutSpacing*2+cardSize.width());
    for (i=0;i<m_stackVector.size();i++)
    {
	m_stackVector[i]->setPos(currPos);
	currPos.setX(currPos.x()+cardSize.width()+GameBoard::LayoutSpacing);
    }

}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void Spider3DeckBoard::createStacks()
{
    setCardResizeAlg(13,ResizeByWidth);

    unsigned int i;
    // create all the widgets for the board
    for(i=0;i<12;i++)
    {
        this->m_homeVector.push_back(new SpiderHomeStack);
	this->m_scene.addItem(m_homeVector[i]);

        // get signals when cards are added to the stack.  So, we can add undo info and
        // see when the game is over.
        this->connect(this->m_homeVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
    }

    for(i=0;i<12;i++)
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

    m_pDeck=new CardStack;
    this->m_scene.addItem(m_pDeck);

    // get signals when the deck is clicked on so we can deal the next set of cards
    this->connect(this->m_pDeck,SIGNAL(cardClicked(CardStack*,uint)),
                  this,SLOT(slotDealNextCards(CardStack*,uint)));
}
