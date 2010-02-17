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

#include "FreeCellHome.h"

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
FreeCellHome::FreeCellHome()
    :CardStack()
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
FreeCellHome::~FreeCellHome()
{
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool FreeCellHome::canAddCards(const PlayingCardVector & newCardVector)
{
    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    if (1==newCardVector.size())
    {

        if (this->isEmpty() && PlayingCard::Ace==newCardVector[0].getIndex())
        {
            rc=true;
        }
        else if (!this->isEmpty() &&
                 cardVector[cardVector.size()-1].isSameSuit(newCardVector[0]) &&
                 cardVector[cardVector.size()-1].isNextCardIndex(newCardVector[0]))
        {
            rc=true;
        }
    }

    return rc;
}
