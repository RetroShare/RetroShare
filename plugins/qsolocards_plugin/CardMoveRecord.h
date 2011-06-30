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

#ifndef CARDMOVERECORD_H
#define CARDMOVERECORD_H

#include "CardDeck.h"
#include <list>
#include <string>

class CardMoveRecordItem
{
    public:
    enum MoveType
    {
        AddCards=0,
        RemoveCards=1,
        FlipCard=2
    };

    CardMoveRecordItem(const std::string & stackName,
                       int flipIndex=-1);  // Case where we are just flipping a card
    CardMoveRecordItem(const std::string & stackName,
                       MoveType m_type,
                       const PlayingCardVector & cardVector);  // case where we are adding or removing cards
    CardMoveRecordItem(const CardMoveRecordItem & rh);

    virtual ~CardMoveRecordItem();

    CardMoveRecordItem & operator=(const CardMoveRecordItem & rh);

    inline MoveType moveType() const {return m_moveType;}
    inline const PlayingCardVector & cardVector() const {return m_cardVector;}
    inline int flipIndex() const {return m_flipIndex;}
    inline const std::string & stackName() const{return m_stackName;}

    private:
    MoveType           m_moveType;
    PlayingCardVector  m_cardVector;
    int                m_flipIndex;
    std::string        m_stackName;
};

typedef std::list<CardMoveRecordItem> CardMoveRecord;


#endif // CARDMOVERECORD_H
