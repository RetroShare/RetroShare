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

#include "FreeCellStack.h"

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
FreeCellStack::FreeCellStack()
    :m_cheat(false),
     m_maxMoveCards(1)
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
FreeCellStack::~FreeCellStack()
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool FreeCellStack::canAddCards(const PlayingCardVector & newCardVector)
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
        else if (cardVector.size()>0 &&
                 (cardVector[cardVector.size()-1].isRed() ^ newCardVector[0].isRed()) &&
                 cardVector[cardVector.size()-1].isPrevCardIndex(newCardVector[0]))
        {
            rc=true;
        }
    }

    return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
bool FreeCellStack::canMoveCard(unsigned int index) const
{
    bool rc=false;
    const PlayingCardVector & cardVector=this->getCardVector();

    if (index<cardVector.size())
    {

	
	if (cardVector.size()-index<=m_maxMoveCards)
	{
	    unsigned int i;
	    rc=true;

	    for(i=index+1;i<cardVector.size();i++)
	    {

		if (!((cardVector[i-1].isRed() ^ cardVector[i].isRed()) &&
		      (cardVector[i-1].isPrevCardIndex(cardVector[i]))))
		{
		    rc=false;
		    break;
		}
	    }
	}
    }

    return rc;
}
