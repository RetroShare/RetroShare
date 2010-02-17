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

#include "SpiderHomeStack.h"

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
SpiderHomeStack::SpiderHomeStack()
        :CardStack()
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
SpiderHomeStack::~SpiderHomeStack()
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool SpiderHomeStack::canSendHome(const PlayingCardVector & cardVector)
{
    bool rc=false;

    if (PlayingCard::MaxCardIndex==cardVector.size() &&
        PlayingCard::King==cardVector[0].getIndex() &&
        PlayingCard::Ace==cardVector[cardVector.size()-1].getIndex())
    {
        rc=true;
        for(unsigned int i=1;i<cardVector.size();i++)
        {
            if (!(cardVector[i-1].isPrevCardIndex(cardVector[i]) &&
                  cardVector[i-1].isSameSuit(cardVector[i])))
            {
                    rc=false;
                    break;
            }
        }
    }

     return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool SpiderHomeStack::canAddCards(const PlayingCardVector & newCards)
{
    bool rc=false;
    if (this->isEmpty())
    {
        rc=SpiderHomeStack::canSendHome(newCards);
    }

    return rc;
}
