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

#include "CardMoveRecord.h"


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CardMoveRecordItem::CardMoveRecordItem(const std::string & stackName,
                                       int flipIndex)
    :m_moveType(CardMoveRecordItem::FlipCard),
     m_cardVector(),
     m_flipIndex(flipIndex),
     m_stackName(stackName)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CardMoveRecordItem::CardMoveRecordItem(const std::string & stackName,
                                       MoveType m_type,
                                       const PlayingCardVector & cardVector)
    :m_moveType(m_type),
     m_cardVector(cardVector),
     m_flipIndex(-1),
     m_stackName(stackName)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CardMoveRecordItem::CardMoveRecordItem(const CardMoveRecordItem & rh)
{
    *this=rh;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CardMoveRecordItem::~CardMoveRecordItem()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
CardMoveRecordItem & CardMoveRecordItem::operator=(const CardMoveRecordItem & rh)
{
    m_moveType=rh.moveType();
    m_cardVector=rh.cardVector();
    m_flipIndex=rh.flipIndex();
    m_stackName=rh.stackName();

    return *this;
}
