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

#include "SpiderBoard.h"
#include <QtGui/QMessageBox>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QResizeEvent>
#include <QtCore/QDateTime>

#include "CardPixmaps.h"
#include "CardDeck.h"


#include <iostream>

const QString  SpiderBoard::GameTypeKeyStr("GameType");


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SpiderBoard::SpiderBoard(QWidget * pWidget)
    :GameBoard(pWidget,QString(tr("Spider Solitaire")).trimmed(),QString("Spider")),
     m_pDeck(NULL),
     m_homeVector(),
     m_stackVector(),
     m_cheat(false),
     m_gameType(SpiderBoard::FourSuits)
{
    this->setHelpFile(":/help/SpiderHelp.html");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SpiderBoard::~SpiderBoard()
{
}

////////////////////////////////////////////////////////////////////////////////
// ok this is an attempt to offer hint moves.  It will prefer same suit moves, then
// consecutive index matches, and then card to empty stack moves.  It will try to
// move all cards that are movable in a stack.  So, there are possibilities for
// tweaks.
////////////////////////////////////////////////////////////////////////////////
bool SpiderBoard::getHint(CardStack * & pSrcWidget,
			  unsigned int & srcStackIndex,
			  CardStack * & pDstWidget)
{
    bool moveFound=false;
    bool sameSuitFound=false;
    bool nonSameSuitFound=false;

    CardStack * pCanMoveToEmpty=NULL;
    CardStack * pCanMoveTo=NULL;
    CardStack * pCanMoveToSameSuit=NULL;
    bool newMoveFound=false;
    int j;

    qsrand(QDateTime::currentDateTime().toTime_t());
    bool fromZero=((0==qrand()%2 ^ 1==qrand()%3)?true:false); // this introduces some randomness by selecting whether we are going
                                                              // to go through the stacks forward or backwards.


    for ((fromZero?(j=0):(j=this->m_stackVector.size()-1));
	 ((fromZero)?(j<(int)this->m_stackVector.size()):(j>=0))&& !sameSuitFound ;((fromZero)?(j++):(j--)))
    {
        PlayingCardVector cardVector;
        unsigned int currSrcStackIndex;
	bool fromZeroInner=((0==qrand()%2 ^ 1==qrand()%3)?true:false); // this introduces some randomness by selecting whether we are going
	                                                               // to go through the stacks forward or backwards.
	int i;

	newMoveFound=false;

        if (this->m_stackVector[j]->getMovableCards(cardVector,currSrcStackIndex))
        {

            // first see if the cards can be sent home
            // the priority will be the same as a move to
            // cards of the same suit
            if (SpiderHomeStack::canSendHome(cardVector))
            {
                // find an open home widget
                for(unsigned int k=0;k<this->m_homeVector.size();k++)
                {
                    if (this->m_homeVector[k]->isEmpty())
                    {
                        pDstWidget=this->m_homeVector[k];
                        break;
                    }
                }

                if (NULL!=pDstWidget)
                {
                    pSrcWidget=this->m_stackVector[j];
                    srcStackIndex=currSrcStackIndex;
                    moveFound=true;
                    break;
                }
            }

	    for ((fromZeroInner?(i=0):(i=this->m_stackVector.size()-1));
		 ((fromZeroInner)?(i<(int)this->m_stackVector.size()):(i>=0));
		 ((fromZeroInner)?(i++):(i--)))
            {

                // ok we are only moving the cards if the hit is not on the same
                // stack they are already located in
                if (this->m_stackVector[i]!=this->m_stackVector[j])
                {
                    // see if we have a perferred move to a stack that has the same suit
                    // we will break out if we find a perferred match
                    if (this->m_stackVector[i]->canAddCardsSameSuit(cardVector))
                    {
                        pCanMoveToSameSuit=this->m_stackVector[i];
                        newMoveFound=true;
                        break;
                    }
                    // otherwise see if we can move the cards to the stack at all
                    else if (this->m_stackVector[i]->canAddCards(cardVector))
                    {
                        if (this->m_stackVector[i]->isEmpty())
                        {
                            pCanMoveToEmpty=this->m_stackVector[i];
                        }
                        else
                        {
                            pCanMoveTo=this->m_stackVector[i];
                        }
                        newMoveFound=true;
                    }
                }
            }

            if (newMoveFound)
            {
                // ok the best move is to a matching suit and consecutive
                // next best is to a consecutive
                // and lastly to an empty stack.
                if (NULL!=pCanMoveToSameSuit)
                {
                    pDstWidget=pCanMoveToSameSuit;
                    pSrcWidget=this->m_stackVector[j];
                    srcStackIndex=currSrcStackIndex;
                    sameSuitFound=true;
                }
                else if (NULL!=pCanMoveTo && !nonSameSuitFound)
                {
                    pDstWidget=pCanMoveTo;
                    pSrcWidget=this->m_stackVector[j];
                    srcStackIndex=currSrcStackIndex;
                    nonSameSuitFound=true;
                }
                else if (NULL!=pCanMoveToEmpty && !nonSameSuitFound)
                {
                    pDstWidget=pCanMoveToEmpty;
                    pSrcWidget=this->m_stackVector[j];
                    srcStackIndex=currSrcStackIndex;
                }
                moveFound=true;
            }
        }
    }


    return moveFound;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::newGame()
{
    // call the base class to do cleanup for us.
    GameBoard::newGame();

    CardDeck * pDeck=NULL;

    unsigned int i;
    unsigned int j;

    switch(this->m_gameType)
    {
        case SpiderBoard::FourSuits:
        pDeck=new CardDeck(2);
        break;

        case SpiderBoard::TwoSuits:
        {
            PlayingCardVector cardVector;
            // use hearts and spades as the two suits
            for (i=PlayingCard::Ace;i<PlayingCard::MaxCardIndex;i++)
            {
                cardVector.push_back(PlayingCard(PlayingCard::Hearts,(PlayingCard::CardIndex)i));
            }
            for (i=PlayingCard::Ace;i<PlayingCard::MaxCardIndex;i++)
            {
                cardVector.push_back(PlayingCard(PlayingCard::Spades,(PlayingCard::CardIndex)i));
            }
            pDeck=new CardDeck(cardVector,4);
        }
        break;

        case SpiderBoard::OneSuit:
        {
            PlayingCardVector cardVector;
            // use spades as the suit
            for (i=PlayingCard::Ace;i<PlayingCard::MaxCardIndex;i++)
            {
                cardVector.push_back(PlayingCard(PlayingCard::Spades,(PlayingCard::CardIndex)i));
            }
            pDeck=new CardDeck(cardVector,8);
        }
        break;
    };

    // if this happens something is very wrong just return
    if (NULL==pDeck)
    {
        return;
    }


    // Put all cards in the m_pDeck stack.  We will deal
    // from this stack.
    while(!pDeck->isEmpty())
    {
        this->m_pDeck->addCard(pDeck->next());
    }

    delete pDeck;


    DealItemVector dealItemVector;
    

    // Create the dealItemVector to direct the DealAnimation object on
    // how to deal the cards.
    for (i=0;i<this->m_stackVector.size();i++)
    {
	unsigned int cardsInStack=((i<4)?6:5);
	
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

////////////////////////////////////////////////////////////////////////////////
// The actions and the groups are create on the stack and then added to the QMenu
// Since, the QMenu is the owner it will clean up the memory.
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::addGameMenuItems(QMenu & menu)
{
    QActionGroup * pNumSuitsGroup=new QActionGroup(&menu);

    QAction * pFourSuitsAction=new QAction(tr("Four Suits").trimmed(),pNumSuitsGroup);
    pFourSuitsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_4));
    pFourSuitsAction->setCheckable(true);
    connect(pFourSuitsAction,SIGNAL(triggered()),
            this,SLOT(slotSetFourSuits()));

    QAction * pTwoSuitsAction=new QAction(tr("Two Suits").trimmed(),pNumSuitsGroup);
    pTwoSuitsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
    pTwoSuitsAction->setCheckable(true);
    connect(pTwoSuitsAction,SIGNAL(triggered()),
            this,SLOT(slotSetTwoSuits()));

    QAction * pOneSuitAction=new QAction(tr("One Suit").trimmed(),pNumSuitsGroup);
    pOneSuitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
    pOneSuitAction->setCheckable(true);
    connect(pOneSuitAction,SIGNAL(triggered()),
            this,SLOT(slotSetOneSuit()));

    // select the correct item in the list
    switch (this->m_gameType)
    {
        case SpiderBoard::FourSuits:
        pFourSuitsAction->setChecked(true);
        break;

        case SpiderBoard::TwoSuits:
        pTwoSuitsAction->setChecked(true);
        break;

        case SpiderBoard::OneSuit:
        pOneSuitAction->setChecked(true);
        break;
    };

    menu.addAction(pFourSuitsAction);
    menu.addAction(pTwoSuitsAction);
    menu.addAction(pOneSuitAction);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::loadSettings(const QSettings & settings)
{
    int gameType=settings.value(this->GameTypeKeyStr,SpiderBoard::FourSuits).toInt();

    switch (gameType)
    {
        case SpiderBoard::OneSuit:
        this->m_gameType=SpiderBoard::OneSuit;
        break;

        case SpiderBoard::TwoSuits:
        this->m_gameType=SpiderBoard::TwoSuits;
        break;

        case SpiderBoard::FourSuits:
        default:
        this->m_gameType=SpiderBoard::FourSuits;
        break;
    };
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::saveSettings(QSettings & settings)
{
    settings.setValue(this->GameTypeKeyStr,this->m_gameType);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::setCheat(bool cheat)
{
    this->m_cheat=cheat;

    for(unsigned int i=0;i<this->m_stackVector.size();i++)
    {
        this->m_stackVector[i]->setCheat(cheat);
    }
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotDealNextCards(CardStack * pCardStackWidget,
                                    unsigned int index)
{
    bool aStackIsEmpty=false;
    unsigned int i;

    Q_UNUSED(pCardStackWidget);
    Q_UNUSED(index);

    // if the deck has cards to deal make sure all the stacks have cards
    // and deal them out
    if (!this->m_pDeck->isEmpty())
    {
        // first make sure none of the 10 stacks is empty.  All must have at least one card.
        // to be able to deal the cards.
        for(i=0;i<this->m_stackVector.size();i++)
        {
            if (this->m_stackVector[i]->isEmpty())
            {
                aStackIsEmpty=true;
                break;
            }
        }


        if (aStackIsEmpty)
        {
            QMessageBox::critical(this,this->gameName(),
                                  tr("All stacks must contain at least one card before the next set of cards can be dealt!").trimmed());
	    this->stopDemo();
            return;
        }


	DealItemVector dealItemVector;
	
	
	// Create the dealItemVector to direct the DealAnimation object on
	// how to deal the cards.
	for (i=0;i<this->m_stackVector.size();i++)
	{
	    dealItemVector.push_back(DealItem(this->m_stackVector[i],m_pDeck));
	    
	    dealItemVector[i].addCard(true);		
        }
	// ok now start the deal.
	m_dealAni.dealCards(dealItemVector);
    }
}

////////////////////////////////////////////////////////////////////////////////
// ok we need to perform the move from the stackWidget to the sentHome widget
// and create the CardMoveRecord.  Then just call slotCardsMoved and let the
// move be processed as if it was a drag and drop
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotSendSuitHome(SpiderStack * pStack,
                                   const PlayingCardVector & cardVector,
                                   const CardMoveRecord & startMoveRecord)
{
    if (NULL!=pStack)
    {
        // ok first find an empty sentHome widget to add the cards too.
        SpiderHomeStack * pHome=NULL;
        unsigned int i;
        for (i=0;i<this->m_homeVector.size();i++)
        {
            if (this->m_homeVector[i]->isEmpty())
            {
                pHome=this->m_homeVector[i];
                break;
            }
        }

        // we should always find an empty home widget.  But check just in case
        if (pHome)
        {
            CardMoveRecord moveRecord(startMoveRecord);
            // add the cards to the update record.  But don't move the
	    // cards
            pHome->addCards(cardVector,moveRecord,true);

	    // perform the move of the cards and animate it if animations
	    // are enabled
	    m_sToSAniMove.moveCards(moveRecord);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotMoveCardsToDiffStack(SpiderStack * pStack,
                                           const PlayingCardVector & cardVector,
                                           const CardMoveRecord & startMoveRecord)
{
    if (NULL!=pStack)
    {
        SpiderStack * pCanMoveToEmpty=NULL;
        SpiderStack * pCanMoveTo=NULL;
        SpiderStack * pCanMoveToSameSuit=NULL;
        bool cardsWillMove=false;

        for (unsigned int i=0;i<this->m_stackVector.size();i++)
        {
            // ok we are only moving the cards if the hit is not on the same
            // stack they are already located in
            if (this->m_stackVector[i]!=pStack)
            {
                // see if we have a perferred move to a stack that has the same suit
                // we will break out if we find a perferred match
                if (this->m_stackVector[i]->canAddCardsSameSuit(cardVector))
                {
                    pCanMoveToSameSuit=this->m_stackVector[i];
                    cardsWillMove=true;
                    break;
                }
                // otherwise see if we can move the cards to the stack at all
                else if (this->m_stackVector[i]->canAddCards(cardVector))
                {
                    if (this->m_stackVector[i]->isEmpty())
                    {
                        pCanMoveToEmpty=this->m_stackVector[i];
                    }
                    else
                    {
                        pCanMoveTo=this->m_stackVector[i];
                    }
                    cardsWillMove=true;
                }
            }
        }

        // if we are going to move the cards
        if (cardsWillMove)
        {
            SpiderStack * pNewStack=NULL;

            // ok the best move is to a matching suit and consecutive
            // next best is to a consecutive
            // and lastly to an empty stack.
            if (NULL!=pCanMoveToSameSuit)
            {
                pNewStack=pCanMoveToSameSuit;
            }
            else if (NULL!=pCanMoveTo)
            {
                pNewStack=pCanMoveTo;
            }
            else
            {
                pNewStack=pCanMoveToEmpty;
            }

            CardMoveRecord moveRecord(startMoveRecord);
            pNewStack->addCards(cardVector,moveRecord,true);

	    // perform the move of the cards and animate it if animations
	    // are enabled
	    m_sToSAniMove.moveCards(moveRecord);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotSetFourSuits()
{
    if (SpiderBoard::FourSuits!=this->m_gameType)
    {
        this->m_gameType=SpiderBoard::FourSuits;
        this->newGame();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotSetTwoSuits()
{
    if (SpiderBoard::TwoSuits!=this->m_gameType)
    {
        this->m_gameType=SpiderBoard::TwoSuits;
        this->newGame();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::slotSetOneSuit()
{
    if (SpiderBoard::OneSuit!=this->m_gameType)
    {
        this->m_gameType=SpiderBoard::OneSuit;
        this->newGame();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::calcScore()
{
    int score=0;
    unsigned int i;

    for(i=0;i<this->m_homeVector.size();i++)
    {
        score+=this->m_homeVector[i]->score();
    }

    for(i=0;i<this->m_stackVector.size();i++)
    {
        score+=this->m_stackVector[i]->score();
    }

    int deals=0;

    // The number of deals left is the number of cards in the deck widget divided by the number of stack widgets
    if (!this->m_pDeck->isEmpty() && this->m_stackVector.size()>0)
    {
        const PlayingCardVector & cardVector=this->m_pDeck->getCardVector();

        deals=cardVector.size()/this->m_stackVector.size();

	if (cardVector.size()%this->m_stackVector.size())
	{
	    deals++;
	}
    }

    QString dealsLeft(tr("Deals remaining:  %1").arg(QString::number(deals)).trimmed());

    emit scoreChanged(score,dealsLeft);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::resizeEvent (QResizeEvent * event)
{
    unsigned int i;

    GameBoard::resizeEvent(event);


    QSize cardSize(CardPixmaps::getInst().getCardSize());

    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);
    
    m_pDeck->setPos(currPos);

    currPos.setX(GameBoard::LayoutSpacing*3 + cardSize.width()*2);

    for (i=0;i<m_homeVector.size();i++)
    {
	m_homeVector[i]->setPos(currPos);
	currPos.setX(currPos.x()+cardSize.width()+GameBoard::LayoutSpacing);
    }

    currPos.setY(GameBoard::LayoutSpacing*2+cardSize.height());
    currPos.setX(GameBoard::LayoutSpacing);
    for (i=0;i<m_stackVector.size();i++)
    {
	m_stackVector[i]->setPos(currPos);
	currPos.setX(currPos.x()+cardSize.width()+GameBoard::LayoutSpacing);
    }

    std::cout<<__FUNCTION__<<std::endl;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool SpiderBoard::runDemo(bool stopWhenNoMore)
{
    bool rc=true;

    if (!GameBoard::runDemo(false))
    {
	if (!m_pDeck->isEmpty())
	{
	    slotDealNextCards();
	}
	else if (stopWhenNoMore)
	{
	    stopDemo();
	    rc=false;
	}
	else
	{
	    rc=false;
	}
    }

    return rc;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void SpiderBoard::createStacks()
{
    setCardResizeAlg(10,ResizeByWidth);

    unsigned int i;
    // create all the widgets for the board
    for(i=0;i<8;i++)
    {
        this->m_homeVector.push_back(new SpiderHomeStack);
	this->m_scene.addItem(m_homeVector[i]);

        // get signals when cards are added to the stack.  So, we can add undo info and
        // see when the game is over.
        this->connect(this->m_homeVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
    }

    for(i=0;i<10;i++)
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

    std::cout<<__FUNCTION__<<__FILE__<<std::endl;


}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool SpiderBoard::isGameWon()const
{
    bool allSuitsSentHome=true;

    for(unsigned int i=0;i<this->m_homeVector.size();i++)
    {
        if (this->m_homeVector[i]->isEmpty())
        {
            allSuitsSentHome=false;
            break;
        }
    }

    return allSuitsSentHome;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool SpiderBoard::isGameWonNotComplete()const
{
    bool rc=m_pDeck->isEmpty();

    if (rc)
    {
	for (unsigned int i=0;i<this->m_stackVector.size();i++)
	{
	    if (!(this->m_stackVector[i]->cardsAscendingTopToBottom() &&
		  this->m_stackVector[i]->allCardsFaceUp()))
	    {
		rc=false;
		break;
	    }
	}
    }

    return rc;
}

