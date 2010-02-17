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

#include "KlondikeStack.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
KlondikeStack::KlondikeStack()
    :VCardStack(),
     m_cheat(false)
{
    this->setAutoTopCardUp();
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
KlondikeStack::~KlondikeStack()
{
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool KlondikeStack::canAddCards(const PlayingCardVector & newCardVector)
{
    if (m_cheat)
    {
        return true;
    }

    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    if (newCardVector.size()>0)
    {
        // if there are no cards in the stack then the first card
	// must be a king.
        if (0==cardVector.size() &&
            PlayingCard::King==newCardVector[0].getIndex())
        {
            rc=true;
        }
        else if (cardVector.size()>0 &&
                 (cardVector[cardVector.size()-1].isRed() ^ newCardVector[0].isRed()) &&
                 cardVector[cardVector.size()-1].isPrevCardIndex(newCardVector[0]) &&
                 cardVector[cardVector.size()-1].isFaceUp())
        {
            rc=true;
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool KlondikeStack::canMoveCard(unsigned int index) const
{
    if (m_cheat)
    {
        return true;
    }

    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    // ok if we have cards in the stack and the index is valid
    // see if we can move it.
    // For klondike as long as the card is faceup we should be able to move the
    // card and all cards after it in the stack.
    if (cardVector.size()>0 && index<cardVector.size() && cardVector[index].isFaceUp())
    {
        rc=true;
    }

    return rc;
}
