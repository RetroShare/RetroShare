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

#include "SpiderStack.h"
#include "SpiderHomeStack.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
SpiderStack::SpiderStack()
    :VCardStack(),
     m_cheat(false)
{
    this->setAutoTopCardUp();


    // connect a slot to get the signal when a movable card is clicked
    this->connect(this,SIGNAL(movableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)),
		  this,SLOT(slotMovableCardsClicked(CardStack*,PlayingCardVector,CardMoveRecord)));
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
SpiderStack::~SpiderStack()
{
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
int SpiderStack::score() const
{
    const PlayingCardVector & cardVector=this->getCardVector();
    int score=0;

    for (unsigned int i=1;i<cardVector.size();i++)
    {
        if (cardVector[i].isNextCardIndex(cardVector[i-1]) &&
            cardVector[i].isSameSuit(cardVector[i-1]) &&
            cardVector[i].isFaceUp() && cardVector[i-1].isFaceUp())
        {
            score++;
        }
    }

    return score;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool SpiderStack::canAddCards(const PlayingCardVector & newCardVector)
{
    if (m_cheat)
    {
        return true;
    }

    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    if (newCardVector.size()>0)
    {
        // if there are no cards in the stack then we will accept any cards
        if (0==cardVector.size())
        {
            rc=true;
        }
        else if (cardVector[cardVector.size()-1].isPrevCardIndex(newCardVector[0]) &&
                 cardVector[cardVector.size()-1].isFaceUp())
        {
            rc=true;
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////////
// similar to can Add cards.  But in this case the
// suit of the last card in this stack is the same as
// the first suit of the cardVector
///////////////////////////////////////////////////////////////////////////////////
bool SpiderStack::canAddCardsSameSuit(const PlayingCardVector & newCardVector)
{
    bool rc=false;

    if (this->canAddCards(newCardVector) && !this->isEmpty() && newCardVector.size()>0)
    {
        const PlayingCardVector & cardVector=this->getCardVector();

        rc=cardVector[cardVector.size()-1].isSameSuit(newCardVector[0]);
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////////
// if a card is clicked on see if it needs to be sent home or can just be moved
// to another stack.
///////////////////////////////////////////////////////////////////////////////////
void SpiderStack::slotMovableCardsClicked(CardStack * pCardStack,
					  const PlayingCardVector & moveCards,
					  const CardMoveRecord & moveRecord)
{
    bool isSentHome=false;

    Q_UNUSED(pCardStack);

    // first see if the item needs to be sent home
    if (SpiderHomeStack::canSendHome(moveCards))
    {
        emit sendSuitHome(this,moveCards,moveRecord);
        isSentHome=true;
    }

    // if this is not a case when the suit can be sent home
    // Then we will just emit a signal to try to move the
    // set of cards.
    if (!isSentHome)
    {
        emit moveCardsToDiffStack(this,moveCards,moveRecord);
    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool SpiderStack::canMoveCard(unsigned int index) const
{
    if (m_cheat)
    {
        return true;
    }

    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    // ok if we have cards in the stack and the index is valid
    // see if we can move it.
    if (cardVector.size()>0 && index<cardVector.size() && cardVector[index].isFaceUp())
    {
        // if the card is the last in the stack, it is movable
        if (cardVector.size()-1==index)
        {
            rc=true;
        }
        else
        {
            rc=true;
            for(unsigned int i=index+1;i<cardVector.size();i++)
            {
                if (!(cardVector[i].isNextCardIndex(cardVector[i-1]) &&
                      cardVector[i].isSameSuit(cardVector[i-1])))
                {
                    rc=false;
                    break;
                }
            }
        }
    }

    return rc;
}
