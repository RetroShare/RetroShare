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

#include "KlondikeBoard.h"
#include <QtGui/QMessageBox>

#include "CardDeck.h"
#include "CardPixmaps.h"
#include "CardAnimationLock.h"

const QString  KlondikeBoard::GameTypeKeyStr("GameType");

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
KlondikeBoard::KlondikeBoard(QWidget * pWidget)
    :GameBoard(pWidget,QString(tr("Klondike Solitaire")).trimmed(),QString("Klondike")),
     m_pDeck(NULL),
     m_pFlipDeck(NULL),
     m_homeVector(),
     m_stackVector(),
     m_cheat(false),
     m_gameType(KlondikeBoard::FlipOne),
     m_flipNum(0),
     m_sToSFlipAni()
{
    this->setHelpFile(":/help/KlondikeHelp.html");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
KlondikeBoard::~KlondikeBoard()
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::undoMove()
{
    bool emptyBefore=false;
    bool emptyAfter=false;

    emptyBefore=m_pDeck->isEmpty();
    
    GameBoard::undoMove();

    emptyAfter=m_pDeck->isEmpty();

    // if we undone a redeal decrement the flip count
    if (!emptyBefore && emptyAfter)
    {
	this->m_flipNum--;
        if (!(KlondikeBoard::FlipOne==this->m_gameType &&
              this->m_flipNum<2) &&
	    !(KlondikeBoard::FlipThree==this->m_gameType))
        {
            this->m_pDeck->setShowRedealCircle(false);
            this->m_pDeck->updateStack();
        }
	else
	{
            this->m_pDeck->setShowRedealCircle(true);
            this->m_pDeck->updateStack();
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::redoMove()
{
    bool emptyBefore=false;
    bool emptyAfter=false;

    emptyBefore=m_pDeck->isEmpty();

    GameBoard::redoMove();

    emptyAfter=m_pDeck->isEmpty();

    // if we are redoing a redeal increment the flip count
    if (emptyBefore && !emptyAfter)
    {
	this->m_flipNum++;

        if (!(KlondikeBoard::FlipOne==this->m_gameType &&
              this->m_flipNum<2) &&
	    !(KlondikeBoard::FlipThree==this->m_gameType))
        {
            this->m_pDeck->setShowRedealCircle(false);
            this->m_pDeck->updateStack();
        }
	else
	{
            this->m_pDeck->setShowRedealCircle(true);
            this->m_pDeck->updateStack();
	}
    }

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::restartGame()
{
    // call the base class to rewind the undo stack
    GameBoard::restartGame();

    // reset the flip count
    this->m_flipNum=0;
    // and set the redeal circle depending on game type
    if (KlondikeBoard::NoRedeals==this->m_gameType)
    {
        this->m_pDeck->setShowRedealCircle(false);
    }
    else
    {
        this->m_pDeck->setShowRedealCircle(true);
    }

}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool KlondikeBoard::getHint(CardStack * & pFromStack,
			    unsigned int & index,
			    CardStack * & pToStack)
{
    // first see if we can move anything to one of the home stacks.
    PlayingCardVector moveCards;
    unsigned int i;
    bool matchFound=false;


    // ok check to see if we have a card from the stacks that can
    // be added to a home stack
    for (i=0;i<this->m_stackVector.size() && !matchFound;i++)
    {
        if (!this->m_stackVector[i]->isEmpty())
        {
            const PlayingCardVector & currCards=this->m_stackVector[i]->getCardVector();
            moveCards.clear();
            moveCards.push_back(currCards[currCards.size()-1]);  // just get the last card since we are moving to the home stack
            index=currCards.size()-1;

            for (unsigned int j=0;j<this->m_homeVector.size();j++)
            {
                if (this->m_homeVector[j]->canAddCards(moveCards))
                {
                    pToStack=this->m_homeVector[j];
                    pFromStack=this->m_stackVector[i];
                    matchFound=true;
                    break;
                }
            }
        }
    }

    // next check for stack to stack moves.
    if (!matchFound)
    {
        moveCards.clear();
        pToStack=NULL;
        pFromStack=NULL;

        for (i=0;i<this->m_stackVector.size() && !matchFound;i++)
        {
            moveCards.clear();
            if (!this->m_stackVector[i]->isEmpty() &&
                this->m_stackVector[i]->getMovableCards(moveCards,index))
            {
                // we want to eliminate a move that starts with a stack that
                // has a faceup king as the first card.  We will just end up
                // suggesting to move it to another empty row.  Which is not
                // really a move.
                if (PlayingCard::King==moveCards[0].getIndex() &&
                    moveCards.size()== this->m_stackVector[i]->getCardVector().size())
                {
                    continue;
                }
                for (unsigned int j=0;j<this->m_stackVector.size();j++)
                {
                    if (this->m_stackVector[i]!=this->m_stackVector[j] &&
                        this->m_stackVector[j]->canAddCards(moveCards))
                    {
                        pToStack=this->m_stackVector[j];
                        pFromStack=this->m_stackVector[i];
                        matchFound=true;
                        break;
                    }
                }
            }
        }
    }


    // check the flip deck.  We can just call getMovableCards for it
    // since, it will always be just one card.
    if (!matchFound)
    {
        moveCards.clear();
        pToStack=NULL;
        pFromStack=NULL;

        if (!this->m_pFlipDeck->isEmpty() &&
            this->m_pFlipDeck->getMovableCards(moveCards,index))
        {
            // first see if we can send a card to one of the home
            // stacks
            for(i=0;i<this->m_homeVector.size();i++)
            {
                if (this->m_homeVector[i]->canAddCards(moveCards))
                {
                    pToStack=this->m_homeVector[i];
                    pFromStack=this->m_pFlipDeck;
                    matchFound=true;
                    break;
                }
            }

            // if no match now check to see if we can add the card to the
            // normal stacks
            if (!matchFound)
            {
                for(i=0;i<this->m_stackVector.size();i++)
                {
                    if (this->m_stackVector[i]->canAddCards(moveCards))
                    {
                        pToStack=this->m_stackVector[i];
                        pFromStack=this->m_pFlipDeck;
                        matchFound=true;
                        break;
                    }
                }
            }
        }
    }


    return matchFound;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::newGame()
{
    // stop any animation of flipping cards
    m_sToSFlipAni.stopAni();
    // call the base class
    GameBoard::newGame();

    CardDeck deck;
    unsigned int i;

    this->m_flipNum=0;

    // only show the redeal circle in the cases that we can redeal
    if (KlondikeBoard::NoRedeals==this->m_gameType)
    {
        this->m_pDeck->setShowRedealCircle(false);
    }
    else
    {
        this->m_pDeck->setShowRedealCircle(true);
    }


    this->m_pFlipDeck->cardsToShow(((KlondikeBoard::FlipThree==this->m_gameType)?3:1));



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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::addGameMenuItems(QMenu & menu)
{
    QActionGroup * pFlipCardsGroup=new QActionGroup(&menu);


    QAction * pFlipOneAction=new QAction(tr("Flip one").trimmed(),pFlipCardsGroup);
    pFlipOneAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
    pFlipOneAction->setCheckable(true);
    connect(pFlipOneAction,SIGNAL(triggered()),
            this,SLOT(slotSetFlipOne()));

    QAction * pFlipThreeAction=new QAction(tr("Flip three").trimmed(),pFlipCardsGroup);
    pFlipThreeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_3));
    pFlipThreeAction->setCheckable(true);
    connect(pFlipThreeAction,SIGNAL(triggered()),
            this,SLOT(slotSetFlipThree()));

    QAction * pNoRedealsAction=new QAction(tr("No redeals (flip one)").trimmed(),pFlipCardsGroup);
    pNoRedealsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    pNoRedealsAction->setCheckable(true);
    connect(pNoRedealsAction,SIGNAL(triggered()),
            this,SLOT(slotSetNoRedeals()));

    // select the correct item in the list
    switch (this->m_gameType)
    {
    case KlondikeBoard::FlipOne:
        pFlipOneAction->setChecked(true);
        break;

    case KlondikeBoard::FlipThree:
        pFlipThreeAction->setChecked(true);
        break;

    case KlondikeBoard::NoRedeals:
        pNoRedealsAction->setChecked(true);
        break;
    };


    menu.addAction(pFlipOneAction);
    menu.addAction(pFlipThreeAction);
    menu.addAction(pNoRedealsAction);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::loadSettings(const QSettings & settings)
{
    int gameType=settings.value(this->GameTypeKeyStr,KlondikeBoard::FlipOne).toInt();

    switch (gameType)
    {
    case KlondikeBoard::NoRedeals:
        this->m_gameType=KlondikeBoard::NoRedeals;
        break;

    case KlondikeBoard::FlipThree:
        this->m_gameType=KlondikeBoard::FlipThree;
        break;

    case KlondikeBoard::FlipOne:
    default:
        this->m_gameType=KlondikeBoard::FlipOne;
        break;
    };
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::saveSettings(QSettings & settings)
{
    settings.setValue(this->GameTypeKeyStr,this->m_gameType);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::setCheat(bool cheat)
{
    this->m_cheat=cheat;

    for(unsigned int i=0;i<this->m_stackVector.size();i++)
    {
        m_stackVector[i]->setCheat(cheat);
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotFlipCards(CardStack * pCardStack,
                                  unsigned int index)
{
    Q_UNUSED(pCardStack);
    Q_UNUSED(index);
    // just use the game type to indicate the number of flip cards
    // the enum is assigned the number of cards that will be flipped
    // the exception is the NoRedeals in that case we are just flipping
    // one card.
    unsigned int numFlipCards=this->m_gameType;
    if (KlondikeBoard::NoRedeals==this->m_gameType)
    {
        numFlipCards=KlondikeBoard::FlipOne;
    }


    m_sToSFlipAni.moveCards(m_pFlipDeck,m_pDeck,numFlipCards);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotRedealCards(CardStack * pCardStack)
{
    Q_UNUSED(pCardStack);
    // ignore an attempt to redeal if the game type is NoRedeals.
    // otherwise take all the cards from the Flip stack and put them back
    // in the deal stack
    if (KlondikeBoard::NoRedeals!=this->m_gameType &&
        ((KlondikeBoard::FlipOne==this->m_gameType &&
	  this->m_flipNum<2) ||
	 KlondikeBoard::FlipThree==this->m_gameType))
    {
	unsigned int numFlipCards=this->m_gameType;
	if (KlondikeBoard::NoRedeals==this->m_gameType)
	{
	    numFlipCards=KlondikeBoard::FlipOne;
	}

	// flip the cards back.  Flip the cards back over.  Show the bottom 3 if in FlipThree mode.  Otherwise
	// just show one.
	m_sToSFlipAni.moveCards(m_pDeck,m_pFlipDeck,this->m_pFlipDeck->getCardVector().size(),numFlipCards);

        this->m_flipNum++;

        if (!(KlondikeBoard::FlipOne==this->m_gameType &&
              this->m_flipNum<2) &&
	    !(KlondikeBoard::FlipThree==this->m_gameType))
        {
            this->m_pDeck->setShowRedealCircle(false);
        }
    }
    else
    {
	// if a demo is running stop it.  Can't redeal any more.
	this->stopDemo();
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotStackCardsClicked(CardStack * pCardStack,
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
    // the home will use the same logic so just call that slot
    if (NULL==pFoundStack)
    {
        this->slotHomeCardsClicked(pCardStack,cardVector,startMoveRecord);
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotHomeCardsClicked(CardStack * pCardStack,
					 const PlayingCardVector & cardVector,
					 const CardMoveRecord & startMoveRecord)
{
    unsigned int i=0;
    CardStack * pFoundStack=NULL;

    if (NULL==pCardStack)
    {
        return;
    }

    for(i=0;i<this->m_stackVector.size();i++)
    {
        if (pCardStack!=this->m_stackVector[i] &&
            this->m_stackVector[i]->canAddCards(cardVector))
        {
            pFoundStack=this->m_stackVector[i];
            break;
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


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotSetNoRedeals()
{
    this->m_gameType=KlondikeBoard::NoRedeals;
    this->newGame();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotSetFlipOne()
{
    this->m_gameType=KlondikeBoard::FlipOne;
    this->newGame();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::slotSetFlipThree()
{
    this->m_gameType=KlondikeBoard::FlipThree;
    this->newGame();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::calcScore()
{
    int score=0;

    for(unsigned int i=0;i<this->m_homeVector.size();i++)
    {
        score+=this->m_homeVector[i]->score();
    }

    emit scoreChanged(score,"");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void KlondikeBoard::resizeEvent (QResizeEvent * event)
{
    unsigned int i;

    GameBoard::resizeEvent(event);


    QSize cardSize(CardPixmaps::getInst().getCardSize());

    QPointF currPos(GameBoard::LayoutSpacing,GameBoard::LayoutSpacing);
    
    m_pDeck->setPos(currPos);

    currPos.rx()+=GameBoard::LayoutSpacing + cardSize.width();

    m_pFlipDeck->setPos(currPos);

    currPos.rx()+=GameBoard::LayoutSpacing*3 + cardSize.width()*3;


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
bool KlondikeBoard::runDemo(bool stopWhenNoMore)
{
    bool rc=true;

    if (!GameBoard::runDemo(false))
    {
	if (!m_pDeck->isEmpty())
	{
	    this->slotFlipCards();
	}
	else if (!m_pFlipDeck->isEmpty())
	{
	    this->slotRedealCards();
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
void KlondikeBoard::createStacks()
{
    setCardResizeAlg(8,ResizeByWidth);


    unsigned int i;

    // first create the home widgets where the cards need to be eventually stacked to
    // win the game.
    for(i=0;i<4;i++)
    {
        this->m_homeVector.push_back(new KlondikeHomeStack);
	this->m_scene.addItem(m_homeVector[i]);
        this->connect(this->m_homeVector[i],SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                      this,SLOT(slotCardsMoved(CardMoveRecord)));
        this->connect(this->m_homeVector[i],SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
                      this,SLOT(slotHomeCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));
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

    this->m_pDeck=new CardStack;
    this->m_scene.addItem(this->m_pDeck);

    this->m_pDeck->setShowRedealCircle();


    // if a card is double clicked that means we need to flip over the next set of cards
    this->connect(this->m_pDeck,SIGNAL(cardClicked(CardStack*,uint)),
                  this,SLOT(slotFlipCards(CardStack*,uint)));

    // if there are no cards in the deck and the pad is clicked that means we need
    // to flip the deck back over.  If the version of the game is not No redeals.
    this->connect(this->m_pDeck,SIGNAL(padClicked(CardStack*)),
                  this,SLOT(slotRedealCards(CardStack*)));

    this->m_pFlipDeck=new KlondikeFlipStack;
    this->m_scene.addItem(this->m_pFlipDeck);

    this->connect(this->m_pFlipDeck,SIGNAL(cardsMovedByDragDrop(CardMoveRecord)),
                  this,SLOT(slotCardsMoved(CardMoveRecord)));
    this->connect(this->m_pFlipDeck,SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
                  this,SLOT(slotStackCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));



    // hook up the animation signal for flipping cards over from the deck to the flip
    // stack.  And then back.
    this->connect(&m_sToSFlipAni,SIGNAL(cardsMoved(CardMoveRecord)),
		  this,SLOT(slotCardsMoved(CardMoveRecord)));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool KlondikeBoard::isGameWon()const
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool KlondikeBoard::isGameWonNotComplete()const
{
    bool rc=false;

    rc=m_pDeck->isEmpty();

    if (rc)
    {
	rc=this->m_pFlipDeck->cardsAscendingTopToBottom();
    }

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
